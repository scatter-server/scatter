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
#include <boost/thread.hpp>
#include "../chat/ChatServer.h"
#include "EventConfig.h"

namespace wss {
namespace event {

class EventNotifier {
 public:
    explicit EventNotifier(wss::ChatServer *ws) :
        ws(ws),
        maxRetries(3),
        intervalSeconds(10) {

    }

    void setRetryIntervalSeconds(int seconds) {
        intervalSeconds = seconds;
    }

    void setMaxTries(int tries) {
        maxRetries = tries;
    }

    void addTarget(std::unique_ptr<WebTarget> target) {
        targets.push_back(target);
    }

    void addTarget(std::unique_ptr<WebTarget> &&target) {
        targets.push_back(std::move(target));
    }

    void send(const wss::MessagePayload &payload) {
        for (auto &target: targets) {
            target->send(payload);
        }
    }

    void subscribe() {
        ws->addListener(std::bind(&wss::event::EventNotifier::onMessage, this, std::placeholders::_1));
    }

 private:
    wss::ChatServer *ws;
    int maxRetries;
    int intervalSeconds;

    std::vector<std::unique_ptr<WebTarget>> targets;
    std::shared_ptr<wss::ChatServer> server;
    std::unordered_map<std::string, std::queue<wss::MessagePayload>> undeliveredEvents;
    std::unordered_map<std::string, int> undeliveredRetries;

    void onMessage(const wss::MessagePayload &payload) {
        send(payload);
    }

};

}
}

#endif //WSSERVER_EVENTNOTIFIER_H
