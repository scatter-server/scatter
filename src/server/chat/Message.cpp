/**
 * wsserver
 * Message.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include "Message.h"
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
    id({0, 0, 0, 0}) {
}

wss::MessagePayload::MessagePayload(user_id_t from, user_id_t to, std::string &&message) :
    id(wss::unid::generator()()),
    sender(from),
    recipients(std::vector<user_id_t> {to}),
    text(std::move(message)),
    type(TYPE_TEXT),
    timestamp(wss::helpers::getNowISODateTimeFractional()) {

}
wss::MessagePayload::MessagePayload(user_id_t from, std::vector<user_id_t> &&to, std::string &&message) :
    id(wss::unid::generator()()),
    sender(from),
    recipients(std::move(to)),
    text(std::move(message)),
    type(TYPE_TEXT),
    timestamp(wss::helpers::getNowISODateTimeFractional()) {
    validate();
}
MessagePayload::MessagePayload(user_id_t from, user_id_t to, const std::string &message) :
    id(wss::unid::generator()()),
    sender(from),
    recipients(to),
    text(message),
    type(TYPE_TEXT),
    timestamp(wss::helpers::getNowISODateTimeFractional()) {
}
wss::MessagePayload::MessagePayload(user_id_t from, const std::vector<user_id_t> &to, const std::string &message) :
    id(wss::unid::generator()()),
    sender(from),
    recipients(to),
    text(message),
    type(TYPE_TEXT),
    timestamp(wss::helpers::getNowISODateTimeFractional()) {
    validate();
}
wss::MessagePayload::MessagePayload(const std::string &json) noexcept:
    id(wss::unid::generator()()) {
    if (json.length() == 0) {
        errorCause = "Empty message";
        valid = false;
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
    id(wss::unid::generator()()) {
    fromJson(obj);
    validate();
}

void wss::MessagePayload::validate() {
    if (recipients.empty()) {
        valid = false;
        errorCause = "Recipients can't be empty";
    }
}

void wss::MessagePayload::fromJson(const json &obj) {
    from_json(obj, *this);
}
const unid_t MessagePayload::getId() const {
    return id;
}
user_id_t wss::MessagePayload::getSender() const {
    return sender;
}
const std::vector<user_id_t> wss::MessagePayload::getRecipients() const {
    return recipients;
}
const std::string wss::MessagePayload::toJson() const {
    json obj;
    to_json(obj, *this);

    const std::string result = obj.dump();
    return result;
}
const std::string wss::MessagePayload::getText() const {
    return text;
}
bool wss::MessagePayload::isMyMessage(user_id_t id) const {
    return getSender() == id;
}
bool wss::MessagePayload::isValid() const {
    return valid && !recipients.empty();
}

bool MessagePayload::isBotMessage() {
    return sender == 0;
}
bool wss::MessagePayload::haveSingleRecipient() const {
    return recipients.size() == 1;
}
bool wss::MessagePayload::isBinary() const {
    return typeIs(TYPE_BINARY);
}
bool wss::MessagePayload::isTypeOfSentStatus() const {
    return typeIs(TYPE_NOTIFICATION_RECEIVED);
}
bool wss::MessagePayload::typeIs(const char *t) const {
    const char *lc = type.c_str();
    return strcmp(lc, t) == 0;
}
const std::string MessagePayload::getType() const {
    return type;
}
const std::string wss::MessagePayload::getError() const {
    return errorCause;
}

wss::MessagePayload &MessagePayload::setRecipient(user_id_t id) {
    recipients.clear();
    recipients.push_back(id);
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(const std::vector<user_id_t> &recipients) {
    this->recipients = recipients;
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(std::vector<user_id_t> &&recipients) {
    this->recipients = std::move(recipients);
    return *this;
}

void wss::MessagePayload::handleJsonException(const std::exception &e, const std::string &) {
    valid = false;
    std::stringstream ss;
    ss << "Can't deserialize json: " << e.what();
    errorCause = ss.str();
    L_WARN("Chat::Message::Payload", ss.str().c_str())
}
bool MessagePayload::operator==(wss::MessagePayload const &rhs) {
    return id == rhs.id;
}

wss::MessagePayload &MessagePayload::addRecipient(user_id_t to) {
    recipients.push_back(to);
    return *this;
}

void wss::to_json(wss::json &j, const wss::MessagePayload &in) {
    j = json {
        {"id",         in.id},
        {"type",       in.type},
        {"text",       in.text},
        {"timestamp",  in.timestamp},
        {"sender",     in.sender},
        {"recipients", in.recipients},
        {"data",       in.data}
    };
}
void wss::from_json(const wss::json &j, wss::MessagePayload &in) {
    in.type = j.value("type", std::string(TYPE_TEXT));

    if (strcmp(in.type.c_str(), TYPE_TEXT) == 0) {
        in.text = j.at("text").get<std::string>();
    } else {
        in.text = j.value("text", "");
    }

    in.sender = j.at("sender").get<user_id_t>();
    in.recipients = j.at("recipients").get<std::vector<user_id_t>>();

    in.data = j.value("data", json());
    in.timestamp = wss::helpers::getNowISODateTimeFractional();
}

wss::MessagePayload MessagePayload::createSendStatus(user_id_t to) {
    MessagePayload payload;
    payload.sender = 0;
    payload.addRecipient(to);
    payload.type = TYPE_NOTIFICATION_RECEIVED;

    return payload;
}
wss::MessagePayload MessagePayload::createSendStatus(const MessagePayload &payload) {
    return createSendStatus(payload.getSender());
}



