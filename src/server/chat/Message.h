/**
 * wsserver
 * Message.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_MESSAGE_HPP
#define WSSERVER_MESSAGE_HPP

#include <string>
#include <iostream>
#include <type_traits>
#include <toolboxpp.h>
#include "json.hpp"
#include "../defs.h"

namespace wss {

extern const char *TYPE_TEXT;
extern const char *TYPE_BINARY;
extern const char *TYPE_B64_IMAGE;
extern const char *TYPE_URL_IMAGE;
extern const char *TYPE_NOTIFICATION_RECEIVED;

class MessagePayload {
 private:
    UserId sender;
    std::vector<UserId> recipients;
    std::string text;
    std::string type;
    json data;
    bool valid = true;
    std::string errorCause;

    void fromJson(const json &json);
    void validate();
    void handleJsonException(const std::exception &e, const std::string &data);

    friend void to_json(wss::json &j, const wss::MessagePayload &in);
    friend void from_json(const wss::json &j, wss::MessagePayload &in);
 public:
    static MessagePayload createSendStatus(UserId to);
    static MessagePayload createSendStatus(const MessagePayload &payload);

    MessagePayload() = default;
    MessagePayload(UserId from, UserId to, const std::string &message);
    MessagePayload(UserId from, UserId to, std::string &&message);
    MessagePayload(UserId from, const std::vector<UserId> &to, const std::string &message);
    MessagePayload(UserId from, std::vector<UserId> &&to, std::string &&message);
    MessagePayload(const MessagePayload &payload) = default;
    MessagePayload(MessagePayload &&payload) = default;
    MessagePayload &operator=(const MessagePayload &payload) = default;
    MessagePayload &operator=(MessagePayload &&payload) = default;

    explicit MessagePayload(const std::string &json) noexcept;
    explicit MessagePayload(const nlohmann::json &obj) noexcept;

    /**
     *
     * @return Sender ID
     */
    const UserId getSender() const;
    /**
     * Recipients ids
     * @see Dogsy::UserId
     * @return
     */
    const std::vector<UserId> getRecipients() const;

    /**
     * Message type
     * @return
     */
    const std::string getType() const {
        return type;
    }
    /**
     * Text message
     * @return
     */
    const std::string getText() const;
    /**
     * Converts this payload to json string
     * @return
     */
    const std::string toJson() const;
    /**
     * Checks by passed id, that current payload belongs to sender
     * @see getSender()
     * @param id
     * @return
     */
    bool isMyMessage(UserId id) const;
    /**
     * If can't parse input json or some input data is not valid, false will returned
     * @return
     */
    bool isValid() const;
    /**
     * Check message type
     * @param type
     * @return
     */
    bool typeIs(const char *type) const;

    /**
     * Whether message type is just send status
     * @return
     */
    bool isSentStatus() const;
    /**
     * Whether message type is binary
     * @return
     */
    bool isBinary() const;
    /**
     * Check recipients count is only one
     * @return
     */
    bool isSingleRecipient() const;
    /**
     * Error message
     * @return Empty string if no one error. Check @see isValid() before
     */
    const std::string getError() const;

    MessagePayload &setRecipient(UserId id);
    MessagePayload &setRecipients(const std::vector<UserId> &recipients);
    MessagePayload &setRecipients(std::vector<UserId> &&recipients);
    MessagePayload &addRecipient(UserId to);
};

};

#endif //WSSERVER_MESSAGE_HPP
