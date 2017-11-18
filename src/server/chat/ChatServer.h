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
#include <vector>
#include <time.h>
#include <toolboxpp.h>
#include "server_ws.hpp"
#include "json.hpp"
#include "../defs.h"
#include "Message.h"
#include "../BlockingQueue.hpp"
#include "../helpers/helpers.h"

namespace wss {

using namespace std;
using namespace std::placeholders;

using QueryParams = std::unordered_map<std::string, std::string>;
using MessageQueue = wss::BlockingQueue<MessagePayload>;

namespace cal = boost::gregorian;
namespace pt = boost::posix_time;

class ChatServer {
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

    /**
     *  0000 0000 (0)   - продолжение фрагмента (FIN bit clear=0x0, rsv_opcode=0x1: fin & rsv_opcode == 0x0)
     *  0000 0001 (1)   - начало фрагментированных (текстовых) фреймов, FIN bit = 0x1
     *  0000 0010 (2)   - начало фрагментированных (бинарных) фреймов, FIN bit  = 0x1
     *  1000 0001 (129) - текстовые данные
     *  1000 0010 (130) - бинарные данные
     *  1000 0000 (128) - конец фрагментированных (текстовых?) фреймов (129 ^ 1) (FIN bit = 0x1, opcode = 0x0)
     *  1000 0000 (128) - конец фрагментированных (бинарных?) фреймов - предположительно (130 ^ 2), нужно проверять
     */
    const unsigned char RSV_OPCODE_FRAGMENT_CONTINUATION = 0;
    const unsigned char RSV_OPCODE_FRAGMENT_BEGIN_TEXT = 1;
    const unsigned char RSV_OPCODE_FRAGMENT_BEGIN_BINARY = 2; // правда 2? https://tools.ietf.org/html/rfc6455#page-65
    const unsigned char RSV_OPCODE_FRAGMENT_END = 128;
    const unsigned char RSV_OPCODE_CONNECTION_CLOSE = 8;
    const unsigned char RSV_OPCODE_PING = 9;
    const unsigned char RSV_OPCODE_PONG = 10;
    const unsigned char RSV_OPCODE_TEXT = 129;
    const unsigned char RSV_OPCODE_BINARY = 130;

 public:
    ChatServer(const std::string &host, unsigned short port, const std::string &regexPath);
    ~ChatServer();

    void setThreadPoolSize(std::size_t size);
    void run();
    void stop();
    void waitThread();
    void detachThread();

    bool send(const MessagePayload &payload);

 protected:
    void onMessage(WsConnectionPtr connection, const WsMessagePtr &payload);
    void onConnected(const WsConnectionPtr &connection);
    void onDisconnected(const WsConnectionPtr &connection, int status, const std::string &reason);

    inline bool hasConnectionFor(UserId id);
    inline void setConnectionFor(UserId id, const WsConnectionPtr &connection);
    inline void removeConnectionFor(UserId id);
    inline const WsConnectionPtr getConnectionFor(UserId id);
    const std::vector<WsConnectionPtr> getConnectionsFor(const MessagePayload &payload);

    inline bool hasUndeliveredMessages(UserId id);
    MessageQueue &getUndeliveredMessages(UserId id);
    std::vector<MessageQueue *> getUndeliveredMessages(const MessagePayload &payload);
    void enqueueUndeliveredMessage(const MessagePayload &payload);
    int redeliverMessagesTo(UserId id);
    int redeliverMessagesTo(const MessagePayload &payload);

 private:
    std::recursive_mutex connLock;
    std::mutex frameBufferLok;

    std::thread *workerThread;

    WsServer::Endpoint *endpoint;
    WsServer server;

    std::unordered_map<UserId, time_t> transferTimers;
    std::unordered_map<UserId, std::shared_ptr<std::stringstream>> frameBuffer;
    std::unordered_map<UserId, WsConnectionPtr> idConnectionMap;
    std::unordered_map<UserId, wss::BlockingQueue<wss::MessagePayload>> undeliveredMessagesMap;

    bool hasFrameBuffer(UserId id);
    bool writeFrameBuffer(UserId id, const std::string &input, bool clear = false);
    const std::string readFrameBuffer(UserId id, bool clear = true);

    bool parseRawQuery(const std::string &query, QueryParams &params);
    std::size_t getThreadName();

};

}

#endif //WSSERVER_SERVER_H
