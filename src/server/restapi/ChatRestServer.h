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
#include "src/server/wsserver_core.h"
#include "RestServer.h"
#include "../base/Auth.h"
#include "json.hpp"

namespace wss {

using namespace std::placeholders;

class ChatRestServer : public RestServer {
 public:
    #ifdef USE_SECURE_SERVER
    explicit ChatRestServer(
        std::shared_ptr<ChatServer> &chatMessageServer,
        const std::string &crtPath, const std::string &keyPath
    );

    ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer,
                   const std::string &crtPath, const std::string &keyPath,
                   const std::string &host, unsigned short port);
    #else
    explicit ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer);
    ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer, const std::string &host, unsigned short port);
    #endif
 protected:
    // actions
    /// \brief Statistics list method: GET /stats
    /// \param response Http response
    /// \param request Http request
    ACTION_DEFINE(actionStats);

    /// \brief Entire user statistics method: GET /stat?id={UserId}
    /// \param response Http response
    /// \param request Http request
    ACTION_DEFINE(actionStat);

    /// \brief Entire user online checking method: GET /check-online?id={UserId}
    /// \param response Http response
    /// \param request Http request
    ACTION_DEFINE(actionCheckOnline);

    /// \brief Send message to recipient: POST /send-message
    /// content-type must be JSON and data must have a valid structure
    /// \see wss::MessagePayload
    /// \param response Http response
    /// \param request Http request
    ACTION_DEFINE(actionSendMessage);

 protected:
    void createEndpoints() override;

 private:
    std::shared_ptr<ChatServer> m_ws;
};
}

#endif //WSSERVER_CHATRESTAPI_H
