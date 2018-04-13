/**
 * wsserver
 * RedisTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_REDISTARGET_H
#define WSSERVER_REDISTARGET_H

#include "Target.hpp"

class RedisTarget : public wss::event::Target {
 public:
    RedisTarget(const nlohmann::json &config);

    bool send(const wss::MessagePayload &payload, std::string &error) override;
    std::string getType() override;
};

#endif //WSSERVER_REDISTARGET_H
