/*! 
 * scatter. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef SCATTER_BEARERAUTH_H
#define SCATTER_BEARERAUTH_H

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
    void performAuth(wss::web::Request &) const override;
    bool validateAuth(const wss::web::Request &) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const wss::web::Request &request) const override;
};

}

#endif //SCATTER_BEARERAUTH_H
