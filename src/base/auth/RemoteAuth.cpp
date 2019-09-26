/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include <fmt/format.h>
#include <vector>
#include <string>
#include <toolboxpp.hpp>
#include "RemoteAuth.h"

// REMOTE AUTH
wss::RemoteAuth::RemoteAuth(const nlohmann::json &data, std::unique_ptr<Auth> &&source) :
    m_source(std::move(source)) {
    if (data.find("data") != data.end()) {
        if (data.at("data").is_string()) {
            m_data = data.at("data").get<std::string>();
        } else if (data.at("data").is_object()) {
            m_data = data.at("data").get<nlohmann::json>().dump();
        }
    }

    m_url = data.value("url", "");
    m_method = httb::request::methodFromString(data.value("method", "POST"));

    std::vector<nlohmann::json> headers = data.value("headers", std::vector<nlohmann::json>(0));
    for (auto &obj: headers) {
        for (nlohmann::json::iterator it = obj.begin(); it != obj.end(); ++it) {
            const std::string k = it.key();
            const std::string v = it.value();
            m_headers[k] = v;
        }
    }
}

std::string wss::RemoteAuth::getType() {
    return "remote";
}
void wss::RemoteAuth::performAuth(httb::request &) const {
    // do nothing
}
bool wss::RemoteAuth::validateAuth(const httb::request &request) const {
    const std::string value = m_source->getRemoteValue(request);
    if (value.empty()) {
        // is this make sense to try auth with empty value?
        return false;
    }

    httb::request r(m_url, m_method);
    for (const auto &kv: m_headers) {
        r.addHeader(kv);
    }

    std::string outData;
    if (!m_data.empty()) {
        std::vector<std::string> outputs{value};
        std::vector<std::string> inputs(outputs.size());
        for (size_t i = 0; i < outputs.size(); i++) {
            inputs[i] = fmt::format("{{0}}", i);
        }

        outData = toolboxpp::strings::substringReplaceAll(inputs, outputs, m_data);
        r.setBody(outData);
    }

    httb::client client;
    client.setEnableVerbose(false);

    boost::asio::io_context ctx;
    bool success = false;
    client.executeInContext(ctx, r, [&success](httb::response resp) {
      success = resp.isSuccess();
    });
    ctx.run();

    return success;
}
std::string wss::RemoteAuth::getLocalValue() const {
    return Auth::getLocalValue();
}
std::string wss::RemoteAuth::getRemoteValue(const httb::request &request) const {
    return Auth::getRemoteValue(request);
}
