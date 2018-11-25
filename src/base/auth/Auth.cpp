/**
 * scatter
 * Auth.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include <fmt/format.h>
#include <vector>
#include <memory>
#include "Auth.h"
#include "BasicAuth.h"
#include "HeaderAuth.h"
#include "BearerAuth.h"
#include "CookieAuth.h"
#include "OneOfAuth.h"
#include "AllOfAuth.h"
#include "RemoteAuth.h"

// NO-AUTH
std::string wss::Auth::getType() {
    return "noauth";
}
void wss::Auth::performAuth(wss::web::Request &) const {
    //do nothing
}
bool wss::Auth::validateAuth(const wss::web::Request &) const {
    return true;
}
std::string wss::Auth::getLocalValue() const {
    return std::string();
}
std::string wss::Auth::getRemoteValue(const wss::web::Request &) const {
    return std::string();
}

std::unique_ptr<wss::Auth> wss::auth::registry::createFromConfig(const nlohmann::json &config) {
    std::unique_ptr<wss::Auth> out;

    if (config.is_null()) {
        out = std::make_unique<wss::Auth>();
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
        out = std::make_unique<wss::BasicAuth>(
            data.at("user").get<std::string>(),
            data.at("password").get<std::string>()
        );
    } else if (eq(authType, "header")) {
        out = std::make_unique<wss::HeaderAuth>(
            data.at("name").get<std::string>(),
            data.value("value", "")
        );
    } else if (eq(authType, "bearer")) {
        out = std::make_unique<wss::BearerAuth>(
            data.at("value").get<std::string>()
        );
    } else if (eq(authType, "cookie")) {
        out = std::make_unique<wss::CookieAuth>(
            data.at("name").get<std::string>(),
            data.value("value", "")
        );
    } else if (eq(authType, "oneOf") || eq(authType, "allOf")) {
        bool isOneOf = eq(authType, "oneOf");
        std::vector<std::unique_ptr<wss::Auth>> types(data.size());
        std::vector<nlohmann::json> items = data.at("types").get<std::vector<nlohmann::json>>();

        for (size_t i = 0; i < items.size(); i++) {
            types[i] = createFromConfig(items.at(i));
        }

        if (isOneOf) {
            out = std::make_unique<wss::OneOfAuth>(std::move(types));
        } else {
            out = std::make_unique<wss::AllOfAuth>(std::move(types));
        }

    } else if (eq(authType, "remote")) {
        std::unique_ptr<wss::Auth> source = createFromConfig(data.at("source").get<nlohmann::json>());
        out = std::make_unique<wss::RemoteAuth>(data, std::move(source));
    } else {
        out = std::make_unique<wss::Auth>();
    }

    return out;
}

