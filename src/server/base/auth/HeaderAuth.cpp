/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "HeaderAuth.h"

// HEADER
wss::HeaderAuth::HeaderAuth(std::string &&headerName, std::string &&value) :
    m_name(std::move(headerName)),
    m_value(std::move(value)) {
}
wss::HeaderAuth::HeaderAuth(const std::string &headerName, const std::string &value) :
    m_name(headerName),
    m_value(value) {
}
std::string wss::HeaderAuth::getType() {
    return "header";
}
void wss::HeaderAuth::performAuth(wss::web::Request &request) const {
    request.setHeader({m_name, getLocalValue()});
}
bool wss::HeaderAuth::validateAuth(const wss::web::Request &response) const {
    return response.compareHeaderValue(m_name, getLocalValue());
}
std::string wss::HeaderAuth::getLocalValue() const {
    return m_value;
}
std::string wss::HeaderAuth::getRemoteValue(const wss::web::Request &request) const {
    return request.getHeader(m_name);
}
