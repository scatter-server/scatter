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
#include "server_ws.hpp"

namespace wss {

typedef unsigned long UserId;

using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using WsMessageStream = WsServer::SendStream;
using WsConnectionPtr = std::shared_ptr<WsServer::Connection>;
using WsMessagePtr = std::shared_ptr<WsServer::Message>;
using json = nlohmann::json;

}

#endif //WSSERVER_TYPES_H
