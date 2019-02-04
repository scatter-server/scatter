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
#include <httb/httb.h>
#include <json.hpp>
#include "Target.hpp"
#include "../chat/ChatServer.h"
#include "../base/auth/Auth.h"

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
    /// \see httb::client
    /// \return
    httb::client &getClient();

    /// \brief Return auth object for authentication while sending event
    /// \see wss::WebAuth
    /// \return
    std::unique_ptr<wss::Auth> &getAuth();

 private:
    template<class T>
    void setAuth(T &&auth);

    httb::request::method m_httpMethod;
    std::unique_ptr<wss::Auth> m_auth;
    httb::client m_client;
    std::string m_url;

};

}
}

#endif //WSSERVER_POSTBACKTARGET_H
