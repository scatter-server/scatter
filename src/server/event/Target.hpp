/**
 * wsserver
 * EventConfig.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_EVENTCONFIG_H
#define WSSERVER_EVENTCONFIG_H

#include <string>
#include <curl/curl.h>
#include "../helpers/base64.h"
#include "../chat/Message.h"
#include "../web/HttpClient.h"

namespace wss {
namespace event {

/// \brief Available:
/// postback: PostbackTarget
///     "url": "http://example.com/postback",
class Target {
 public:
    /// \brief Accept json config of entire target object
    /// \param config
    explicit Target(const nlohmann::json &config) :
        m_config(config),
        m_validState(true),
        m_errorMessage("") { }

    /// \brief Send event to entire target
    /// \param payload Payload to send
    /// \param error if method returned false, error will contains error message
    /// \return true if sending complete
    virtual bool send(const wss::MessagePayload &payload, std::string &error) = 0;
    virtual std::string getType() = 0;

    /// \brief Check target is in valid state
    /// \return valid state of target object
    bool isValid() const {
        return m_validState;
    }

    /// \brief If target is not valid, message will be available
    /// \return error message
    std::string getErrorMessage() const {
        return m_errorMessage;
    }

 protected:
    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg
    void setErrorMessage(const std::string &msg) {
        m_errorMessage = msg;
        m_validState = false;
    }

    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg rvalue string
    void setErrorMessage(std::string &&msg) {
        m_errorMessage = std::move(msg);
        m_validState = false;
    }

    /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg rvalue error message
    void appendErrorMessage(std::string &&msg) {
        if (m_errorMessage.empty()) {
            setErrorMessage(std::move(msg));
            return;
        }

        m_errorMessage = fmt::format("{0}\n{1}", m_errorMessage, msg);
    }

    /// \brief Appends to existing message new message separated by new line. If message did not set before, it will be a single message.
    /// Mark object as in invalid state/
    /// \param msg lvalue error message
    void appendErrorMessage(const std::string &msg) {
        if (m_errorMessage.empty()) {
            setErrorMessage(msg);
            return;
        }

        m_errorMessage = fmt::format("{0}\n{1}", m_errorMessage, msg);
    }
 private:
    nlohmann::json m_config;
    bool m_validState;
    std::string m_errorMessage;
};

}
}

#endif //WSSERVER_EVENTCONFIG_H
