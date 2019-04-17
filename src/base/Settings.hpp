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
#include <thread>

#ifdef setConfig
#undef setConfig
#endif

#ifdef setConfigDef
#undef setConfigDef
#endif

#define setConfig(path, src, name) path = (src).value(std::string(name), path)
#define setConfigDef(path, src, name, def) path = (src).value(std::string(name), def)

namespace wss {

struct AuthSettings {
  std::string type = "noauth";
  nlohmann::json data;
};

struct Secure {
  bool enabled = false;
  std::string crtPath;
  std::string keyPath;
};

struct Server {
  /// @todo create simple field
  struct Watchdog {
    bool enabled = false;
  };

  Secure secure;
  std::string endpoint = "/chat";
  std::string address = "*";
  uint16_t port = 8085;
  uint32_t workers = 8;
  std::string tmpDir = "/tmp";
  Watchdog watchdog;
  AuthSettings auth;
  std::string timezone;
  uint16_t logLevel = 0xFFFF;
};
struct RestApi {
  bool enabled = false;
  std::string address = "*";
  uint16_t port = 8082;
  AuthSettings auth;
  Secure secure;
};
struct Chat {
  struct Message {
    std::string maxSize = "10M";
    bool enableDeliveryStatus = false;
    bool enableSendBack = false;
    std::vector<std::string> ignoreTypesSendBack;
  };
  Message message = Message();
  bool enableUndeliveredQueue = false;
};
struct Event {
  bool enabled = false;
  bool enableRetry = false;
  bool sendBotMessages = false;
  int retryIntervalSeconds = 10;
  int retryCount = 3;
  uint32_t maxParallelWorkers = 8;
  std::vector<std::string> ignoreTypes;
  nlohmann::json targets;
};

struct Settings {
  static Settings &get() {
      static Settings s;
      return s;
  }
  Server server;
  RestApi restApi;
  Chat chat;
  Event event;
};

inline void from_json(const nlohmann::json &j, wss::Settings &in) {
    nlohmann::json server = j.at("server");
    setConfigDef(in.server.address, server, "address", "*");
    setConfigDef(in.server.endpoint, server, "endpoint", "/chat");
    setConfigDef(in.server.port, server, "port", (uint16_t) 8085);
    setConfigDef(in.server.timezone, server, "timezone", "UTC");
    setConfigDef(in.server.logLevel, server, "logLevel", 0xFFFF);


    if (server.find("secure") != server.end()) {
        setConfig(in.server.secure.crtPath, server["secure"], "crtPath");
        setConfig(in.server.secure.keyPath, server["secure"], "keyPath");
        setConfig(in.server.secure.enabled, server["secure"], "enabled");
    }

    if (server.find("auth") != server.end()) {
        in.server.auth = wss::AuthSettings();
        setConfigDef(in.server.auth.type, server["auth"], "type", "noauth");
        in.server.auth.data = server.at("auth");
    }

    uint32_t nativeThreadsMax =
        (uint32_t) (std::thread::hardware_concurrency() == 0 ? 2 : std::thread::hardware_concurrency());
    setConfigDef(in.server.workers, server, "workers", (uint32_t) nativeThreadsMax);
    setConfigDef(in.server.tmpDir, server, "tmpDir", "/tmp");
    if (server.find("watchdog") != server.end()) {
        setConfig(in.server.watchdog.enabled, server["watchdog"], "enabled");
    }

    if (j.find("restApi") != j.end() && j["restApi"].value("enabled", in.restApi.enabled)) {
        nlohmann::json restApi = j.at("restApi");

        setConfigDef(in.restApi.enabled, restApi, "enabled", false);
        setConfigDef(in.restApi.port, restApi, "port", (uint16_t) 8082);
        setConfigDef(in.restApi.address, restApi, "address", "*");
        if (restApi.find("auth") != restApi.end()) {
            in.restApi.auth = wss::AuthSettings();
            setConfigDef(in.restApi.auth.type, restApi["auth"], "type", "noauth");
            in.restApi.auth.data = restApi["auth"];
        }

        if (restApi.find("secure") != restApi.end()) {
            setConfig(in.restApi.secure.crtPath, restApi["secure"], "crtPath");
            setConfig(in.restApi.secure.keyPath, restApi["secure"], "keyPath");
            setConfig(in.restApi.secure.enabled, restApi["secure"], "enabled");
        }
    }

    if (j.find("chat") != j.end()) {
        nlohmann::json chat = j.at("chat");
        setConfigDef(in.chat.message.enableDeliveryStatus, chat, "enableDeliveryStatus", false);

        if (chat.find("message") != chat.end()) {
            nlohmann::json chatMessage = chat.at("message");

            setConfigDef(in.chat.message.maxSize, chatMessage, "maxSize", "10M");
            setConfigDef(in.chat.enableUndeliveredQueue, chatMessage, "enableUndeliveredQueue", false);
            setConfigDef(in.chat.message.enableSendBack, chatMessage, "enableSendBack", false);

            if (chatMessage.find("ignoredTypesSendBack") != chatMessage.end()) {
                in.chat.message.ignoreTypesSendBack =
                    chatMessage.at("ignoredTypesSendBack").get<std::vector<std::string>>();
            } else {
                in.chat.message.ignoreTypesSendBack.resize(0);
            }
        }
    }

    if (j.find("event") != j.end()) {
        nlohmann::json event = j.at("event");
        setConfig(in.event.enabled, event, "enabled");
        if (in.event.enabled) {
            if (event.find("targets") == event.end()) {
                in.event.targets = nlohmann::json();
                in.event.enabled = false;
                // no one target, disabling event notifier
                return;
            }

            in.event.targets = event.at("targets");

            setConfigDef(in.event.enableRetry, event, "enableRetry", true);
            setConfigDef(in.event.sendBotMessages, event, "sendBotMessages", false);
            setConfigDef(in.event.retryIntervalSeconds, event, "retryIntervalSeconds", 10);
            setConfigDef(in.event.retryCount, event, "retryCount", 3);
            setConfigDef(in.event.maxParallelWorkers, event, "maxParallelWorkers", (uint32_t) (nativeThreadsMax * 2));

            if (event.find("ignoreTypes") != event.end() && event.at("ignoreTypes").is_array()) {
                in.event.ignoreTypes = event.at("ignoreTypes").get<std::vector<std::string>>();
            } else {
                in.event.ignoreTypes.resize(0);
            }
        }
    }
}

}

#endif //WSSERVER_SETTINGS_HPP
