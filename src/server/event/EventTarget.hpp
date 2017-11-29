/**
 * wsserver
 * EventConfig.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_EVENTCONFIG_H
#define WSSERVER_EVENTCONFIG_H

#include <string>
#include <curl/curl.h>
#include "../helpers/base64.h"
#include "../chat/Message.h"
#include "../web/HttpClient.h"

namespace wss {
namespace event {

/**
 * Available:
 * postback: PostbackTarget
 *  "url": "http://example.com/postback",
 *
 */
class Target {
 public:
    explicit Target(const nlohmann::json &config) :
        config(config),
        valid(true),
        errorMessage() { }
    virtual bool send(const wss::MessagePayload &payload, std::string &error) = 0;
    virtual std::string getType() = 0;

    bool isValid() const {
        return valid;
    }

    std::string getErrorMessage() const {
        return errorMessage;
    }

 protected:
    void setErrorMessage(const std::string &msg) {
        errorMessage = msg;
        valid = false;
    }

    void setErrorMessage(std::string &&msg) {
        errorMessage = std::move(msg);
        valid = false;
    }
 private:
    nlohmann::json config;
    bool valid;
    std::string errorMessage;
};

}
}

#endif //WSSERVER_EVENTCONFIG_H
