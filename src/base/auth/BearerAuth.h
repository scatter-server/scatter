/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_BEARERAUTH_H
#define WSSERVER_BEARERAUTH_H

#include "Auth.h"
#include "HeaderAuth.h"

namespace wss {

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
    void performAuth(httb::request &) const override;
    bool validateAuth(const httb::request &) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const httb::request &request) const override;
};

}

#endif //WSSERVER_BEARERAUTH_H
