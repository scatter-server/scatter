/**
 * wsserver
 * Server.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ChatServer.h"
wss::ChatServer::ChatServer(const std::string &host, unsigned short port, const std::string &regexPath) :
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
    endpoint->on_message = std::bind(&wss::ChatServer::onMessage, this, _1, _2);
    endpoint->on_open = std::bind(&wss::ChatServer::onConnected, this, _1);
    endpoint->on_close = std::bind(&wss::ChatServer::onDisconnected, this, _1, _2, _3);

}

wss::ChatServer::~ChatServer() {
    if (workerThread != nullptr)
        delete workerThread;
}

void wss::ChatServer::setThreadPoolSize(std::size_t size) {
    server.config.thread_pool_size = size;
}
void wss::ChatServer::run() {
    std::string hostname = "<any:address>";
    if (not server.config.address.empty()) {
        hostname = server.config.address;
    }
    L_INFO_F("WebSocket Server", "Started at ws://%s:%d", hostname.c_str(), server.config.port);
    workerThread = new std::thread([this] {
      this->server.start();
    });
}
void wss::ChatServer::stop() {
    this->server.stop();
}
void wss::ChatServer::waitThread() {
    if (workerThread != nullptr) {
        workerThread->join();
    }
}
void wss::ChatServer::detachThread() {
    if (workerThread != nullptr) {
        workerThread->detach();
    }
}

void wss::ChatServer::onMessage(WsConnectionPtr connection, WsMessagePtr message) {
//    L_DEBUG_F("OnMessage", "Server Received message (opcode: %d, len: %lu) in thread: %lu", message->fin_rsv_opcode, message->size(), getThreadName());

    MessagePayload payload;
    const short opcode = message->fin_rsv_opcode;
    if (message->fin_rsv_opcode < RSV_OPCODE_TEXT) {
        UserId id = connection->getId();

        if (opcode == RSV_OPCODE_FRAGMENT_BEGIN_TEXT || opcode == RSV_OPCODE_FRAGMENT_BEGIN_BINARY) {
            time(&transferTimers[id]);
            L_DEBUG_F("OnMessage", "Fragmented frame begin (opcode: %d)", opcode);
            writeFrameBuffer(id, message->string(), true);
            return;
        } else if (opcode == RSV_OPCODE_FRAGMENT_CONTINUATION) {
//            L_DEBUG_F("OnMessage", "Fragmented frame continue (opcode: %d)", opcode);
            writeFrameBuffer(id, message->string(), false);
            return;
        } else if (opcode == RSV_OPCODE_FRAGMENT_END) {
            L_DEBUG_F("OnMessage", "Fragmented frame end (opcode: %d)", opcode);
            auto *final = new std::stringstream();
            (*final) << readFrameBuffer(id, true);
            (*final) << message->string();
            std::string buffered = final->str();
            final->str("");
            final->clear();
            delete final;

            time_t finishedTime;
            time(&finishedTime);

            double seconds = difftime(finishedTime, transferTimers[id]);
            L_DEBUG_F("OnMessage", "Data transfer took: %lf seconds", seconds);
            double kbps = ((buffered.length() * 8) / seconds) / 1000;
            L_DEBUG_F("OnMessage", "Data transfer average speed: %lf kbps/s (%lf mbps/s)", kbps, kbps / 1000);

            transferTimers.erase(id);

            using profile = toolboxpp::Profiler;

            profile::get().begin("Decoding payload");
            payload = MessagePayload(buffered);
            profile::get().end("Decoding payload");
        }
    } else {
//        L_DEBUG_F("OnMessage", "Single frame data (opcode: %d)", opcode);
        payload = MessagePayload(message);
    }

    if (!payload.isValid()) {
        connection->send_close(4000, "Invalid payload. " + payload.getError());
        return;
    }

    send(payload);
}

bool wss::ChatServer::hasFrameBuffer(wss::UserId id) {
    return frameBuffer.count(id) > 0;
}
bool wss::ChatServer::writeFrameBuffer(wss::UserId id, const std::string &input, bool clear) {
    std::lock_guard<std::mutex> fbLock(frameBufferLok);
    if (!hasFrameBuffer(id)) {
        frameBuffer[id] = std::make_shared<std::stringstream>();
    } else if (clear) {
        frameBuffer[id]->str("");
        frameBuffer[id]->clear();
    }

    (*frameBuffer[id]) << input;
//    L_DEBUG_F("FrameBuffer", "Writing partial content with length: %lu, total size: %lu", input.length(), frameBuffer[id]->str().length());
    return true;
}
const std::string wss::ChatServer::readFrameBuffer(wss::UserId id, bool clear) {
    if (!hasFrameBuffer(id)) {
        return std::string();
    }

    std::lock_guard<std::mutex> fbLock(frameBufferLok);
    const std::string out = frameBuffer[id]->str();
    if (clear) {
        frameBuffer.erase(id);
    }
    return out;
}

void wss::ChatServer::onConnected(WsConnectionPtr connection) {
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

    connection->setId(id);
    setConnectionFor(id, connection);

    L_DEBUG_F("OnConnected", "User %lu connected on thread %lu", id, getThreadName());

    redeliverMessagesTo(id);
}
void wss::ChatServer::onDisconnected(WsConnectionPtr connection, int status, const std::string &reason) {
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

bool wss::ChatServer::hasConnectionFor(UserId id) {
    return idConnectionMap.count(id) > 0;
}
void wss::ChatServer::setConnectionFor(UserId id, const WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(connLock);
    idConnectionMap[id] = connection;
}
void wss::ChatServer::removeConnectionFor(UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connLock);
    idConnectionMap.erase(id);
}
const wss::WsConnectionPtr wss::ChatServer::getConnectionFor(UserId id) {
    return idConnectionMap[id];
}

const std::vector<wss::WsConnectionPtr> wss::ChatServer::getConnectionsFor(const wss::MessagePayload &payload) {
    std::vector<wss::WsConnectionPtr> out;

    for (UserId id: payload.getRecipients()) {
        if (!hasConnectionFor(id)) {
            continue;
        }

        out.push_back(getConnectionFor(id));
    }

    return out;
}

int wss::ChatServer::redeliverMessagesTo(const wss::MessagePayload &payload) {
    int cnt = 0;
    for (UserId id: payload.getRecipients()) {
        cnt += redeliverMessagesTo(id);
    }

    return cnt;
}
bool wss::ChatServer::hasUndeliveredMessages(UserId id) {
    L_DEBUG_F("Server", "Check for undelivered messages for user %lu: %lu", id, undeliveredMessagesMap[id].size());
    return undeliveredMessagesMap[id].size() > 0;
}
wss::MessageQueue &wss::ChatServer::getUndeliveredMessages(UserId id) {
    return undeliveredMessagesMap[id];
}

std::vector<wss::MessageQueue *>
wss::ChatServer::getUndeliveredMessages(const MessagePayload &payload) {
    std::vector<wss::MessageQueue *> out;
    for (UserId id: payload.getRecipients()) {
        out.push_back(&undeliveredMessagesMap[id]);
    }

    return out;
}
void wss::ChatServer::enqueueUndeliveredMessage(const wss::MessagePayload &payload) {
    // map is just for reading (STL guaranteed safe for multiple threads read), and queue is blocking by mutex and condition_variable
    for (auto recipient: payload.getRecipients()) {
        undeliveredMessagesMap[recipient].push(payload);
    }
}
int wss::ChatServer::redeliverMessagesTo(UserId id) {
    if (!hasUndeliveredMessages(id)) {
        return 0;
    }

    int cnt = 0;
    auto &queue = getUndeliveredMessages(id);
    L_DEBUG_F("Server", "Redeliver %lu message(s) to user %lu", queue.size(), id);
    while (!queue.empty()) {
        MessagePayload payload = queue.pop();
        send(payload);
        cnt++;
    }

    return cnt;
}

bool wss::ChatServer::send(const wss::MessagePayload &payload) {

    const std::vector<WsConnectionPtr> connections = getConnectionsFor(payload);

    for (UserId id: payload.getRecipients()) {
        if (!hasConnectionFor(id)) {
            MessagePayload inaccessibleUserPayload = payload;
            inaccessibleUserPayload.setRecipient(id);
            // так как месага не дошла только кому-то конкретному, то ему и перешлем заново
            enqueueUndeliveredMessage(inaccessibleUserPayload);
            L_DEBUG_F("Send message", "User %lu is unavailable. Adding message to queue", id);
            continue;
        }
    }

    // обертка над std::istream, потэтому работает так же
    auto sendStream = std::make_shared<WsMessageStream>();
    *sendStream << payload.toJson();

    unsigned char fin_rcv_opcode = 129;//static_cast<unsigned char>(payload.isBinary() ? 130 : 129);
    for (const WsConnectionPtr &connection: connections) {
        std::mutex lock;
        lock.lock();
        if (transferTimers.count(connection->getId())) {
            time(&transferTimers[connection->getId()]);
        }

        lock.unlock();

        const std::size_t strLen = sendStream->size();
        // connection->send is an asynchronous function
        connection->send(sendStream, [this, connection, strLen](const SimpleWeb::error_code &errorCode) {
          if (errorCode) {
              // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
              L_ERR_F("Send message", "Server: Error sending message. %s, error message: %s",
                      errorCode.category().name(),
                      errorCode.message().c_str()
              );
          } else {
              if (transferTimers.count(connection->getId())) {
                  time_t finishedTime;
                  time(&finishedTime);
                  std::mutex lock;
                  lock.lock();
                  double seconds = difftime(finishedTime, transferTimers[connection->getId()]);
                  L_DEBUG_F("OnMessage", "Data back transfer took: %lf seconds", seconds);
                  double kbps = ((strLen * 8) / seconds) / 1000;
                  L_DEBUG_F("OnMessage",
                            "Data back transfer average speed: %lf kbps/s (%lf mbps/s)",
                            kbps,
                            kbps / 1000);
                  transferTimers.erase(connection->getId());
                  lock.unlock();
              }
          }
        }, fin_rcv_opcode);
    }

    return true;
}

bool wss::ChatServer::parseRawQuery(const std::string &query, wss::QueryParams &out) {
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
std::size_t wss::ChatServer::getThreadName() {
    const std::thread::id id = std::this_thread::get_id();
    static std::size_t nextindex = 0;
    static std::mutex my_mutex;
    static std::map<std::thread::id, std::size_t> ids;
    std::lock_guard<std::mutex> lock(my_mutex);
    if (ids.find(id) == ids.end())
        ids[id] = nextindex++;

    return ids[id];
}
void wss::ChatServer::enableTLS(const std::string &crtPath, const std::string &keyPath) {
    enableTls = true;
    this->crtPath = crtPath;
    this->keyPath = keyPath;
}
void wss::ChatServer::setMessageSizeLimit(size_t bytes) {
    maxMessageSize = bytes;
}
