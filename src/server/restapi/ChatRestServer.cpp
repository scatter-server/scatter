/**
 * wsserver
 * ChatRestApi.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ChatRestServer.h"

wss::ChatRestServer::ChatRestServer(std::shared_ptr<ChatMessageServer> chatMessageServer) :
    RestServer("*", 8081),
    chatMessageServer(chatMessageServer) {
}
wss::ChatRestServer::ChatRestServer(std::shared_ptr<ChatMessageServer> chatMessageServer,
                                    const std::string &host,
                                    unsigned short port) :
    RestServer(host, port),
    chatMessageServer(chatMessageServer) {
}

void wss::ChatRestServer::createEndpoints() {
    RestServer::createEndpoints();
    addEndpoint("connected_users", "GET", ACTION_BIND(ChatRestServer, actionListConnected));
    addEndpoint("send_message", "POST", ACTION_BIND(ChatRestServer, actionSendMessage));
}

void wss::ChatRestServer::actionListConnected(wss::HttpResponse response, wss::HttpRequest request) {
    L_DEBUG_F("Http Server", "%s %s", request->method.c_str(), request->path.c_str())
    json content;
    content["success"] = true;
    content["data"] = json();

    const std::string out = content.dump();

    setResponseStatus(response, HttpStatus::success_ok, out.length());
    setContent(response, out, "application/json");
}

void wss::ChatRestServer::actionSendMessage(wss::HttpResponse response, wss::HttpRequest request) {
    response->close_connection_after_response = true;
    auto ctype = request->header.find("content-type");

    if (ctype == request->header.end() || ctype->second != "application/json") {
        setError(response, HttpStatus::client_error_bad_request, 400, "Content-Type must be application/json");
        return;
    }

    const MessagePayload payload(request->content.string());
    if (!payload.isValid()) {
        setError(response, HttpStatus::client_error_bad_request, 400, payload.getError());
        return;
    }

    using profile = toolboxpp::Profiler;

    profile::get().begin("api-send");
    chatMessageServer->send(payload);
    profile::get().end("api-send");

    profile::get().begin("api-send-respond");
    setResponseStatus(response, HttpStatus::success_accepted, 0);
    profile::get().end("api-send-respond");

}
void wss::ChatRestServer::setError(wss::HttpResponse &response,
                                   wss::HttpStatus status,
                                   int code,
                                   const std::string &message) {
    json errorOut;
    errorOut["status"] = code;
    errorOut["message"] = message;
    const std::string out = errorOut.dump();
    setResponseStatus(response, status, out.length());
    setContent(response, out, "application/json");
}
void wss::ChatRestServer::setError(wss::HttpResponse &response,
                                   wss::HttpStatus status,
                                   int code,
                                   std::string &&message) {
    json errorOut;
    errorOut["status"] = code;
    errorOut["message"] = message;
    const std::string out = errorOut.dump();
    setResponseStatus(response, status, out.length());
    setContent(response, out, "application/json");
}





