/**
 * wsserver
 * types.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_DEFS_H
#define WSSERVER_DEFS_H

#include <memory>
#include <thread>
#include <string>
#include "json.hpp"

#include "base/ws/WebsocketServer.hpp"

namespace wss {

using user_id_t = unsigned long;
using conn_id_t = unsigned long;

using WsBase = wss::server::websocket::SocketServerBase;
using WsServer = wss::server::websocket::SocketServer;
using WssServer = wss::server::websocket::SocketServerSecure;

using WsMessageStream = WsBase::SendStream;
using WsConnectionPtr = std::shared_ptr<WsBase::Connection>;
using WsMessagePtr = std::shared_ptr<WsBase::Message>;
using json = nlohmann::json;


template<typename T>
using UserMap = std::unordered_map<user_id_t, T>;

template<typename T>
using ConnectionMap = std::unordered_map<conn_id_t, T>;

}

#endif //WSSERVER_DEFS_H
