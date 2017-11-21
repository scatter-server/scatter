/**
 * wsserver
 * PostbackTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "PostbackTarget.h"

wss::event::PostbackTarget::PostbackTarget(const nlohmann::json &config) : WebTarget(config) {
    url = config.at("url");
}

bool wss::event::PostbackTarget::send(const wss::MessagePayload &payload) {
    wss::web::Response response = getClient().postJson(wss::web::QueryBuilder::parse(url), payload.toJson());
    return response.isSuccess();
}

std::string wss::event::PostbackTarget::getType() {
    return "postback";
}


