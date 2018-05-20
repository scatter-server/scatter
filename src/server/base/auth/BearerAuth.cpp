/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "BearerAuth.h"

// BEARER
wss::BearerAuth::BearerAuth(std::string &&value) :
    HeaderAuth("Authorization", std::string("Bearer " + value)) { }

wss::BearerAuth::BearerAuth(const std::string &value) :
    HeaderAuth("Authorization", std::string("Bearer " + value)) {

}
std::string wss::BearerAuth::getType() {
    return "bearer";
}
bool wss::BearerAuth::validateAuth(const wss::web::Request &response) const {
    return HeaderAuth::validateAuth(response);
}
std::string wss::BearerAuth::getLocalValue() const {
    return HeaderAuth::getLocalValue();
}
void wss::BearerAuth::performAuth(wss::web::Request &request) const {
    HeaderAuth::performAuth(request);
}
std::string wss::BearerAuth::getRemoteValue(const wss::web::Request &request) const {
    return HeaderAuth::getRemoteValue(request);
}
