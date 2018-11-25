/*! 
 * scatter. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef SCATTER_COOKIEAUTH_H
#define SCATTER_COOKIEAUTH_H

#include "Auth.h"

namespace wss {

/// \brief
/// Example config.json
/// "auth": {
///      "type": "cookie",
///      "name": "cookie_name",
///      "value": "cookie_value"
///    }
class CookieAuth : public Auth {
 public:
    CookieAuth(const std::string &cookieName, const std::string &cookieValue);

    CookieAuth(std::string &&cookieName, std::string &&cookieValue);

    std::string getType() override;
    void performAuth(wss::web::Request &request) const override;
    bool validateAuth(const wss::web::Request &request) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const wss::web::Request &request) const override;
 private:
    std::string m_name, m_value;
};

}

#endif //SCATTER_COOKIEAUTH_H
