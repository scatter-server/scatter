/**
 * wsserver
 * Server.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ChatMessageServer.h"
#include "../helpers/helpers.h"
#include "../base/Settings.hpp"

#ifdef USE_SECURE_SERVER
wss::ChatMessageServer::ChatMessageServer(
    const std::string &crtPath, const std::string &keyPath,
    const std::string &host, unsigned short port, const std::string &regexPath) :
    useSSL(true),
    crtPath(),
    keyPath(),
    maxMessageSize(10 * 1024 * 1024),
    connectionStorage(std::make_unique<wss::ConnectionStorage>())
{
    server = std::make_unique<WsServer>(crtPath, keyPath);
    server->config.port = port;
    server->config.thread_pool_size = std::thread::hardware_concurrency();
    server->config.max_message_size = maxMessageSize;

    if (host.length() > 1) {
        server->config.address = host;
    };

    endpoint = &server->endpoint[regexPath];
    endpoint->on_message = [this](WsConnectionPtr connectionPtr, WsMessagePtr messagePtr) {
      onMessage(connectionPtr, messagePtr);
    };

    endpoint->on_open = std::bind(&wss::ChatMessageServer::onConnected, this, std::placeholders::_1);
    endpoint->on_close = std::bind(&wss::ChatMessageServer::onDisconnected,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3);

}
#else
wss::ChatMessageServer::ChatMessageServer(const std::string &host, unsigned short port, const std::string &regexPath) :
    useSSL(false),
    crtPath(),
    keyPath(),
    maxMessageSize(10 * 1024 * 1024),
    connectionStorage(std::make_unique<wss::ConnectionStorage>()) {
    server = std::make_unique<WsServer>();
    server->config.port = port;
    server->config.thread_pool_size = std::thread::hardware_concurrency();
    server->config.max_message_size = maxMessageSize;

    if (host.length() == 15) {
        server->config.address = host;
    };

    endpoint = &server->endpoint[regexPath];
    endpoint->on_message = [this](WsConnectionPtr connectionPtr, WsMessagePtr messagePtr) {
      onMessage(connectionPtr, messagePtr);
    };

    endpoint->on_open = std::bind(&wss::ChatMessageServer::onConnected, this, std::placeholders::_1);
    endpoint->on_close = std::bind(&wss::ChatMessageServer::onDisconnected,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2,
                                   std::placeholders::_3);

}
#endif

wss::ChatMessageServer::~ChatMessageServer() {
    stopService();
    joinThreads();
}

void wss::ChatMessageServer::setThreadPoolSize(std::size_t size) {
    server->config.thread_pool_size = size;
}
void wss::ChatMessageServer::joinThreads() {
    if (workerThread && workerThread->joinable()) {
        workerThread->join();
    }

    if (watchdogThread && watchdogThread->joinable()) {
        watchdogThread->join();
    }
}
void wss::ChatMessageServer::detachThreads() {
    if (workerThread) {
        workerThread->detach();
    }
    if (watchdogThread) {
        watchdogThread->detach();
    }
}
void wss::ChatMessageServer::runService() {
    std::string hostname = "[any:address]";
    if (not server->config.address.empty()) {
        hostname = server->config.address;
    }
    const char *proto = useSSL ? "wss" : "ws";
    L_INFO_F("WebSocket Server", "Started at %s://%s:%d", proto, hostname.c_str(), server->config.port);
    workerThread = std::make_unique<boost::thread>([this] {
      this->server->start();
    });

    if (wss::Settings::get().server.watchdog.enabled) {
        const long lifetime = wss::Settings::get().server.watchdog.connectionLifetimeSeconds;
        L_INFO_F("Watchdog", "Started with interval in 1 minute and lifetime=%lu", lifetime);
        watchdogThread = std::make_unique<boost::thread>([this, lifetime] {
          try {
              while (true) {
                  boost::this_thread::sleep_for(boost::chrono::minutes(1));
                  L_DEBUG("Watchdog", "Checking for inactive connections");
                  for (auto &t: connectionStorage->get()) {
                      const auto &ref = getStat(t.first);

                      if (ref->getInactiveTime() >= lifetime) {
                          for (const auto &conn: t.second) {
                              conn->send_close(STATUS_INACTIVE_CONNECTION,
                                               "Inactive more than " + std::to_string(ref->getInactiveTime()) + " (of "
                                                   + std::to_string(lifetime) + ") seconds");
                          }

                      }
                  }
              }
          } catch (const boost::thread_interrupted &) {
              L_INFO("Watchdog", "Stopping...");
          }
        });
    }
}
void wss::ChatMessageServer::stopService() {
    this->server->stop();
    if (watchdogThread) {
        watchdogThread->interrupt();
    }
}

void wss::ChatMessageServer::onMessage(WsConnectionPtr &connection, WsMessagePtr message) {
    MessagePayload payload;
    const short opcode = message->fin_rsv_opcode;

    if (message->fin_rsv_opcode < RSV_OPCODE_TEXT) {
        // fragmented frame message
        UserId id = connection->getId();

        if (opcode == RSV_OPCODE_FRAGMENT_BEGIN_TEXT || opcode == RSV_OPCODE_FRAGMENT_BEGIN_BINARY) {
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

            /**
             * @Deprecated
             */
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
        payload = MessagePayload(message->string());
    }

    if (!payload.isValid()) {
        connection->send_close(STATUS_INVALID_MESSAGE_PAYLOAD, "Invalid payload. " + payload.getError());
        return;
    }


    send(payload);
}

void wss::ChatMessageServer::onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent) {
    if (payload.isSentStatus()) return;

    getStat(payload.getSender())
        ->addSendMessage()
        .addBytesTransferred(bytesTransferred);

    for (auto &i: payload.getRecipients()) {
        if (hasSent) {
            getStat(i)->addReceivedMessage().addBytesTransferred(bytesTransferred);
        }
    }

    if (enableMessageDeliveryStatus && hasSent) {
        wss::MessagePayload status = MessagePayload::createSendStatus(payload);
        send(status);
    }

    for (auto &listener: messageListeners) {
        listener(std::move(payload), hasSent);
    }
}

void wss::ChatMessageServer::onConnected(WsConnectionPtr connection) {
    wss::web::Request request;
    request.parseParamsString(connection->query_string);
    request.setHeaders(connection->header);

    if (!auth->validateAuth(request)) {
        connection->send_close(STATUS_UNAUTHORIZED, "Unauthorized");
        return;
    }

    if (request.getParams().empty()) {
        L_WARN_F("OnConnected", "Invalid request: %s", connection->query_string.c_str());
        connection->send_close(STATUS_INVALID_QUERY_PARAMS, "Invalid request");
        return;
    } else if (!request.hasParam("id") || request.getParam("id").empty()) {
        L_WARN("OnConnected", "Id required in query parameter: ?id={id}");

        connection->send_close(STATUS_INVALID_QUERY_PARAMS, "Id required in query parameter: ?id={id}");
        return;
    }

    UserId id;
    try {
        id = std::stoul(request.getParam("id"));
    } catch (const std::invalid_argument &e) {
        const std::string errReason = "Passed invalid id: id=" + request.getParam("id") + ". " + e.what();
        L_WARN("OnConnected", errReason);
        connection->send_close(STATUS_INVALID_QUERY_PARAMS, errReason);
        return;
    }

    connectionStorage->add(id, connection);
    getStat(id)->addConnection();

    L_DEBUG_F("OnConnected", "User %lu connected (%s:%d) on thread %lu",
              id,
              connection->remote_endpoint_address().c_str(),
              connection->remote_endpoint_port(),
              getThreadName()
    );

    redeliverMessagesTo(id);
}
void wss::ChatMessageServer::onDisconnected(WsConnectionPtr connection, int status, const std::string &reason) {
    if (!connectionStorage->exists(connection->getId())) {
        return;
    }

    getStat(connection->getId())->addDisconnection();
    connectionStorage->remove(connection);
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
    if (not wss::Settings::get().chat.enableUndeliveredQueue) {
        return 0;
    }

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

void wss::ChatMessageServer::send(const wss::MessagePayload &payload) {
    // обертка над std::istream, потэтому работает так же
    std::string pl = payload.toJson();
    std::size_t plSize = pl.length();
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);

    const auto &handleUndeliverable = [this, payload](UserId uid) {
      if (!wss::Settings::get().chat.enableUndeliveredQueue) {
          L_DEBUG_F("Send message", "User %lu is unavailable. Skipping message.", uid);
          return;
      }
      MessagePayload inaccessibleUserPayload = payload;
      inaccessibleUserPayload.setRecipient(uid);
      // так как месага не дошла только кому-то конкретному, то ему и перешлем заново
      enqueueUndeliveredMessage(inaccessibleUserPayload);
      L_DEBUG_F("Send message", "User %lu is unavailable. Adding message to queue", uid);
    };

    unsigned char fin_rcv_opcode = 129;//static_cast<unsigned char>(payload.isBinary() ? 130 : 129);
    for (UserId uid: payload.getRecipients()) {
        if (!connectionStorage->exists(uid)) {
            handleUndeliverable(uid);
            MessagePayload sent = payload;
            sent.setRecipient(uid);
            onMessageSent(std::move(sent), plSize, false);
            continue;
        }

        try {
            const std::vector<WsConnectionPtr> &connections = connectionStorage->get(uid);
            // connection->send is an asynchronous function
            int i = 0;
            for (auto &connection: connections) {

                // DO NOT reuse stream, it will die after first sending
                auto sendStream = std::make_shared<WsMessageStream>();
                *sendStream << pl;

                L_DEBUG_F("OnSend",
                          "Sending message to recipient %lu, connection[%d]=%lu",
                          uid,
                          i,
                          connection->getUniqueId());

                connection
                    ->send(sendStream,
                           [this, uid, payload, handleUndeliverable, plSize](const SimpleWeb::error_code &errorCode) {
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
                                 onMessageSent(std::move(sent), plSize, true);
                             }
                           }, fin_rcv_opcode);

                i++;
            }
        } catch (const ConnectionNotFound &e) {
            cout << "Connection not found exception. Adding payload to undelivered" << endl;
            handleUndeliverable(uid);
        }
    }
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
void wss::ChatMessageServer::setMessageSizeLimit(size_t bytes) {
    maxMessageSize = bytes;
    server->config.max_message_size = maxMessageSize;
}
void wss::ChatMessageServer::setAuth(const nlohmann::json &config) {
    auth = wss::auth::createFromConfig(config);
}

void wss::ChatMessageServer::setEnabledMessageDeliveryStatus(bool enabled) {
    enableMessageDeliveryStatus = enabled;
}

void wss::ChatMessageServer::addMessageListener(wss::ChatMessageServer::OnMessageSentListener callback) {
    messageListeners.push_back(callback);
}
void wss::ChatMessageServer::addStopListener(wss::ChatMessageServer::OnServerStopListener callback) {
    stopListeners.push_back(callback);
}
std::unique_ptr<wss::Statistics> &wss::ChatMessageServer::getStat(wss::UserId id) {
    if (statistics.find(id) == statistics.end()) {
        statistics[id] = std::make_unique<wss::Statistics>(id);
    }

    return statistics[id];
}

const wss::UserMap<std::unique_ptr<wss::Statistics>> &wss::ChatMessageServer::getStats() {
    return statistics;
}
