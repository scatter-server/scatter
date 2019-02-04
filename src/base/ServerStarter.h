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
#include "../event/EventNotifier.h"
#include "../restapi/ChatRestServer.h"
#include "..//chat/ChatServer.h"
#include "StandaloneService.h"
#include "Settings.hpp"

namespace wss {

using json = nlohmann::json;

class ServerStarter {
 private:
    bool m_valid = true;
    bool m_isConfigTest = false;
    cmdline::parser m_args;

    std::vector<std::shared_ptr<wss::StandaloneService>> m_services;

    //services
    std::shared_ptr<wss::ChatServer> m_webSocket;
    std::shared_ptr<wss::ChatRestServer> m_restServer;
    std::shared_ptr<wss::event::EventNotifier> m_eventNotifier;

 public:
    ServerStarter(int argc, const char **argv);
    virtual ~ServerStarter();

    /// \brief Stop all services
    void stop();
    /// \brief Start all services
    void run();
    /// \brief valid state
    /// \return false if not configured
    bool isValid();

 private:
    /// \brief POSIX singal handler
    /// \param signum
    static void signalHandler(int signum);

    /// \brief running commands, as we have a multiple service with one interface that can control services thread, we will store using this function - commands, see below
    /// \tparam T StandaloneService interface implementer
    /// \param s StandaloneService shared instance
    template<class T>
    void enqueueService(std::shared_ptr<T> &s) {
        static_assert(std::is_base_of<wss::StandaloneService, T>::value,
                      "Service must be instance of wss::StandaloneService");

        if (m_isConfigTest || !m_valid) return;
        m_services.push_back(s);
    }

    /// \brief Check json object contains entire key
    /// \param obj json
    /// \param key string
    /// \return bool
    bool hasKey(nlohmann::json &obj, const std::string &key);
    /// \brief Setup chat settings
    /// \param settings
    void configureChat(wss::Settings &settings);
    /// \brief Setup common server settings
    /// \param settings
    void configureServer(wss::Settings &settings);
    /// \brief Setup event notifier settings
    /// \param settings
    /// \return false if not valid config
    bool configureEventNotifier(wss::Settings &settings);
};

}
#endif //WSSERVER_WSSERVER_H
