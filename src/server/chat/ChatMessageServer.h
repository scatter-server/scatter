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
#include "../base/StandaloneService.h"
#include "../helpers/threadsafe.hpp"
#include "ConnectionStorage.h"
#include "Statistics.h"

namespace wss {

using namespace std;
using namespace std::placeholders;

using QueryParams = std::unordered_map<std::string, std::string>;
using MessageQueue = std::queue<MessagePayload>;

namespace cal = boost::gregorian;
namespace pt = boost::posix_time;

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

    const UserMap<std::unique_ptr<wss::Statistics>> &getStats();

 protected:
    void onMessage(WsConnectionPtr &connection, WsMessagePtr payload);
    void onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent);
    void onConnected(WsConnectionPtr connection);
    void onDisconnected(WsConnectionPtr connection, int status, const std::string &reason);

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

    std::unique_ptr<boost::thread> workerThread;
    std::unique_ptr<boost::thread> watchdogThread;

    WsServer::Endpoint *endpoint;
    std::unique_ptr<WsServer> server;

    std::unique_ptr<wss::ConnectionStorage> connectionStorage;
    UserMap<std::shared_ptr<std::stringstream>> frameBuffer;
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
