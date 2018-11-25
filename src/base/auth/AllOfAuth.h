/*! 
 * scatter. 2018
 * 
 * \author Eduard Maximovich <edward.vstock@gmail.com>
 * \link https://github.com/edwardstock
 */

#ifndef SCATTER_ALLOFAUTH_H
#define SCATTER_ALLOFAUTH_H

#include "Auth.h"
#include "OneOfAuth.h"

namespace wss {

/// \brief Combination of autorizations. User can setup multiple variants of auth types
/// auth will success if allOf auth methods validated incoming request
class AllOfAuth : public OneOfAuth {
 public:
    AllOfAuth(std::vector<std::unique_ptr<Auth>> &&types);
    std::string getType() override;
    bool validateAuth(const wss::web::Request &request) const override;
};

}

#endif //SCATTER_ALLOFAUTH_H
