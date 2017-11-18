/**
 * wsserver
 * ChatRestApi.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_CHATRESTAPI_H
#define WSSERVER_CHATRESTAPI_H

#include <functional>
#include "../defs.h"
#include "RestServer.h"
#include "json.hpp"

namespace wss {

using namespace std::placeholders;

class ChatRestApi : public RestServer {
 public:
    ChatRestApi(const std::string &host, unsigned short port);

    void attachToChat(ChatServer *server);
 protected:
    ACTION_DEFINE(actionListConnected);
    ACTION_DEFINE(actionSendMessage);

 private:
    ChatServer *chatServer;
};
}

#endif //WSSERVER_CHATRESTAPI_H
