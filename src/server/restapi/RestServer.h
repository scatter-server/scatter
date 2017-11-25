/**
 * wsserver
 * RestServer.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef DOGSY_REST_SERVER_H
#define DOGSY_REST_SERVER_H

#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include <memory>
#include <algorithm>
#include "server_http.hpp"
#include "client_http.hpp"
#include "../defs.h"
#include "../chat/ChatMessageServer.h"
#include "../StandaloneService.h"

#define ACTION_DEFINE(name) void name(HttpResponse response, HttpRequest request)
#define ACTION_BIND(cName, mName) std::bind(&cName::mName, this, std::placeholders::_1, std::placeholders::_2)

namespace wss {

using HttpStatus = SimpleWeb::StatusCode;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpResponse = std::shared_ptr<HttpServer::Response>;
using HttpRequest = std::shared_ptr<HttpServer::Request>;

class RestServer : public virtual StandaloneService {
 public:
    RestServer(const std::string &host, unsigned short port);
    virtual ~RestServer();

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

    void setAddress(const std::string &address);
    void setAddress(std::string &&host);
    void setPort(uint16_t portNumber);

    template<typename ResponseCallback = std::function<void(HttpResponse, HttpRequest)> >
    RestServer &addEndpoint(const std::string &path, const std::string &methodName, ResponseCallback &&callback) {
        const std::string endpoint = "^/" + path + "$";
        L_INFO_F("Http Server", "Endpoint: %s /%s", methodName.c_str(), path.c_str());
        server.resource[endpoint][toolboxpp::strings::toUpper(methodName)] = std::forward<ResponseCallback>(callback);
        return *this;
    }

 protected:
    virtual void createEndpoints();

    void setResponseStatus(HttpResponse &response, HttpStatus status, std::size_t contentLength = 0u);
    void setContent(HttpResponse &response,
                    const std::string &content,
                    const std::string &contentType = "text/html");

    void setContent(HttpResponse &response,
                    std::string &&content,
                    std::string &&contentType = "text/html");
    std::string buildResponse(const std::vector<std::pair<std::string, std::string>> &parts);
 private:
    HttpServer server;
    std::unique_ptr<std::thread> workerThread;

    void cleanupEndpoints();
};

}
#endif //DOGSY_REST_SERVER_H
