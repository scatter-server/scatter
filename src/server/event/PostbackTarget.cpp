/**
 * wsserver
 * PostbackTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "PostbackTarget.h"

bool wss::event::PostbackTarget::send(const wss::MessagePayload &payload, std::string *error) {
    request.setBody(payload.toJson())
           .setMethod(wss::web::Request::POST);

    auth.performAuth(request);

    wss::web::Response response = getClient().execute(request);
    bool success = response.isSuccess();
    if (!success) {
        *error = response.statusMessage;
    }

    return success;
}

std::string wss::event::PostbackTarget::getType() {
    return "postback";
}

wss::event::PostbackTarget::PostbackTarget(const nlohmann::json &config) :
    Target(config),
    request() {
    using namespace toolboxpp::strings;
    const auto &eq = equalsIgnoreCase;

    try {
        request.setUrl(config.at("url"));

        if (config.find("auth") != config.end()) {
            const std::string authType = config["auth"].at("type").get<std::string>();

            if (eq(authType, "basic")) {
                setAuth(BasicAuth(
                    config["auth"].at("user").get<std::string>(),
                    config["auth"].at("password").get<std::string>()
                ));
            } else if (eq(authType, "header")) {
                setAuth(HeaderAuth(
                    config["auth"].at("name").get<std::string>(),
                    config["auth"].at("value").get<std::string>()
                ));
            } else if (eq(authType, "bearer")) {
                setAuth(BearerAuth(
                    config["auth"].at("value").get<std::string>()
                ));
            } else {
                throw std::runtime_error("Unsupported auth method: " + authType);
            }
        } else {
            setAuth(WebAuth());
        }
    } catch (const std::exception &e) {
        setErrorMessage("Invalid postback target configuration. " + std::string(e.what()));
    }
}
void wss::event::PostbackTarget::setAuth(wss::event::WebAuth &&auth) {
    this->auth = std::move(auth);
}
wss::web::HttpClient &wss::event::PostbackTarget::getClient() {
    return client;
}
wss::event::WebAuth wss::event::PostbackTarget::getAuth() {
    return auth;
}


