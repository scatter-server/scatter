/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_EXECAUTH_H
#define WSSERVER_EXECAUTH_H

#include <unordered_map>
#include "Auth.h"

namespace wss {

/// \brief Authorizes on remote target
/// At all cases, this method checks response code: status >= 200 && status < 400 == ok, otherwise != ok
///
/// Config example:
/// {
///          "type": "remote",
///          "source": {
///            "type": "cookie",
///            "name": "auth_token"
///          },
///          "url": "https://api.mywebapp/auth/verify",
///          "method": "POST",
///          "headers": [
/// important thing if you sending not a x-www-form-urlencode
///            {"Content-Type": "application/json"}
///          ],
/// raw data, will be converted to string
/// if json, you can just setup a json object
///          "data": {
///            "token": "{0}"
///          }
/// or if it will be x-www-form-urlencode, set:
///          "data": "param1=value1&param2=value2" et cetera
///        }
///
class RemoteAuth : public Auth {
 public:
    RemoteAuth(const nlohmann::json &data, std::unique_ptr<Auth> &&source);
    std::string getType() override;
    void performAuth(httb::request &request) const override;
    bool validateAuth(const httb::request &request) const override;

    std::string getLocalValue() const override;
    std::string getRemoteValue(const httb::request &request) const override;
 private:
    std::unique_ptr<wss::Auth> m_source;
    std::string m_data;
    std::unordered_map<std::string, std::string> m_headers;
    std::string m_url;
    httb::request::method m_method;
};

}

#endif //WSSERVER_EXECAUTH_H
