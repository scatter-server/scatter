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
#include "../base/Auth.h"

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
    const int STATUS_INACTIVE_CONNECTION = 4010;
    const int STATUS_UNAUTHORIZED = 4050;

    /**
     * @link https://tools.ietf.org/html/rfc6455#page-65
     *
     * close:
     *  fin=1, opcode=0x08 = 0x80 | 0x08 = 136
     * ping:
     *  fin=1, opcode=0x09 = 0x80 | 0x09 = 137
     * pong:
     *  fin=1, opcode=0x0A = 0x80 | 0x0A = 138
     *
     * unfragmented:
     *  text: fin=1, opcode=1 = 0x80 | 0x01 = 129
     *  bin:  fin=1, opcode=1 = 0x80 | 0x02 = 130
     *
     * fragmented:
     *  begin:
     *  text: fin=0, opcode = 1 = 0x00 | 0x01 = 1
     *  bin:  fin=0, opcode = 2 = 0x00 | 0x02 = 2
     *
     *  continuation:
     *  text: fin=0, opcode=0 = 0x00 | 0x00 = 0
     *  bin:  fin=0, opcode=0 = 0x00 | 0x00 = 0
     *
     *  end:
     *  text: fin=1, opcode=0 = 0x80 | 0x00 = 128
     *  bin:  fin=1, opcode=0 = 0x80 | 0x00 = 128
     *
     */
    //@formatter:off
    /// \brief fin set bit: 1000 0000
    const unsigned char FIN1 = 0x80;
    /// \brief fin clear bit: 0000 0000
    const unsigned char FIN0 = 0x00;
    //opcodes
    /// \brief Continuation Frame
    const unsigned char OPCODE_CONTINUE = 0x00;
    /// \brief Text Frame
    const unsigned char OPCODE_TEXT = 0x01;
    /// \brief Binary Frame
    const unsigned char OPCODE_BINARY = 0x02;
    /// \brief Connection Close Frame
    const unsigned char OPCODE_CLOSE = 0x08;
    /// \brief Ping Frame
    const unsigned char OPCODE_PING = 0x09;
    /// \brief Pong Frame
    const unsigned char OPCODE_PONG = 0x0A;
    // fragment frames flags
    const unsigned char FLAG_PING = FIN1 | OPCODE_PING;
    const unsigned char FLAG_PONG = FIN1 | OPCODE_PONG;
    const unsigned char FLAG_FRAGMENT_BEGIN_TEXT = FIN0 | OPCODE_TEXT;
    const unsigned char FLAG_FRAGMENT_BEGIN_BINARY = FIN0 | OPCODE_BINARY;
    const unsigned char FLAG_FRAGMENT_CONTINUE = FIN0 | OPCODE_CONTINUE;
    const unsigned char FLAG_FRAGMENT_END = FIN1 | OPCODE_CONTINUE;
    // single frames flags
    const unsigned char FLAG_FRAME_TEXT = FIN1 | OPCODE_TEXT;
    const unsigned char FLAG_FRAME_BINARY = FIN1 | OPCODE_BINARY;
    //@formatter:on

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
    void setAuth(const nlohmann::json &config);
    void setEnabledMessageDeliveryStatus(bool enabled);
    void addMessageListener(wss::ChatMessageServer::OnMessageSentListener callback);
    void addStopListener(wss::ChatMessageServer::OnServerStopListener callback);

    const UserMap<std::unique_ptr<wss::Statistics>> &getStats();

 protected:
    void onPong(WsConnectionPtr &connection, WsMessagePtr payload);
    void onMessage(WsConnectionPtr &connection, WsMessagePtr payload);
    void onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent);
    void onConnected(WsConnectionPtr connection);
    void onDisconnected(WsConnectionPtr connection, int status, const std::string &reason);
    void watchdogWorker(long lifetime);

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

    std::unique_ptr<wss::WebAuth> auth;

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

    std::size_t getThreadName();
};

}

#endif //WSSERVER_SERVER_H
