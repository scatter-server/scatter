/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_BASICAUTH_H
#define WSSERVER_BASICAUTH_H

#include "Auth.h"

namespace wss {

/// \brief Basic web authorization with base64(username:password) value
/// Example config.json
/// "auth": {
///      "type": "basic",
///      "user": "user",
///      "password": "password"
///    }
class BasicAuth : public Auth {
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
    void performAuth(httb::request &) const override;
    bool validateAuth(const httb::request &) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const httb::request &request) const override;
 private:
    std::string m_username, password;
};

}

#endif //WSSERVER_BASICAUTH_H
