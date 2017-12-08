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
    const std::string out = payload.toJson();
    if (out.length() < 1000) {
//        L_DEBUG_F("Event-Send", "Request body: %s", out.c_str());
    }

    wss::web::Request request(url);
    request.setBody(out);
    request.setMethod(wss::web::Request::POST);
    request.setHeader({"Content-Type", "application/json"});

    auth->performAuth(request);
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
    Target(config) {

    try {
        url = config.at("url");

        if (config.find("auth") != config.end()) {
            this->auth = wss::auth::createFromConfig(config);
        } else {
            this->auth = std::make_unique<wss::WebAuth>();
        }

        client.enableVerbose(false);
        client.setConnectionTimeout(config.value("connectionTimeoutSeconds", 10L));
    } catch (const std::exception &e) {
        setErrorMessage("Invalid postback target configuration. " + std::string(e.what()));
    }
}
template<class T>
void wss::event::PostbackTarget::setAuth(T &&auth) {
    static_assert(std::is_base_of<WebAuth, T>::value, "Only subclass of Base can be passed");
    this->auth = std::make_unique<T>(auth);
}
wss::web::HttpClient &wss::event::PostbackTarget::getClient() {
    return client;
}
std::unique_ptr<wss::WebAuth> &wss::event::PostbackTarget::getAuth() {
    return auth;
}


