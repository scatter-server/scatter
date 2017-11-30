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
    args.add<uint16_t>("verbosity", 'v', "Log level: 0 (error,critical), 1(0 + info), 2(all)", false, 2);

    //@TODO boost::program_options
    if (!args.parse(argc, reinterpret_cast<const char *const *>(argv))) {
        std::cerr << args.error_full();
        std::cout << args.usage();
        valid = false;
        return;
    }

    toolboxpp::Logger::get().setVerbosity(args.get<uint16_t>("verbosity"));

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

    nlohmann::json config;
    configFileStream >> config;
    wss::Settings &settings = wss::Settings::get();
    settings = config;

    #ifdef USE_SECURE_SERVER
    if (settings.server.secure.enabled) {
        const std::string crtPath = settings.server.secure.crtPath;
        const std::string keyPath = settings.server.secure.keyPath;
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

        std::stringstream endpointStream;
        endpointStream << "^" << settings.server.endpoint << "?$";
        const std::string res = endpointStream.str();

        webSocket = std::make_shared<wss::ChatMessageServer>(
            crtPath,
            keyPath,
            settings.server.address,
            settings.server.port,
            res
        );
    }
    #else
    // creating ws service
    std::stringstream endpointStream;
    endpointStream << "^" << settings.server.endpoint << "?$";
    const std::string res = endpointStream.str();
    webSocket = std::make_shared<wss::ChatMessageServer>(settings.server.address, settings.server.port, res);
    #endif



    // configuring ws service
    configureServer(settings);
    // configuring ws service chat
    configureChat(settings);

    // creating event notifier service
    eventNotifier = std::make_shared<wss::event::EventNotifier>(webSocket);
    // creating rest api service
    restServer = std::make_shared<wss::ChatRestServer>(webSocket);

    // adding commands to run websocket and to join it threads
    runService(webSocket);

    // configuring event notifier

    bool validConfig = configureEventNotifier(settings);
    if (!validConfig) {
        valid = false;
        return;
    } else {
        // adding commands to run event notifier and to join it threads
        runService(eventNotifier);
    }

    // configuring rest api service
    if (settings.restApi.enabled) {
        restServer->setAddress(settings.restApi.address);
        restServer->setPort(settings.restApi.port);
        restServer->setAuth(settings.restApi.auth.data);


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
void wss::ServerStarter::configureChat(wss::Settings &settings) {
    using toolboxpp::strings::matchRegexp;

    std::string messageMaxSize = settings.chat.message.maxSize;

    std::smatch res = matchRegexp(R"(^(\d+)(M|K)$)", messageMaxSize);
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
    webSocket->setEnabledMessageDeliveryStatus(settings.chat.message.enableDeliveryStatus);
}
void wss::ServerStarter::configureServer(wss::Settings &settings) {

    // setting num of workers (threads in thread pool)
    webSocket->setThreadPoolSize(settings.server.workers);
}
bool wss::ServerStarter::configureEventNotifier(wss::Settings &settings) {
    if (!settings.event.enabled) {
        return true;
    }

    using namespace wss::event;

    if (settings.event.targets.is_null()) {
        cerr << "If event marked as enabled, you must set at least one target" << endl;
        return false;
    }

    if (!settings.event.targets.is_array()) {
        cerr << "event.targets must be an array of objects" << endl;
        return false;
    }

    eventNotifier->setMaxTries(settings.event.retryCount);
    eventNotifier->setRetryIntervalSeconds(settings.event.retryIntervalSeconds);
    eventNotifier->setSendStrategy(settings.event.sendStrategy);

    int i = 0;
    for (auto &target: settings.event.targets) {
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
