/**
 * wsserver
 * ChatRestApi.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ChatRestApi.h"

wss::ChatRestApi::ChatRestApi(const std::string &host, unsigned short port) : RestServer(host, port) {

    addEndpoint("connected_users", "GET", ACTION_BIND(ChatRestApi, actionListConnected));
    addEndpoint("send_message", "POST", ACTION_BIND(ChatRestApi, actionSendMessage));
}

void wss::ChatRestApi::actionListConnected(wss::HttpResponse response, wss::HttpRequest request) {
    L_DEBUG_F("Http Server", "%s %s", request->method.c_str(), request->path.c_str())
    json content;
    content["success"] = true;
    const std::string out = content.dump();

    setResponseStatus(response, HttpStatus::success_ok, out.length());
    setContent(response, out, "text/json");
}

void wss::ChatRestApi::actionSendMessage(wss::HttpResponse response, wss::HttpRequest request) {
    setResponseStatus(response, HttpStatus::success_ok, 0);
}
void wss::ChatRestApi::attachToChat(wss::ChatServer *server) {
    chatServer = server;
}
