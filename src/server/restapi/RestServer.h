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
#include "../defs.h"
#include "../chat/ChatServer.h"

#define ACTION_DEFINE(name) void name(HttpResponse response, HttpRequest request)
#define ACTION_BIND(cName, mName) std::bind(&cName::mName, this, std::placeholders::_1, std::placeholders::_2)

namespace wss {

using HttpStatus = SimpleWeb::StatusCode;

class RestServer {
 public:
    typedef std::function<void(HttpResponse, HttpRequest)> ResponseCallback;

    RestServer(const std::string &host, unsigned short port);

    void run();
    void stop();
    void waitThread();
    void detachThread();

    RestServer &addEndpoint(const std::string &path, const std::string &methodName, ResponseCallback &&callback);

 protected:
    void setResponseStatus(const HttpResponse &response, HttpStatus status, std::size_t contentLength = 0u);
    void setContent(const HttpResponse &response,
                    const std::string &content,
                    const std::string &contentType = "text/html");
    std::string buildResponse(const std::vector<std::pair<std::string, std::string>> &parts);
 private:
    HttpServer server;
    std::thread *workerThread;
};

}
#endif //DOGSY_REST_SERVER_H
