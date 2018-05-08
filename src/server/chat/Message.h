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
#include "src/server/wsserver_core.h"
#include "src/server/base/unid.h"

namespace wss {

extern const char *TYPE_TEXT;
extern const char *TYPE_BINARY;
extern const char *TYPE_NOTIFICATION_RECEIVED;

struct InvalidPayloadException : std::exception {
  std::string err;

  InvalidPayloadException(std::string &&err) {
      this->err = std::move(err);
  }
  const char *what() const throw() override {
      return err.c_str();
  }
};

/// \brief Main structured message payload
/// \todo Protobuf support
class MessagePayload {
 private:
    unid_t m_id;
    user_id_t m_sender;
    std::vector<user_id_t> m_recipients;
    std::string m_text;
    std::string m_type;
    std::string m_timestamp;
    json m_data;
    bool m_validState = true;
    std::string m_errorCause;

    mutable std::string m_cachedJson;
    mutable bool m_isCached = false;

    void fromJson(const json &json);
    void validate();
    void handleJsonException(const std::exception &e, const std::string &data);
    void clearCachedJson();

    friend void to_json(wss::json &j, const wss::MessagePayload &in);
    friend void from_json(const wss::json &j, wss::MessagePayload &in);
 public:
    /// \brief Creates simple send-status messages, using only UserId
    /// \param to sender UserId
    /// \return valid payload object
    static MessagePayload createSendStatus(user_id_t to);

    /// \brief Creates simple send-status messages, using other payload (extracts sender)
    /// \param payload
    /// \return
    static MessagePayload createSendStatus(const MessagePayload &payload);

    MessagePayload();
    MessagePayload(user_id_t from, user_id_t to, const std::string &message);
    MessagePayload(user_id_t from, user_id_t to, std::string &&message);
    MessagePayload(user_id_t from, const std::vector<user_id_t> &to, const std::string &message);
    MessagePayload(user_id_t from, std::vector<user_id_t> &&to, std::string &&message);
    MessagePayload(const MessagePayload &payload) = default;
    MessagePayload(MessagePayload &&payload) = default;
    MessagePayload &operator=(const MessagePayload &payload) = default;
    MessagePayload &operator=(MessagePayload &&payload) = default;

    explicit MessagePayload(const std::string &json) noexcept;
    explicit MessagePayload(const nlohmann::json &obj) noexcept;

    bool operator==(wss::MessagePayload const &);

    /// \brief Return sender UserId
    /// \return Sender UserId
    user_id_t getSender() const;

    /// \brief Return message id
    /// \return string identifier
    const unid_t getId() const;

    /// \brief Recipients ids
    /// \return std::vector<UserId>
    const std::vector<user_id_t> getRecipients() const;

    /// \brief Message type
    /// \return string type. Predefined types:
    /// \see constants TYPE_TEXT, TYPE_BINARY, TYPE_B64_IMAGE, TYPE_URL_IMAGE, TYPE_NOTIFICATION_RECEIVED
    const std::string getType() const;

    /// \brief Text message
    /// \return string or empty if type not a TYPE_TEXT
    const std::string getText() const;

    /// \brief Converts this payload to json string
    /// \return valid json string
    const std::string toJson() const;

    /// \brief Checks by passed id, that current payload belongs to sender
    /// \param id UserId
    /// \return true if is my message, otherwise message belongs to my chat-friend
    bool isMyMessage(user_id_t id) const;

    /// \brief If can't parse input json or some input data is not valid, false will returned
    /// \return
    bool isValid() const;

    /// \brief Check if this messages is sent from bot (senderId=0)
    /// \return
    bool isFromBot() const;

    /// \brief Check is this message must be sent to bot (recipients contains exact 1 element and recipients[0] == 0L )
    /// \return
    bool isForBot() const;

    /// \brief Check message type
    /// \param type
    /// \return
    bool typeIs(const char *type) const;

    /// \brief Whether message type is just send status
    /// \return
    bool isTypeOfSentStatus() const;

    /// \brief Whether message type is binary
    /// \return
    bool isBinary() const;

    /// \brief Check recipients count is only one
    /// \return
    bool haveSingleRecipient() const;

    /// \brief Error message
    /// \return Empty string if no one error. Check @see isValid() before
    const std::string getError() const;

    MessagePayload &setSender(user_id_t id);
    MessagePayload &setRecipient(user_id_t id);
    MessagePayload &setRecipients(const std::vector<user_id_t> &recipients);
    MessagePayload &setRecipients(std::vector<user_id_t> &&recipients);
    MessagePayload &addRecipient(user_id_t to);
};

void to_json(wss::json &j, const wss::MessagePayload &in);
void from_json(const wss::json &j, wss::MessagePayload &in);

}

#endif //WSSERVER_MESSAGE_HPP
