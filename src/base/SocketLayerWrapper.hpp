/*!
 * wsserver.
 * SocketLayerWrapper.hpp
 *
 * \date 2018
 * \author Eduard Maximovich (edward.vstock@gmail.com)
 * \link   https://github.com/edwardstock
 */

#ifndef WSSERVER_SOCKETLAYERWRAPPER_HPP
#define WSSERVER_SOCKETLAYERWRAPPER_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <functional>
#include <memory>
#include <vector>

using namespace boost;

class SocketLayerWrapper {
 private:
    asio::ip::tcp::socket *m_insecure;
    asio::ssl::stream<asio::ip::tcp::socket> *m_secure;

 public:
    template<typename ...Args>
    SocketLayerWrapper(Args &&...args) {
        if (sizeof...(args) == 1) {
            m_insecure = new asio::ip::tcp::socket(std::forward<Args>(args)...);
            m_secure = nullptr;
        } else {
            m_secure = new asio::ssl::stream<asio::ip::tcp::socket>(std::forward<Args>(args)...);
            m_insecure = nullptr;
        }

    }
    SocketLayerWrapper(boost::asio::io_context &ioContext) {
        m_insecure = new asio::ip::tcp::socket(ioContext);
        m_secure = nullptr;
    }

    SocketLayerWrapper(boost::asio::io_context &ioContext, asio::ssl::context &sslContext) {
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

    const asio::basic_socket<asio::ip::tcp> &lowest_layer() const {
        if (isSecure()) {
            return m_secure->lowest_layer();
        }

        return m_insecure->lowest_layer();
    }

    asio::basic_socket<asio::ip::tcp> &lowest_layer() {
        if (isSecure()) {
            return m_secure->lowest_layer();
        }

        return m_insecure->lowest_layer();
    }

    const asio::io_context &get_io_service() const {
        if (isSecure()) {
            return m_secure->get_io_service();
        }

        return m_insecure->get_io_service();
    }

    asio::io_context &get_io_service() {
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

#endif //WSSERVER_SOCKETLAYERWRAPPER_HPP
