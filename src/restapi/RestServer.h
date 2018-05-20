/**
 * wsserver
 * RestServer.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_REST_SERVER_H
#define WSSERVER_REST_SERVER_H

#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include <memory>
#include <algorithm>
#include <fmt/format.h>
#include "server_https.hpp"
#include "server_http.hpp"
#include "../wsserver_core.h"
#include "../chat/ChatServer.h"
#include "../base/StandaloneService.h"
#include "../base/auth/Auth.h"
#include "../helpers/helpers.h"

#define ACTION_DEFINE(name) void name(HttpResponse response, HttpRequest request)
#define ACTION_BIND(cName, mName) std::bind(&cName::mName, this, std::placeholders::_1, std::placeholders::_2)

namespace wss {

#ifdef USE_SECURE_SERVER
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTPS>;
#else
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
#endif

using HttpStatus = SimpleWeb::StatusCode;
using HttpResponse = std::shared_ptr<HttpServer::Response>;
using HttpRequest = std::shared_ptr<HttpServer::Request>;

class RestServer : public virtual StandaloneService {
 public:
    #ifdef USE_SECURE_SERVER
    RestServer(const std::string &crtPath, const std::string &keyPath,
               const std::string &host, uint16_t port);
    #else
    RestServer(const std::string &host, unsigned short port);
    #endif
    virtual ~RestServer();

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

    void setAuth(const nlohmann::json &config);
    void setAddress(const std::string &address);
    void setAddress(std::string &&host);
    void setPort(uint16_t portNumber);

    template<typename ResponseCallback = std::function<void(HttpResponse, HttpRequest)> >
    RestServer &addEndpoint(const std::string &path, const std::string &methodName, ResponseCallback &&callback) {
        const std::string endpoint = "^/" + path + "$";
        L_INFO_F("HttpServer", "Endpoint: %s /%s", methodName.c_str(), path.c_str());
        m_server.resource[endpoint][toolboxpp::strings::toUpper(methodName)] =
            [this, callback](wss::HttpResponse response, wss::HttpRequest request) {
              const wss::web::Request verifyRequest(request);
              response->close_connection_after_response = true;
              if (!m_auth->validateAuth(verifyRequest)) {

                  if (m_auth->getType() == "basic") {

                      json errorOut;
                      errorOut["success"] = false;
                      errorOut["status"] = 401;
                      errorOut["message"] = "Unauthorized";
                      const std::string out = errorOut.dump();

                      *response << buildResponse({
                                                     {"HTTP/1.1",         "401 Unauthorized"},
                                                     {"Server",           "WS Rest Server"},
                                                     {"Connection",       "keep-alive"},
                                                     {"Content-Length",   wss::helpers::toString(out.length())},
                                                     {"WWW-Authenticate", "Basic realm=\"Come to the dark side, we have cookies!\""},
                                                 });

                      *response << "\r\n";
                      *response << out;
                  } else {
                      setError(response, HttpStatus::client_error_unauthorized, 401, "Unauthorized");
                  }

                  return;
              }
              callback(response, request);
            };
        return *this;
    }

 protected:
    virtual void createEndpoints();

    std::unique_ptr<wss::Auth> &getAuth();

    void setResponseStatus(HttpResponse &response, HttpStatus status, std::size_t contentLength = 0u);
    void setContent(HttpResponse &response,
                    const std::string &content,
                    const std::string &contentType = "text/html");

    void setContent(HttpResponse &response,
                    std::string &&content,
                    std::string &&contentType = "text/html");

    void setError(HttpResponse &response, HttpStatus status, int code, const std::string &message);
    void setError(HttpResponse &response, HttpStatus status, int code, std::string &&message);
    std::string buildResponse(const std::vector<std::pair<std::string, std::string>> &parts);
 private:
    std::unique_ptr<Auth> m_auth;
    HttpServer m_server;
    std::unique_ptr<std::thread> m_workerThread;

    void cleanupEndpoints();
};

}
#endif //WSSERVER_REST_SERVER_H
