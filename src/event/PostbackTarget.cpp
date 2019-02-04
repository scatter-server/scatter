/**
 * wsserver
 * PostbackTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include <type_traits>
#include "PostbackTarget.h"

bool wss::event::PostbackTarget::send(const wss::MessagePayload &payload, std::string &error) {
    const std::string out = payload.toJson();
    if (out.length() < 1000) {
        //L_DEBUG_F("Event-Send", "Request body: %s", out.c_str());
    }

    httb::request request(m_url);
    request.setBody(out);
    request.setMethod(m_httpMethod);
    request.setHeader({"Content-Type", "application/json"});

    m_auth->performAuth(request);
    httb::response response = getClient().execute(request);
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
        m_url = config.at("url");

        if (config.find("auth") != config.end()) {
            this->m_auth = wss::auth::registry::createFromConfig(config);
        } else {
            this->m_auth = std::make_unique<wss::Auth>();
        }

        if (config.find("method") != config.end()) {
            m_httpMethod = httb::request::methodFromString(config.value(std::string("method"), "POST"));
        }

        m_client.setEnableVerbose(false);
        m_client.setConnectionTimeout(config.value("connectionTimeoutSeconds", 10L));
    } catch (const std::exception &e) {
        setErrorMessage("Invalid postback target configuration. " + std::string(e.what()));
    }
}
template<class T>
void wss::event::PostbackTarget::setAuth(T &&auth) {
    static_assert(std::is_base_of<Auth, T>::value, "Only subclass of Base can be passed");
    this->m_auth = std::make_unique<T>(auth);
}
httb::client &wss::event::PostbackTarget::getClient() {
    return m_client;
}
std::unique_ptr<wss::Auth> &wss::event::PostbackTarget::getAuth() {
    return m_auth;
}


