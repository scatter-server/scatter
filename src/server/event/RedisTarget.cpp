/**
 * wsserver
 * RedisTarget.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "RedisTarget.h"
wss::event::RedisTarget::RedisTarget(const nlohmann::json &config) :
    Target(config),
    modeTargetName("wsserver_events_queue"),
    mode(Queue) {

    if (config.find("unixSocket") != config.end()) {
        const std::string sock = config.at("unixSocket").get<std::string>();
        client.connect(sock,
                       0,
                       [this, &config](const std::string &host, size_t, cpp_redis::client::connect_state status) {
                         if (status == cpp_redis::client::connect_state::failed) {
                             appendErrorMessage(fmt::format("Can't connect to redis: {0}", host));
                         } else if (status == cpp_redis::client::connect_state::ok) {
                             onConnected(config);
                         }
                       });
    } else {
        const std::string addr = config.value("address", "127.0.0.1");
        size_t portNum = config.value("port", (size_t) 6379);
        try {
            client.connect(addr,
                           portNum,
                           [this, &config](const std::string &host,
                                           size_t port,
                                           cpp_redis::client::connect_state status) {
                             if (status == cpp_redis::client::connect_state::failed) {
                                 appendErrorMessage(fmt::format("Can't connect to redis: {0}:{1}", host, port));
                             } else if (status == cpp_redis::client::connect_state::ok) {
                                 onConnected(config);
                             }
                           });
        } catch (const std::exception &err) {
            appendErrorMessage(err.what());
            return;
        }

    }

    client.sync_commit();
}

bool wss::event::RedisTarget::send(const wss::MessagePayload &msg, std::string &err) {
    const std::string jsonMsg = msg.toJson();

    bool success = true;

    const auto result = [&success, &err](const cpp_redis::reply &reply) {
      if (reply.is_error()) {
          success = false;
          err = reply.error();
      }
    };

    switch (mode) {
        case Queue:client.rpush(modeTargetName, {msg.toJson()}, result);
            break;
        case Channel:client.publish(modeTargetName, msg.toJson(), result);
            break;
    }

    client.sync_commit();

    return success;
}
std::string wss::event::RedisTarget::getType() {
    return "redis";
}
wss::event::RedisTarget::~RedisTarget() {
    if (client.is_connected()) {
        client.disconnect(true);
    }
}
void wss::event::RedisTarget::onConnected(const nlohmann::json &config) {
    if (config.find("database") != config.end()) {
        client.select(config.at("database").get<int>(), [this](cpp_redis::reply &reply) {
          if (reply.is_error()) {
              appendErrorMessage(std::string(reply.error()));
          }
        });
    }

    if (config.find("password") != config.end()) {
        const std::string pass = config.at("password").get<std::string>();
        client.auth(pass, [this](const cpp_redis::reply &reply) {
          if (reply.is_error()) {
              appendErrorMessage(std::string(reply.error()));
          }
        });
    }

    if (config.find("mode") != config.end()) {
        const nlohmann::json modeObj = config.at("mode");
        const std::string m = modeObj.value("type", "queue");

        if (toolboxpp::strings::equalsIgnoreCase(m, "queue")) {
            mode = Queue;
            modeTargetName = modeObj.value("name", "wsserver_events_queue");
        } else if (toolboxpp::strings::equalsIgnoreCase(m, "channel")) {
            mode = Channel;
            modeTargetName = modeObj.value("name", "wsserver_events_channel");
        } else {
            appendErrorMessage(fmt::format("Unknown mode for redis target: {0}", m));
            return;
        }
    }
}
