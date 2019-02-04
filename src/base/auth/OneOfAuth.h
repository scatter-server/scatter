/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_ONEOFAUTH_H
#define WSSERVER_ONEOFAUTH_H

#include "Auth.h"

namespace wss {

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
class OneOfAuth : public Auth {
 public:
    /// \brief Inits with list of auth objects unique_ptrs
    /// \param types
    explicit OneOfAuth(std::vector<std::unique_ptr<wss::Auth>> &&types);
    /// \brief Auth type
    /// \return string type used by json config
    std::string getType() override;
    /// \brief Set required auth data to request
    void performAuth(httb::request &request) const override;
    /// \brief Validate responsed auth data
    /// \return true if validated
    bool validateAuth(const httb::request &request) const override;

    /// \brief Dummy value
    /// \return for this auth type, values is not required
    std::string getLocalValue() const override;

 protected:
    std::vector<std::unique_ptr<wss::Auth>> types;
};

}

#endif //WSSERVER_ONEOFAUTH_H
