/**
 * wsserver
 * WsServer.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ServerStarter.h"

static wss::ServerStarter *self; // for signal instance

wss::ServerStarter::ServerStarter(int argc, const char **argv) : args() {
    args.add<std::string>("config", 'C', "Config file path /path/to/config.json", true);
    args.add<bool>("test", 'T', "Test config", false, false);
    args.add<unsigned short>("verbosity", 'v', "Log level: 0 (error,critical), 1(0 + info), 2(all)", false, 2);

    //@TODO boost::program_options
    if (!args.parse(argc, reinterpret_cast<const char *const *>(argv))) {
        std::cerr << args.error_full();
        std::cout << args.usage();
        valid = false;
        return;
    }

    toolboxpp::Logger::get().setVerbosity(args.get<unsigned short>("verbosity"));

    args.parse_check(argc, const_cast<char **>(argv));
    const std::string configPath = args.get<std::string>("config");
    isConfigTest = args.get<bool>("test");

    // Getting config file
    std::ifstream configFileStream(configPath);
    if (configFileStream.fail()) {
        perror("Can't open file");
        valid = false;
        return;
    }

    json config;
    configFileStream >> config;

    // Check server config
    if (config.find("server") == config.end()) {
        cerr << "server config required" << endl;
        valid = false;
        return;
    }

    json serverConfig = config["server"];
    // check server config is an object
    if (!serverConfig.is_object()) {
        cerr << "server config must be an object" << endl;
        valid = false;
        return;
    }

    // getting ws server port
    int port = serverConfig.value("port", 9091);
    if (port < 1 || port > 65535) {
        cerr << "Invalid server.port range: must be from 1 to 65535" << endl;
        valid = false;
        return;
    }

    // getting ws server ip
    std::string address = serverConfig.value("address", "*");
    // websocket endpoint path
    std::string endpoint = serverConfig.value("endpoint", "/chat/");

    // check for enabled rest api server
    bool enableRestApi = false;
    if (hasKey(config, "restApi")) {
        enableRestApi = config["restApi"].value("enabled", false);
    }

    #ifdef USE_SECURE_SERVER
    if (hasKey(serverConfig, "secure")) {
        const std::string crtPath = serverConfig["secure"].at("crtPath");
        const std::string keyPath = serverConfig["secure"].at("crtPath");
        if (crtPath.empty() || keyPath.empty()) {
            cerr << "Certificate and private key paths can'be empty";
            valid = false;
            return;
        }

        using toolboxpp::fs::exists;
        if (!exists(crtPath)) {
            cerr << "Certificate file not found at " << crtPath << endl;
            valid = false;
            return;
        }
        if (!exists(keyPath)) {
            cerr << "Private Key file not found at " << keyPath << endl;
            valid = false;
            return;
        }

        webSocket = std::make_shared<wss::ChatMessageServer>(
            serverConfig["secure"].at("crtPath"),
            serverConfig["secure"].at("keyPath"),
            address,
            (std::uint16_t) port,
            "^" + endpoint + "?$"
        );
    }
    #else
    // creating ws service
    std::stringstream endpointStream;
    endpointStream << "^" << endpoint << "?$";
    const std::string res = endpointStream.str();
    webSocket = std::make_shared<wss::ChatMessageServer>(address, (std::uint16_t) port, res);
    #endif



    // configuring ws service
    configureServer(config);
    // configuring ws service chat
    configureChat(config);

    // creating event notifier service
    eventNotifier = std::make_shared<wss::event::EventNotifier>(webSocket);
    // creating rest api service
    restServer = std::make_shared<wss::ChatRestServer>(webSocket);

    // adding commands to run websocket and to join it threads
    runService(webSocket);

    // configuring event notifier
    if (hasKey(config, "event") && config["event"].value("enabled", false)) {
        bool validConfig = configureEventNotifier(config);
        if (!validConfig) {
            valid = false;
            return;
        }

        // adding commands to run event notifier and to join it threads
        runService(eventNotifier);
    }

    // configuring rest api service
    if (enableRestApi) {
        const json restConfig = config["restApi"];
        restServer->setAddress(restConfig.value("address", "*"));
        restServer->setPort(restConfig.value("port", (uint16_t) 8081));

        runService(restServer);
    }

    if (isConfigTest) {
        std::cout << "Configuration verified - everything is ok!" << std::endl;
        return;
    }

    self = this;

    signal(SIGINT, &ServerStarter::signalHandler);
    signal(SIGTERM, &ServerStarter::signalHandler);
}
wss::ServerStarter::~ServerStarter() {
    delete self;
}
void wss::ServerStarter::stop() {
    for (auto &service: services) {
        service->stopService();
    }
}
void wss::ServerStarter::run() {
    for (auto &service: services) {
        service->runService();
    }

    std::for_each(services.rbegin(), services.rend(), [](const std::shared_ptr<wss::StandaloneService> &s) {
      s->joinThreads();
    });
}
void wss::ServerStarter::signalHandler(int signum) {
    std::cout << "[" << signum << "] Stopping server..." << std::endl;
    self->stop();
    std::cout << "Stopped!" << std::endl;

    // Terminate program
    exit(signum);
}
bool wss::ServerStarter::hasKey(nlohmann::json &obj, const std::string &key) {
    return obj.find(key) != obj.end();
}
void wss::ServerStarter::configureChat(nlohmann::json &config) {

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
        webSocket->setEnabledMessageDeliveryStatus(chatConfig["message"].value("enableDeliveryStatus", false));
    }
}
void wss::ServerStarter::configureServer(nlohmann::json &config) {
    if (!hasKey(config, "server")) {
        return;
    }

    json serverConfig = config["server"];

    // setting num of workers (threads in thread pool)
    webSocket->setThreadPoolSize(static_cast<size_t>(
                                     serverConfig.value("workers", std::thread::hardware_concurrency())
                                 ));
}
bool wss::ServerStarter::configureEventNotifier(nlohmann::json &config) {
    if (!hasKey(config, "event")) {
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

    eventNotifier->setMaxTries(eventConfig.value("retryCount", 3));
    eventNotifier->setRetryIntervalSeconds(eventConfig.value("retryIntervalSeconds", 10));

    int i = 0;
    for (auto &target: targets) {
        if (!hasKey(target, "type")) {
            cerr << "event.targets[" << i << "].type - required" << endl;
            return false;
        }

        try {
            eventNotifier->addTarget(target);
        } catch (const std::runtime_error &e) {
            cerr << e.what() << endl;
            return false;
        }

        i++;
    }

    return true;
}
bool wss::ServerStarter::isValid() {
    return valid;
}
