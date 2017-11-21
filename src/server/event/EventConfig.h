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
#include "../chat/Message.h"
#include "../web/HttpClient.h"
#include "PostbackTarget.h"

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
    WebAuth() = default;
    virtual std::string getType() = 0;
    virtual void performAuth(wss::web::QueryBuilder &queryBuilder) = 0;
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

    string getType() override {
        return "basic";
    }
    void performAuth(wss::web::QueryBuilder &queryBuilder) override {
//        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long) CURLAUTH_BASIC);
//        curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
//        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
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

    string getType() override {
        return "header";
    }
    void performAuth(CURL *curl) override {
        struct curl_slist *headers = nullptr;
        const std::string authHeader = std::string(name + ": " + value);
        curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    }

 private:
    std::string name, value;
};

class BearerAuth : public HeaderAuth {
 public:
    BearerAuth(const std::string &value) :
        HeaderAuth("Authorization", std::string("Bearer " + value)) { }

    string getType() override {
        return "bearer";
    }
    void performAuth(CURL *curl) override {
        HeaderAuth::performAuth(curl);
    }
};

/**
 * Available:
 * postback:
 *  "url": "http://example.com/postback",
 *
 * Every target can require an Auth config
 */
class WebTarget {
 public:
    static std::unique_ptr<WebTarget> create(const nlohmann::json &config) {
        using namespace toolboxpp::strings;
        if (equalsIgnoreCase(config.at("type"), "postback")) {
            std::unique_ptr<WebTarget> target = std::make_unique<wss::event::PostbackTarget>(config);

            if (config.find("auth") != config.end()) {
                const std::string authType = config["auth"].at("type");

                if (equalsIgnoreCase(authType, "basic")) {
                    target->setAuth(BasicAuth(
                        config["auth"].at("user"),
                        config["auth"].at("password")
                    ));
                } else if (equalsIgnoreCase(authType, "header")) {
                    target->setAuth(HeaderAuth(
                        config["auth"].at("name"),
                        config["auth"].at("value")
                    ));
                } else if (equalsIgnoreCase(authType, "bearer")) {
                    target->setAuth(BearerAuth(
                        config["auth"].at("value")
                    ));
                } else {
                    throw std::runtime_error("Unsupported auth method: " + authType);
                }
            }

            return target;

        }

        throw std::runtime_error("Unsupported target " + config.at("type"));
    }

    WebTarget(nlohmann::json config) {

    }

    virtual bool send(const wss::MessagePayload &payload) = 0;
    virtual std::string getType() = 0;

    void setAuth(WebAuth &&auth) {
        this->auth = std::move(auth);
    }

    void performAuth() {
//        if (auth == nullptr) return;
//        auth->performAuth(client.getCurl());
    }

 protected:
    wss::web::HttpClient &getClient() {
        return client;
    }

    WebAuth getAuth() {
        return auth;
    }

 private:
    WebAuth auth;
    wss::web::HttpClient client;
};

}
}

#endif //WSSERVER_EVENTCONFIG_H
