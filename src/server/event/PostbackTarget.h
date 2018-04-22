/**
 * wsserver
 * PostbackTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_POSTBACKTARGET_H
#define WSSERVER_POSTBACKTARGET_H

#include <string>
#include "json.hpp"
#include "Target.hpp"
#include "../web/HttpClient.h"
#include "src/server/chat/ChatServer.h"
#include "../base/Auth.h"

namespace wss {
namespace event {

class PostbackTarget : public Target {
 public:
    /// \brief json object of postback configuration
    /// \param config
    explicit PostbackTarget(const json &config);

    bool send(const wss::MessagePayload &payload, std::string &error) override;
    std::string getType() override;

 protected:
    /// \brief Return http client
    /// \see wss::web::HttpClient
    /// \return
    wss::web::HttpClient &getClient();

    /// \brief Return auth object for authentication while sending event
    /// \see wss::WebAuth
    /// \return
    std::unique_ptr<wss::WebAuth> &getAuth();

 private:
    wss::web::Request::Method httpMethod;
    std::unique_ptr<wss::WebAuth> auth;
    wss::web::HttpClient client;
    std::string url;
    template<class T>
    void setAuth(T &&auth);

};

}
}

#endif //WSSERVER_POSTBACKTARGET_H
