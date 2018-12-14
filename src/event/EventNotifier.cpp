/**
 * scatter
 * EventNotifier.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "EventNotifier.h"
#include "../base/Settings.hpp"

wss::event::EventNotifier::EventNotifier(std::shared_ptr<wss::ChatServer> &ws) :
    m_keepGoing(true),
    m_readCondition(),
    m_ws(ws),
    m_enableRetry(wss::Settings::get().event.enableRetry),
    m_maxParallelWorkers(wss::Settings::get().event.maxParallelWorkers),
    m_maxRetries(3),
    m_retryIntervalSeconds(10),
    m_ioService(),
    m_threadGroup(),
    m_work(m_ioService) { }

wss::event::EventNotifier::~EventNotifier() {
    m_keepGoing = false;
    m_readCondition.notify_one();
    onStop();
}

void wss::event::EventNotifier::setRetryIntervalSeconds(int seconds) {
    m_retryIntervalSeconds = seconds;
}

std::shared_ptr<wss::event::Target> wss::event::EventNotifier::createTargetByConfig(const nlohmann::json &json) {
    using namespace toolboxpp::strings;
    const auto &eq = equalsIgnoreCase;

    // exception safe method: value()
    const std::string type = json.value("type", "");
    if (type.empty()) {
        throw std::runtime_error("Target type required");
    }

    std::shared_ptr<wss::event::Target> out;

    if (eq(type, "postback")) {
        out = std::make_shared<wss::event::PostbackTarget>(json);
    } else {
        out = std::make_shared<wss::event::TargetLoader>(json, type);

        wss::event::TargetLoader *tl = (wss::event::TargetLoader *) out.get();

        std::string loadError;
        if (!tl->load(wss::Settings::get().server.extSearchPaths, loadError)) {
            throw std::runtime_error(fmt::format("Unable to load target {0}: {1}", type, loadError));
        }
    }

    if (json.find("fallback") != json.end()) {
        for (auto &obj: json.at("fallback").get<std::vector<nlohmann::json>>()) {
            out->addFallback(createTargetByConfig(obj));
        }
    }

    if (!out->isValid()) {
        throw std::runtime_error(fmt::format("Unable to init event target {0}: {1}", type, out->getErrorMessage()));
    }

    return out;
}

void wss::event::EventNotifier::setMaxTries(int tries) {
    m_maxRetries = tries;
}
void wss::event::EventNotifier::addTarget(const nlohmann::json &targetConfig) {
    addTarget(createTargetByConfig(targetConfig));
}
void wss::event::EventNotifier::addTarget(const std::shared_ptr<wss::event::Target> &target) {
    if (!target->isValid()) {
        throw std::runtime_error(target->getErrorMessage());
    }

    m_targets.insert({target->getType(), target});
}
void wss::event::EventNotifier::addTarget(std::shared_ptr<wss::event::Target> &&target) {
    if (!target->isValid()) {
        throw std::runtime_error(target->getErrorMessage());
    }

    m_targets.insert({target->getType(), std::move(target)});
}

void wss::event::EventNotifier::subscribe() {
    for (int i = 0; i < 4; i++) {
        m_threadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &m_ioService)
        );
    }

    m_ws->addMessageListener(std::bind(&EventNotifier::onMessage, this, std::placeholders::_1));
    m_ws->addStopListener(std::bind(&EventNotifier::onStop, this));
    addErrorListener(std::bind(&EventNotifier::onErrorSending, this, std::placeholders::_1));
    m_ioService.post(boost::bind(&EventNotifier::handleMessageQueue, this));
}
void wss::event::EventNotifier::joinThreads() {
    m_threadGroup.join_all();
}
void wss::event::EventNotifier::detachThreads() {

}
void wss::event::EventNotifier::runService() {
    L_INFO("EventNotifier", "Started with targets:");
    for (const auto &target: m_targets) {
        Logger::get().info(__FILE__, __LINE__, "EventNotifier", fmt::format(" - {0}", target.second->getType()));
        for (const auto &fb: target.second->getFallbacks()) {
            Logger::get().info(__FILE__, __LINE__, "EventNotifier", fmt::format("   - fallback: {0}", fb->getType()));
        }
    }
    subscribe();
}
void wss::event::EventNotifier::stopService() {
    onStop();
}
void wss::event::EventNotifier::onStop() {
    m_ioService.stop();
    m_keepGoing = false;
    m_threadGroup.interrupt_all();
}

void wss::event::EventNotifier::executeOnTarget(wss::event::EventNotifier::SendStatus s) {
    // prepare timer, we must receive callback, for 3*3 seconds, otherwise break trying to send and re-enqueue message
    // in any case, we must deliver message, event if it will be sent in a next hours
    // todo: persistency (again, i thing about it again, very bad idea to store all undelivered messages in memory)
    boost::mutex waitMutex;
    boost::unique_lock<boost::mutex> lock(waitMutex);
    boost::condition_variable cv;
    // maximum wait for receve callbacks = 3 times by 3 seconds
    // see below cv.wait_for(lock, std::chrono::seconds(3))
    const int maxTries = 3;
    int tries = 0;

    SendStatus status = std::move(s);
    boost::atomic_bool complete;
    complete = false;

    // sending

    if (!status.target->isValid()) {
        Logger::get().warning(__FILE__,
                              __LINE__,
                              "Event::Send",
                              fmt::format(
                                  "Can't send message to target {0}: {1} - Target is in error state, re-enqueue message.",
                                  status.target->getType(),
                                  status.target->getErrorMessage()));
        m_sendQueue.enqueue(std::move(status));
        return;
    }

    const std::string typeName = status.target->getType();
    status.target->send(
        status.payload,
        // success callback
        [typeName, &complete, &cv]() {
          Logger::get().info(__FILE__,
                             __LINE__,
                             "Event::Send",
                             fmt::format("Message has sent to target: {0}", typeName));

          complete = true;
          cv.notify_one();
        },
        // error callback
        [this, &status, &complete, &cv](const std::string errorMessage) {
          if (m_enableRetry) {
              Logger::get().warning(__FILE__,
                                    __LINE__,
                                    "Event::Send",
                                    fmt::format("Can't send message to target {0}: {1}",
                                              status.target->getType(),
                                              errorMessage));

              // if tries < maxRetries
              if (status.sendTries < m_maxRetries) {
                  status.sendTries++;
                  status.sendTime = std::time(nullptr);
                  m_sendQueue.enqueue(std::move(status));
              } else {
                  // can't send over maxTries times
                  // notify listeners
                  for (auto &listener: m_sendErrorListeners) {
                      listener(std::move(status));
                  }
              }
          } else {
              // our target is in error state, probably some target server unavailable,
              // add to queue, wait for target be in ready-state
              if (!status.target->isValid()) {
                  Logger::get().warning(__FILE__,
                                        __LINE__,
                                        "Event::Send",
                                        fmt::format(
                                          "Can't send message to target {0}: {1} - Target is in error state, re-enqueue message.",
                                          status.target->getType(),
                                          errorMessage));
                  m_sendQueue.enqueue(std::move(status));
              } else {
                  // everything is bad, but don't do anything with it
                  Logger::get().warning(__FILE__,
                                        __LINE__,
                                        "Event::Send",
                                        fmt::format("Can't send message to target {0}: {1}",
                                                    status.target->getType(),
                                                    errorMessage));

                  // can't send via current target, move to listeners and fallback targets (if they're exists)
                  // notify listeners
                  for (auto &listener: m_sendErrorListeners) {
                      listener(std::move(status));
                  }
              }
          }


          complete = true;
          cv.notify_one();
        });

    // spurious wake protection with simple bool variable
    while (!complete) {
        cv.wait(lock);
    }

    m_readCondition.notify_one();
    std::cout << "Done" << std::endl;
}

void wss::event::EventNotifier::handleMessageQueue() {
    boost::unique_lock<boost::mutex> lock(m_readMutex);
    const auto &ready = [this](SendStatus status) {
      // don't send to target, which is feeling bad
      if (!status.target->isValid()) {
          return false;
      }
      if (!m_enableRetry) return true;

      const long diff = abs(std::time(nullptr) - status.sendTime);
      return diff >= m_retryIntervalSeconds;
    };

    // handle queue
    while (m_keepGoing) {
        // @TODO play around this time, check performance and cpu consumption

        // wait for new messages or something else
        // see EventNotifier::executeOnTarget
        m_readCondition.wait_for(lock, boost::chrono::seconds(m_retryIntervalSeconds));

        std::vector<SendStatus> bulk;

        try {
            size_t extract;
            // get portions size
            const size_t approxSize = m_sendQueue.size_approx();
            if (approxSize > m_maxParallelWorkers) {
                // maximum connections per cycle
                extract = m_maxParallelWorkers;
            } else {
                extract = approxSize;
            }

            // prepare portion of messages to send
            bulk.resize(extract);
            m_sendQueue.try_dequeue_bulk(bulk.begin(), bulk.size());
        } catch (const std::exception &e) {
            //@TODO test this case, i've never catched it
            L_DEBUG_F("Event::Send", "Can't dequeue bulk: %s", e.what());
            continue;
        }

        int i = 0;
        for (auto it: bulk) {
            if (!ready(it)) {
                m_sendQueue.enqueue(std::move(it));
                continue;
            }

            auto st = boost::thread(boost::bind(&EventNotifier::executeOnTarget, this, it));
            // we don't need to join this thread back, we just need to send, and if not, re-enqueue payload
            st.detach();

            i++;
        }

        bulk.clear();
        if (i > 0) {
            L_DEBUG_F("Event::Send", "Prepared %d messages", i);
        }
    }
}
void wss::event::EventNotifier::addMessage(wss::MessagePayload payload) {
    for (auto &target: m_targets) {
        m_sendQueue.enqueue(SendStatus(target.second, payload, 0L, 1));
        m_readCondition.notify_one();
    }
}

void wss::event::EventNotifier::onMessage(wss::MessagePayload &&payload) {
    if (payload.isFromBot() && not wss::Settings::get().event.sendBotMessages) {
        L_DEBUG("Event::Enqueue", "Skipping Bot message (sender=0)");
        return;
    }

    bool isIgnoredType = false;
    for (const auto &ignoredType: wss::Settings::get().event.ignoreTypes) {
        if (toolboxpp::strings::equalsIgnoreCase(ignoredType, payload.getType())) {
            isIgnoredType = true;
            break;
        }
    }

    if (!isIgnoredType) {
        m_ioService.post(boost::bind(&EventNotifier::addMessage, this, payload));
    }
}

void wss::event::EventNotifier::onErrorSending(wss::event::EventNotifier::SendStatus &&status) {
    if (status.fallbackQueue.empty()) {
        return;
    }

    // getting new target from fallback
    status.target = status.fallbackQueue.front();
    status.fallbackQueue.pop();
    status.sendTries = 0;
    // and re-re-enqueue this message (and reset tries)
    m_sendQueue.enqueue(std::move(status));
}

void wss::event::EventNotifier::addErrorListener(wss::event::EventNotifier::OnSendError listener) {
    m_sendErrorListeners.push_back(listener);
}



