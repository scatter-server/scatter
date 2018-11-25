/*! 
 * scatter. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "BasicAuth.h"
#include "../../helpers/base64.h"

// BASIC
wss::BasicAuth::BasicAuth(std::string &&username, std::string &&password) :
    m_username(std::move(username)),
    password(std::move(password)) {
}
wss::BasicAuth::BasicAuth(const std::string &username, const std::string &password) :
    m_username(username),
    password(password) {
}
std::string wss::BasicAuth::getType() {
    return "basic";
}
void wss::BasicAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({"Authorization", getLocalValue()});
}
bool wss::BasicAuth::validateAuth(const wss::web::Request &request) const {
    return request.compareHeaderValue("Authorization", getLocalValue());
}
std::string wss::BasicAuth::getLocalValue() const {
    std::stringstream ss;
    ss << m_username << ":" << password;
    const std::string glued = ss.str();
    const std::string encoded = wss::utils::base64_encode(
        reinterpret_cast<const unsigned char *>(glued.c_str()),
        static_cast<unsigned int>(glued.length())
    );
    return "Basic " + encoded;
}
std::string wss::BasicAuth::getRemoteValue(const wss::web::Request &request) const {
    return request.getHeader("Authorization");
}
