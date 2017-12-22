/**
 * wsserver
 * Auth.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include "Auth.h"

// NO-AUTH
std::string wss::WebAuth::getType() {
    return "noauth";
}
void wss::WebAuth::performAuth(wss::web::Request &) const {
    //do nothing
}
bool wss::WebAuth::validateAuth(const wss::web::Request &) const {
    return true;
}
std::string wss::WebAuth::getValue() const {
    return std::string();
}

// BASIC
wss::BasicAuth::BasicAuth(std::string &&username, std::string &&password) :
    username(std::move(username)),
    password(std::move(password)) {
}
wss::BasicAuth::BasicAuth(const std::string &username, const std::string &password) :
    username(username),
    password(password) {
}
std::string wss::BasicAuth::getType() {
    return "basic";
}
void wss::BasicAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({"Authorization", getValue()});
}
bool wss::BasicAuth::validateAuth(const wss::web::Request &request) const {
    return request.compareHeaderValue("Authorization", getValue());
}
std::string wss::BasicAuth::getValue() const {
    std::stringstream ss;
    ss << username << ":" << password;
    const std::string glued = ss.str();
    const std::string encoded = wss::helpers::base64_encode(
        reinterpret_cast<const unsigned char *>(glued.c_str()),
        static_cast<unsigned int>(glued.length())
    );
    return "Basic " + encoded;
}

// HEADER
wss::HeaderAuth::HeaderAuth(std::string &&headerName, std::string &&value) :
    name(std::move(headerName)),
    value(std::move(value)) {
}
wss::HeaderAuth::HeaderAuth(const std::string &headerName, const std::string &value) :
    name(headerName),
    value(value) {
}
std::string wss::HeaderAuth::getType() {
    return "header";
}
void wss::HeaderAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({name, getValue()});
}
bool wss::HeaderAuth::validateAuth(const wss::web::Request &response) const {
    return response.compareHeaderValue(name, getValue());
}
std::string wss::HeaderAuth::getValue() const {
    return value;
}

// BEARER
wss::BearerAuth::BearerAuth(std::string &&value) :
    HeaderAuth("Authorization", std::string("Bearer " + value)) { }

wss::BearerAuth::BearerAuth(const std::string &value) :
    HeaderAuth("Authorization", std::string("Bearer " + value)) {

}
std::string wss::BearerAuth::getType() {
    return "bearer";
}
bool wss::BearerAuth::validateAuth(const wss::web::Request &response) const {
    return HeaderAuth::validateAuth(response);
}
std::string wss::BearerAuth::getValue() const {
    return HeaderAuth::getValue();
}
void wss::BearerAuth::performAuth(wss::web::Request &request) const {
    HeaderAuth::performAuth(request);
}
std::unique_ptr<wss::WebAuth> wss::auth::createFromConfig(const nlohmann::json &config) {
    std::unique_ptr<WebAuth> out;

    if (config.is_null()) {
        out = std::make_unique<wss::WebAuth>();
        return out;
    }

    using namespace toolboxpp::strings;
    const auto &eq = equalsIgnoreCase;

    nlohmann::json data;

    if (config.find("auth") != config.end()) {
        data = config["auth"];
    } else if (config.find("type") != config.end()) {
        data = config;
    }

    const std::string authType = data.at("type").get<std::string>();

    if (eq(authType, "basic")) {
        out = std::make_unique<BasicAuth>(
            data.at("user").get<std::string>(),
            data.at("password").get<std::string>()
        );
    } else if (eq(authType, "header")) {
        out = std::make_unique<HeaderAuth>(
            data.at("name").get<std::string>(),
            data.at("value").get<std::string>()
        );
    } else if (eq(authType, "bearer")) {
        out = std::make_unique<BearerAuth>(
            data.at("value").get<std::string>()
        );
    } else {
        out = std::make_unique<wss::WebAuth>();
    }

    return out;
}
