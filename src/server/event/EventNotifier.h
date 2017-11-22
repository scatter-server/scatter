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
#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include "../chat/ChatServer.h"
#include "EventConfig.h"
#include "PostbackTarget.h"

namespace wss {
namespace event {

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
        } //@TODO другие варианты

        throw std::runtime_error("Unsupported target type: " + type);
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

    void send(const wss::MessagePayload &payload) {
        for (auto &target: targets) {
            target.second->send(payload);
        }
    }

    void subscribe() {
        threadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &ioService)
        );

        ws.addMessageListener(std::bind(&EventNotifier::onMessage, this, std::placeholders::_1));
        ws.addStopListener(std::bind(&EventNotifier::onStop, this));
        ioService.post(boost::bind(&EventNotifier::dequeueMessages, this));
    }

    void onStop() {
        ioService.stop();
        running = false;
    }

 private:
    std::condition_variable handler, uHandler;
    std::condition_variable_any emitter;

    std::atomic_bool running;

    wss::ChatServer &ws;
    int maxRetries;
    int intervalSeconds;
    boost::asio::io_service ioService;
    boost::thread_group threadGroup;
    boost::asio::io_service::work work;

    std::unordered_map<std::string, std::shared_ptr<Target>> targets;
    std::unordered_map<std::string, std::queue<wss::MessagePayload>> sendQueue;
    std::unordered_map<std::string, std::queue<wss::MessagePayload>> undeliveredQueue;
    std::unordered_map<std::string, int> undeliveredRetries;

    void sendTry(const std::string &targetName, std::queue<wss::MessagePayload> queue) {
        // iterate target queue
        while (!queue.empty()) {
            // if can't send
            if (!targets[targetName]->send(queue.front())) {

                int tries = undeliveredRetries.count(targetName) ? undeliveredRetries[targetName] : 0;

                // if tries < 3
                if (tries < 3) {
                    tries++;
                    // adding to undelivered queue, we will try to resend later
                    undeliveredQueue[targetName].push(queue.front());
                    undeliveredRetries[targetName] = tries;
                } else {
                    // else we exceed retries, erasing counters and just pop message from queue
                    undeliveredRetries.erase(targetName);
                }
            }

            // remove top element
            queue.pop();
        }
    }

    void dequeueMessages() {
        while (running) {
            handler.notify_one();
            std::mutex m;
            m.lock();
            emitter.wait(m);
            // iterate all targets
            for (auto &sq: sendQueue) {
                sendTry(sq.first, sq.second);
            }
            m.unlock();

            boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
        }
    }

    void dequeueUndeliveredMessages() {
        while (running) {
            // iterate all targets
            for (auto &sq: undeliveredQueue) {
                sendTry(sq.first, sq.second);
            }

            boost::this_thread::sleep_for(boost::chrono::seconds(intervalSeconds));
        }
    }

    void addMessage(const wss::MessagePayload &payload) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);

        handler.wait(lock);
        for (auto &target: targets) {
            sendQueue[target.first].push(payload);
        }

        emitter.notify_one();
    }

    void onMessage(const wss::MessagePayload &payload) {
        ioService.post(boost::bind(&EventNotifier::addMessage, this, payload));
    }

};

}
}

#endif //WSSERVER_EVENTNOTIFIER_H
