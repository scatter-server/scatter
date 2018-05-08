/**
 * wsserver
 * WsServer.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ServerStarter.h"

static wss::ServerStarter *self; // for signal instance

wss::ServerStarter::ServerStarter(int argc, const char **argv) : m_args() {
    m_args.add<std::string>("config", 'C', "Config file path /path/to/config.json", true);
    m_args.add<bool>("test", 'T', "Test config", false, false);
    m_args.add<uint16_t>("verbosity", 'l', "Log level: 0 (error,critical), 1(0 + info), 2(all)", false, 2);

    if (argc == 2 && strcmp(argv[argc - 1], "-v") == 0) {
        std::cout << WSSERVER_VERSION << std::endl;
        return;
    }

    bool parsed = m_args.parse(argc, reinterpret_cast<const char *const *>(argv));

    //@TODO boost::program_options
    if (!parsed) {
        std::cerr << m_args.error_full();
        std::cout << m_args.usage();
        m_valid = false;
        return;
    }

    toolboxpp::Logger::get().setVerbosity(m_args.get<uint16_t>("verbosity"));

    m_args.parse_check(argc, const_cast<char **>(argv));
    const std::string configPath = m_args.get<std::string>("config");
    m_isConfigTest = m_args.get<bool>("test");

    // Getting config file
    std::ifstream configFileStream(configPath);
    if (configFileStream.fail()) {
        perror("Can't open file");
        m_valid = false;
        return;
    }

    nlohmann::json config;
    configFileStream >> config;
    wss::Settings &settings = wss::Settings::get();
    settings = config;

    #ifdef USE_SECURE_SERVER
    const std::string crtPath = settings.server.secure.crtPath;
    const std::string keyPath = settings.server.secure.keyPath;
    if (crtPath.empty() || keyPath.empty()) {
        cerr << "Certificate and private key paths can'be empty";
        m_valid = false;
        return;
    }

    using toolboxpp::fs::exists;
    if (!exists(crtPath)) {
        cerr << "Certificate file not found at " << crtPath << endl;
        m_valid = false;
        return;
    }
    if (!exists(keyPath)) {
        cerr << "Private Key file not found at " << keyPath << endl;
        m_valid = false;
        return;
    }

    std::stringstream endpointStream;
    endpointStream << "^" << settings.server.endpoint << "?$";
    const std::string res = endpointStream.str();

    m_webSocket = std::make_shared<wss::ChatServer>(
        crtPath,
        keyPath,
        settings.server.address,
        settings.server.port,
        res
    );

    // creating rest api service
    m_restServer = std::make_shared<wss::ChatRestServer>(m_webSocket, crtPath, keyPath);
    #else
    // creating ws service
    std::stringstream endpointStream;
    endpointStream << "^" << settings.server.endpoint << "?$";
    const std::string res = endpointStream.str();
    webSocket = std::make_shared<wss::ChatServer>(settings.server.address, settings.server.port, res);

    restServer = std::make_shared<wss::ChatRestServer>(webSocket);
    #endif

    // configuring ws service
    configureServer(settings);
    // configuring ws service chat
    configureChat(settings);

    // creating event notifier service
    m_eventNotifier = std::make_shared<wss::event::EventNotifier>(m_webSocket);


    // adding commands to run websocket and to join it threads
    runService(m_webSocket);

    // configuring event notifier
    if (settings.event.enabled) {
        bool validConfig = configureEventNotifier(settings);
        if (!validConfig) {
            m_valid = false;
            return;
        } else {
            // adding commands to run event notifier and to join it threads
            runService(m_eventNotifier);
        }
    }

    // configuring rest api service
    if (settings.restApi.enabled) {
        m_restServer->setAddress(settings.restApi.address);
        m_restServer->setPort(settings.restApi.port);
        m_restServer->setAuth(settings.restApi.auth.data);

        runService(m_restServer);
    }

    if (m_isConfigTest) {
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
    for (auto &service: m_services) {
        service->stopService();
    }
}
void wss::ServerStarter::run() {
    for (auto &service: m_services) {
        service->runService();
    }

    std::for_each(m_services.rbegin(), m_services.rend(), [](const std::shared_ptr<wss::StandaloneService> &s) {
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

    auto res = matchRegexp(R"(^(\d+)(M|K)$)", messageMaxSize);
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
    m_webSocket->setMessageSizeLimit(maxBytes);
    m_webSocket->setEnabledMessageDeliveryStatus(settings.chat.message.enableDeliveryStatus);
}
void wss::ServerStarter::configureServer(wss::Settings &settings) {

    // setting num of workers (threads in thread pool)
    m_webSocket->setThreadPoolSize(settings.server.workers);
    m_webSocket->setAuth(settings.server.auth.data);
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

    m_eventNotifier->setMaxTries(settings.event.retryCount);
    m_eventNotifier->setRetryIntervalSeconds(settings.event.retryIntervalSeconds);

    int i = 0;
    for (auto &target: settings.event.targets) {
        if (!hasKey(target, "type")) {
            cerr << "event.targets[" << i << "].type - required" << endl;
            return false;
        }

        try {
            m_eventNotifier->addTarget(target);
        } catch (const std::runtime_error &e) {
            cerr << e.what() << endl;
            return false;
        }

        i++;
    }

    return true;
}
bool wss::ServerStarter::isValid() {
    return m_valid;
}
