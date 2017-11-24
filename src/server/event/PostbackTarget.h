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
#include "../chat/ChatMessageServer.h"

namespace wss {
namespace event {

class PostbackTarget : public Target {
 public:
    explicit PostbackTarget(const json &config);
    bool send(const wss::MessagePayload &payload, std::string &error) override;
    std::string getType() override;

 protected:
    wss::web::HttpClient &getClient();
    std::unique_ptr<WebAuth> &getAuth();

 private:
    wss::web::Request request;
    std::unique_ptr<WebAuth> auth;
    wss::web::HttpClient client;
    std::string url;
    template<class T>
    void setAuth(T &&auth);

};

}
}

#endif //WSSERVER_POSTBACKTARGET_H
