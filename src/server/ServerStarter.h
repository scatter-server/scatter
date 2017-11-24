/**
 * wsserver
 * WsServer.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_WSSERVER_H
#define WSSERVER_WSSERVER_H

#include <csignal>
#include <istream>
#include <iostream>
#include <ostream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <functional>
#include "json.hpp"

#include "cmdline.hpp"
#include "event/EventNotifier.h"
#include "restapi/ChatRestServer.h"
#include "chat/ChatMessageServer.h"
#include "StandaloneService.h"

namespace wss {

using json = nlohmann::json;

class ServerStarter {
 private:
    bool valid = true;
    bool isConfigTest = false;
    cmdline::parser args;

    std::vector<std::shared_ptr<wss::StandaloneService>> services;

 private: //services
    std::shared_ptr<wss::ChatMessageServer> webSocket;
    std::shared_ptr<wss::ChatRestServer> restServer;
    std::shared_ptr<wss::event::EventNotifier> eventNotifier;

 public:
    ServerStarter(int argc, char **argv);
    ~ServerStarter();

    void stop();
    void run();
    bool isValid();

 private:
    static void signalHandler(int signum);

    // running commands, as we have a multiple service with one interface that can control
    // services thread, we will store using this function - commands, see below
    template<class T>
    void runService(std::shared_ptr<T> &s) {
        static_assert(std::is_base_of<wss::StandaloneService, T>::value,
                      "Service must be instance of wss::StandaloneService");

        if (isConfigTest || !valid) return;
        services.push_back(s);
    }

    bool hasKey(nlohmann::json &obj, const std::string &key);

    void configureChat(nlohmann::json &config);
    void configureServer(nlohmann::json &config);
    bool configureEventNotifier(nlohmann::json &config);
};

}
#endif //WSSERVER_WSSERVER_H
