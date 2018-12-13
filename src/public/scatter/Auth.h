/**
 * scatter
 * Auth.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_AUTH_HPP
#define SCATTER_AUTH_HPP

#include <string>
#include <sstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "HttpClient.h"

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
class SCATTER_EXPORT Auth {
 public:
    virtual ~Auth() = default;

    /// \brief Auth type
    /// \return string type used by json config
    virtual std::string getType();

    /// \brief Set required auth data to request
    virtual void performAuth(wss::web::Request &) const;

    /// \brief Validate responsed auth data
    /// \return true if validated
    virtual bool validateAuth(const wss::web::Request &) const;

    /// \brief Value setled in config
    /// \return
    virtual std::string getLocalValue() const;

    /// \brief Value fetched from connected client
    /// \return
    virtual std::string getRemoteValue(const wss::web::Request &) const;
};

namespace auth {
namespace registry {
/// \brief Creates WebAuth authenticator from json config
/// \param config Json config
/// \return unique_ptr of entire auth type,
/// if unsupported type, will return base WebAuth that does not requires authorization
SCATTER_EXPORT std::unique_ptr<wss::Auth> createFromConfig(const nlohmann::json &config);
}

}
}

#endif //SCATTER_AUTH_HPP