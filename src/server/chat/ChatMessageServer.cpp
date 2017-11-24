/**
 * wsserver
 * Server.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ChatMessageServer.h"
wss::ChatMessageServer::ChatMessageServer(const std::string &host, unsigned short port, const std::string &regexPath) :
    enableTls(false),
    crtPath(),
    keyPath(),
    maxMessageSize(10 * 1024 * 1024) {
    server.config.port = port;
    server.config.thread_pool_size = 10;

    if (host.length() > 1) {
        server.config.address = host;
    };

    endpoint = &server.endpoint[regexPath];
    endpoint->on_message = [this](WsConnectionPtr connectionPtr, WsMessagePtr messagePtr) {
      onMessage(findOrCreateConnection(connectionPtr), messagePtr);
    };

//    endpoint->on_message = std::bind(&wss::ChatMessageServer::onMessage, this, std::placeholders::_1, std::placeholders::_2);
    endpoint->on_open = std::bind(&wss::ChatMessageServer::onConnected, this, std::placeholders::_1);
    endpoint->on_close = std::bind(&wss::ChatMessageServer::onDisconnected,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3);

}

wss::ChatMessageServer::~ChatMessageServer() {
    // raii
    stopService();
    joinThreads();
}

void wss::ChatMessageServer::setThreadPoolSize(std::size_t size) {
    server.config.thread_pool_size = size;
}
void wss::ChatMessageServer::joinThreads() {
    if (workerThread != nullptr) {
        workerThread->join();
    }
}
void wss::ChatMessageServer::detachThreads() {
    if (workerThread != nullptr) {
        workerThread->detach();
    }
}
void wss::ChatMessageServer::runService() {
    std::string hostname = "[any:address]";
    if (not server.config.address.empty()) {
        hostname = server.config.address;
    }
    L_INFO_F("WebSocket Server", "Started at ws://%s:%d", hostname.c_str(), server.config.port);
    workerThread = std::make_unique<std::thread>([this] {
      this->server.start();
    });
}
void wss::ChatMessageServer::stopService() {
    this->server.stop();
}

void wss::ChatMessageServer::onMessage(ConnectionInfo &connection, WsMessagePtr message) {
    MessagePayload payload;
    const short opcode = message->fin_rsv_opcode;
    if (message->fin_rsv_opcode < RSV_OPCODE_TEXT) {
        // fragmented frame message
        UserId id = connection->getId();

        if (opcode == RSV_OPCODE_FRAGMENT_BEGIN_TEXT || opcode == RSV_OPCODE_FRAGMENT_BEGIN_BINARY) {
            time(&transferTimers[id]);
            L_DEBUG_F("OnMessage", "Fragmented frame begin (opcode: %d)", opcode);
            writeFrameBuffer(id, message->string(), true);
            return;
        } else if (opcode == RSV_OPCODE_FRAGMENT_CONTINUATION) {
            writeFrameBuffer(id, message->string(), false);
            return;
        } else if (opcode == RSV_OPCODE_FRAGMENT_END) {
            L_DEBUG_F("OnMessage", "Fragmented frame end (opcode: %d)", opcode);
            std::stringstream final;
            final << readFrameBuffer(id, true);
            final << message->string();
            std::string buffered = final.str();
            final.str("");
            final.clear();

            using profile = toolboxpp::Profiler;

            if (buffered.length() > maxMessageSize) {
                connection->send_close(STATUS_MESSAGE_TOO_BIG,
                                       "Message to big. Maximum size: "
                                           + wss::helpers::humanReadableBytes(maxMessageSize));
                return;
            }

            profile::get().begin("Decoding payload");
            payload = MessagePayload(buffered);
            profile::get().end("Decoding payload");
        }
    } else {
        // one frame message
        payload = MessagePayload(message);
    }

    if (!payload.isValid()) {
        connection->send_close(4000, "Invalid payload. " + payload.getError());
        return;
    }

    send(payload);
}

void wss::ChatMessageServer::onMessageSent(wss::MessagePayload &&payload) {
    for (auto &listener: messageListeners) {
        listener(std::move(payload));
    }
}

void wss::ChatMessageServer::onConnected(WsConnectionPtr connection) {
    QueryParams params;
    bool parsed = parseRawQuery("?" + connection->query_string, params);
    if (!parsed) {
        L_WARN_F("OnConnected", "Invalid request: %s", connection->query_string.c_str());
        connection->send_close(4000, "Invalid request");
        return;
    } else if (!params.count("id") || params["id"].empty()) {
        L_WARN("OnConnected", "Id required in query parameter: ?id={id}");

        connection->send_close(4000, "Id required in query parameter: ?id={id}");
        return;
    }

    UserId id;
    try {
        id = std::stoul(params["id"]);
    } catch (const std::invalid_argument &e) {
        const std::string errReason = "Passed invalid id: ?id=" + params["id"];
        L_WARN("OnConnected", errReason);
        connection->send_close(4000, errReason);
        return;
    }

    setConnectionFor(id, connection);

    L_DEBUG_F("OnConnected", "User %lu connected on thread %lu", id, getThreadName());

    redeliverMessagesTo(id);
}
void wss::ChatMessageServer::onDisconnected(WsConnectionPtr connection, int status, const std::string &reason) {
    if (!hasConnectionFor(connection->getId())) {
        return;
    }

    removeConnectionFor(connection->getId());
    L_DEBUG_F("OnDisconnected", "User %lu has disconnected by reason: %s[%d]",
              connection->getId(),
              reason.c_str(),
              status
    );
}

bool wss::ChatMessageServer::hasFrameBuffer(wss::UserId id) {
    return frameBuffer.find(id) != frameBuffer.end();
}

bool wss::ChatMessageServer::writeFrameBuffer(wss::UserId id, const std::string &input, bool clear) {
    std::lock_guard<std::mutex> fbLock(frameBufferMutex);
    if (!hasFrameBuffer(id)) {
        frameBuffer[id] = std::make_shared<std::stringstream>();
    } else if (clear) {
        frameBuffer[id]->str("");
        frameBuffer[id]->clear();
    }

    (*frameBuffer[id]) << input;
    return true;
}
const std::string wss::ChatMessageServer::readFrameBuffer(wss::UserId id, bool clear) {
    std::lock_guard<std::mutex> fbLock(frameBufferMutex);
    if (!hasFrameBuffer(id)) {
        return std::string();
    }

    const std::string out = frameBuffer[id]->str();
    if (clear) {
        frameBuffer.erase(id);
    }
    return out;
}

bool wss::ChatMessageServer::hasConnectionFor(UserId id) {
    return connectionMap.find(id) != connectionMap.end();
}
void wss::ChatMessageServer::setConnectionFor(UserId id, const WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    connection->setId(id);
    connectionMap[id].setConnection(connection);
}
void wss::ChatMessageServer::removeConnectionFor(UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    connectionMap.erase(id);
}
wss::ConnectionInfo &wss::ChatMessageServer::getConnectionFor(UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    if (!hasConnectionFor(id)) {
        throw ConnectionNotFound();
    }
    return connectionMap[id];
}

wss::ConnectionInfo &wss::ChatMessageServer::findOrCreateConnection(wss::WsConnectionPtr connection) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    ConnectionInfo &conn = connectionMap[connection->getId()];
    if (!conn) {
        conn.setConnection(connection);
    }

    return conn;
}

int wss::ChatMessageServer::redeliverMessagesTo(const wss::MessagePayload &payload) {
    int cnt = 0;
    for (UserId id: payload.getRecipients()) {
        cnt += redeliverMessagesTo(id);
    }

    return cnt;
}
bool wss::ChatMessageServer::hasUndeliveredMessages(UserId id) {
    std::lock_guard<std::mutex> locker(undeliveredMutex);
    L_DEBUG_F("Server", "Check for undelivered messages for user %lu: %lu", id, undeliveredMessagesMap[id].size());
    return !undeliveredMessagesMap[id].empty();
}
wss::MessageQueue &wss::ChatMessageServer::getUndeliveredMessages(UserId id) {
    std::lock_guard<std::mutex> locker(undeliveredMutex);
    return undeliveredMessagesMap[id];
}

std::vector<wss::MessageQueue *>
wss::ChatMessageServer::getUndeliveredMessages(const MessagePayload &payload) {
    std::vector<wss::MessageQueue *> out;
    {
        std::lock_guard<std::mutex> locker(undeliveredMutex);
        for (UserId id: payload.getRecipients()) {
            out.push_back(&undeliveredMessagesMap[id]);
        }
    }

    return out;
}

void wss::ChatMessageServer::enqueueUndeliveredMessage(const wss::MessagePayload &payload) {
    std::unique_lock<std::mutex> uniqueLock(undeliveredMutex);
    for (auto recipient: payload.getRecipients()) {
        undeliveredMessagesMap[recipient].push(payload);
    }
}
int wss::ChatMessageServer::redeliverMessagesTo(UserId id) {
    if (!hasUndeliveredMessages(id)) {
        return 0;
    }

    int cnt = 0;
    auto &queue = getUndeliveredMessages(id);
    L_DEBUG_F("Server", "Redeliver %lu message(s) to user %lu", queue.size(), id);
    while (!queue.empty()) {
        MessagePayload payload = queue.front();
        queue.pop();
        send(payload);
        cnt++;
    }

    return cnt;
}

void wss::ChatMessageServer::send(const wss::MessagePayload &payload,
                                  std::function<void(wss::MessagePayload &&payload)> &&sentPayload) {
    // обертка над std::istream, потэтому работает так же
    auto sendStream = std::make_shared<WsMessageStream>();
    *sendStream << payload.toJson();
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);

    const auto &handleUndeliverable = [this, payload](UserId uid) {
      MessagePayload inaccessibleUserPayload = payload;
      inaccessibleUserPayload.setRecipient(uid);
      // так как месага не дошла только кому-то конкретному, то ему и перешлем заново
      enqueueUndeliveredMessage(inaccessibleUserPayload);
      L_DEBUG_F("Send message", "User %lu is unavailable. Adding message to queue", uid);
    };

    unsigned char fin_rcv_opcode = 129;//static_cast<unsigned char>(payload.isBinary() ? 130 : 129);
    for (UserId uid: payload.getRecipients()) {
        if (!hasConnectionFor(uid)) {
            handleUndeliverable(uid);
            continue;
        }

        try {
            const ConnectionInfo &connection = getConnectionFor(uid);
            // connection->send is an asynchronous function
            connection
                ->send(sendStream, [this, uid, payload, handleUndeliverable](const SimpleWeb::error_code &errorCode) {
                  if (errorCode) {
                      // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                      L_ERR_F("Send message", "Server: Error sending message: %s, error message: %s",
                              errorCode.category().name(),
                              errorCode.message().c_str()
                      );
                      handleUndeliverable(uid);
                  } else {
                      MessagePayload sent = payload;
                      sent.setRecipient(uid);
                      onMessageSent(std::move(sent));
                  }
                }, fin_rcv_opcode);
        } catch (const ConnectionNotFound &e) {
            cout << "Connection not found exception. Adding payload to undelivered" << endl;
            handleUndeliverable(uid);
        }
    }
}

bool wss::ChatMessageServer::parseRawQuery(const std::string &query, wss::QueryParams &out) {
    // reset the std::istringstream with the query string
    std::istringstream iss(query);

    iss.clear();
    iss.str(query);

    std::string url;

    // remove the URL part
    if (!std::getline(iss, url, '?')) {
        L_ERR("Parse query", "Error parsing request host: host does not have any GET parameters");
        return false;
    }

    std::string keyVal, key, val;

    // split each term
    while (std::getline(iss, keyVal, '&')) {
        std::istringstream iss(keyVal);

        // split key/value pairs
        if (std::getline(std::getline(iss, key, '='), val)) {
            out[key] = val;
        }
    }

    return true;
}
std::size_t wss::ChatMessageServer::getThreadName() {
    const std::thread::id id = std::this_thread::get_id();
    static std::size_t nextindex = 0;
    static std::mutex my_mutex;
    static std::map<std::thread::id, std::size_t> ids;
    std::lock_guard<std::mutex> lock(my_mutex);
    if (ids.find(id) == ids.end())
        ids[id] = nextindex++;

    return ids[id];
}
void wss::ChatMessageServer::enableTLS(const std::string &crtPath, const std::string &keyPath) {
    enableTls = true;
    this->crtPath = crtPath;
    this->keyPath = keyPath;
}
void wss::ChatMessageServer::setMessageSizeLimit(size_t bytes) {
    maxMessageSize = bytes;
}

const char *wss::ConnectionNotFound::what() const throw() {
    return "Connection not found or already disconnected";
}
