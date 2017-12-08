/**
 * wsserver
 * Settings.hpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_SETTINGS_HPP
#define WSSERVER_SETTINGS_HPP

#include "json.hpp"
#include <iostream>

#define setConfig(path, src, name) path = (src).value(std::string(name), path)
#define setConfigDef(path, src, name, def) path = (src).value(std::string(name), def)

namespace wss {

struct AuthSettings {
  std::string type = "noauth";
  nlohmann::json data;
};

struct Server {
  struct Secure {
    bool enabled = false;
    std::string crtPath;
    std::string keyPath;
  };

  struct Watchdog {
    bool enabled = false;
    long connectionLifetimeSeconds = 600;
  };

  Secure secure = Secure();
  std::string endpoint = "/chat";
  std::string address = "*";
  uint16_t port = 8085;
  uint32_t workers = 8;
  std::string tmpDir = "/tmp";
  bool allowOverrideConnection = false;
  Watchdog watchdog = Watchdog();
  AuthSettings auth = AuthSettings();
};
struct RestApi {
  bool enabled = false;
  std::string address = "*";
  uint16_t port = 8082;
  AuthSettings auth = AuthSettings();
};
struct Chat {
  struct Message {
    std::string maxSize = "10M";
    bool enableDeliveryStatus = false;
  };
  Message message = Message();
  bool enableUndeliveredQueue = true;
};
struct Event {
  bool enabled = false;
  bool enableRetry = false;
  int retryIntervalSeconds = 10;
  int retryCount = 3;
  uint32_t maxParallelWorkers = 8;
  std::string sendStrategy = "onlineOnly";
  nlohmann::json targets;
};

struct Settings {
  static Settings &get() {
      static Settings s;
      return s;
  }
  Server server = Server();
  RestApi restApi = RestApi();
  Chat chat = Chat();
  Event event = Event();
};

inline void from_json(const nlohmann::json &j, wss::Settings &in) {
    nlohmann::json server = j.at("server");
    setConfig(in.server.address, server, "address");
    setConfig(in.server.endpoint, server, "endpoint");
    setConfig(in.server.port, server, "port");
    setConfig(in.server.allowOverrideConnection, server, "allowOverrideConnection");

    if (server.find("secure") != server.end() && server["secure"].value("enabled", false)) {
        in.server.secure.enabled = true;
        setConfig(in.server.secure.crtPath, server["secure"], "crtPath");
        setConfig(in.server.secure.keyPath, server["secure"], "keyPath");
    }

    if (server.find("auth") != server.end()) {
        in.server.auth = wss::AuthSettings();
        setConfig(in.server.auth.type, server["auth"], "type");
        in.server.auth.data = server.at("auth");
    }

    uint32_t nativeThreadsMax = std::thread::hardware_concurrency() == 0 ? 2 : std::thread::hardware_concurrency();
    setConfigDef(in.server.workers, server, "workers", nativeThreadsMax);
    setConfig(in.server.tmpDir, server, "tmpDir");
    if (server.find("watchdog") != server.end()) {
        setConfig(in.server.watchdog.enabled, server["watchdog"], "enabled");
        if (in.server.watchdog.enabled) {
            setConfig(in.server.watchdog.connectionLifetimeSeconds, server["watchdog"], "connectionLifetimeSeconds");
        }
    }

    if (j.find("restApi") != j.end() && j["restApi"].value("enabled", in.restApi.enabled)) {
        nlohmann::json restApi = j.at("restApi");

        setConfig(in.restApi.enabled, restApi, "enabled");
        setConfig(in.restApi.port, restApi, "port");
        setConfig(in.restApi.address, restApi, "address");
        if (restApi.find("auth") != restApi.end()) {
            in.restApi.auth = wss::AuthSettings();
            setConfig(in.restApi.auth.type, restApi["auth"], "type");
            in.restApi.auth.data = restApi["auth"];
        }
    }

    if (j.find("chat") != j.end()) {
        nlohmann::json chat = j.at("chat");
        setConfig(in.chat.message.maxSize, chat, "maxSize");
        setConfig(in.chat.message.enableDeliveryStatus, chat, "enableDeliveryStatus");
        setConfig(in.chat.enableUndeliveredQueue, chat, "enableUndeliveredQueue");
    } else {
        std::cout << "Chat config not found" << std::endl;
    }

    if (j.find("event") != j.end()) {
        nlohmann::json event = j.at("event");
        setConfig(in.event.enabled, event, "enabled");
        if (in.event.enabled) {
            setConfig(in.event.enableRetry, event, "enableRetry");
            setConfig(in.event.retryIntervalSeconds, event, "retryIntervalSeconds");
            setConfig(in.event.retryCount, event, "retryCount");
            setConfig(in.event.sendStrategy, event, "sendStrategy");
            setConfig(in.event.maxParallelWorkers, event, "maxParallelWorkers");
            in.event.targets = event.at("targets");
        }
    }
}

}

#endif //WSSERVER_SETTINGS_HPP
