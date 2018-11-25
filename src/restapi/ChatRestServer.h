/**
 * scatter
 * ChatRestApi.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_CHATRESTAPI_H
#define SCATTER_CHATRESTAPI_H

#include <functional>
#include <memory>
#include "..//scatter_core.h"
#include "RestServer.h"
#include "../base/auth/Auth.h"
#include "json.hpp"

namespace wss {

using namespace std::placeholders;

class ChatRestServer : public RestServer {
 public:
    explicit ChatRestServer(
        std::shared_ptr<ChatServer> &chatMessageServer,
        const std::string &crtPath, const std::string &keyPath
    );

    ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer,
                   const std::string &crtPath, const std::string &keyPath,
                   const std::string &host, unsigned short port);

    explicit ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer);
    ChatRestServer(std::shared_ptr<ChatServer> &chatMessageServer, const std::string &host, unsigned short port);
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

    /// \brief Check server is online
    /// \param response
    /// \param request
    ACTION_DEFINE(actionStatus);

 protected:
    void createEndpoints() override;

 private:
    std::shared_ptr<ChatServer> m_ws;
};
}

#endif //SCATTER_CHATRESTAPI_H
