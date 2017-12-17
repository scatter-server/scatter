/**
 * wsserver
 * types.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_TYPES_H
#define WSSERVER_TYPES_H

#include <memory>
#include <thread>
#include <string>
#include "json.hpp"

#ifdef USE_SECURE_SERVER
#include "server_wss.hpp"
#else
#include "server_ws.hpp"
#endif

namespace wss {

using user_id_t = unsigned long;
using ConnectionId = unsigned long;

#ifdef USE_SECURE_SERVER
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WSS>;
#else
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
#endif

using WsMessageStream = WsServer::SendStream;
using WsConnectionPtr = std::shared_ptr<WsServer::Connection>;
using WsMessagePtr = std::shared_ptr<WsServer::Message>;
using json = nlohmann::json;


template<typename T>
using UserMap = std::unordered_map<user_id_t, T>;

template<typename T>
using ConnectionMap = std::unordered_map<ConnectionId, T>;

}

#endif //WSSERVER_TYPES_H
