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
#include "../chat/ChatMessageServer.h"
#include "EventConfig.h"
#include "PostbackTarget.h"
#include "../StandaloneService.h"

namespace wss {
namespace event {

using toolboxpp::Logger;

class EventNotifier : public virtual wss::StandaloneService {
 private:
    std::shared_ptr<Target> createTargetByConfig(const nlohmann::json &json);
    void onStop();

    /**
     * Locks overview:
     * onMessage -> addMessage uniqueLock[sendQueue[w]]
     * dequeueMessages -> uniqueLock[sendQueue[rw]]
     */
    void subscribe();

 public:
    explicit EventNotifier(std::shared_ptr<wss::ChatMessageServer> ws);
    ~EventNotifier();

    void setRetryIntervalSeconds(int seconds);
    void setMaxTries(int tries);
    void addTarget(const nlohmann::json &targetConfig);
    void addTarget(const std::shared_ptr<Target> &target);
    void addTarget(std::shared_ptr<Target> &&target);

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

 private:
    struct SendStatus {
      wss::MessagePayload payload;
      std::time_t sendTime;
      int sendTries;
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

    std::shared_ptr<wss::ChatMessageServer> ws;
    int maxRetries;
    int intervalSeconds;
    boost::asio::io_service ioService;
    boost::thread_group threadGroup;
    boost::asio::io_service::work work;

    std::unordered_map<std::string, std::shared_ptr<Target>> targets;
    std::unordered_map<std::string, std::deque<SendStatus>> sendQueue;

    boost::shared_mutex sendQueueMutex;

    /**
     * @return string hex number thread id
     */
    const char *getThreadId();

    /**
     * Calling on event
     * Send post to io_service with payload
     * @param payload
     */
    void onMessage(wss::MessagePayload &&payload);
    /**
     * Calling after event in separate thread (io_service.post)
     * Adds message to send queue
     * @param payload
     */
    void addMessage(wss::MessagePayload payload);
    /**
     * Running in separate thread with sleep timer
     * Handling queued messages and trying to send them
     */
    void handleMessageQueue();
    /**
     * Calling inside dequeueMessage when trying to send message
     * Sends messages to targets, if cannot then adds its to sendQueue again with new schedule time and trying-counter
     * @see maxRetries
     * @see intervalSeconds
     * @param targetQueueMap
     */
    void sendTry(std::unordered_map<std::string, std::deque<SendStatus>> targetQueueMap);

};

}
}

#endif //WSSERVER_EVENTNOTIFIER_H
