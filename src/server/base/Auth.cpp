/**
 * wsserver
 * Auth.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include <fmt/format.h>
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
    } else if (eq(authType, "cookie")) {
        out = std::make_unique<CookieAuth>(
            data.at("name").get<std::string>(),
            data.at("value").get<std::string>()
        );
    } else if (eq(authType, "oneOf") || eq(authType, "allOf")) {
        bool isOneOf = eq(authType, "oneOf");
        std::vector<std::unique_ptr<wss::WebAuth>> types(data.size());
        std::vector<nlohmann::json> items = data.at("types").get<std::vector<nlohmann::json>>();

        for (size_t i = 0; i < items.size(); i++) {
            types[i] = createFromConfig(items.at(i));
        }

        if (isOneOf) {
            out = std::make_unique<OneOfAuth>(std::move(types));
        } else {
            out = std::make_unique<AllOfAuth>(std::move(types));
        }

    } else {
        out = std::make_unique<wss::WebAuth>();
    }

    return out;
}

/// COOKIE
wss::CookieAuth::CookieAuth(const std::string &cookieName, const std::string &cookieValue) :
    name(cookieName),
    value(cookieValue) {
}
wss::CookieAuth::CookieAuth(std::string &&cookieName, std::string &&cookieValue) :
    name(std::move(cookieName)),
    value(std::move(cookieValue)) {

}
std::string wss::CookieAuth::getType() {
    return "cookie";
}
void wss::CookieAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({"Cookie", getValue()});
}
bool wss::CookieAuth::validateAuth(const wss::web::Request &request) const {
    if (!request.hasHeader("cookie")) {
        return false;
    }
    std::string cookieHeader = request.getHeader("cookie");
    std::vector<std::string> cookies = toolboxpp::strings::split(cookieHeader, ";");

    const std::string pattern = "([a-zA-Z0-9-_]+)=([a-zA-Z0-9-_]+)";
    for (auto &cookie: cookies) {
        if (toolboxpp::strings::hasRegex(pattern, cookie)) {
            const auto &match = toolboxpp::strings::matchRegexp(pattern, cookie);
            if (match.size() != 3) {
                continue;
            }

            const std::string n = match[1];
            const std::string v = match[2];

            if (toolboxpp::strings::equalsIgnoreCase(name, n) && v.compare(value) == 0) {
                return true;
            }
        }
    }

    return false;
}
std::string wss::CookieAuth::getValue() const {
    return fmt::format("{0}={1}", name, value);
}

// OneOfAuth
wss::OneOfAuth::OneOfAuth(std::vector<std::unique_ptr<wss::WebAuth>> &&data) : types(std::move(data)) {
}

std::string wss::OneOfAuth::getType() {
    return "oneOf";
}
void wss::OneOfAuth::performAuth(wss::web::Request &request) const {
    for (auto &auth: types) {
        auth->performAuth(request);
    }
}
bool wss::OneOfAuth::validateAuth(const wss::web::Request &request) const {
    for (auto &auth: types) {
        if (auth->validateAuth(request)) {
            return true;
        }
    }
    return false;
}
std::string wss::OneOfAuth::getValue() const {
    return WebAuth::getValue();
}

wss::AllOfAuth::AllOfAuth(std::vector<std::unique_ptr<wss::WebAuth>> &&types) : OneOfAuth(std::move(types)) {
}

std::string wss::AllOfAuth::getType() {
    return "allOf";
}
bool wss::AllOfAuth::validateAuth(const wss::web::Request &request) const {
    for (auto &type: types) {
        if (!type->validateAuth(request)) {
            return false;
        }
    }

    return true;
}



