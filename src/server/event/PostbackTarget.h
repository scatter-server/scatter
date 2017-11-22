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
#include "EventConfig.h"
#include "../web/HttpClient.h"
#include "../chat/ChatServer.h"

namespace wss {
namespace event {

class PostbackTarget : public Target {
 public:
    explicit PostbackTarget(const json &config);
    bool send(const wss::MessagePayload &payload) override;
    std::string getType() override;

 protected:
    wss::web::HttpClient &getClient();
    WebAuth getAuth();

 private:
    wss::web::Request request;
    WebAuth auth;
    wss::web::HttpClient client;
    std::string url;

    void setAuth(WebAuth &&auth);

};

}
}

#endif //WSSERVER_POSTBACKTARGET_H
