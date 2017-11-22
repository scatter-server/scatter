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
 * available:
 * type: basic
 *      user: basic_username
 *      password: basic_password
 *
 * type: header,
 *      name: header_name
 *      value: header_value
 *
 * type: bearer
 *      value: bearer_token
 */
class WebAuth {
 public:
    virtual std::string getType() {
        return "noauth";
    }

    virtual void performAuth(wss::web::Request &request) {
        //do nothing
    };
};


class BasicAuth : public WebAuth {
 public:
    BasicAuth(std::string &&username, std::string &&password) :
        username(std::move(username)),
        password(std::move(password)) {
    }
    BasicAuth(const std::string &username, const std::string &password) :
        username(username),
        password(password) {
    }

    std::string getType() override {
        return "basic";
    }
    void performAuth(wss::web::Request &request) override {
        std::stringstream ss;
        ss << username << ":" << password;
        const std::string glued = ss.str();
        const std::string encoded = wss::helpers::base64_encode(
            reinterpret_cast<const unsigned char *>(glued.c_str()),
            static_cast<unsigned int>(glued.length())
        );

        request.addHeader({"Authorization", "Basic " + encoded});
    }
 private:
    std::string username, password;
};

class HeaderAuth : public WebAuth {
 public:
    HeaderAuth(std::string &&headerName, std::string &&value) :
        name(std::move(headerName)),
        value(std::move(value)) {
    }

    HeaderAuth(const std::string &headerName, const std::string &value) :
        name(headerName),
        value(value) {
    }

    std::string getType() override {
        return "header";
    }
    void performAuth(wss::web::Request &request) override {
        request.addHeader({name, value});
    }

 private:
    std::string name, value;
};

class BearerAuth : public HeaderAuth {
 public:
    explicit BearerAuth(std::string &&value) :
        HeaderAuth("Authorization", std::string("Bearer " + value)) { }

    std::string getType() override {
        return "bearer";
    }
    void performAuth(wss::web::Request &request) override {
        HeaderAuth::performAuth(request);
    }
};

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
    virtual bool send(const wss::MessagePayload &payload, std::string *error = nullptr) = 0;
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
