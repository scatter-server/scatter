/**
 * wsserver
 * Auth.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_AUTH_HPP
#define WSSERVER_AUTH_HPP

#include <string>
#include <sstream>
#include <memory>
#include "json.hpp"
#include "../web/HttpClient.h"
#include "../helpers/base64.h"

namespace wss {

/// \brief  available:
/// type: basic
///    user: basic_username
///    password: basic_password
///
/// type: header,
///    name: header_name
///    value: header_value
///
/// type: bearer
///   value: bearer_token
class WebAuth {
 public:
    /// \brief Auth type
    /// \return string type used by json config
    virtual std::string getType();

    /// \brief Set required auth data to request
    virtual void performAuth(wss::web::Request &) const;

    /// \brief Validate responsed auth data
    /// \return true if validated
    virtual bool validateAuth(const wss::web::Request &) const;

 protected:
    /// \brief Prepare value for send/validate
    /// \return value used to authorize
    virtual std::string getValue() const;
};

/// \brief Basic web authorization with base64(username:password) value
class BasicAuth : public WebAuth {
 public:
    /// \brief rvalue constructor
    /// \param username
    /// \param password
    BasicAuth(std::string &&username, std::string &&password);

    /// \brief lvalue constructor
    /// \param username
    /// \param password
    BasicAuth(const std::string &username, const std::string &password);

    /// \brief Auth type
    /// \return string: basic
    std::string getType() override;
    void performAuth(wss::web::Request &) const override;
    bool validateAuth(const wss::web::Request &) const override;
 protected:
    std::string getValue() const override;
 private:
    std::string username, password;
};

/// \brief Simple header authorization, using custom header value
class HeaderAuth : public WebAuth {
 public:
    /// \brief rvalue constructor
    /// \param headerName name of the header
    /// \param value header value
    HeaderAuth(std::string &&headerName, std::string &&value);

    /// \brief lvalue constructor
    /// \param headerName name of the header
    /// \param value header value
    HeaderAuth(const std::string &headerName, const std::string &value);

    /// \brief Auth type
    /// \return string: header
    std::string getType() override;
    void performAuth(wss::web::Request &) const override;
    bool validateAuth(const wss::web::Request &) const override;
 protected:
    std::string getValue() const override;
 private:
    std::string name, value;
};

/// \brief OAuth bearer authorization, using header "Authorization: Bearer {token}"
class BearerAuth : public HeaderAuth {
 public:
    /// \brief
    /// \param value Bearer oauth token
    explicit BearerAuth(std::string &&value);

    /// \brief Auth type
    /// \return string: bearer
    std::string getType() override;
    void performAuth(wss::web::Request &) const override;
    bool validateAuth(const wss::web::Request &) const override;
 protected:
    std::string getValue() const override;
};

namespace auth {
/// \brief Creates WebAuth authenticator from json config
/// \param config Json config
/// \return unique_ptr of entire auth type,
/// if unsupported type, will return base WebAuth that does not requires authorization
std::unique_ptr<wss::WebAuth> createFromConfig(const nlohmann::json &config);
}

}

#endif //WSSERVER_AUTH_HPP
