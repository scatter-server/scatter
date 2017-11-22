/**
 * wsserver
 * EventNotifier.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_EVENTNOTIFIER_H
#define WSSERVER_EVENTNOTIFIER_H

#include <vector>
#include <string>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <algorithm>
#include <boost/thread.hpp>
#include <cmath>
#include <boost/asio/io_service.hpp>
#include "../chat/ChatServer.h"
#include "EventConfig.h"
#include "PostbackTarget.h"

namespace wss {
namespace event {

using toolboxpp::Logger;

class EventNotifier {
 public:
    explicit EventNotifier(wss::ChatServer &ws) :
        ws(ws),
        maxRetries(3),
        intervalSeconds(10),
        ioService(),
        threadGroup(),
        work(ioService),
        running(true) {
    }

    void setRetryIntervalSeconds(int seconds) {
        intervalSeconds = seconds;
    }

    void setMaxTries(int tries) {
        maxRetries = tries;
    }

    std::shared_ptr<Target> createTargetByConfig(const nlohmann::json &json) {
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

    void addTarget(const nlohmann::json &targetConfig) {
        addTarget(createTargetByConfig(targetConfig));
    }

    void addTarget(const std::shared_ptr<Target> &target) {
        if (!target->isValid()) {
            throw std::runtime_error(target->getErrorMessage());
        }
        targets.insert({target->getType(), target});
    }

    void addTarget(std::shared_ptr<Target> &&target) {
        if (!target->isValid()) {
            throw std::runtime_error(target->getErrorMessage());
        }
        targets.insert({target->getType(), std::move(target)});
    }

    /**
     * Locks overview:
     * onMessage -> addMessage uniqueLock[sendQueue[w]]
     * dequeueMessages -> uniqueLock[sendQueue[rw]]
     */
    void subscribe() {
        for (int i = 0; i < 4; i++) {
            threadGroup.create_thread(
                boost::bind(&boost::asio::io_service::run, &ioService)
            );
        }

        ws.addMessageListener(std::bind(&EventNotifier::onMessage, this, std::placeholders::_1));
        ws.addStopListener(std::bind(&EventNotifier::onStop, this));
        ioService.post(boost::bind(&EventNotifier::dequeueMessages, this));

        join();
    }

    void join() {
        threadGroup.join_all();
    }

    void onStop() {
        ioService.stop();
        running = false;
    }

 private:
    struct SendStatus {
      wss::MessagePayload payload;
      std::time_t sendTime = 0;
      int sendTries = 0;
      SendStatus() = default;
      ~SendStatus() = default;
      SendStatus(const SendStatus &another) = default;
      SendStatus(SendStatus &&another) = default;
      SendStatus &operator=(const SendStatus &another) {
          this->payload = another.payload;
          this->sendTries = another.sendTries;
          this->sendTime = another.sendTime;
          return *this;
      }
      SendStatus &operator=(SendStatus &&another) noexcept {
          this->payload = std::move(another.payload);
          this->sendTries = another.sendTries;
          this->sendTime = another.sendTime;
          return *this;
      }
    };


    std::atomic_bool running;

    wss::ChatServer &ws;
    int maxRetries;
    int intervalSeconds;
    boost::asio::io_service ioService;
    boost::thread_group threadGroup;
    boost::asio::io_service::work work;

    std::unordered_map<std::string, std::shared_ptr<Target>> targets;
    std::unordered_map<std::string, std::deque<SendStatus>> sendQueue;

    boost::shared_mutex dataMutex;

    const char *getThreadId() {
        std::stringstream ss;
        ss << boost::this_thread::get_id();
        const char *tid = ss.str().c_str();
        return tid;
    }

    void sendTry(std::unordered_map<std::string, std::deque<SendStatus>> targetQueueMap) {
        boost::shared_lock<boost::shared_mutex> lock(dataMutex);

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

//        targetQueueMap.clear();
    }

    void dequeueMessages() {
        const auto &ready = [this](SendStatus status) {
          const long diff = abs(std::time(nullptr) - status.sendTime);
          return diff >= intervalSeconds;
        };

        while (running) {
            std::unordered_map<std::string, std::deque<SendStatus>> tmp;

            {
                boost::shared_lock<boost::shared_mutex> slock(dataMutex);
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
                L_DEBUG_F("Event-DeQueue", "[%s] DeQueue messages: %lu", getThreadId(), tmp.size());
                sendTry(tmp);
                tmp.clear();
            }

            boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
        }
    }

    void addMessage(const wss::MessagePayload &payload) {
        boost::upgrade_lock<boost::shared_mutex> lock(dataMutex);
        boost::upgrade_to_unique_lock<boost::shared_mutex> uniqueLock(lock);
        for (auto &target: targets) {
            sendQueue[target.first].push_back(SendStatus{payload, 0L, 1});
        }
        L_DEBUG_F("Event-Enqueue", "[%s] Adding message to send queue", getThreadId());
    }

    void onMessage(const wss::MessagePayload &payload) {
        ioService.post(boost::bind(&EventNotifier::addMessage, this, payload));
    }

};

}
}

#endif //WSSERVER_EVENTNOTIFIER_H
