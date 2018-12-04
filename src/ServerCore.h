/**
 * scatter
 * scatter_core.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_DEFS_H
#define SCATTER_DEFS_H

#include <memory>
#include <thread>
#include <string>
#include <json.hpp>
#include <scatter/ScatterCore.h>

#include "base/ws/WebsocketServer.hpp"

namespace wss {

using WsBase = wss::server::websocket::SocketServerBase;
using WsServer = wss::server::websocket::SocketServer;
using WssServer = wss::server::websocket::SocketServerSecure;

using WsMessageStream = WsBase::SendStream;
using WsConnectionPtr = std::shared_ptr<WsBase::Connection>;
using WsMessagePtr = std::shared_ptr<WsBase::Message>;

}

#endif //SCATTER_DEFS_H
