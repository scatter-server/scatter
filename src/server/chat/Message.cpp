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
const char *wss::TYPE_B64_IMAGE = "b64image";
const char *wss::TYPE_NOTIFICATION_RECEIVED = "notification_received";

wss::MessagePayload::MessagePayload(UserId from, UserId to, std::string &&message) :
    sender(from),
    text(std::move(message)),
    valid(true),
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
MessagePayload::MessagePayload(UserId from, const std::vector<UserId> &to, const std::string &message) :
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

wss::MessagePayload::MessagePayload(const nlohmann::json &obj) noexcept {
    fromJson(obj);
    validate();
}
wss::MessagePayload::MessagePayload(const WsMessagePtr &message) noexcept {
    const std::string data = message->string();
    if (data.length() == 0) {
        errorCause = "Empty message";
        valid = false;
        return;
    }
    try {
        auto obj = json::parse(data);
        fromJson(obj);
        validate();
    } catch (const std::exception &e) {
        handleJsonException(e, data);
    }
}

MessagePayload::MessagePayload(const MessagePayload &another) {
    *this = another;
}
MessagePayload::MessagePayload(MessagePayload &&another) {
    *this = another;
}
MessagePayload &MessagePayload::operator=(const MessagePayload &another) {
    sender = another.sender;
    recipients = another.recipients;
    text = another.text;
    type = another.type;
    data = another.data;
    valid = another.valid;
    errorCause = another.errorCause;

    return *this;
}

MessagePayload &MessagePayload::operator=(MessagePayload &&another) {
    sender = another.sender;
    recipients.insert(recipients.begin(), another.recipients.begin(), another.recipients.end());
    another.recipients.clear();
    text = another.text;
    another.text.clear();
    type = another.type;
    another.type.clear();
    data = std::move(another.data);
    valid = another.valid;
    errorCause = another.errorCause;
    another.errorCause.clear();
    return *this;
}

void MessagePayload::validate() {
    if (recipients.empty()) {
        valid = false;
        errorCause = "Recipients can't be empty";
    }
}

void wss::MessagePayload::fromJson(const json &obj) {
    from_json(obj, *this);
}
const UserId wss::MessagePayload::getSender() const {
    return sender;
}
const std::vector<UserId> wss::MessagePayload::getRecipients() const {
    return recipients;
}
const std::string wss::MessagePayload::toJson() const {
    json obj;
    to_json(obj, *this);

    std::string result = obj.dump();
    return result;
}
const std::string wss::MessagePayload::getText() const {
    return text;
}
const bool wss::MessagePayload::isMyMessage(UserId id) const {
    return getSender() == id;
}
const bool MessagePayload::isValid() const {
    return valid && !recipients.empty();
}
const bool MessagePayload::isSingleRecipient() {
    return recipients.size() == 1;
}
const bool MessagePayload::isBinary() const {
    return strcmp(type.c_str(), TYPE_B64_IMAGE) == 0;
}
const std::string MessagePayload::getError() const {
    return errorCause;
}

MessagePayload &MessagePayload::setRecipient(UserId id) {
    recipients.clear();
    recipients.push_back(id);
    return *this;
}
MessagePayload &MessagePayload::setRecipients(const std::vector<UserId> &recipients) {
    this->recipients = recipients;
    return *this;
}
MessagePayload &MessagePayload::setRecipients(std::vector<UserId> &&recipients) {
    this->recipients = std::move(recipients);
    return *this;
}

void MessagePayload::handleJsonException(const std::exception &e, const std::string &data) {
    valid = false;
    std::stringstream ss;
    ss << "Can't deserialize json: " << e.what();
    errorCause = ss.str();
    L_WARN("MessagePayload", ss.str().c_str())
}
MessagePayload &MessagePayload::addRecipient(UserId to) {
    recipients.push_back(to);
    return *this;
}

void wss::to_json(json &j, const MessagePayload &in) {
    j = json {
        {"type",       in.type},
        {"text",       in.text},
        {"sender",     in.sender},
        {"recipients", in.recipients},
        {"data",       in.data}
    };
}
void wss::from_json(const json &j, MessagePayload &in) {
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


