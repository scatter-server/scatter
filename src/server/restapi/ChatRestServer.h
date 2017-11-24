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
#include <memory>
#include "../defs.h"
#include "RestServer.h"
#include "json.hpp"

namespace wss {

using namespace std::placeholders;

class ChatRestServer : public RestServer {
 public:
    explicit ChatRestServer(std::shared_ptr<ChatMessageServer> chatMessageServer);
    ChatRestServer(std::shared_ptr<ChatMessageServer> chatMessageServer, const std::string &host, unsigned short port);
 protected:
    // actions
    ACTION_DEFINE(actionListConnected);
    ACTION_DEFINE(actionSendMessage);

 protected:
    void createEndpoints() override;

 private:
    void setError(HttpResponse &response, HttpStatus status, int code, const std::string &message);
    void setError(HttpResponse &response, HttpStatus status, int code, std::string &&message);
    std::shared_ptr<ChatMessageServer> chatMessageServer;
};
}

#endif //WSSERVER_CHATRESTAPI_H
