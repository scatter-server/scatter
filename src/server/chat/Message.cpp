/**
 * wsserver
 * Message.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */
#include "Message.h"

#include <toolboxpp.h>

using namespace wss;
using std::cout;
using std::cerr;
using std::endl;

const char *wss::TYPE_TEXT = "text";
const char *wss::TYPE_BINARY = "binary";
const char *wss::TYPE_B64_IMAGE = "b64image";
const char *wss::TYPE_URL_IMAGE = "url_image";
const char *wss::TYPE_NOTIFICATION_RECEIVED = "notification_received";

wss::MessagePayload::MessagePayload(UserId from, UserId to, std::string &&message) :
    sender(from),
    text(std::move(message)),
    type(TYPE_TEXT) {
    addRecipient(to);
}
wss::MessagePayload::MessagePayload(UserId from, std::vector<UserId> &&to, std::string &&message) :
    sender(from),
    recipients(std::move(to)),
    text(std::move(message)),
    type(TYPE_TEXT) {
    validate();
}
MessagePayload::MessagePayload(UserId from, UserId to, const std::string &message) :
    sender(from),
    text(message),
    type(TYPE_TEXT) {
    addRecipient(to);
}
wss::MessagePayload::MessagePayload(UserId from, const std::vector<UserId> &to, const std::string &message) :
    sender(from),
    recipients(to),
    text(message),
    type(TYPE_TEXT) {
    validate();
}
wss::MessagePayload::MessagePayload(const std::string &json) noexcept {
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

wss::MessagePayload::MessagePayload(const wss::json &obj) noexcept {
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
UserId wss::MessagePayload::getSender() const {
    return sender;
}
const std::vector<UserId> wss::MessagePayload::getRecipients() const {
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
bool wss::MessagePayload::isMyMessage(UserId id) const {
    return getSender() == id;
}
bool wss::MessagePayload::isValid() const {
    return valid && !recipients.empty();
}
bool wss::MessagePayload::isSingleRecipient() const {
    return recipients.size() == 1;
}
bool wss::MessagePayload::isBinary() const {
    return typeIs(TYPE_B64_IMAGE);
}
bool wss::MessagePayload::isSentStatus() const {
    return typeIs(TYPE_NOTIFICATION_RECEIVED);
}
bool wss::MessagePayload::typeIs(const char *t) const {
    const char *lc = type.c_str();
    return strcmp(lc, t) == 0;
}
const std::string wss::MessagePayload::getError() const {
    return errorCause;
}

wss::MessagePayload &MessagePayload::setRecipient(UserId id) {
    recipients.clear();
    recipients.push_back(id);
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(const std::vector<UserId> &recipients) {
    this->recipients = recipients;
    return *this;
}
wss::MessagePayload &MessagePayload::setRecipients(std::vector<UserId> &&recipients) {
    this->recipients = std::move(recipients);
    return *this;
}

void wss::MessagePayload::handleJsonException(const std::exception &e, const std::string &) {
    valid = false;
    std::stringstream ss;
    ss << "Can't deserialize json: " << e.what();
    errorCause = ss.str();
    L_WARN("MessagePayload", ss.str().c_str())
}
wss::MessagePayload &MessagePayload::addRecipient(UserId to) {
    recipients.push_back(to);
    return *this;
}

void wss::to_json(wss::json &j, const wss::MessagePayload &in) {
    j = json {
        {"type",       in.type},
        {"text",       in.text},
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

    in.sender = j.at("sender").get<UserId>();
    in.recipients = j.at("recipients").get<std::vector<UserId>>();

    in.data = j.value("data", json());
}

wss::MessagePayload MessagePayload::createSendStatus(UserId to) {
    MessagePayload payload;
    payload.sender = 0;
    payload.addRecipient(to);
    payload.type = TYPE_NOTIFICATION_RECEIVED;

    return payload;
}
wss::MessagePayload MessagePayload::createSendStatus(const MessagePayload &payload) {
    return createSendStatus(payload.getSender());
}


