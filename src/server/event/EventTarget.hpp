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
        config(config),
        valid(true),
        errorMessage() { }

    /// \brief Send event to entire target
    /// \param payload Payload to send
    /// \param error if method returned false, error will contains error message
    /// \return true if sending complete
    virtual bool send(const wss::MessagePayload &payload, std::string &error) = 0;
    virtual std::string getType() = 0;

    /// \brief Check target is in valid state
    /// \return valid state of target object
    bool isValid() const {
        return valid;
    }

    /// \brief If target is not valid, message will be available
    /// \return error message
    std::string getErrorMessage() const {
        return errorMessage;
    }

 protected:
    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg
    void setErrorMessage(const std::string &msg) {
        errorMessage = msg;
        valid = false;
    }

    /// \brief Set error message to read when object is invalid state. Mark object as in invalid state/
    /// \param msg rvalue string
    void setErrorMessage(std::string &&msg) {
        errorMessage = std::move(msg);
        valid = false;
    }
 private:
    nlohmann::json config;
    bool valid;
    std::string errorMessage;
};

}
}

#endif //WSSERVER_EVENTCONFIG_H
