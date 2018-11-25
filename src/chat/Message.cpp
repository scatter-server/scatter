/**
 * scatter
 * Message.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include "../public/Message.h"
#include "../helpers/helpers.h"

#include <fmt/format.h>
#include <toolboxpp.h>

using namespace wss;
using std::cout;
using std::cerr;
using std::endl;

const char *wss::TYPE_TEXT = "text";
const char *wss::TYPE_BINARY = "binary";
const char *wss::TYPE_NOTIFICATION_RECEIVED = "notification_received";

MessagePayload::MessagePayload() :
    m_id({0, 0, 0, 0}) {
}

wss::MessagePayload::MessagePayload(user_id_t from, user_id_t to, std::string &&message) :
    m_id(wss::unid::generator()()),
    m_sender(from),
    m_recipients(std::vector<user_id_t>{to}),
    m_text(std::move(message)),
    m_type(TYPE_TEXT),
    m_timestamp(wss::utils::getNowISODateTimeFractionalConfigAware()) {

}
wss::MessagePayload::MessagePayload(user_id_t from, std::vector<user_id_t> &&to, std::string &&message) :
    m_id(wss::unid::generator()()),
    m_sender(from),
    m_recipients(std::move(to)),
    m_text(std::move(message)),
    m_type(TYPE_TEXT),
    m_timestamp(wss::utils::getNowISODateTimeFractionalConfigAware()) {
    validate();
}
MessagePayload::MessagePayload(user_id_t from, user_id_t to, const std::string &message) :
    m_id(wss::unid::generator()()),
    m_sender(from),
    m_recipients(to),
    m_text(message),
    m_type(TYPE_TEXT),
    m_timestamp(wss::utils::getNowISODateTimeFractionalConfigAware()) {
}
wss::MessagePayload::MessagePayload(user_id_t from, const std::vector<user_id_t> &to, const std::string &message) :
    m_id(wss::unid::generator()()),
    m_sender(from),
    m_recipients(to),
    m_text(message),
    m_type(TYPE_TEXT),
    m_timestamp(wss::utils::getNowISODateTimeFractionalConfigAware()) {
    validate();
}
wss::MessagePayload::MessagePayload(const std::string &json) noexcept:
    m_id(wss::unid::generator()()) {
    if (json.length() == 0) {
        m_errorCause = "Empty message";
        m_validState = false;
        return;
    }
    try {
        auto obj = json::parse(json);
        fromJson(obj);
        validate();
    } catch (const std::exception &e) {
        handleJsonException(e, json);
    }
}

wss::MessagePayload::MessagePayload(const wss::json &obj) noexcept:
    m_id(wss::unid::generator()()) {
    fromJson(obj);
    validate();
}

void wss::MessagePayload::validate() {
    if (m_recipients.empty()) {
        m_validState = false;
        m_errorCause = "Recipients can't be empty";
    }
}

void wss::MessagePayload::fromJson(const json &obj) {
    from_json(obj, *this);
}
const unid_t MessagePayload::getId() const {
    return m_id;
}
user_id_t wss::MessagePayload::getSender() const {
    return m_sender;
}
const std::vector<user_id_t> wss::MessagePayload::getRecipients() const {
    return m_recipients;
}
const std::string wss::MessagePayload::toJson() const {
    if (m_isCached) {
        return m_cachedJson;
    }

    json obj;
    to_json(obj, *this);

    m_cachedJson = obj.dump();
    m_isCached = true;
    return m_cachedJson;
}
const std::string wss::MessagePayload::getText() const {
    return m_text;
}
bool wss::MessagePayload::isMyMessage(user_id_t id) const {
    return getSender() == id;
}
bool wss::MessagePayload::isValid() const {
    return m_validState && !m_recipients.empty();
}

bool MessagePayload::isFromBot() const {
    return m_sender == 0;
}

bool MessagePayload::isForBot() const {
    return m_recipients.size() == 1 && m_recipients[0] == 0L;
}

bool wss::MessagePayload::haveSingleRecipient() const {
    return m_recipients.size() == 1;
}
bool wss::MessagePayload::isBinary() const {
    return typeIs(TYPE_BINARY);
}
bool wss::MessagePayload::isTypeOfSentStatus() const {
    return typeIs(TYPE_NOTIFICATION_RECEIVED);
}
bool wss::MessagePayload::typeIs(const char *t) const {
    const char *lc = m_type.c_str();
    return strcmp(lc, t) == 0;
}
const std::string MessagePayload::getType() const {
    return m_type;
}
const std::string wss::MessagePayload::getError() const {
    return m_errorCause;
}

MessagePayload &MessagePayload::setSender(user_id_t id) {
    m_sender = id;
    clearCachedJson();
    return *this;
}

wss::MessagePayload &MessagePayload::setRecipient(user_id_t id) {
    m_recipients.clear();
    m_recipients.push_back(id);
    clearCachedJson();
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(const std::vector<user_id_t> &recipients) {
    this->m_recipients = recipients;
    clearCachedJson();
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(std::vector<user_id_t> &&recipients) {
    this->m_recipients = std::move(recipients);
    clearCachedJson();
    return *this;
}

void wss::MessagePayload::handleJsonException(const std::exception &e, const std::string &) {
    m_validState = false;
    std::stringstream ss;
    ss << "Invalid payload: " << e.what();
    m_errorCause = ss.str();
    L_WARN("Chat::Message::Payload", ss.str().c_str())
}

void MessagePayload::clearCachedJson() {
    if (m_isCached) {
        m_isCached = false;
        m_cachedJson.clear();
    }
}

bool MessagePayload::operator==(wss::MessagePayload const &rhs) {
    return m_id == rhs.m_id;
}

wss::MessagePayload &MessagePayload::addRecipient(user_id_t to) {
    m_recipients.push_back(to);
    clearCachedJson();
    return *this;
}

void wss::to_json(wss::json &j, const wss::MessagePayload &in) {
    j = json{
        {"id",         in.m_id},
        {"type",       in.m_type},
        {"text",       in.m_text},
        {"timestamp",  in.m_timestamp},
        {"sender",     in.m_sender},
        {"recipients", in.m_recipients},
        {"data",       in.m_data}
    };
}

void wss::from_json(const wss::json &j, wss::MessagePayload &in) {
    if (j.find("type") == j.end() || j.at("type").is_null()) {
        throw InvalidPayloadException("$.type must be a string");
    } else if (j.find("sender") == j.end() || j.at("sender").is_null() || !j.at("sender").is_number()) {
        throw InvalidPayloadException("$.sender must be uint64_t");
    } else if (j.find("recipients") == j.end() || j.at("recipients").is_null() || !j.at("recipients").is_array()) {
        throw InvalidPayloadException("$.recipients[] must be uint64_t[]");
    }

    in.m_type = j.value("type", std::string(TYPE_TEXT));

    if (strcmp(in.m_type.c_str(), TYPE_TEXT) == 0) {
        if (!j.at("text").is_string()) {
            throw InvalidPayloadException("$.text must be string");
        }
        in.m_text = j.at("text").get<std::string>();
    } else {
        if (j.find("text") == j.end() || !j.at("text").is_string()) {
            in.m_text = std::string();
        } else {
            in.m_text = j.value("text", "");
        }
    }

    in.m_sender = j.at("sender").get<user_id_t>();
    in.m_recipients = j.at("recipients").get<std::vector<user_id_t>>();
    if (in.m_recipients.empty()) {
        throw InvalidPayloadException("$.recipients[] must contains at least 1 value");
    }

    if (j.find("data") != j.end()) {
        in.m_data = j.value("data", json());
    } else {
        in.m_data = json();
    }

    if (j.find("timestamp") != j.end() && j.at("timestamp").is_string()) {
        in.m_timestamp = j.at("timestamp").get<std::string>();
    } else {
        in.m_timestamp = wss::utils::getNowISODateTimeFractionalConfigAware();
    }
}

wss::MessagePayload MessagePayload::createSendStatus(user_id_t to) {
    MessagePayload payload;
    payload.m_sender = 0;
    payload.addRecipient(to);
    payload.m_type = TYPE_NOTIFICATION_RECEIVED;

    return payload;
}
wss::MessagePayload MessagePayload::createSendStatus(const MessagePayload &payload) {
    return createSendStatus(payload.getSender());
}



