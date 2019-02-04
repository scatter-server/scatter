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
#include "../base/BaseServer.h"
#include "../wsserver_core.h"
#include "../base/http/HttpServer.h"
#include "../chat/ChatServer.h"
#include "../base/StandaloneService.h"
#include "../base/auth/Auth.h"
#include "../helpers/helpers.h"
#include "../base/Settings.hpp"

#define ACTION_DEFINE(name) void name(wss::HttpResponse response, wss::HttpRequest request)
#define ACTION_BIND(cName, mName) std::bind(&cName::mName, this, std::placeholders::_1, std::placeholders::_2)

namespace wss {

using namespace wss::server;

using HttpBase = wss::server::http::ServerBase;
using HttpsServer = wss::server::http::ServerSecure;
using HttpServer = wss::server::http::Server;
using HttpStatus = wss::server::StatusCode;
using HttpResponse = std::shared_ptr<HttpBase::Response>;
using HttpsResponse = std::shared_ptr<HttpBase::Response>;
using HttpRequest = std::shared_ptr<HttpBase::Request>;
using HttpsRequest = std::shared_ptr<HttpBase::Request>;

class RestServer : public virtual StandaloneService {
 public:
    RestServer(const std::string &crtPath, const std::string &keyPath,
               const std::string &host, uint16_t port);
    RestServer(const std::string &host, unsigned short port);
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

        if (!(m_auth)) {
            m_auth = std::make_unique<wss::Auth>();
        }

        m_server->resource[endpoint][toolboxpp::strings::toUpper(methodName)] =
            [this, callback](wss::HttpResponse response, wss::HttpRequest request) {
              response->close_connection_after_response = true;
              const httb::request verifyRequest = request->toHttbRequest();

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
                                                     {"Content-Length",   wss::utils::toString(out.length())},
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

// private:
    std::unique_ptr<Auth> m_auth;
    std::unique_ptr<HttpBase> m_server;
    std::unique_ptr<std::thread> m_workerThread;
    bool m_useSSL;

    void cleanupEndpoints();
};

}
#endif //WSSERVER_REST_SERVER_H
