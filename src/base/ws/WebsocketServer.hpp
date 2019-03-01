/*!
 * wsserver.
 * WebsocketServer.h
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef WSSERVER_WEBSOCKETSERVER_H
#define WSSERVER_WEBSOCKETSERVER_H

#include "../BaseServer.h"
#include "../SocketLayerWrapper.hpp"

#include "crypto.hpp"
#include "utility.hpp"

#include <atomic>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <functional>
#include <thread>
#include <unordered_set>
#include <toolboxpp.hpp>
#include <string>
#include <algorithm>
#include <openssl/ssl.h>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace wss {
namespace server {
namespace websocket {

using ErrorCode = boost::system::error_code;
namespace asio = boost::asio;
namespace errc = boost::system::errc;
namespace make_error_code = boost::system::errc;
using WS = asio::ip::tcp::socket;
using SendCallback = std::function<void(const ErrorCode, std::size_t)>;
using namespace wss::utils;

class SocketServer;
class SocketServerSecure;

class SocketServerBase : public BaseServer {
 public:
    /// The buffer is not consumed during send operations.
    /// Do not alter while sending.
    class SendStream : public std::ostream {
        friend class SocketServerBase;
        friend class SocketServer;
        friend class SocketServerSecure;

        asio::streambuf streambuf;

     public:
        SendStream() noexcept : std::ostream(&streambuf) { }

        /// Returns the size of the buffer
        std::size_t size() const noexcept {
            return streambuf.size();
        }
    };

    class Connection : public std::enable_shared_from_this<Connection> {
        friend class SocketServerBase;
        friend class SocketServer;
        friend class SocketServerSecure;

     public:
        Connection(std::unique_ptr<SocketLayerWrapper> &&socket) noexcept
            : socket(std::move(socket)),
              closed(false),
              id(0),
              timeoutIdle(0),
              strand(this->socket->get_io_service()) { }

        std::string method, path, queryString, httpVersion;
        wss::utils::CaseInsensitiveMultimap header;
        regexns::smatch pathMatch;
        asio::ip::tcp::endpoint remoteEndpoint;

        std::string remoteEndpointAddress() const noexcept {
            try {
                return remoteEndpoint.address().to_string();
            }
            catch (...) {
                return std::string();
            }
        }

        unsigned short remoteEndpointPort() const noexcept {
            return remoteEndpoint.port();
        }

        void setId(unsigned long _id) {
            id = _id;

            using toolboxpp::strings::substringReplaceAll;
            std::stringstream ss;
            ss << substringReplaceAll(".", "", remoteEndpointAddress()) << remoteEndpointPort();
            uniqueId = std::stoul(ss.str());
        }

        unsigned long getId() const {
            return id;
        }

        unsigned long getUniqueId() const {
            return uniqueId;
        }

     private:
        class SendData {
         public:
            SendData(std::shared_ptr<SendStream> headerStream,
                     std::shared_ptr<SendStream> messageStream,
                     wss::server::websocket::SendCallback callback) noexcept
                : headerStream(std::move(headerStream)),
                  messageStream(std::move(messageStream)),
                  callback(std::move(callback)) { }

            std::shared_ptr<SendStream> headerStream;
            std::shared_ptr<SendStream> messageStream;
            wss::server::websocket::SendCallback callback;
        };

        Connection(std::shared_ptr<ScopeRunner> handler_runner,
                   long timeout_idle,
                   wss::io_context_service &ioContext) noexcept
            : handlerRunner(std::move(handler_runner)),
              socket(std::make_unique<SocketLayerWrapper>(ioContext)),
                     closed(false),
                     uniqueId(0),
                     timeoutIdle(timeout_idle),
                     strand(socket->get_io_service()) { }

        Connection(std::shared_ptr<ScopeRunner> handler_runner,
                   long timeout_idle,
                   wss::io_context_service &ioContext,
                   asio::ssl::context &sslContext) noexcept:
            handlerRunner(std::move(handler_runner)),
            socket(std::make_unique<SocketLayerWrapper>(ioContext, sslContext)),
            closed(false),
            uniqueId(0),
            timeoutIdle(timeout_idle),
            strand(socket->get_io_service()) { }

        std::shared_ptr<ScopeRunner> handlerRunner;

        /// \brief Socket must be unique_ptr since asio::ssl::stream<asio::ip::tcp::socket> is not movable
        std::unique_ptr<SocketLayerWrapper> socket;
        std::mutex socketCloseMutex;
        std::mutex readIdMutex;
        std::list<SendData> sendQueue;
        asio::streambuf readBuffer;
        std::atomic<bool> closed;

        uint64_t id;
        uint64_t uniqueId;
        long timeoutIdle;
        std::unique_ptr<asio::steady_timer> timer;
        std::mutex timerMutex;
        asio::io_service::strand strand;

        void close() noexcept {
            ErrorCode ec;
            /// The following operations seems to be needed to run sequentially
            std::unique_lock<std::mutex> lock(socketCloseMutex);
            socket->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            socket->lowest_layer().close(ec);
        }

        void timeoutSet(long seconds = -1L) noexcept {
            bool useTimeoutIdle = false;
            if (seconds == -1L) {
                useTimeoutIdle = true;
                seconds = timeoutIdle;
            }

            std::unique_lock<std::mutex> lock(timerMutex);

            if (seconds == 0) {
                timer = nullptr;
                return;
            }

            timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(socket->get_io_service()));
            timer->expires_from_now(std::chrono::seconds(seconds));
            std::weak_ptr<Connection> connectionWeak
                (this->shared_from_this()); // To avoid keeping Connection instance alive longer than needed
            timer->async_wait([connectionWeak, useTimeoutIdle](const ErrorCode &ec) {
              if (!ec) {
                  if (auto connection = connectionWeak.lock()) {
                      if (useTimeoutIdle) {
                          connection->sendClose(1000, "idle timeout"); // 1000=normal closure
                      } else {
                          connection->close();
                      }
                  }
              }
            });
        }

        void timeoutCancel() noexcept {
            std::unique_lock<std::mutex> lock(timerMutex);
            if (timer) {
                ErrorCode ec;
                timer->cancel(ec);
            }
        }

        bool handshakeGenerate(const std::shared_ptr<asio::streambuf> &writeBuffer) {
            std::ostream handshake(writeBuffer.get());

            auto headerIterator = header.find("Sec-WebSocket-Key");
            if (headerIterator == header.end())
                return false;

            static auto wsMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            auto sha1 = Crypto::sha1(headerIterator->second + wsMagicString);

            handshake << "HTTP/1.1 101 Web Socket Protocol Handshake\r\n";
            handshake << "Upgrade: websocket\r\n";
            handshake << "Connection: Upgrade\r\n";
            handshake << "Sec-WebSocket-Accept: " << Crypto::Base64::encode(sha1) << "\r\n";
            handshake << "\r\n";

            return true;
        }

        void sendFromQueue() {
            const std::shared_ptr<Connection> self = this->shared_from_this();

            strand.post([self]() {

              std::vector<asio::const_buffer> bufs(2);
              const SendData data = ((SendData) *self->sendQueue.begin());
              // headers
              bufs.push_back(data.headerStream->streambuf.data());
              // body
              bufs.push_back(data.messageStream->streambuf.data());

              self->socket->async_write(bufs, self->strand.wrap([self](const ErrorCode &ec, std::size_t ts) {
                std::unique_ptr<ScopeRunner::SharedLock> lock = self->handlerRunner->continueLock();
                if (!lock) {
                    return;
                }

                const SendData sendDataQueued = ((SendData) *self->sendQueue.begin());
                // if error occured, cleanup queue
                if (ec) {
                    if (sendDataQueued.callback) {
                        sendDataQueued.callback(ec, ts);
                    }

                    self->sendQueue.clear();

                    return;
                }

                if (sendDataQueued.callback) {
                    sendDataQueued.callback(ec, ts);
                }

                self->sendQueue.erase(self->sendQueue.begin());
                if (self->sendQueue.size() > 0) {
                    self->sendFromQueue();
                }
              }));

            });
        }

        void readRemoteEndpoint() noexcept {
            try {
                remoteEndpoint = socket->lowest_layer().remote_endpoint();
            }
            catch (...) {
            }
        }

     public:
        /// fin_rsv_opcode: 129=one fragment, text, 130=one fragment, binary, 136=close connection.
        /// See http://tools.ietf.org/html/rfc6455#section-5.2 for more information
        void send(std::shared_ptr<SendStream> messageStream,
                  const SendCallback &callback = nullptr,
                  uint8_t fin_rsv_opcode = 129) {
            timeoutCancel();
            timeoutSet();

            std::shared_ptr<SendStream> headerStream = std::make_shared<SendStream>();
            std::size_t length = messageStream->size();

            headerStream->put(static_cast<char>(fin_rsv_opcode));
            // Unmasked (first length byte<128)
            if (length >= 126) {
                std::size_t numBytes;
                if (length > 0xffff) {
                    numBytes = 8;
                    headerStream->put(127);
                } else {
                    numBytes = 2;
                    headerStream->put(126);
                }

                for (std::size_t c = numBytes - 1; c != static_cast<std::size_t>(-1); c--)
                    headerStream->put(static_cast<char>((length >> (8 * c)) % 256));
            } else {
                headerStream->put(static_cast<char>(length));
            }

            const std::shared_ptr<Connection> self = this->shared_from_this();
            strand.post([self, headerStream, messageStream, callback]() {
              self->sendQueue.emplace_back(std::move(headerStream), std::move(messageStream), std::move(callback));
              if (self->sendQueue.size() == 1)
                  self->sendFromQueue();
            });
        }

        void sendClose(int status, const std::string &reason = "", const SendCallback &callback = nullptr) {
            // Send close only once (in case close is initiated by server)
            if (closed) {
                return;
            }

            closed = true;

            auto sendStream = std::make_shared<SendStream>();

            sendStream->put(static_cast<char>(status >> 8));
            sendStream->put(static_cast<char>(status % 256));

            *sendStream << reason;

            // fin_rsv_opcode=136: message close
            send(std::move(sendStream), callback, 136);
        }
    };

    class Message : public std::istream {
        friend class SocketServerBase;
        friend class SocketServer;
        friend class SocketServerSecure;

     public:
        unsigned char fin_rsv_opcode;
        std::size_t size() noexcept {
            return length;
        }

        /// Convenience function to return std::string. The stream buffer is consumed.
        std::string string() noexcept {
            try {
                std::stringstream ss;
                ss << rdbuf();
                return ss.str();
            } catch (...) {
                return std::string();
            }
        }

     private:
        Message() noexcept : std::istream(&streambuf) { }
        std::size_t length;
        asio::streambuf streambuf;
    };

    class Endpoint {
        friend class SocketServerBase;
        friend class SocketServer;
        friend class SocketServerSecure;

     private:
        std::unordered_set<std::shared_ptr<Connection>> connections;
        std::mutex connectionsMutex;

     public:
        std::function<void(std::shared_ptr<Connection>)> onOpen;
        std::function<void(std::shared_ptr<Connection>, std::shared_ptr<Message>)> onMessage;
        std::function<void(std::shared_ptr<Connection>, int, const std::string &)> onClose;
        std::function<void(std::shared_ptr<Connection>, const ErrorCode &)> onError;

        std::unordered_set<std::shared_ptr<Connection>> getConnections() noexcept {
            std::unique_lock<std::mutex> lock(connectionsMutex);
            auto copy = connections;
            return copy;
        }
    };

    class Config {
     public:
        Config() noexcept: port(80) { }
        Config(unsigned short port) noexcept : port(port) { }
        /// Port number to use. Defaults to 80 for HTTP and 443 for HTTPS.
        unsigned short port;
        /// If io_service is not set, number of threads that the server will use when start() is called.
        /// Defaults to 1 thread.
        std::size_t threadPoolSize = 1;
        /// Timeout on request handling. Defaults to 5 seconds.
        long timeoutRequest = 5;
        /// Idle timeout. Defaults to no timeout.
        long timeoutIdle = 0;
        /// Maximum size of incoming messages. Defaults to architecture maximum.
        /// Exceeding this limit will result in a message_size error code and the connection will be closed.
        std::size_t maxMessageSize = std::numeric_limits<std::size_t>::max();
        /// IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
        /// If empty, the address will be any address.
        std::string address;
        /// Set to false to avoid binding the socket to an address that is already in use. Defaults to true.
        bool reuseAddress = true;
    };

    void start() override {
        if (!ioService) {
            ioService = std::make_shared<asio::io_service>();
            internalIoService = true;
        }

        if (ioService->stopped()) {
            ioService->reset();
        }

        asio::ip::tcp::endpoint endpoint;
        if (config.address.size() > 0) {
            endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(config.address), config.port);
        } else {
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), config.port);
        }

        if (!acceptor) {
            acceptor = std::make_unique<asio::ip::tcp::acceptor>(*ioService);
        }

        acceptor->open(endpoint.protocol());
        acceptor->set_option(asio::socket_base::reuse_address(config.reuseAddress));
        acceptor->bind(endpoint);
        acceptor->listen();

        accept();

        if (internalIoService) {
            // If thread_pool_size>1, start m_io_service.run() in (thread_pool_size-1) threads for thread-pooling
            for (std::size_t c = 1; c < config.threadPoolSize; c++) {
                threadGroup.create_thread(
                    boost::bind(&boost::asio::io_service::run, ioService)
                );
            }
            // Main thread
            if (config.threadPoolSize > 0) {
                ioService->run();
            }

            // Wait for the rest of the threads, if any, to finish as well
            threadGroup.join_all();
        }
    }

    void stop() override {
        if (acceptor) {
            ErrorCode ec;
            acceptor->close(ec);

            for (auto &pair : endpoint) {
                std::unique_lock<std::mutex> lock(pair.second.connectionsMutex);
                for (auto &connection : pair.second.connections) {
                    connection->close();
                }
                pair.second.connections.clear();
            }

            if (internalIoService) {
                ioService->stop();
            }

            threadGroup.interrupt_all();
        }
    }

    /**
     * Upgrades a request, from for instance Simple-Web-Server, to a WebSocket connection.
     * The parameters are moved to the Connection object.
     * See also Server::on_upgrade in the Simple-Web-Server project.
     * The socket's io_service is used, thus running start() is not needed.
     *
     * Example use:
     * server.on_upgrade=[&socket_server] (auto socket, auto request) {
     *   auto connection=std::make_shared<wss::server::websocket::SocketServer<wss::server::websocket::WS>::Connection>(std::move(socket));
     *   connection->method=std::move(request->method);
     *   connection->path=std::move(request->path);
     *   connection->query_string=std::move(request->query_string);
     *   connection->http_version=std::move(request->http_version);
     *   connection->header=std::move(request->header);
     *   connection->remote_endpoint=std::move(*request->remote_endpoint);
     *   socket_server.upgrade(connection);
     * }
     */
    void upgrade(const std::shared_ptr<Connection> &connection) {
        connection->handlerRunner = handlerRunner;
        connection->timeoutIdle = config.timeoutIdle;
        handshakeWrite(connection);
    }

    /// If you have your own asio::io_service, store its pointer here before running start().
    std::shared_ptr<asio::io_service> ioService;

    Config &getConfig() {
        return config;
    }

    std::map<RegexOrderable, Endpoint> &getEndpoint() {
        return endpoint;
    }

 protected:
    /// Set before calling start().
    Config config;
    /// Warning: do not add or remove endpoints after start() is called
    std::map<RegexOrderable, Endpoint> endpoint;

    bool internalIoService = false;

    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
    boost::thread_group threadGroup;

    std::shared_ptr<ScopeRunner> handlerRunner;

    SocketServerBase(unsigned short port) noexcept: config(port), handlerRunner(new ScopeRunner()) { }

    void handshakeRead(const std::shared_ptr<Connection> &connection) {
        connection->readRemoteEndpoint();

        connection->timeoutSet(config.timeoutRequest);
        // reading handshake headers until \r\n\r\n
        connection->socket->async_read_until(
            connection->readBuffer,
            "\r\n\r\n",
            [this, connection](const ErrorCode &ec, std::size_t /*bytes_transferred*/) {
              connection->timeoutCancel();
              auto lock = connection->handlerRunner->continueLock();
              if (!lock)
                  return;
              if (!ec) {
                  std::istream stream(&connection->readBuffer);
                  if (RequestMessage::parse(stream,
                                            connection->method,
                                            connection->path,
                                            connection->queryString,
                                            connection->httpVersion,
                                            connection->header))

                      // after success incoming handshake, sending server handshake
                      handshakeWrite(connection);
              }
            });
    }

    void handshakeWrite(const std::shared_ptr<Connection> &connection) {
        for (auto &regexEndpoint : endpoint) {
            regexns::smatch pathMatch;
            if (regexns::regex_match(connection->path, pathMatch, regexEndpoint.first)) {
                auto writeBuffer = std::make_shared<asio::streambuf>();

                if (connection->handshakeGenerate(writeBuffer)) {
                    connection->pathMatch = std::move(pathMatch);
                    connection->timeoutSet(config.timeoutRequest);
                    connection->socket->async_write(
                        *writeBuffer,
                        [this, connection, writeBuffer, &regexEndpoint](const ErrorCode &ec, std::size_t) {
                          connection->timeoutCancel();
                          auto lock = connection->handlerRunner->continueLock();
                          if (!lock) {
                              return;
                          }

                          if (!ec) {
                              onConnectionOpen(connection, regexEndpoint.second);
                              readMessage(connection, regexEndpoint.second);
                          } else
                              onConnectionError(connection, regexEndpoint.second, ec);
                        });
                }
                return;
            }
        }
    }

    void readMessage(const std::shared_ptr<Connection> &connection, Endpoint &endpoint) const {
        connection->strand.post([this, connection, &endpoint] {
          // first read - detecting size
          connection->socket->async_read(
              connection->readBuffer,
              asio::transfer_exactly(2),
              connection->strand.wrap([this, connection, &endpoint](const ErrorCode &ec,
                                                                    std::size_t bytesTransferred) {
                auto lock = connection->handlerRunner->continueLock();
                if (!lock) {
                    return;
                }

                if (ec) {
                    onConnectionError(connection, endpoint, ec);
                    return;
                }

                if (bytesTransferred == 0) { // TODO: why does this happen sometimes?
                    readMessage(connection, endpoint);
                    return;
                }

                std::istream stream(&connection->readBuffer);

                std::vector<unsigned char> firstBytes(2);
                stream.read((char *) &firstBytes[0], 2);

                unsigned char fin_rsv_opcode = firstBytes[0];

                // Close connection if unmasked message from client (protocol error)
                if (firstBytes[1] < 128) {
                    const std::string reason("message from client not masked");
                    connection->sendClose(1002, reason);
                    connectionClose(connection, endpoint, 1002, reason);
                    return;
                }

                std::size_t length = (firstBytes[1] & 127u);

                if (length == 126) {
                    // if length == 126 means
                    // 2 next bytes is the size of content
                    connection->socket->async_read(
                        connection->readBuffer,
                        asio::transfer_exactly(2),
                        [this, connection, &endpoint, fin_rsv_opcode](const ErrorCode &ec, std::size_t) {
                          auto sublock = connection->handlerRunner->continueLock();
                          if (!sublock) {
                              return;
                          }

                          if (ec) {
                              onConnectionError(connection, endpoint, ec);
                              return;
                          }

                          std::istream readStream(&connection->readBuffer);

                          std::vector<unsigned char> lengthBytes(2);
                          readStream.read((char *) &lengthBytes[0], 2);

                          std::size_t toReadLength = 0;
                          std::size_t numBytes = 2;
                          for (std::size_t c = 0; c < numBytes; c++) {
                              toReadLength +=
                                  static_cast<std::size_t>(lengthBytes[c]) << (8 * (numBytes - 1 - c));
                          }

                          // now we know real content length, reading whole message
                          readMessageContent(connection, toReadLength, endpoint, fin_rsv_opcode);
                        });

                } else if (length == 127) {
                    // 8 next bytes is the size of content
                    connection->socket->async_read(
                        connection->readBuffer,
                        asio::transfer_exactly(8),
                        [this, connection, &endpoint, fin_rsv_opcode](const ErrorCode &ec, std::size_t) {
                          auto lock = connection->handlerRunner->continueLock();
                          if (!lock) {
                              return;
                          }

                          if (ec) {
                              onConnectionError(connection, endpoint, ec);
                              return;
                          }

                          std::istream readStream(&connection->readBuffer);

                          std::vector<unsigned char> lengthBytes(8);
                          readStream.read((char *) &lengthBytes[0], 8);

                          std::size_t toReadLength = 0;
                          std::size_t num_bytes = 8;
                          for (std::size_t c = 0; c < num_bytes; c++) {
                              toReadLength +=
                                  static_cast<std::size_t>(lengthBytes[c]) << (8 * (num_bytes - 1 - c));
                          }

                          // now we know real content length, reading whole message
                          readMessageContent(connection, toReadLength, endpoint, fin_rsv_opcode);
                        });
                } else {
                    // real message content length is that length that we have now
                    readMessageContent(connection, length, endpoint, fin_rsv_opcode);
                }
              }));
        });
    }

    void readMessageContent(
        const std::shared_ptr<Connection> &connection,
        std::size_t length,
        Endpoint &endpoint,
        unsigned char fin_rsv_opcode) const {
        if (length > config.maxMessageSize) {
            onConnectionError(connection, endpoint, make_error_code::make_error_code(errc::message_size));
            const int status = 1009;
            const std::string reason = "message too big";
            connection->sendClose(status, reason);
            connectionClose(connection, endpoint, status, reason);
            return;
        }

        connection->strand.post([this, connection, &endpoint, length, fin_rsv_opcode] {
          connection->socket->async_read(
              connection->readBuffer,
              asio::transfer_exactly(4 + length),
              [this, connection, length, &endpoint, fin_rsv_opcode](const ErrorCode &ec, std::size_t) {
                auto lock = connection->handlerRunner->continueLock();
                if (!lock) {
                    return;
                }

                if (ec) {
                    onConnectionError(connection, endpoint, ec);
                    return;
                }

                std::istream rawMessageData(&connection->readBuffer);

                // Read mask
                std::vector<uint8_t> mask(4);
                rawMessageData.read((char *) &mask[0], 4);

                std::shared_ptr<Message> message(new Message());
                message->length = length;
                message->fin_rsv_opcode = fin_rsv_opcode;

                std::ostream messageDataOutStream(&message->streambuf);
                for (std::size_t c = 0; c < length; c++) {
                    messageDataOutStream.put(rawMessageData.get() ^ mask[c % 4]);
                }

                // If connection close
                if ((fin_rsv_opcode & 0x0f) == 8) {
                    int status = 0;
                    if (length >= 2) {
                        unsigned char byte1 = static_cast<unsigned char>(message->get());
                        unsigned char byte2 = static_cast<unsigned char>(message->get());
                        status = (byte1 << 8) + byte2;
                    }

                    auto reason = message->string();
                    connection->sendClose(status, reason);
                    connectionClose(connection, endpoint, status, reason);
                    return;
                } else {
                    // If ping
                    if ((fin_rsv_opcode & 0x0f) == 9) {
                        // Send pong
                        auto empty_send_stream = std::make_shared<SendStream>();
                        connection->send(std::move(empty_send_stream),
                                         nullptr,
                                         static_cast<unsigned char>(fin_rsv_opcode + 1));
                    } else if (endpoint.onMessage) {
                        connection->timeoutCancel();
                        connection->timeoutSet();
                        endpoint.onMessage(connection, message);
                    }

                    // Next message
                    readMessage(connection, endpoint);
                }

              });
        });
    }

    void onConnectionOpen(const std::shared_ptr<Connection> &connection, Endpoint &endpoint) const {
        connection->timeoutCancel();
        connection->timeoutSet();

        {
            std::unique_lock<std::mutex> lock(endpoint.connectionsMutex);
            endpoint.connections.insert(connection);
        }

        if (endpoint.onOpen)
            endpoint.onOpen(connection);
    }

    void connectionClose(
        const std::shared_ptr<Connection> &connection,
        Endpoint &endpoint,
        int status,
        const std::string &reason) const {
        connection->timeoutCancel();
        connection->timeoutSet();

        {
            std::unique_lock<std::mutex> lock(endpoint.connectionsMutex);
            endpoint.connections.erase(connection);
        }

        if (endpoint.onClose)
            endpoint.onClose(connection, status, reason);
    }

    void onConnectionError(
        const std::shared_ptr<Connection> &connection,
        Endpoint &endpoint,
        const ErrorCode &ec) const {
        connection->timeoutCancel();
        connection->timeoutSet();

        {
            std::unique_lock<std::mutex> lock(endpoint.connectionsMutex);
            endpoint.connections.erase(connection);
        }

        if (endpoint.onError) {
            endpoint.onError(connection, ec);
        }
    }
};

/// @TODO socket wrapper instead of templates
class SocketServer : public SocketServerBase {
 public:
    SocketServer() noexcept : SocketServerBase((uint16_t) 80) { }

 protected:
    void accept() override {
        std::shared_ptr<Connection> connection(new Connection(handlerRunner, config.timeoutIdle, *ioService));

        acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const ErrorCode &ec) {
          auto lock = connection->handlerRunner->continueLock();
          if (!lock)
              return;
          // Immediately start accepting a new connection (if ioService hasn't been stopped)
          if (ec != asio::error::operation_aborted)
              accept();

          if (!ec) {
              asio::ip::tcp::no_delay option(true);
              connection->socket->set_option(option);

              handshakeRead(connection);
          }
        });
    }
};

class SocketServerSecure : public SocketServerBase {

 public:
    SocketServerSecure(const std::string &certFile,
                       const std::string &privateKeyFile,
                       const std::string &verifyFile = std::string())
        : SocketServerBase(443), context(asio::ssl::context::tlsv12) {

        context.use_certificate_chain_file(certFile);
        context.use_private_key_file(privateKeyFile, asio::ssl::context::pem);
        context.set_options(SSL_OP_NO_COMPRESSION);

        if (verifyFile.size() > 0) {
            context.load_verify_file(verifyFile);
            context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert |
                asio::ssl::verify_client_once);
            setSessionIdContext = true;
        }
    }

    void start() override {
        if (setSessionIdContext) {
            // Creating sessionIdContext from address:port but reversed due to small SSL_MAX_SSL_SESSION_ID_LENGTH
            sessionIdContext = std::to_string(config.port) + ':';
            sessionIdContext.append(config.address.rbegin(), config.address.rend());

            SSL_CTX_set_session_id_context(
                context.native_handle(),
                reinterpret_cast<const unsigned char *>(sessionIdContext.data()),
                (unsigned int) std::min<std::size_t>(sessionIdContext.size(), SSL_MAX_SSL_SESSION_ID_LENGTH)
            );
        }
        SocketServerBase::start();
    }

 protected:
    std::string sessionIdContext;
    bool setSessionIdContext = false;
    asio::ssl::context context;

    void accept() override {
        std::shared_ptr<Connection>
            connection(new Connection(handlerRunner, config.timeoutIdle, *ioService, context));

        acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const ErrorCode &ec) {
          auto lock = connection->handlerRunner->continueLock();
          if (!lock) {
              return;
          }

          // Immediately start accepting a new connection (if ioService hasn't been stopped)
          if (ec != asio::error::operation_aborted) {
              accept();
          }

          if (!ec) {
              asio::ip::tcp::no_delay option(true);
              connection->socket->lowest_layer().set_option(option);

              connection->timeoutSet(config.timeoutRequest);
              connection->socket
                        ->async_handshake([this, connection](const ErrorCode &ec) {
                          auto sublock = connection->handlerRunner->continueLock();
                          if (!sublock) {
                              return;
                          }

                          connection->timeoutCancel();
                          if (!ec)
                              handshakeRead(connection);
                        });
          }
        });
    }
};

} //wss
} //server
} //websocket


#endif //WSSERVER_WEBSOCKETSERVER_H
