/**
 * wsserver
 * RedisTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "RedisTarget.h"
RedisTarget::RedisTarget(const nlohmann::json &config) : Target(config) {

}
bool RedisTarget::send(const wss::MessagePayload &, std::string &) {
    return false;
}
std::string RedisTarget::getType() {
    return std::string();
}
