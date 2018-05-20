/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_HEADERAUTH_H
#define WSSERVER_HEADERAUTH_H

#include "Auth.h"

namespace wss {

/// \brief Simple header authorization, using custom header value
/// Example config.json
/// "auth": {
///      "type": "header",
///      "name": "X-Api-Token",
///      "value": "token"
///    }
class HeaderAuth : public Auth {
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

    std::string getLocalValue() const override;
    std::string getRemoteValue(const wss::web::Request &request) const override;
 private:
    std::string m_name, m_value;
};

}

#endif //WSSERVER_HEADERAUTH_H
