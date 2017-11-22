#include <iostream>
#include <unordered_map>
#include <regex>
#include "chat/ChatServer.h"
#include "cmdline.hpp"
#include "restapi/ChatRestApi.h"
#include "event/EventNotifier.h"
#include "event/PostbackTarget.h"

using std::cout;
using std::cerr;
using std::endl;

using json = nlohmann::json;

size_t handleResponseData(void *buffer, size_t size, size_t nitems, void *userData) {
    ((std::string *) userData)->append((char *) buffer, size * nitems);
    return size * nitems;
}

bool hasKey(nlohmann::json &obj, const std::string &key) {
    return obj.find(key) != obj.end();
}

void setChatConfig(wss::ChatServer *webSocket, nlohmann::json &config) {

    if (!hasKey(config, "chat")) {
        return;
    }

    json chatConfig = config["chat"];

    std::string messageMaxSize = "10M";
    if (hasKey(chatConfig, "message")) {
        messageMaxSize = chatConfig["message"].value("maxSize", "10M");

        std::smatch res = toolboxpp::strings::matchRegexp(R"(^(\d+)(M|K)$)", messageMaxSize);
        if (res.size() != 3) {
            cerr
                << "Invalid message.maxSize value format. Must be: 10M or 1000K which means 10 megabytes or 1000 kilobytes"
                << endl;
            return;
        }

        unsigned long sz;
        try {
            sz = std::stoul(res[1]);
        } catch (const std::exception &e) {
            cerr << "Invalid value message.maxSize. " << e.what() << endl;
            return;
        }

        std::size_t maxBytes = 0;

        if (res[2] == "M") {
            // megabytes
            maxBytes = sz * 1024 * 1024;
        } else {
            // kilobytes
            maxBytes = sz * 1024;
        }
        webSocket->setMessageSizeLimit(maxBytes);
    }
}

void setServerConfig(wss::ChatServer *webSocket, nlohmann::json &config) {
    if (!hasKey(config, "server")) {
        return;
    }

    json serverConfig = config["server"];

    if (hasKey(serverConfig, "secure") && serverConfig["secure"].value("enabled", false)) {
        webSocket->enableTLS(
            serverConfig["secure"].value("crtPath", ""),
            serverConfig["secure"].value("keyPath", "")
        );
    }
}

bool setEventConfig(wss::ChatServer *webSocket, nlohmann::json &config) {
    if (hasKey(config, "event")) {
        return true;
    }

    using namespace wss::event;

    json eventConfig = config["event"];
    const bool enabledEvent = eventConfig.value("enabled", false);
    if (!enabledEvent) {
        return true;
    }

    if (!hasKey(eventConfig, "targets")) {
        cerr << "If event marked as enabled, you must set at least one target" << endl;
        return false;
    }

    json targets = eventConfig["targets"];
    if (!targets.is_array()) {
        cerr << "event.targets must be an array of objects" << endl;
        return false;
    }

    EventNotifier notifier(*webSocket);
    notifier.setMaxTries(eventConfig.value("retryCount", 3));
    notifier.setRetryIntervalSeconds(eventConfig.value("retryIntervalSeconds", 10));

    int i = 0;
    for (auto &target: targets) {
        if (!hasKey(target, "type")) {
            cerr << "event.targets[" << i << "].type - required" << endl;
            return false;
        }

        try {
            notifier.addTarget(target);
        } catch (const std::runtime_error &e) {
            cerr << e.what() << endl;
            return false;
        }

        i++;
    }

    notifier.subscribe();

    return true;
}

int main(int argc, char **argv) {
    cmdline::parser args;
    args.add<std::string>("config", 'C', "Use: wsserver -C [--config] /path/to/wsserver.json", true);
    args.parse(argc, argv);
    args.parse_check(argc, argv);
    const std::string configPath = args.get<std::string>("config");

    std::ifstream configFileStream(configPath);
    if (configFileStream.fail()) {
        perror("Can't open file");
        return 1;
    }

    json config;
    configFileStream >> config;

    if (config.find("server") == config.end()) {
        cerr << "server config required" << endl;
        return 1;
    }
    json serverConfig = config["server"];
    if (!serverConfig.is_object()) {
        cerr << "server config must be an object" << endl;
        return 1;
    }

    std::uint16_t port = serverConfig.value("port", (std::uint16_t) 9091);
    std::string address = serverConfig.value("address", "*");
    bool secure = serverConfig.value("secure.enabled", false);
    std::string endpoint = serverConfig.value("endpoint", "/chat/");
    bool enableApi = false;
    if (hasKey(config, "restApi")) {
        enableApi = config["restApi"].value("enabled", false);
    }

    wss::ChatServer webSocket(address, port, "^" + endpoint + "?$");

    setChatConfig(&webSocket, config);
    setServerConfig(&webSocket, config);

    bool hasSet = setEventConfig(&webSocket, config);
    if (!hasSet) {
        return 1;
    }

    webSocket.setThreadPoolSize(static_cast<size_t>(
                                    serverConfig.value("workers", std::thread::hardware_concurrency() * 2)
                                ));


    // run ws server
    webSocket.run();
    if (enableApi) {
        wss::ChatRestApi restApi("*", 8081);
        restApi.attachToChat(&webSocket);
        // run rest server
        restApi.run();
        restApi.waitThread();
    } else {
        webSocket.waitThread();
    }

    return 0;
}