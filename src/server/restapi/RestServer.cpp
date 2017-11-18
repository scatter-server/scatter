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

wss::RestServer &wss::RestServer::addEndpoint(const std::string &path,
                                              const std::string &methodName,
                                              ResponseCallback &&callback) {

    const std::string endpoint = "^/" + path + "$";
    server.resource[endpoint][toolboxpp::strings::toUpper(methodName)] = std::move(callback);
    return *this;
}

void wss::RestServer::run() {
    workerThread = new std::thread([this]() {
      // Start server
      this->server.start();
    });
    const char *hostname = server.config.address.empty() ? "<any:address>" : server.config.address.c_str();
    L_INFO_F("Http Server", "Started at http://%s:%d", hostname, server.config.port);
}

void wss::RestServer::stop() {
    this->server.stop();
}

void wss::RestServer::waitThread() {
    if (workerThread != nullptr) {
        workerThread->join();
    }
}
void wss::RestServer::detachThread() {
    if (workerThread != nullptr) {
        workerThread->detach();
    }
}
void wss::RestServer::setResponseStatus(const wss::HttpResponse &response,
                                        HttpStatus status,
                                        std::size_t contentLength) {

    const std::string &sCode = SimpleWeb::status_code(status);
    std::stringstream ss;
    ss << contentLength;

    *response << buildResponse(
        {
            {"HTTP/1.1 ",       sCode},
            {"Content-Length:", ss.str()}
        });
}

void wss::RestServer::setContent(const wss::HttpResponse &response,
                                 const std::string &content,
                                 const std::string &contentType) {
    *response << buildResponse({{"Content-Type:", contentType}});
    *response << "\r\n" << content;
}
std::string wss::RestServer::buildResponse(const std::vector<std::pair<std::string, std::string>> &parts) {
    std::stringstream ss;
    for (const auto &part: parts) {
        ss << part.first << " " << part.second << "\r\n";
    }

    return ss.str();
}




