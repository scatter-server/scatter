/**
 * wsserver
 * PostbackTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "PostbackTarget.h"
#include <type_traits>

bool wss::event::PostbackTarget::send(const wss::MessagePayload &payload, std::string &error) {
    getClient().enableVerbose(true);

    request.setBody(payload.toJson())
           .setMethod(wss::web::Request::POST)
           .setHeader({"Content-Length", std::to_string(request.getBody().length())})
           .setHeader({"Content-Type", "application/json"});

    auth->performAuth(request);;
    wss::web::Response response = getClient().execute(request);
    bool success = response.isSuccess();
    std::stringstream ss;
    ss << response.statusMessage << "\n" << response.data;
    if (!success) {
        error = ss.str();
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
template<class T>
void wss::event::PostbackTarget::setAuth(T &&auth) {
    static_assert(std::is_base_of<wss::event::WebAuth, T>::value, "Only subclass of Base can be passed");
    this->auth = std::make_unique<T>(auth);
}
wss::web::HttpClient &wss::event::PostbackTarget::getClient() {
    return client;
}
std::unique_ptr<wss::event::WebAuth> &wss::event::PostbackTarget::getAuth() {
    return auth;
}


