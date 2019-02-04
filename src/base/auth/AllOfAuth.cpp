/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#include "AllOfAuth.h"
#include "OneOfAuth.h"

wss::AllOfAuth::AllOfAuth(std::vector<std::unique_ptr<wss::Auth>> &&types) : OneOfAuth(std::move(types)) {
}

std::string wss::AllOfAuth::getType() {
    return "allOf";
}
bool wss::AllOfAuth::validateAuth(const httb::request &request) const {
    for (auto &type: types) {
        if (!type->validateAuth(request)) {
            return false;
        }
    }

    return true;
}
