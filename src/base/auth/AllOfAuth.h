/*! 
 * wsserver. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef WSSERVER_ALLOFAUTH_H
#define WSSERVER_ALLOFAUTH_H

#include "Auth.h"
#include "OneOfAuth.h"

namespace wss {

/// \brief Combination of autorizations. User can setup multiple variants of auth types
/// auth will success if allOf auth methods validated incoming request
class AllOfAuth : public OneOfAuth {
 public:
    AllOfAuth(std::vector<std::unique_ptr<Auth>> &&types);
    std::string getType() override;
    bool validateAuth(const httb::request &request) const override;
};

}

#endif //WSSERVER_ALLOFAUTH_H
