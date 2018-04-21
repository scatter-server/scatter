/**
 * wsserver
 * EventNotifier.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "EventNotifier.h"
#include "../base/Settings.hpp"

#ifdef ENABLE_REDIS_TARGET
#include "RedisTarget.h"
#endif

wss::event::EventNotifier::EventNotifier(std::shared_ptr<wss::ChatMessageServer> &ws) :
    running(true),
    ws(ws),
    enableRetry(wss::Settings::get().event.enableRetry),
    maxParallelWorkers(wss::Settings::get().event.maxParallelWorkers),
    maxRetries(3),
    intervalSeconds(10),
    ioService(),
    threadGroup(),
    work(ioService) { }

wss::event::EventNotifier::~EventNotifier() {
    running = false;
    onStop();
}

void wss::event::EventNotifier::setRetryIntervalSeconds(int seconds) {
    intervalSeconds = seconds;
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
    } else
        //@TODO shared modules and target map in config, instead of hardcode
        #ifdef ENABLE_REDIS_TARGET
        if(eq(type, "redis")) {
            out = std::make_shared<wss::event::RedisTarget>(json);
        } else
        #endif
    {
        throw std::runtime_error("Unsupported target type: " + type);
    }

    if (!out->isValid()) {
        throw std::runtime_error(fmt::format("Unable to init event target {0}: {1}", type, out->getErrorMessage()));
    }

    return out;
}

void wss::event::EventNotifier::setMaxTries(int tries) {
    maxRetries = tries;
}
void wss::event::EventNotifier::addTarget(const nlohmann::json &targetConfig) {
    addTarget(createTargetByConfig(targetConfig));
}
void wss::event::EventNotifier::addTarget(const std::shared_ptr<wss::event::Target> &target) {
    if (!target->isValid()) {
        throw std::runtime_error(target->getErrorMessage());
    }
    targets.insert({target->getType(), target});
}
void wss::event::EventNotifier::addTarget(std::shared_ptr<wss::event::Target> &&target) {
    if (!target->isValid()) {
        throw std::runtime_error(target->getErrorMessage());
    }
    targets.insert({target->getType(), std::move(target)});
}

void wss::event::EventNotifier::subscribe() {
    for (int i = 0; i < 4; i++) {
        threadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &ioService)
        );
    }

    ws->addMessageListener(std::bind(&EventNotifier::onMessage, this, std::placeholders::_1));
    ws->addStopListener(std::bind(&EventNotifier::onStop, this));
    ioService.post(boost::bind(&EventNotifier::handleMessageQueue, this));
}
void wss::event::EventNotifier::joinThreads() {
    threadGroup.join_all();
}
void wss::event::EventNotifier::detachThreads() {

}
void wss::event::EventNotifier::runService() {
    L_INFO("EventNotifier", "Started with targets:");
    for (auto &target: targets) {
        const char *name = target.first.c_str();
        L_INFO_F("EventNotifier", " - %s", name);
    }
    subscribe();
}
void wss::event::EventNotifier::stopService() {
    onStop();
}
void wss::event::EventNotifier::onStop() {
    ioService.stop();
    running = false;
    threadGroup.interrupt_all();
}

void wss::event::EventNotifier::handleMessageQueue() {
    const auto &ready = [this](SendStatus status) {
      if (!enableRetry) return true;

      const long diff = abs(std::time(nullptr) - status.sendTime);
      return diff >= intervalSeconds;
    };

    while (running) {
        // @TODO play around this time, check performance and cpu consumption
        // probably we should use notifiers, calculate data additions or average send times, instead of just checking every .5 seconds
        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));

        std::vector<SendStatus> bulk;

        try {
            size_t extract;
            const size_t approxSize = sendQueue.size_approx();
            if (approxSize > maxParallelWorkers) {
                // maximum connections per cycle
                extract = maxParallelWorkers;
            } else {
                extract = approxSize;
            }

            bulk.resize(extract);
            sendQueue.try_dequeue_bulk(bulk.begin(), bulk.size());
        } catch (const std::exception &e) {
            L_DEBUG_F("Event::Send", "Can't dequeue bulk: %s", e.what());
            continue;
        }

        int i = 0;
        for (auto it: bulk) {
            if (!ready(it)) {
                sendQueue.enqueue(std::move(it));
                continue;
            }

            auto sender = boost::thread([this, s = std::move(it)]() {
              SendStatus status = s;
              status.hasSent = status.target->send(status.payload, status.sendResult);
              if (enableRetry && !status.hasSent) {
                  std::stringstream ss;
                  ss << "Can't send message to target " << status.target->getType() << ": " << status.sendResult;
                  Logger::get().debug("Event-Send", ss.str());

                  // if tries < maxRetries
                  if (status.sendTries < maxRetries) {
                      status.sendTries++;
                      status.sendTime = std::time(nullptr);
                      sendQueue.enqueue(status);
                  } else {
                      // can't send over maxTries times
                      // notify listeners
                      for (auto &listener: sendErrorListeners) {
                          listener(std::move(status.payload));
                      }
                  }
              } else {
                  const char *typeName = status.target->getType().c_str();
                  L_DEBUG_F("Event::Send", "Message has sent to target: %s", typeName);
              }
            });
            // we don't need to join this thread back, we just need to send, and if not, re-enqueue payload
            sender.detach();

            i++;
        }

        bulk.clear();
        if (i > 0) {
            L_DEBUG_F("Event::Send", "Prepared %d messages", i);
        }
    }
}
void wss::event::EventNotifier::addMessage(wss::MessagePayload payload) {
    for (auto &target: targets) {
        sendQueue.enqueue(SendStatus(target.second, payload, 0L, 1));
    }
}

void wss::event::EventNotifier::onMessage(wss::MessagePayload &&payload) {
    if (payload.isFromBot() && not wss::Settings::get().event.sendBotMessages) {
        L_DEBUG("Event::Enqueue", "Skipping Bot message (sender=0)");
        return;
    }

    for (const auto &ignoredType: wss::Settings::get().event.ignoreTypes) {
        if (toolboxpp::strings::equalsIgnoreCase(ignoredType, payload.getType())) {
            return;
        }
    }

    ioService.post(boost::bind(&EventNotifier::addMessage, this, payload));
}
void wss::event::EventNotifier::addErrorListener(wss::event::EventNotifier::OnSendError listener) {
    sendErrorListeners.push_back(listener);
}
/*
 *  INFO ssh: Execute: mkdir -p /vagrant (sudo=true)
DEBUG ssh: stderr: /var/tmp/sclxtHVCU: line 8: -E: command not found
 */



