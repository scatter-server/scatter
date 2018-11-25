/*! 
 * scatter. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "OneOfAuth.h"

// OneOfAuth
wss::OneOfAuth::OneOfAuth(std::vector<std::unique_ptr<wss::Auth>> &&data) : types(std::move(data)) {
}

std::string wss::OneOfAuth::getType() {
    return "oneOf";
}
void wss::OneOfAuth::performAuth(wss::web::Request &request) const {
    for (auto &auth: types) {
        auth->performAuth(request);
    }
}
bool wss::OneOfAuth::validateAuth(const wss::web::Request &request) const {
    for (auto &auth: types) {
        if (auth->validateAuth(request)) {
            return true;
        }
    }
    return false;
}
std::string wss::OneOfAuth::getLocalValue() const {
    return Auth::getLocalValue();
}
