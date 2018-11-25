/**
 * scatter
 * RedisTarget.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_REDISTARGET_H
#define SCATTER_REDISTARGET_H

#include <cpp_redis/core/client.hpp>
#include "../public/Target.hpp"

namespace wss {
namespace event {
class RedisTarget : public wss::event::Target {
 public:
    enum Mode {
      Queue,
      Channel
    };

    RedisTarget(const nlohmann::json &config);
    ~RedisTarget();

    bool send(const wss::MessagePayload &payload, std::string &error) override;
    std::string getType() override;

 private:
    cpp_redis::client client;
    std::string modeTargetName;
    Mode mode;

    void onConnected(const nlohmann::json &config);
};
}
}

#endif //SCATTER_REDISTARGET_H
