/**
 * wsserver
 * RestServer.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "RestServer.h"

wss::RestServer::RestServer(
    const std::string &crtPath, const std::string &keyPath,
    const std::string &host, uint16_t port) :
    m_server(std::make_unique<HttpsServer>(crtPath, keyPath)),
    m_useSSL(true) {

    m_server->config.port = port;
    if (host.length() > 1) {
        m_server->config.address = host;
    }
}
wss::RestServer::RestServer(const std::string &host, unsigned short port) :
    m_server(std::make_unique<HttpServer>()),
    m_useSSL(false) {

    m_server->config.port = port;
    if (host.length() > 1) {
        m_server->config.address = host;
    }
}

wss::RestServer::~RestServer() {
    stopService();
    joinThreads();
}

void wss::RestServer::createEndpoints() {
}

void wss::RestServer::cleanupEndpoints() {
    m_server->resource.clear();
}

void wss::RestServer::setAddress(const std::string &address) {
    if (address.length() > 1) {
        m_server->config.address = address;
    }
}
void wss::RestServer::setAddress(std::string &&address) {
    if (address.length() > 1) {
        m_server->config.address = std::move(address);
    }
}
void wss::RestServer::setPort(uint16_t portNumber) {
    m_server->config.port = portNumber;
}

void wss::RestServer::setResponseStatus(wss::HttpResponse &response,
                                        HttpStatus status,
                                        std::size_t contentLength) {

    const std::string &sCode = wss::server::status_code(status);
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
    if (m_workerThread != nullptr && m_workerThread->joinable()) {
        m_workerThread->join();
    }
}
void wss::RestServer::detachThreads() {
    if (m_workerThread != nullptr) {
        m_workerThread->detach();
    }
}
void wss::RestServer::runService() {
    createEndpoints();

    m_workerThread = std::make_unique<std::thread>([this]() {
      // Start server
      m_server->start();
    });

    const char *proto = m_useSSL ? "https" : "http";
    const char *hostname = m_server->config.address.empty() ? "0.0.0.0" : m_server->config.address.c_str();
    L_INFO_F("HttpServer", "Started at %s://%s:%d", proto, hostname, m_server->config.port);

}
void wss::RestServer::stopService() {
    m_server->stop();
    cleanupEndpoints();
}

void wss::RestServer::setAuth(const nlohmann::json &config) {
    m_auth = wss::auth::registry::createFromConfig(config);
}

std::unique_ptr<wss::Auth> &wss::RestServer::getAuth() {
    return m_auth;
}





