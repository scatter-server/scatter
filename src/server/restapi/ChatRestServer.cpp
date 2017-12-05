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
    addEndpoint("stats", "GET", ACTION_BIND(ChatRestServer, actionStats));
    addEndpoint("send_message", "POST", ACTION_BIND(ChatRestServer, actionSendMessage));
}

void wss::ChatRestServer::actionStats(wss::HttpResponse response, wss::HttpRequest request) {
    L_DEBUG_F("Http Server", "%s %s", request->method.c_str(), request->path.c_str())
    json content;
    content["success"] = true;

    std::vector<json> statItems(chatMessageServer->getStats().size());
    L_DEBUG_F("Http Server", "Statistics: available %lu records", chatMessageServer->getStats().size());
    std::size_t i = 0;
    for (auto &idStat: chatMessageServer->getStats()) {
        json statItem;
        statItem["id"] = idStat.first;
        statItem["isOnline"] = idStat.second->isOnline();
        statItem["lastConnection"] = idStat.second->getConnectionTime();
        statItem["connectedTimes"] = idStat.second->getConnectedTimes();
        statItem["disconnectedTimes"] = idStat.second->getDisconnectedTimes();
        statItem["lastMessageTime"] = idStat.second->getLastMessageTime();
        statItem["timeOnline"] = idStat.second->getOnlineTime();
        statItem["timeOffline"] = idStat.second->getOfflineTime();
        statItem["timeInactivity"] = idStat.second->getInactiveTime();
        statItem["sentMessages"] = idStat.second->getSentMessages();
        statItem["receivedMessages"] = idStat.second->getReceivedMessages();
        statItem["bytesTransferred"] = idStat.second->getBytesTransferred();

        statItems[i] = std::move(statItem);
        i++;
    }

    content["data"] = statItems;

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

    chatMessageServer->send(payload);
    setResponseStatus(response, HttpStatus::success_accepted, 0);

}






