/*!
 * scatter.
 * SocketLayerWrapper.hpp
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef SCATTER_SOCKETLAYERWRAPPER_HPP
#define SCATTER_SOCKETLAYERWRAPPER_HPP

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/context.hpp>
#include <functional>
#include <memory>
#include <vector>

using namespace boost;
using namespace boost::asio;

#if (BOOST_VERSION >= 106600)
#include <boost/asio/io_context.hpp>

namespace wss {
using basic_socket_name = asio::basic_socket<asio::ip::tcp>;
using io_context_service = asio::io_context;
}
#else
#include <boost/asio/io_service.hpp>
namespace wss {
using basic_socket_name = asio::basic_socket<asio::ip::tcp, asio::stream_socket_service<asio::ip::tcp>>;
using io_context_service = asio::io_service;
}
#endif

class SocketLayerWrapper {
 private:
    asio::ip::tcp::socket *m_insecure;
    asio::ssl::stream<asio::ip::tcp::socket> *m_secure;

 public:
    template<typename ...Args>
    SocketLayerWrapper(Args &&...args) {
        asio::io_service srv;
        if (sizeof...(args) == 1) {
            m_insecure = new asio::ip::tcp::socket(std::forward<Args>(args)...);
            m_secure = nullptr;
        } else {
            m_secure = new asio::ssl::stream<asio::ip::tcp::socket>(std::forward<Args>(args)...);
            m_insecure = nullptr;
        }

    }
    SocketLayerWrapper(wss::io_context_service &ioContext) {
        m_insecure = new asio::ip::tcp::socket(ioContext);
        m_secure = nullptr;
    }

    SocketLayerWrapper(wss::io_context_service &ioContext, asio::ssl::context &sslContext) {
        m_secure = new asio::ssl::stream<asio::ip::tcp::socket>(ioContext, sslContext);
        m_insecure = nullptr;
    }

    asio::ip::tcp::socket *rawInsecure() {
        return m_insecure;
    }

    asio::ssl::stream<asio::ip::tcp::socket> *rawSecure() {
        return m_secure;
    }

    template<typename ReadHandler = std::function<void(system::error_code, std::size_t)>>
    void async_read_until(asio::streambuf &streambuf, const std::string &match_condition, const ReadHandler &handler) {
        if (isSecure()) {
            asio::async_read_until(*rawSecure(), streambuf, match_condition, handler);
        } else {
            asio::async_read_until(*rawInsecure(), streambuf, match_condition, handler);
        }
    }

    template<typename CompleteCondition, typename ReadHandler = std::function<void(system::error_code, std::size_t)>>
    void async_read(asio::streambuf &streambuf, CompleteCondition cond, const ReadHandler &handler) {
        if (isSecure()) {
            asio::async_read(*rawSecure(), streambuf, cond, handler);
        } else {
            asio::async_read(*rawInsecure(), streambuf, cond, handler);
        }
    }

    template<typename... Args>
    void set_option(Args &&... args) {
        if (isSecure()) {
            m_secure->lowest_layer().set_option(std::forward<Args>(args)...);
        } else {
            m_insecure->set_option(std::forward<Args>(args)...);
        }
    }

    template<typename WriteHandler = std::function<void(system::error_code, std::size_t)>>
    void async_write(asio::streambuf &streambuf, const WriteHandler &handler) {
        if (isSecure()) {
            asio::async_write(*rawSecure(), streambuf, handler);
        } else {
            asio::async_write(*rawInsecure(), streambuf, handler);
        }
    }

    template<typename WriteHandler = std::function<void(system::error_code, std::size_t)>>
    void async_write(const std::vector<asio::const_buffer> &streambuf, const WriteHandler &handler) {
        if (isSecure()) {
            asio::async_write(*rawSecure(), streambuf, handler);
        } else {
            asio::async_write(*rawInsecure(), streambuf, handler);
        }
    }

    template<typename HandshakeHandler = std::function<void(boost::system::error_code)>>
    void async_handshake(const HandshakeHandler &handler) {
        if (isSecure()) {
            m_secure->async_handshake(asio::ssl::stream_base::server, handler);
        }
    }

    const wss::basic_socket_name &lowest_layer() const {
        if (isSecure()) {
            return m_secure->lowest_layer();
        }

        return m_insecure->lowest_layer();
    }

    wss::basic_socket_name &lowest_layer() {
        if (isSecure()) {
            return m_secure->lowest_layer();
        }

        return m_insecure->lowest_layer();
    }

    const wss::io_context_service &get_io_service() const {
        if (isSecure()) {
            return m_secure->get_io_service();
        }

        return m_insecure->get_io_service();
    }

    wss::io_context_service &get_io_service() {
        if (isSecure()) {
            return m_secure->get_io_service();
        }

        return m_insecure->get_io_service();
    }

    ~SocketLayerWrapper() {
        if (m_secure != nullptr) {
            delete m_secure;
        }

        if (m_insecure != nullptr) {
            delete m_insecure;
        }
    }

    bool isSecure() const {
        return m_secure != nullptr;
    }

};

#endif //SCATTER_SOCKETLAYERWRAPPER_HPP
