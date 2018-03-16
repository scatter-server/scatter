/**
 * wsserver
 * MicroserviceController.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "RestServer.h"

wss::RestServer::RestServer(const std::string &host, unsigned short port) {
    server.config.port = port;
    if (host.length() > 1) {
        server.config.address = host;
    }
}

wss::RestServer::~RestServer() {
    stopService();
    joinThreads();
}

void wss::RestServer::createEndpoints() {
}

void wss::RestServer::cleanupEndpoints() {
    server.resource.clear();
}

void wss::RestServer::setAddress(const std::string &address) {
    if (address.length() > 1) {
        server.config.address = address;
    }
}
void wss::RestServer::setAddress(std::string &&address) {
    if (address.length() > 1) {
        server.config.address = std::move(address);
    }
}
void wss::RestServer::setPort(uint16_t portNumber) {
    server.config.port = portNumber;
}

void wss::RestServer::setResponseStatus(wss::HttpResponse &response,
                                        HttpStatus status,
                                        std::size_t contentLength) {

    const std::string &sCode = SimpleWeb::status_code(status);
    std::stringstream ss;
    ss << contentLength;

    *response << buildResponse(
        {
            {"HTTP/1.1",       sCode},
            {"Server",         "WS Rest Server"},
            {"Content-Length", ss.str()}
        });
}

void wss::RestServer::setContent(wss::HttpResponse &response,
                                 const std::string &content,
                                 const std::string &contentType) {
    *response << buildResponse({{"Content-Type", contentType}});
    *response << "\r\n" << content;
}

void wss::RestServer::setContent(wss::HttpResponse &response, std::string &&content, std::string &&contentType) {
    *response << buildResponse({{"Content-Type", std::move(contentType)}});
    *response << "\r\n" << content;
}

void wss::RestServer::setError(wss::HttpResponse &response,
                               wss::HttpStatus status,
                               int code,
                               const std::string &message) {
    json errorOut;
    errorOut["success"] = false;
    errorOut["status"] = code;
    errorOut["message"] = message;
    const std::string out = errorOut.dump();
    setResponseStatus(response, status, out.length());
    setContent(response, out, "application/json");
}
void wss::RestServer::setError(wss::HttpResponse &response,
                               wss::HttpStatus status,
                               int code,
                               std::string &&message) {
    json errorOut;
    errorOut["success"] = false;
    errorOut["status"] = code;
    errorOut["message"] = message;
    const std::string out = errorOut.dump();
    setResponseStatus(response, status, out.length());
    setContent(response, out, "application/json");
}

std::string wss::RestServer::buildResponse(const std::vector<std::pair<std::string, std::string>> &parts) {
    std::stringstream ss;
    for (const auto &part: parts) {
        if (part.first.substr(0, 4) == "HTTP") {
            ss << part.first << " " << part.second << "\r\n";
            continue;
        }
        ss << part.first << ": " << part.second << "\r\n";
    }

    return ss.str();
}
void wss::RestServer::joinThreads() {
    if (workerThread != nullptr && workerThread->joinable()) {
        workerThread->join();
    }
}
void wss::RestServer::detachThreads() {
    if (workerThread != nullptr) {
        workerThread->detach();
    }
}
void wss::RestServer::runService() {
    createEndpoints();

    workerThread = std::make_unique<std::thread>([this]() {
      // Start server
      this->server.start();
    });
    const char *hostname = server.config.address.empty() ? "[any:address]" : server.config.address.c_str();
    L_INFO_F("HttpServer", "Started at http://%s:%d", hostname, server.config.port);
}
void wss::RestServer::stopService() {
    this->server.stop();
    cleanupEndpoints();
}

void wss::RestServer::setAuth(const nlohmann::json &config) {
    auth = wss::auth::createFromConfig(config);
}

std::unique_ptr<wss::WebAuth> &wss::RestServer::getAuth() {
    return auth;
}





