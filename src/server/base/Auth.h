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
/// type: noauth
///    : has no fields
///
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
///
/// type: cookie
///   name: cookie_name
///   value: cookie_value
///
/// type: oneOf
///   types: [...list of auth objects...]
///
/// type: allOf
///   types: [...list of auth objects...]
///
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

/// \brief Combination of autorizations. User can setup multiple variants of auth types
/// auth will success if oneOf auth methods validated incoming request
/// Example config.json (same example for AllOfAuth):
/// "auth": {
///      "type": "oneOf",
///      "types": [
///        {
///          "type": "cookie",
///          "name": "myjswebsocket",
///          "value": "auth_token"
///        },
///        {
///          "type": "header",
///          "name": "some_header",
///          "value": "some_header_value"
///        }
///      ]
///    }
class OneOfAuth : public WebAuth {
 public:
    /// \brief Inits with list of auth objects unique_ptrs
    /// \param types
    explicit OneOfAuth(std::vector<std::unique_ptr<wss::WebAuth>> &&types);
    /// \brief Auth type
    /// \return string type used by json config
    std::string getType() override;
    /// \brief Set required auth data to request
    void performAuth(wss::web::Request &request) const override;
    /// \brief Validate responsed auth data
    /// \return true if validated
    bool validateAuth(const wss::web::Request &request) const override;
 protected:
    /// \brief Dummy value
    /// \return for this auth type, values is not required
    std::string getValue() const override;
    std::vector<std::unique_ptr<wss::WebAuth>> types;
};

/// \brief Combination of autorizations. User can setup multiple variants of auth types
/// auth will success if allOf auth methods validated incoming request
class AllOfAuth : public OneOfAuth {
 public:
    AllOfAuth(std::vector<std::unique_ptr<WebAuth>> &&types);
    std::string getType() override;
    bool validateAuth(const wss::web::Request &request) const override;
};

/// \brief Basic web authorization with base64(username:password) value
/// Example config.json
/// "auth": {
///      "type": "basic",
///      "user": "user",
///      "password": "password"
///    }
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
    std::string m_username, password;
};

/// \brief Simple header authorization, using custom header value
/// Example config.json
/// "auth": {
///      "type": "header",
///      "name": "X-Api-Token",
///      "value": "token"
///    }
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
    std::string m_name, m_value;
};

/// \brief OAuth bearer authorization, using header "Authorization: Bearer {token}"
/// Example config.json
/// "auth": {
///      "type": "bearer",
///      "value": "token" - !!! without bearer prefix !!!
///    }
class BearerAuth : public HeaderAuth {
 public:
    /// \brief rvalue ctr
    /// \param value Bearer oauth token
    explicit BearerAuth(std::string &&value);
    /// \brief lvalue ctr
    /// \param value
    explicit BearerAuth(const std::string &value);

    /// \brief Auth type
    /// \return string: bearer
    std::string getType() override;
    void performAuth(wss::web::Request &) const override;
    bool validateAuth(const wss::web::Request &) const override;
 protected:
    std::string getValue() const override;
};

/// \brief
/// Example config.json
/// "auth": {
///      "type": "cookie",
///      "name": "cookie_name",
///      "value": "cookie_value"
///    }
class CookieAuth : public WebAuth {
 public:
    CookieAuth(const std::string &cookieName, const std::string &cookieValue);

    CookieAuth(std::string &&cookieName, std::string &&cookieValue);

    std::string getType() override;
    void performAuth(wss::web::Request &request) const override;
    bool validateAuth(const wss::web::Request &request) const override;
 protected:
    std::string getValue() const override;
 private:
    std::string m_name, m_value;
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
