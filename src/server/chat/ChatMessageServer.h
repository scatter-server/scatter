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
#include "server_ws.hpp"
#include "json.hpp"
#include "Message.h"
#include "../defs.h"
#include "../StandaloneService.h"
#include "../threadsafe.hpp"

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
  std::atomic<time_t> lastDisconnectionTime;
  std::atomic_size_t connectedTimes;
  std::atomic_size_t disconnectedTimes;
  std::atomic_size_t bytesTransferred;
  std::atomic_size_t sentMessages;
  std::atomic_size_t receivedMessages;
  std::atomic<time_t> lastMessageTime;

  Statistics(UserId id) :
      id(id),
      lastConnectionTime(time(nullptr)),
      lastDisconnectionTime(0),
      connectedTimes(0),
      disconnectedTimes(0),
      bytesTransferred(0),
      sentMessages(0),
      receivedMessages(0),
      lastMessageTime(0) { }

  Statistics() :
      id(0),
      lastConnectionTime(time(nullptr)),
      lastDisconnectionTime(0),
      connectedTimes(0),
      disconnectedTimes(0),
      bytesTransferred(0),
      sentMessages(0),
      receivedMessages(0),
      lastMessageTime(0) { }

  Statistics &setId(UserId _id) {
      id = _id;
      return *this;
  }

  Statistics &addConnection() {
      lastConnectionTime = time(nullptr);
      connectedTimes++;
      return *this;
  }

  Statistics &addDisconnection() {
      lastDisconnectionTime = time(nullptr);
      disconnectedTimes++;
      return *this;
  }

  Statistics &addBytesTransferred(std::size_t bytes) {
      bytesTransferred += bytes;
      return *this;
  }
  Statistics &addSentMessages(std::size_t sent) {
      sentMessages += sent;
      lastMessageTime = time(nullptr);
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

  std::size_t getDisconnectedTimes() {
      return disconnectedTimes;
  }

  time_t getOnlineTime() const {
      if (!isOnline()) {
          return 0;
      }
      return time(nullptr) - lastConnectionTime;
  }

  time_t getOfflineTime() const {
      if (isOnline()) {
          return 0;
      }

      return time(nullptr) - lastDisconnectionTime;
  }

  time_t getLastMessageSecondsAgo() const {
      if (!isOnline() || lastMessageTime) {
          return 0;
      }

      return time(nullptr) - lastMessageTime;
  }

  bool isOnline() const {
      return connectedTimes > disconnectedTimes;
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
    const int STATUS_INACTIVE_CONNECTION = 4010;

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

    typedef std::function<void(wss::MessagePayload &&, bool hasSent)> OnMessageSentListener;
    typedef std::function<void()> OnServerStopListener;

 public:
    #ifdef USE_SECURE_SERVER
    ChatMessageServer(
        const std::string &crtPath, const std::string &keyPath,
        const std::string &host, unsigned short port, const std::string &regexPath);
    #else
    ChatMessageServer(const std::string &host, unsigned short port, const std::string &regexPath);
    #endif

    ~ChatMessageServer();

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

    void send(const MessagePayload &payload);

    void setThreadPoolSize(std::size_t size);
    void setMessageSizeLimit(size_t bytes);
    void setEnabledMessageDeliveryStatus(bool enabled);
    void addMessageListener(wss::ChatMessageServer::OnMessageSentListener callback);
    void addStopListener(wss::ChatMessageServer::OnServerStopListener callback);

    const UserMap<std::unique_ptr<Statistics>> &getStats();

 protected:
    void onMessage(ConnectionInfo &connection, WsMessagePtr payload);
    void onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent);
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
    const bool useSSL;
    std::string crtPath;
    std::string keyPath;

    // chat
    std::size_t maxMessageSize; // 10 megabytes by default
    bool enableMessageDeliveryStatus = false;

    // events
    std::vector<wss::ChatMessageServer::OnMessageSentListener> messageListeners;
    std::vector<OnServerStopListener> stopListeners;

    std::mutex frameBufferMutex;
    std::recursive_mutex connectionMutex;
    std::mutex undeliveredMutex;

    //@TODO boost thread
    std::unique_ptr<std::thread> workerThread;
    std::unique_ptr<boost::thread> watchdogThread;

    WsServer::Endpoint *endpoint;
    std::unique_ptr<WsServer> server;

    UserMap<std::shared_ptr<std::stringstream>> frameBuffer;
    UserMap<ConnectionInfo> connectionMap;
    UserMap<std::queue<wss::MessagePayload>> undeliveredMessagesMap;
    UserMap<std::unique_ptr<Statistics>> statistics;

    // @TODO вынести все на сторону сервера и убрать отсюда
    bool hasFrameBuffer(UserId id);
    bool writeFrameBuffer(UserId id, const std::string &input, bool clear = false);
    const std::string readFrameBuffer(UserId id, bool clear = true);

    bool parseRawQuery(const std::string &query, QueryParams &params);
    std::size_t getThreadName();
};

}

#endif //WSSERVER_SERVER_H
