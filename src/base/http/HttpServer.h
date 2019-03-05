/*!
 * wsserver.
 * HttpServer.h
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef WSSERVER_HTTPSERVER_H
#define WSSERVER_HTTPSERVER_H

#include "utility.hpp"
#include "crypto.hpp"
#include "../BaseServer.h"
#include "../SocketLayerWrapper.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <memory>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <algorithm>
#include <openssl/ssl.h>
#include <boost/asio/ssl.hpp>
#include <httb/request.h>
#include <httb/response.h>

namespace wss {
namespace server {
namespace http {

using namespace wss::utils;
namespace asio = boost::asio;
using error_code = boost::system::error_code;
namespace errc = boost::system::errc;
namespace make_error_code = boost::system::errc;

class Server;
class ServerSecure;

class ServerBase : public BaseServer {
 protected:
    class Session;

 public:
    class Response : public std::enable_shared_from_this<Response>, public std::ostream {
        friend class ServerBase;
        friend class Server;
        friend class ServerSecure;

        asio::streambuf streambuf;

        std::shared_ptr<Session> session;
        long timeout_content;

        Response(std::shared_ptr<Session> session, long timeout_content) noexcept
            : std::ostream(&streambuf), session(std::move(session)), timeout_content(timeout_content) { }

        template<typename size_type>
        void write_header(const CaseInsensitiveMultimap &header, size_type size) {
            bool content_length_written = false;
            bool chunked_transfer_encoding = false;
            for (auto &field : header) {
                if (!content_length_written && case_insensitive_equal(field.first, "content-length"))
                    content_length_written = true;
                else if (!chunked_transfer_encoding && case_insensitive_equal(field.first, "transfer-encoding")
                    && case_insensitive_equal(field.second, "chunked"))
                    chunked_transfer_encoding = true;

                *this << field.first << ": " << field.second << "\r\n";
            }
            if (!content_length_written && !chunked_transfer_encoding && !close_connection_after_response)
                *this << "Content-Length: " << size << "\r\n\r\n";
            else
                *this << "\r\n";
        }

     public:
        std::size_t size() noexcept {
            return streambuf.size();
        }

        /// Use this function if you need to recursively send parts of a longer message
        void send(const std::function<void(const error_code &)> &callback = nullptr) noexcept {
            session->connection->set_timeout(timeout_content);
            auto self = this->shared_from_this(); // Keep Response instance alive through the following async_write
            session->connection->socket->async_write(
                streambuf,
                [self, callback](const error_code &ec, std::size_t /*bytes_transferred*/) {
                  self->session->connection->cancel_timeout();
                  auto lock = self->session->connection->handler_runner->continueLock();
                  if (!lock)
                      return;
                  if (callback)
                      callback(ec);
                });
        }

        /// Write directly to stream buffer using std::ostream::write
        void write(const char_type *ptr, std::streamsize n) {
            std::ostream::write(ptr, n);
        }

        /// Convenience function for writing status line, potential header fields, and empty content
        void write(StatusCode status_code = StatusCode::success_ok,
                   const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
            *this << "HTTP/1.1 " << wss::server::status_code(status_code) << "\r\n";
            write_header(header, 0);
        }

        /// Convenience function for writing status line, header fields, and content
        void write(StatusCode status_code,
                   const std::string &content,
                   const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
            *this << "HTTP/1.1 " << wss::server::status_code(status_code) << "\r\n";
            write_header(header, content.size());
            if (!content.empty())
                *this << content;
        }

        /// Convenience function for writing status line, header fields, and content
        void write(StatusCode status_code,
                   std::istream &content,
                   const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
            *this << "HTTP/1.1 " << wss::server::status_code(status_code) << "\r\n";
            content.seekg(0, std::ios::end);
            auto size = content.tellg();
            content.seekg(0, std::ios::beg);
            write_header(header, size);
            if (size)
                *this << content.rdbuf();
        }

        /// Convenience function for writing success status line, header fields, and content
        void write(const std::string &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
            write(StatusCode::success_ok, content, header);
        }

        /// Convenience function for writing success status line, header fields, and content
        void write(std::istream &content, const CaseInsensitiveMultimap &header = CaseInsensitiveMultimap()) {
            write(StatusCode::success_ok, content, header);
        }

        /// Convenience function for writing success status line, and header fields
        void write(const CaseInsensitiveMultimap &header) {
            write(StatusCode::success_ok, std::string(), header);
        }

        /// If true, force server to close the connection after the response have been sent.
        ///
        /// This is useful when implementing a HTTP/1.0-server sending content
        /// without specifying the content length.
        bool close_connection_after_response = false;
    };

    class Content : public std::istream {
        friend class ServerBase;

     public:
        std::size_t size() noexcept {
            if (!m_cacheInited) {
                return streambuf.size();
            }

            return m_dataSize;
        }

        /// Convenience function to return std::string. The stream buffer is consumed.
        /// UPD: stream buffer is consumed, BUT, cached. To consume cache, call consume()
        std::string string() const noexcept {
            try {
                if (!m_cacheInited) {
                    m_dataSize = streambuf.size();
                    std::ostringstream os;
                    os << rdbuf();
                    m_cachedBuf = os.str();
                    m_cacheInited = true;
                }

                return m_cachedBuf;
            } catch (...) {
                return std::string();
            }
        }

        void consume() {
            m_cachedBuf = "";
            m_dataSize = 0;
        }

     private:
        mutable std::string m_cachedBuf;
        mutable bool m_cacheInited = false;
        mutable size_t m_dataSize;
        asio::streambuf &streambuf;
        Content(asio::streambuf &streambuf) noexcept : std::istream(&streambuf), streambuf(streambuf) { }
    };

    class Request {
        friend class ServerBase;
        friend class Server;
        friend class ServerSecure;
        friend class Session;

        asio::streambuf streambuf;

        Request(std::size_t max_request_streambuf_size,
                std::shared_ptr<asio::ip::tcp::endpoint> remote_endpoint) noexcept
            : streambuf(max_request_streambuf_size), content(streambuf), remote_endpoint(std::move(remote_endpoint)) { }

     public:
        std::string method, path, query_string, http_version;

        Content content;

        CaseInsensitiveMultimap header;

        regexns::smatch path_match;

        std::shared_ptr<asio::ip::tcp::endpoint> remote_endpoint;

        std::string remote_endpoint_address() noexcept {
            try {
                return remote_endpoint->address().to_string();
            }
            catch (...) {
                return std::string();
            }
        }

        httb::request toHttbRequest() const {
            httb::request out;
            out.setBody(content.string());
            out.parseParamsString(query_string);
            for (const auto &kv: header) {
                out.addHeader(kv);
            }

            return out;
        }

        unsigned short remote_endpoint_port() noexcept {
            return remote_endpoint->port();
        }

        /// Returns query keys with percent-decoded values.
        CaseInsensitiveMultimap parse_query_string() noexcept {
            return QueryString::parse(query_string);
        }
    };

 protected:
    class Connection : public std::enable_shared_from_this<Connection> {
     public:
        template<typename... Args>
        Connection(std::shared_ptr<ScopeRunner> handler_runner, Args &&... args) noexcept :
            handler_runner(std::move(handler_runner)),
            socket(new SocketLayerWrapper(std::forward<Args>(args)...)) { }

        std::shared_ptr<ScopeRunner> handler_runner;
        // Socket must be unique_ptr since asio::ssl::stream<asio::ip::tcp::socket> is not movable
        std::unique_ptr<SocketLayerWrapper> socket;
        std::mutex socket_close_mutex;

        std::unique_ptr<asio::steady_timer> timer;

        std::shared_ptr<asio::ip::tcp::endpoint> remote_endpoint;

        void close() noexcept {
            error_code ec;
            std::unique_lock<std::mutex>
                lock(socket_close_mutex); // The following operations seems to be needed to run sequentially
            socket->lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
            socket->lowest_layer().close(ec);
        }

        void set_timeout(long seconds) noexcept {
            if (seconds == 0) {
                timer = nullptr;
                return;
            }

            timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(socket->get_io_service()));
            timer->expires_from_now(std::chrono::seconds(seconds));
            auto self = this->shared_from_this();
            timer->async_wait([self](const error_code &ec) {
              if (!ec)
                  self->close();
            });
        }

        void cancel_timeout() noexcept {
            if (timer) {
                error_code ec;
                timer->cancel(ec);
            }
        }
    };

    class Session {
     public:
        Session(std::size_t max_request_streambuf_size, std::shared_ptr<Connection> connection) noexcept : connection(
            std::move(connection)) {
            if (!this->connection->remote_endpoint) {
                error_code ec;
                this->connection->remote_endpoint =
                    std::make_shared<asio::ip::tcp::endpoint>(this->connection->socket->lowest_layer()
                                                                  .remote_endpoint(ec));
            }
            request =
                std::shared_ptr<Request>(new Request(max_request_streambuf_size, this->connection->remote_endpoint));
        }

        std::shared_ptr<Connection> connection;
        std::shared_ptr<Request> request;
    };

 public:
    class Config {
        friend class ServerBase;

        Config(unsigned short port) noexcept : port(port) { }

     public:
        /// Port number to use. Defaults to 80 for HTTP and 443 for HTTPS.
        unsigned short port;
        /// If io_service is not set, number of threads that the server will use when start() is called.
        /// Defaults to 1 thread.
        std::size_t thread_pool_size = 1;
        /// Timeout on request handling. Defaults to 5 seconds.
        long timeout_request = 5;
        /// Timeout on content handling. Defaults to 300 seconds.
        long timeout_content = 300;
        /// Maximum size of request stream buffer. Defaults to architecture maximum.
        /// Reaching this limit will result in a message_size error code.
        std::size_t max_request_streambuf_size = std::numeric_limits<std::size_t>::max();
        /// IPv4 address in dotted decimal form or IPv6 address in hexadecimal notation.
        /// If empty, the address will be any address.
        std::string address;
        /// Set to false to avoid binding the socket to an address that is already in use. Defaults to true.
        bool reuse_address = true;
    };
    /// Set before calling start().
    Config config;

 public:
    using DefaultResourceEndpoint = std::map<std::string,
                                             std::function<void(std::shared_ptr<typename ServerBase::Response>,
                                                                std::shared_ptr<typename ServerBase::Request>)>>;
    using ResourceEndpoint = std::map<
        RegexOrderable,
        DefaultResourceEndpoint
    >;
    using OnErrorFunc = std::function<void(std::shared_ptr<typename ServerBase::Request>, const error_code &)>;
    using OnUpgradeFunc = std::function<void(std::unique_ptr<SocketLayerWrapper> &,
                                             std::shared_ptr<typename ServerBase::Request>)>;
    /// Warning: do not add or remove resources after start() is called
    ResourceEndpoint resource;
    DefaultResourceEndpoint default_resource;

    OnErrorFunc on_error;
    OnUpgradeFunc on_upgrade;

    /// If you have your own asio::io_service, store its pointer here before running start().
    std::shared_ptr<asio::io_service> io_service;

    void start() override {
        on_error = [](std::shared_ptr<typename ServerBase::Request>, const error_code &) { };
        if (!io_service) {
            io_service = std::make_shared<asio::io_service>();
            internal_io_service = true;
        }

        if (io_service->stopped()) {
            io_service->reset();
        }

        asio::ip::tcp::endpoint endpoint;
        if (config.address.size() > 0)
            endpoint = asio::ip::tcp::endpoint(asio::ip::address::from_string(config.address), config.port);
        else
            endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), config.port);

        if (!acceptor) {
            acceptor = std::make_unique<asio::ip::tcp::acceptor>(*io_service);
        }

        acceptor->open(endpoint.protocol());
        acceptor->set_option(asio::socket_base::reuse_address(config.reuse_address));
        acceptor->bind(endpoint);
        acceptor->listen();

        accept();

        if (internal_io_service) {
            // If thread_pool_size>1, start m_io_service.run() in (thread_pool_size-1) threads for thread-pooling
            threads.clear();
            for (std::size_t c = 1; c < config.thread_pool_size; c++) {
                threads.emplace_back([this]() {
                  this->io_service->run();
                });
            }

            // Main thread
            if (config.thread_pool_size > 0)
                io_service->run();

            // Wait for the rest of the threads, if any, to finish as well
            for (auto &t : threads)
                t.join();
        }
    }

    /// Stop accepting new requests, and close current connections.
    void stop() override {
        if (acceptor) {
            error_code ec;
            acceptor->close(ec);

            {
                std::unique_lock<std::mutex> lock(*connections_mutex);
                for (auto &connection : *connections)
                    connection->close();
                connections->clear();
            }

            if (internal_io_service) {
                io_service->stop();
            }

        }
    }

    virtual ~ServerBase() noexcept {
        handler_runner->stop();
        stop();
    }

 protected:
    bool internal_io_service = false;

    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
    std::vector<std::thread> threads;

    std::shared_ptr<std::unordered_set<Connection *>> connections;
    std::shared_ptr<std::mutex> connections_mutex;

    std::shared_ptr<ScopeRunner> handler_runner;

    ServerBase(unsigned short port) noexcept
        : config(port),
          connections(new std::unordered_set<Connection *>()),
          connections_mutex(new std::mutex()),
          handler_runner(new ScopeRunner()) { }

    virtual void accept() override = 0;

    template<typename... Args>
    std::shared_ptr<Connection> create_connection(Args &&... args) noexcept {
        auto connections = this->connections;
        auto connections_mutex = this->connections_mutex;
        auto connection = std::shared_ptr<Connection>(
            new Connection(handler_runner, std::forward<Args>(args)...),
            [connections, connections_mutex](Connection *connection) {
              {
                  std::unique_lock<std::mutex> lock(*connections_mutex);
                  auto it = connections->find(connection);
                  if (it != connections->end())
                      connections->erase(it);
              }
              delete connection;
            });
        {
            std::unique_lock<std::mutex> lock(*connections_mutex);
            connections->emplace(connection.get());
        }
        return connection;
    }

    void read(const std::shared_ptr<Session> &session) {
        session->connection->set_timeout(config.timeout_request);

        session->connection->socket->async_read_until(
            session->request->streambuf,
            "\r\n\r\n",
            [this, session](const error_code &ec, std::size_t bytes_transferred) {
              session->connection->cancel_timeout();
              auto lock = session->connection->handler_runner->continueLock();
              if (!lock) {
                  return;
              }

              if ((!ec || ec == asio::error::not_found)
                  && session->request->streambuf.size() == session->request->streambuf.max_size()) {
                  auto response = std::shared_ptr<Response>(new Response(session, this->config.timeout_content));
                  response->write(StatusCode::client_error_payload_too_large);
                  response->send();
                  if (this->on_error)
                      this->on_error(session->request, make_error_code::make_error_code(errc::message_size));
                  return;
              }
              if (!ec) {
                  // request->streambuf.size() is not necessarily the same as bytes_transferred, from Boost-docs:
                  // "After a successful async_read_until operation, the streambuf may contain additional data beyond the delimiter"
                  // The chosen solution is to extract lines from the stream directly when parsing the header. What is left of the
                  // streambuf (maybe some bytes of the content) is appended to in the async_read-function below (for retrieving content).
                  std::size_t num_additional_bytes = session->request->streambuf.size() - bytes_transferred;

                  if (!RequestMessage::parse(session->request->content,
                                             session->request->method,
                                             session->request->path,
                                             session->request->query_string,
                                             session->request->http_version,
                                             session->request->header)) {
                      if (this->on_error)
                          this->on_error(session->request, make_error_code::make_error_code(errc::protocol_error));
                      return;
                  }

                  // If content, read that as well
                  auto header_it = session->request->header.find("Content-Length");
                  if (header_it != session->request->header.end()) {
                      unsigned long long content_length = 0;
                      try {
                          content_length = stoull(header_it->second);
                      }
                      catch (const std::exception &e) {
                          if (this->on_error)
                              this->on_error(session->request, make_error_code::make_error_code(errc::protocol_error));
                          return;
                      }
                      if (content_length > num_additional_bytes) {
                          session->connection->set_timeout(config.timeout_content);
                          session->connection->socket->async_read(
                              session->request->streambuf,
                              asio::transfer_exactly(content_length - num_additional_bytes),
                              [this, session](const error_code &ec, std::size_t /*bytes_transferred*/) {
                                session->connection->cancel_timeout();
                                auto lock = session->connection->handler_runner->continueLock();
                                if (!lock) {
                                    return;
                                }

                                if (!ec) {
                                    if (session->request->streambuf.size() == session->request->streambuf.max_size()) {
                                        auto response =
                                            std::shared_ptr<Response>(new Response(session,
                                                                                   this->config.timeout_content));

                                        response->write(StatusCode::client_error_payload_too_large);
                                        response->send();
                                        if (this->on_error)
                                            this->on_error(session->request,
                                                           make_error_code::make_error_code(errc::message_size));
                                        return;
                                    }
                                    this->find_resource(session);
                                } else if (this->on_error)
                                    this->on_error(session->request, ec);
                              });
                      } else {
                          this->find_resource(session);
                      }
                  } else if (
                      (header_it = session->request->header.find("Transfer-Encoding")) != session->request->header.end()
                          && header_it->second == "chunked") {
                      auto
                          chunks_streambuf = std::make_shared<asio::streambuf>(this->config.max_request_streambuf_size);
                      this->read_chunked_transfer_encoded(session, chunks_streambuf);
                  } else
                      this->find_resource(session);
              } else if (this->on_error)
                  this->on_error(session->request, ec);
            });
    }

    void read_chunked_transfer_encoded(const std::shared_ptr<Session> &session,
                                       const std::shared_ptr<asio::streambuf> &chunks_streambuf) {
        session->connection->set_timeout(config.timeout_content);
        session->connection->socket->async_read_until(
            session->request->streambuf,
            "\r\n",
            [this, session, chunks_streambuf](const error_code &ec, size_t bytes_transferred) {
              session->connection->cancel_timeout();
              auto lock = session->connection->handler_runner->continueLock();
              if (!lock)
                  return;
              if ((!ec || ec == asio::error::not_found)
                  && session->request->streambuf.size() == session->request->streambuf.max_size()) {
                  auto response =
                      std::shared_ptr<Response>(new Response(session, this->config.timeout_content));
                  response->write(StatusCode::client_error_payload_too_large);
                  response->send();
                  if (this->on_error)
                      this->on_error(session->request,
                                     make_error_code::make_error_code(errc::message_size));
                  return;
              }
              if (!ec) {
                  std::string line;
                  getline(session->request->content, line);
                  bytes_transferred -= line.size() + 1;
                  line.pop_back();
                  unsigned long length = 0;
                  try {
                      length = stoul(line, 0, 16);
                  }
                  catch (...) {
                      if (this->on_error)
                          this->on_error(session->request,
                                         make_error_code::make_error_code(errc::protocol_error));
                      return;
                  }

                  auto num_additional_bytes = session->request->streambuf.size() - bytes_transferred;

                  if ((2 + length) > num_additional_bytes) {
                      session->connection->set_timeout(config.timeout_content);
                      session->connection->socket->async_read(
                          session->request->streambuf,
                          asio::transfer_exactly(2 + length - num_additional_bytes),
                          [this, session, chunks_streambuf, length](const error_code &ec,
                                                                    size_t /*bytes_transferred*/) {
                            session->connection->cancel_timeout();
                            auto lock =
                                session->connection->handler_runner->continueLock();
                            if (!lock)
                                return;
                            if (!ec) {
                                if (session->request->streambuf.size()
                                    == session->request->streambuf.max_size()) {
                                    auto response =
                                        std::shared_ptr<Response>(new Response(session,
                                                                               this->config
                                                                                   .timeout_content));
                                    response
                                        ->write(StatusCode::client_error_payload_too_large);
                                    response->send();
                                    if (this->on_error)
                                        this->on_error(session->request,
                                                       make_error_code::make_error_code(
                                                           errc::message_size));
                                    return;
                                }
                                this->read_chunked_transfer_encoded_chunk(session,
                                                                          chunks_streambuf,
                                                                          length);
                            } else if (this->on_error)
                                this->on_error(session->request, ec);
                          });
                  } else
                      this->read_chunked_transfer_encoded_chunk(session, chunks_streambuf, length);
              } else if (this->on_error)
                  this->on_error(session->request, ec);
            });
    }

    void read_chunked_transfer_encoded_chunk(const std::shared_ptr<Session> &session,
                                             const std::shared_ptr<asio::streambuf> &chunks_streambuf,
                                             unsigned long length) {
        std::ostream tmp_stream(chunks_streambuf.get());
        if (length > 0) {
            std::unique_ptr<char[]> buffer(new char[length]);
            session->request->content.read(buffer.get(), static_cast<std::streamsize>(length));
            tmp_stream.write(buffer.get(), static_cast<std::streamsize>(length));
            if (chunks_streambuf->size() == chunks_streambuf->max_size()) {
                auto response = std::shared_ptr<Response>(new Response(session, this->config.timeout_content));
                response->write(StatusCode::client_error_payload_too_large);
                response->send();
                if (this->on_error)
                    this->on_error(session->request, make_error_code::make_error_code(errc::message_size));
                return;
            }
        }

        // Remove "\r\n"
        session->request->content.get();
        session->request->content.get();

        if (length > 0)
            read_chunked_transfer_encoded(session, chunks_streambuf);
        else {
            if (chunks_streambuf->size() > 0) {
                std::ostream ostream(&session->request->streambuf);
                ostream << chunks_streambuf.get();
            }
            this->find_resource(session);
        }
    }

    void find_resource(const std::shared_ptr<Session> &session) {
        // Upgrade connection
        if (on_upgrade) {
            auto it = session->request->header.find("Upgrade");
            if (it != session->request->header.end()) {
                // remove connection from connections
                {
                    std::unique_lock<std::mutex> lock(*connections_mutex);
                    auto it = connections->find(session->connection.get());
                    if (it != connections->end())
                        connections->erase(it);
                }

                on_upgrade(session->connection->socket, session->request);
                return;
            }
        }
        // Find path- and method-match, and call write
        for (auto &regex_method : resource) {
            auto it = regex_method.second.find(session->request->method);
            if (it != regex_method.second.end()) {
                regexns::smatch sm_res;
                if (regexns::regex_match(session->request->path, sm_res, regex_method.first)) {
                    session->request->path_match = std::move(sm_res);
                    write(session, it->second);
                    return;
                }
            }
        }
        auto it = default_resource.find(session->request->method);
        if (it != default_resource.end())
            write(session, it->second);
    }

    void write(const std::shared_ptr<Session> &session,
               std::function<void(std::shared_ptr<typename ServerBase::Response>,
                                  std::shared_ptr<typename ServerBase::Request>)> &resource_function) {
        session->connection->set_timeout(config.timeout_content);
        auto response =
            std::shared_ptr<Response>(new Response(session, config.timeout_content), [this](Response *response_ptr) {
              auto response = std::shared_ptr<Response>(response_ptr);
              response->send([this, response](const error_code &ec) {
                if (!ec) {
                    if (response->close_connection_after_response)
                        return;

                    auto range = response->session->request->header.equal_range("Connection");
                    for (auto it = range.first; it != range.second; it++) {
                        if (case_insensitive_equal(it->second, "close"))
                            return;
                        else if (case_insensitive_equal(it->second, "keep-alive")) {
                            auto new_session = std::make_shared<Session>(this->config.max_request_streambuf_size,
                                                                         response->session->connection);
                            this->read(new_session);
                            return;
                        }
                    }
                    if (response->session->request->http_version >= "1.1") {
                        auto new_session = std::make_shared<Session>(this->config.max_request_streambuf_size,
                                                                     response->session->connection);
                        this->read(new_session);
                        return;
                    }
                } else if (this->on_error)
                    this->on_error(response->session->request, ec);
              });
            });

        try {
            resource_function(response, session->request);
        }
        catch (const std::exception &) {
            if (on_error)
                on_error(session->request, make_error_code::make_error_code(errc::operation_canceled));
            return;
        }
    }
};

class Server : public ServerBase {
 public:
    Server() noexcept : ServerBase(80) { }

 protected:
    void accept() override {
        auto connection = create_connection(*io_service);

        acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const error_code &ec) {
          auto lock = connection->handler_runner->continueLock();
          if (!lock)
              return;

          // Immediately start accepting a new connection (unless io_service has been stopped)
          if (ec != asio::error::operation_aborted)
              this->accept();

          auto session = std::make_shared<Session>(config.max_request_streambuf_size, connection);

          if (!ec) {
              asio::ip::tcp::no_delay option(true);
              error_code ec;
              session->connection->socket->set_option(option, ec);

              this->read(session);
          } else if (this->on_error)
              this->on_error(session->request, ec);
        });
    }
};

class ServerSecure : public ServerBase {
    std::string session_id_context;
    bool set_session_id_context = false;

 public:
    ServerSecure(
        const std::string &cert_file,
        const std::string &private_key_file,
        const std::string &verify_file = std::string()) :
        ServerBase(443), context(asio::ssl::context::tlsv12) {
        context.use_certificate_chain_file(cert_file);
        context.use_private_key_file(private_key_file, asio::ssl::context::pem);

        if (verify_file.size() > 0) {
            context.load_verify_file(verify_file);
            context.set_verify_mode(
                asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert | asio::ssl::verify_client_once);
            set_session_id_context = true;
        }
    }

    void start() override {
        if (set_session_id_context) {
            // Creating session_id_context from address:port but reversed due to small SSL_MAX_SSL_SESSION_ID_LENGTH
            session_id_context = std::to_string(config.port) + ':';
            session_id_context.append(config.address.rbegin(), config.address.rend());
            SSL_CTX_set_session_id_context(
                context.native_handle(),
                reinterpret_cast<const unsigned char *>(session_id_context.data()),
                std::min<std::size_t>(session_id_context.size(), SSL_MAX_SSL_SESSION_ID_LENGTH)
            );
        }
        ServerBase::start();
    }

 protected:
    asio::ssl::context context;

    void accept() override {
        auto connection = create_connection(*io_service, context);

        acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const error_code &ec) {
          auto lock = connection->handler_runner->continueLock();
          if (!lock) {
              return;
          }

          if (ec != asio::error::operation_aborted)
              this->accept();

          auto session = std::make_shared<Session>(config.max_request_streambuf_size, connection);

          if (!ec) {
              asio::ip::tcp::no_delay option(true);
              error_code ec;
              session->connection->socket->lowest_layer().set_option(option, ec);

              session->connection->set_timeout(config.timeout_request);
              session->connection->socket
                     ->async_handshake([this, session](const error_code &ec) {
                       session->connection->cancel_timeout();
                       auto lock = session->connection->handler_runner->continueLock();
                       if (!lock) {
                           return;
                       }

                       if (!ec) {
                           this->read(session);
                       } else if (on_error) {
                           on_error(session->request, ec);
                       }
                     });

          } else if (on_error)
              on_error(session->request, ec);
        });
    }
};

} //namespace wss
} //namespace server
} //namespace http

#endif //WSSERVER_HTTPSERVER_H
