/**
 * wsserver
 * Server.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_SERVER_H
#define WSSERVER_SERVER_H

#include <string>
#include <iostream>
#include <unordered_map>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <exception>
#include <utility>
#include <vector>
#include <atomic>
#include <ctime>
#include <functional>
#include <toolboxpp.h>
#include <boost/thread.hpp>
#include <future>
#include "server_ws.hpp"
#include "json.hpp"
#include "Message.h"
#include "../defs.h"
#include "../StandaloneService.h"
#include "../threadsafe.hpp"
#include "../helpers/helpers.h"

namespace wss {

using namespace std;
using namespace std::placeholders;

using QueryParams = std::unordered_map<std::string, std::string>;
using MessageQueue = std::queue<MessagePayload>;

namespace cal = boost::gregorian;
namespace pt = boost::posix_time;

struct ConnectionNotFound : std::exception {
  const char *what() const throw() override;
};

struct Statistics {
  std::atomic<UserId> id;
  std::atomic<time_t> lastConnectionTime;
  std::atomic_size_t connectedTimes;
  std::atomic_size_t bytesTransferred;
  std::atomic_size_t sentMessages;
  std::atomic_size_t receivedMessages;

  Statistics(UserId id) :
      id(id),
      bytesTransferred(0),
      connectedTimes(0),
      sentMessages(0),
      receivedMessages(0),
      lastConnectionTime(time(nullptr)) { }

  Statistics() :
      id(0),
      bytesTransferred(0),
      connectedTimes(0),
      sentMessages(0),
      receivedMessages(0),
      lastConnectionTime(time(nullptr)) { }

  Statistics &setId(UserId _id) {
      id = _id;
      return *this;
  }

  Statistics &addConnection() {
      lastConnectionTime = time(nullptr);
      connectedTimes++;
      return *this;
  }

  Statistics &addBytesTransferred(std::size_t bytes) {
      bytesTransferred += bytes;
      return *this;
  }
  Statistics &addSentMessages(std::size_t sent) {
      sentMessages += sent;
      return *this;
  }
  Statistics &addSendMessage() {
      return addSentMessages(1);
  }

  Statistics &addReceivedMessages(std::size_t received) {
      receivedMessages += received;
      return *this;
  }

  Statistics &addReceivedMessage() {
      return addReceivedMessages(1);
  }

  std::size_t getConnectedTimes() {
      return connectedTimes;
  }

  time_t getSessionTime() const {
      return time(nullptr) - lastConnectionTime;
  }

  time_t getConnectionTime() const {
      return lastConnectionTime;
  }

  std::size_t getBytesTransferred() const {
      return bytesTransferred;
  }
  std::size_t getSentMessages() const {
      return sentMessages;
  }
  std::size_t getReceivedMessages() const {
      return receivedMessages;
  }

};

class ConnectionInfo {
 private:
    WsConnectionPtr connection;

 public:
    ConnectionInfo() = default;
    ConnectionInfo(const ConnectionInfo &other) = delete;
    ConnectionInfo(ConnectionInfo &&other) = delete;
    ConnectionInfo &operator=(ConnectionInfo &&other) = delete;
    ConnectionInfo &operator=(const ConnectionInfo &other) = delete;

    explicit ConnectionInfo(const WsConnectionPtr &rawConnection) :
        connection(rawConnection) {
    }

    void setConnection(const WsConnectionPtr &c) {
        if (!connection) {
            connection = c;
        }
    }

    WsServer::Connection *get() const {
        return connection.get();
    }

    WsServer::Connection *operator->() const noexcept {
        return connection.operator->();
    }

    bool operator==(const ConnectionInfo &another) {
        return another.connection->getId() == connection->getId();
    }

    operator bool() const noexcept {
        if (!connection) {
            return false;
        }

        return true;
    }
};

class ChatMessageServer : public virtual StandaloneService {
 public:
    const int STATUS_OK = 1000;
    const int STATUS_GOING_AWAY = 1001;
    const int STATUS_PROTOCOL_ERROR = 1002;
    const int STATUS_UNSUPPORTED_DATA = 1003;
    const int STATUS_NO_STATUS_RCVD = 1004;
    const int STATUS_ABNORMAL_CLOSURE = 1005;
    const int STATUS_INVALID_FRAME_PAYLOAD = 1007;
    const int STATUS_POLICY_VIOLATION = 1008;
    const int STATUS_MESSAGE_TOO_BIG = 1009;
    const int STATUS_MISSING_EXTENSION = 1010;
    const int STATUS_INTERNAL_ERROR = 1011;
    const int STATUS_SERVICE_RESTART = 1012;
    const int STATUS_TRY_AGAIN_LATER = 1013;
    const int STATUS_BAD_GATEWAY = 1014;
    const int STATUS_TLS_HANDSHAKE = 1015;

    const int STATUS_INVALID_QUERY_PARAMS = 4000;
    const int STATUS_INVALID_MESSAGE_PAYLOAD = 4001;
    const int STATUS_ALREADY_CONNECTED = 4002;

    /**
     *  0000 0000 (0)   - продолжение фрагмента (FIN bit clear=0x0, rsv_opcode=0x1: fin & rsv_opcode == 0x0)
     *  0000 0001 (1)   - начало фрагментированных (текстовых) фреймов, FIN bit = 0x1
     *  0000 0010 (2)   - начало фрагментированных (бинарных) фреймов, FIN bit  = 0x1
     *  1000 0001 (129) - текстовые данные
     *  1000 0010 (130) - бинарные данные
     *  1000 0000 (128) - конец фрагментированных (текстовых?) фреймов (129 ^ 1) (FIN bit = 0x1, opcode = 0x0)
     *  1000 0000 (128) - конец фрагментированных (бинарных?) фреймов - предположительно (130 ^ 2), нужно проверять
     *
     *  @link https://tools.ietf.org/html/rfc6455#page-65
     */
    const unsigned char RSV_OPCODE_FRAGMENT_CONTINUATION = 0;
    const unsigned char RSV_OPCODE_FRAGMENT_BEGIN_TEXT = 1;
    const unsigned char RSV_OPCODE_FRAGMENT_BEGIN_BINARY = 2;
    const unsigned char RSV_OPCODE_FRAGMENT_END = 128;
    const unsigned char RSV_OPCODE_CONNECTION_CLOSE = 8;
    const unsigned char RSV_OPCODE_PING = 9;
    const unsigned char RSV_OPCODE_PONG = 10;
    const unsigned char RSV_OPCODE_TEXT = 129;
    const unsigned char RSV_OPCODE_BINARY = 130;
 public:
    ChatMessageServer(const std::string &host, unsigned short port, const std::string &regexPath);
    ChatMessageServer(
        const std::string &crtPath, const std::string &keyPath,
        const std::string &host, unsigned short port, const std::string &regexPath);
    ~ChatMessageServer();

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

    void setThreadPoolSize(std::size_t size);
    void setMessageSizeLimit(size_t bytes);
    void setEnabledMessageDeliveryStatus(bool enabled);

    void send(
        const MessagePayload &payload,
        std::function<void(wss::MessagePayload &&payload)> &&successPayload = [](wss::MessagePayload &&) { }
    );

    void addMessageListener(std::function<void(wss::MessagePayload &&)> callback);
    void addStopListener(std::function<void()> callback);

    const std::unordered_map<UserId, std::unique_ptr<Statistics>> &getStats();

 protected:
    void onMessage(ConnectionInfo &connection, WsMessagePtr payload);
    void onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred);
    void onConnected(WsConnectionPtr connection);
    void onDisconnected(WsConnectionPtr connection, int status, const std::string &reason);

    inline ConnectionInfo &findOrCreateConnection(WsConnectionPtr connection);
    inline bool hasConnectionFor(UserId id);
    inline void setConnectionFor(UserId id, const WsConnectionPtr &connection);
    inline void removeConnectionFor(UserId id);
    inline ConnectionInfo &getConnectionFor(UserId id);

    inline bool hasUndeliveredMessages(UserId id);
    MessageQueue &getUndeliveredMessages(UserId id);
    std::vector<MessageQueue *> getUndeliveredMessages(const MessagePayload &payload);
    void enqueueUndeliveredMessage(const MessagePayload &payload);
    int redeliverMessagesTo(UserId id);
    int redeliverMessagesTo(const MessagePayload &payload);

    std::unique_ptr<wss::Statistics> &getStat(UserId id);


 private:
    // secure
    bool enableTls;
    std::string crtPath;
    std::string keyPath;

    // chat
    std::size_t maxMessageSize; // 10 megabytes by default
    bool enableMessageDeliveryStatus = false;

    // events
    std::vector<std::function<void(wss::MessagePayload &&)>> messageListeners;
    std::vector<std::function<void()>> stopListeners;

    // mutex
    std::mutex frameBufferMutex;
    std::recursive_mutex connectionMutex;
    std::mutex undeliveredMutex;

    // server
    // гавно какое-то, но блин,
    // либо делать 2 разные сборки с SSL и без
    // либо переделывать класс полностью на шаблонные специализации
    // либо еще куча вариантов
    // @TODO в общем
    std::unique_ptr<WsServer> server;
    std::unique_ptr<WssServer> serverSecure;
    WsServer::Endpoint *endpoint;
    WssServer::Endpoint *endpointSecure;
    std::unique_ptr<std::thread> workerThread;

    // data
    std::unordered_map<UserId, std::shared_ptr<std::stringstream>> frameBuffer;
    std::unordered_map<UserId, ConnectionInfo> connectionMap;
    std::unordered_map<UserId, std::queue<wss::MessagePayload>> undeliveredMessagesMap;
    std::unordered_map<UserId, std::unique_ptr<Statistics>> statistics;

    // @TODO вынести все на сторону сервера и убрать отсюда
    bool hasFrameBuffer(UserId id);
    bool writeFrameBuffer(UserId id, const std::string &input, bool clear = false);
    const std::string readFrameBuffer(UserId id, bool clear = true);

    bool parseRawQuery(const std::string &query, QueryParams &params);
    std::size_t getThreadName();
};

}

#endif //WSSERVER_SERVER_H
