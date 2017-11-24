/**
 * wsserver
 * EventNotifier.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "EventNotifier.h"

wss::event::EventNotifier::EventNotifier(std::shared_ptr<wss::ChatMessageServer> ws) :
    ws(ws),
    maxRetries(3),
    intervalSeconds(10),
    ioService(),
    threadGroup(),
    work(ioService),
    running(true) {
}

wss::event::EventNotifier::~EventNotifier() {
    running = false;
    onStop();
}

std::shared_ptr<wss::event::Target> wss::event::EventNotifier::createTargetByConfig(const nlohmann::json &json) {
    using namespace toolboxpp::strings;
    const auto &eq = equalsIgnoreCase;

    // exception safe method: value()
    const std::string type = json.value("type", "");
    if (type.empty()) {
        throw std::runtime_error("Target type required");
    }

    if (eq(type, "postback")) {
        return std::make_shared<wss::event::PostbackTarget>(json);
    } else {
        throw std::runtime_error("Unsupported target type: " + type);
    }
}

void wss::event::EventNotifier::setRetryIntervalSeconds(int seconds) {
    intervalSeconds = seconds;
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
const char *wss::event::EventNotifier::getThreadId() {
    std::stringstream ss;
    ss << boost::this_thread::get_id();
    const char *tid = ss.str().c_str();
    return tid;
}
void wss::event::EventNotifier::sendTry(std::unordered_map<std::string,
                                                           std::deque<wss::event::EventNotifier::SendStatus>> targetQueueMap) {
    boost::shared_lock<boost::shared_mutex> lock(sendQueueMutex);

    // iterate target-queue
    for (auto &&targetQueue: targetQueueMap) {
        const std::string targetName = targetQueue.first;
        while (!targetQueue.second.empty()) {
            // if can't send
            SendStatus status = targetQueue.second.front();
            // remove top element
            targetQueue.second.pop_front();

            std::string sendResult;
            if (!targets[targetName]->send(status.payload, sendResult)) {

                std::stringstream ss;
                ss << "[" << getThreadId() << "] Can't send message to target " << targetName << ": " << sendResult;
                Logger::get().debug("Event-Send", ss.str());

                L_DEBUG_F("Event-Send", "Send tries: %d", status.sendTries);

                // if tries < maxRetries
                if (status.sendTries < maxRetries) {
                    status.sendTries++;
                    status.sendTime = std::time(nullptr);
                    L_DEBUG_F("Event-Send",
                              "Adding to queue again, will try again after %d seconds",
                              intervalSeconds);
                    sendQueue[targetName].push_back(std::move(status));
                } else {
                    L_DEBUG_F("Event-Send", "After %d times, removing from queue.", maxRetries);
                }
            } else {
                L_DEBUG_F("Event-Send", "[%s] Message has sent to target: %s", getThreadId(), targetName.c_str());
            }
        }
    }
}
void wss::event::EventNotifier::handleMessageQueue() {
    const auto &ready = [this](SendStatus status) {
      const long diff = abs(std::time(nullptr) - status.sendTime);
      return diff >= intervalSeconds;
    };

    while (running) {
        std::unordered_map<std::string, std::deque<SendStatus>> tmp;

        {
            boost::shared_lock<boost::shared_mutex> slock(sendQueueMutex);
            // iterate all targets
            for (auto &sq: sendQueue) {
                for (auto &status: sendQueue[sq.first]) {
                    const long diff = abs(std::time(nullptr) - status.sendTime);
                    if (ready(status)) {
                        tmp[sq.first].push_back(status);
                    }
                }

                sq.second.erase(
                    std::remove_if(sq.second.begin(), sq.second.end(), ready),
                    sq.second.end()
                );
            }

        }

        if (!tmp.empty()) {
            L_DEBUG_F("Event-HandleMQ", "Messages to send: %lu", tmp.size());
            sendTry(tmp);
            tmp.clear();
        }

        boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
    }
}
void wss::event::EventNotifier::addMessage(wss::MessagePayload payload) {
    boost::upgrade_lock<boost::shared_mutex> lock(sendQueueMutex);
    boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
    for (auto &target: targets) {
        sendQueue[target.first].push_back(SendStatus{payload, 0L, 1});
    }
    L_DEBUG("Event-Enqueue", "Adding message to send queue");
}
void wss::event::EventNotifier::onMessage(wss::MessagePayload &&payload) {
    ioService.post(boost::bind(&EventNotifier::addMessage, this, payload));
}

