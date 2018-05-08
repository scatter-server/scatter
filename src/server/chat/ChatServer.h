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
#include "src/server/wsserver_core.h"
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

class ChatServer : public virtual StandaloneService {
 public:
    const int STATUS_OK = 1000;
    const int STATUS_GOING_AWAY = 1001;
    const int STATUS_PROTOCOL_ERROR = 1002;
    const int STATUS_UNSUPPORTED_DATA = 1003;
    const int STATUS_RESERVED_1 = 1004;
    const int STATUS_NO_STATUS_RCVD = 1005;
    const int STATUS_ABNORMAL_CLOSURE = 1006;
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
    const uint8_t FIN1 = 0x80;
    /// \brief fin clear bit: 0000 0000
    const uint8_t FIN0 = 0x00;
    //opcodes
    /// \brief Continuation Frame
    const uint8_t OPCODE_CONTINUE = 0x00;
    /// \brief Text Frame
    const uint8_t OPCODE_TEXT = 0x01;
    /// \brief Binary Frame
    const uint8_t OPCODE_BINARY = 0x02;
    /// \brief Connection Close Frame
    const uint8_t OPCODE_CLOSE = 0x08;
    /// \brief Ping Frame
    const uint8_t OPCODE_PING = 0x09;
    /// \brief Pong Frame
    const uint8_t OPCODE_PONG = 0x0A;
    // fragment frames flags
    const uint8_t FLAG_PING = FIN1 | OPCODE_PING;
    const uint8_t FLAG_PONG = FIN1 | OPCODE_PONG;
    const uint8_t FLAG_FRAGMENT_BEGIN_TEXT = FIN0 | OPCODE_TEXT;
    const uint8_t FLAG_FRAGMENT_BEGIN_BINARY = FIN0 | OPCODE_BINARY;
    const uint8_t FLAG_FRAGMENT_CONTINUE = FIN0 | OPCODE_CONTINUE;
    const uint8_t FLAG_FRAGMENT_END = FIN1 | OPCODE_CONTINUE;
    // single frames flags
    const uint8_t FLAG_FRAME_TEXT = FIN1 | OPCODE_TEXT;
    const uint8_t FLAG_FRAME_BINARY = FIN1 | OPCODE_BINARY;
    //@formatter:on

    typedef std::function<void(wss::MessagePayload &&)> OnMessageSentListener;
    typedef std::function<void()> OnServerStopListener;

 public:
    #ifdef USE_SECURE_SERVER
    ChatServer(
        const std::string &crtPath, const std::string &privKeyPath,
        const std::string &host, unsigned short port, const std::string &regexPath);
    #else
    /// \brief Insecure message server ctr
    /// \param host hostname - ip address or hostname without protocol
    /// \param port port number
    /// \param regexPath endpoint regex: example ^/chat$
    ChatServer(const std::string &host, unsigned short port, const std::string &regexPath);
    #endif

    /// \brief Stops io_service
    ~ChatServer();

    void joinThreads() override;
    void detachThreads() override;
    void runService() override;
    void stopService() override;

    /// \brief Send payload. Payload already contains recipients and sender
    /// \param payload
    void send(const MessagePayload &payload);

    /// \brief Send payload to specified recipient. NOT used payload recipient
    /// \param payload
    void sendTo(user_id_t recipient, const MessagePayload &payload);

    /// \brief Max number of workers for incoming messages
    /// \param size Recommended - core numbers
    void setThreadPoolSize(std::size_t size);

    /// \brief Set maximum websocket message size (for fragmented message - sum of sizes)
    /// \param bytes
    void setMessageSizeLimit(size_t bytes);

    /// \brief Set websocket authorization method. If planning to use browser JS clients, recommended to use Basic Auth
    /// \see wss::BasicAuth - requires basic auth
    /// \see wss::WebAuth - does not requires authorization
    /// \param config
    void setAuth(const nlohmann::json &config);

    /// \brief Whether true, respond to sender simple notification payload with type "notification_received"
    /// \see wss::MessagePayload
    /// \param enabled
    void setEnabledMessageDeliveryStatus(bool enabled);

    /// \brief Adds event listener for message send event
    /// \param callback semantic: void(wss::MessagePayload &&, bool hasSent)
    void addMessageListener(wss::ChatServer::OnMessageSentListener callback);

    /// \brief Adds event listener for server stopping event
    /// \param callback semantic: void(void)
    void addStopListener(wss::ChatServer::OnServerStopListener callback);

    /// \brief Returns map of user connections/sends statistics. Key - user id, value - statistics
    /// \return
    const UserMap<std::unique_ptr<wss::Statistics>> &getStats();

 protected:
    /// \brief Called when pong frame received from client
    /// \param connection
    /// \param payload
    void onPong(WsConnectionPtr &connection, WsMessagePtr payload);

    /// \brief Called when message received from client
    /// \param connection
    /// \param payload
    void onMessage(WsConnectionPtr &connection, WsMessagePtr payload);

    /// \brief Called when message has sent to recipient, for entire recipient. Payload contains only 1 entire recipient.
    /// \param payload
    /// \param bytesTransferred Payload message size
    /// \param hasSent Indicates that message has send or not
    void onMessageSent(wss::MessagePayload &&payload, std::size_t bytesTransferred, bool hasSent);

    /// \brief Called when client has connected
    /// \param connection
    void onConnected(WsConnectionPtr connection);

    /// \brief Called when client has disconnected
    /// \param connection
    /// \param status Disconnection status code
    /// \param reason Disconnection string reason. May be empty.
    void onDisconnected(WsConnectionPtr connection, int status, const std::string &reason);
    void watchdogWorker(long lifetime);

    /// \brief Check for entire user has undelivered message
    /// \param recipientId recipient id
    /// \return
    inline bool hasUndeliveredMessages(user_id_t recipientId);
    /// \brief Returns queue of undelivered messages
    /// \param recipientId recipient id
    /// \return
    MessageQueue &getUndeliveredMessages(user_id_t recipientId);

    /// \brief Returns list of queues of undelivered messages.
    /// \param payload Function takes all recipients from payload
    /// \return
    std::vector<MessageQueue *> getUndeliveredMessages(const MessagePayload &payload);

    /// \brief Store undelivered message for payload recipients (for each recipient - single queue element)
    /// \param payload
    void enqueueUndeliveredMessage(const MessagePayload &payload);

    /// \brief Take from undelivered queue messages for recipient, and tries to resend them
    /// \param recipientId
    /// \return Number of successfully sent messages
    int redeliverMessagesTo(user_id_t recipientId);

    /// \brief Works like wss::ChatMessageServer::redeliverMessagesTo(user_id_t recipientId) but uses multiple recipients from payload
    /// \param payload
    /// \return Number of successfully sent messages
    int redeliverMessagesTo(const MessagePayload &payload);

    /// \brief Returns statistics for entire user
    /// \param id
    /// \return
    std::unique_ptr<wss::Statistics> &getStat(user_id_t id);

 private:
    // secure
    const bool m_useSSL;

    /// \brief Current auth method
    std::unique_ptr<wss::WebAuth> m_auth;

    // chat
    /// \brief Number in bytes
    std::size_t m_maxMessageSize; // 10 megabytes by default
    bool m_enableMessageDeliveryStatus = false;

    // events
    std::vector<wss::ChatServer::OnMessageSentListener> m_messageListeners;
    std::vector<OnServerStopListener> m_stopListeners;

    std::mutex m_frameBufferMutex;
    std::recursive_mutex m_connectionMutex;
    std::mutex m_undeliveredMutex;

    std::unique_ptr<boost::thread> m_workerThread;
    std::unique_ptr<boost::thread> m_watchdogThread;

    WsServer::Endpoint *m_endpoint;
    std::unique_ptr<WsServer> m_server;

    std::unique_ptr<wss::ConnectionStorage> m_connectionStorage;
    UserMap<std::shared_ptr<std::stringstream>> m_frameBuffer;
    UserMap<std::queue<wss::MessagePayload>> m_undeliveredMessagesMap;
    UserMap<std::unique_ptr<Statistics>> m_statistics;
    UserMap<bool> m_sentUniqueId;

    /// \todo move out to server side

    /// \brief Check for existence of fragmented frame was written to temporary buffer
    /// \param senderId
    /// \return
    bool hasFrameBuffer(user_id_t senderId);

    /// \brief Write to buffer fragmented frame
    /// \param senderId
    /// \param input input data
    /// \param clear if true, buffer for will be cleared for entire sender
    /// \return
    bool writeFrameBuffer(user_id_t senderId, const std::string &input, bool clear = false);

    /// \brief Read from buffer fragmented frame
    /// \param senderId
    /// \param clear
    /// \return
    const std::string readFrameBuffer(user_id_t senderId, bool clear = true);

    /// \brief Running thread index
    /// \return incremental simple integer
    std::size_t getThreadName();

    /// \brief Calling message event listeners
    /// \param paylod
    void callOnMessageListeners(wss::MessagePayload paylod);

    void handleUndeliverable(user_id_t uid, const wss::MessagePayload &payload);
};

}

#endif //WSSERVER_SERVER_H
