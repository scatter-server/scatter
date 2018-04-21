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
#include "../base/StandaloneService.h"
#include "Target.hpp"
#include "PostbackTarget.h"
#include "concurrentqueue.h"

namespace wss {
namespace event {

using toolboxpp::Logger;

class EventNotifier : public virtual wss::StandaloneService {
 private:
    /// \brief Creates event target instance from global server config file.
    /// \param json Part of config.
    /// \return shared pointer of target
    std::shared_ptr<Target> createTargetByConfig(const nlohmann::json &json);
    void onStop();

    /// \brief Start the service. Producer: onMessage(), consumer: handleMessageQueue(), but consumer can be a producer at the same time, cause re-enqueues undelivered messages
    /// \todo calculate performance, cause here we create hardcoded 4 threads,
    ///     but event emitter (wss::ChatMessageServer) can generate message from the N threads at the same time.
    ///     Potential bottleneck
    void subscribe();

 public:
    typedef std::function<void(wss::MessagePayload &&)> OnSendError;

    /// \brief Constructs event notifier with shared_ptr of main chat server.
    /// \param ws
    explicit EventNotifier(std::shared_ptr<wss::ChatMessageServer> &ws);
    ~EventNotifier();

    /// \brief Set retry interval seconds.
    /// \param seconds seconds between retries sending event
    void setRetryIntervalSeconds(int seconds);

    /// \brief Set maximum retries to send event
    /// \param tries Tries number
    void setMaxTries(int tries);

    /// \brief Adds event target
    /// \param targetConfig
    void addTarget(const nlohmann::json &targetConfig);
    void addTarget(const std::shared_ptr<Target> &target);
    void addTarget(std::shared_ptr<Target> &&target);

    /// \brief Error listener. Called when can't send message to target #maxRetries times
    /// \param listener
    void addErrorListener(wss::event::EventNotifier::OnSendError listener);

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

 private:
    struct SendStatus {
      std::shared_ptr<wss::event::Target> target;
      wss::MessagePayload payload;
      std::time_t sendTime;
      int sendTries;
      bool hasSent = false;
      std::string sendResult;

      SendStatus(std::shared_ptr<wss::event::Target> target,
                 wss::MessagePayload payload,
                 std::time_t sendTime,
                 int tries) :
          target(target),
          payload(payload),
          sendTime(sendTime),
          sendTries(tries) { }
      SendStatus() = default;
      ~SendStatus() = default;
      SendStatus(const SendStatus &another) = default;
      SendStatus(SendStatus &&another) = default;
      SendStatus &operator=(const SendStatus &another) = default;
      SendStatus &operator=(SendStatus &&another) = default;
    };

    std::atomic_bool running;

    std::shared_ptr<wss::ChatMessageServer> ws;
    const bool enableRetry;
    const uint32_t maxParallelWorkers;
    int maxRetries;
    int intervalSeconds;
    boost::asio::io_service ioService;
    boost::thread_group threadGroup;
    boost::asio::io_service::work work;

    std::unordered_map<std::string, std::shared_ptr<Target>> targets;
    std::unordered_map<std::string, std::shared_ptr<Target>> targetsUndelivered;
    moodycamel::ConcurrentQueue<SendStatus> sendQueue;
    std::vector<wss::event::EventNotifier::OnSendError> sendErrorListeners;

    /// \brief Calling on event
    /// Send post to io_service with payload
    /// \param payload
    /// \param hasSent is recipient online and recieved websocket frame
    void onMessage(wss::MessagePayload &&payload);

    /// \brief Calling after event in separate thread (io_service.post)
    /// Adds message to send queue
    /// \param payload
    void addMessage(wss::MessagePayload payload);

    /// \brief Running in separate thread with sleep timer. Handling queued messages and trying to send them
    void handleMessageQueue();
};

}
}

#endif //WSSERVER_EVENTNOTIFIER_H
