/**
 * wsserver
 * ConnectionStorage.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_CONNECTIONSTORAGE_H
#define WSSERVER_CONNECTIONSTORAGE_H

#include <mutex>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <toolboxpp.h>
#include "../defs.h"
namespace wss {

struct ConnectionNotFound : std::exception {
  const char *what() const throw() override {
      return "Connection not found or already disconnected";
  }
};

/// \brief Container for handling and storing client connections
class ConnectionStorage {
 private:
    mutable std::recursive_mutex connectionMutex;
    mutable std::mutex pongMutex;
    wss::UserMap<wss::ConnectionMap<WsConnectionPtr>> idMap;
    wss::ConnectionMap<std::pair<UserId, bool>> waitForPong;

 public:
    /// \brief Default empty constructor
    ConnectionStorage() = default;

    /// \brief Deleted copy ctr, cause contains mutex
    /// \param other
    ConnectionStorage(const ConnectionStorage &other) = delete;

    /// \brief Deleted move ctr, cause contains mutex
    /// \param other
    ConnectionStorage(ConnectionStorage &&other) = delete;

    /// \brief On destruction, all connections trying to disconnect, than map of connections will be cleaned up
    ~ConnectionStorage();

    /// \brief Check for existing connection using UserId
    /// \param id UserId
    /// \return true if connection with UserId in map
    bool exists(wss::UserId id) const;

    /// \brief Check connection is not nullptr, remove if null
    /// \param connection
    /// \return true if is valid connection, false othwerwise
    bool verify(wss::UserId userId, wss::ConnectionId connectionId);

    /// \brief Count total users in map
    /// \return Size of map user:connections
    std::size_t size() const;

    /// \brief Count total connections for entire user
    /// \param id UserId
    /// \return Size of vector user connections
    std::size_t size(wss::UserId id);

    /// \brief Add to map new connection (non-unique)
    /// \param id UserId
    /// \param connection SimpleWeb::Connection shared_ptr
    void add(wss::UserId id, const wss::WsConnectionPtr &connection);

    /// \brief Remove from map connections by user id
    /// \param id UserId
    void remove(wss::UserId id);

    /// \brief Remove from map connections by user id and connection unique id
    /// \param id UserId
    void remove(wss::UserId id, wss::ConnectionId connectionId);

    /// \brief Remove from connection by unique connection id : std::stoul("ip address without dots" + "remote port num").
    /// \param connection SimpleWeb::Connection shared_ptr
    void remove(const wss::WsConnectionPtr &connection);

    /// \brief Return from map connections for entire user
    /// \param id UserId
    /// \return  Vector of connections
    wss::ConnectionMap<wss::WsConnectionPtr> &get(wss::UserId id);

    /// \brief Return whole map by const reference
    /// \return unordered_map< UserId, vector<WsConnectionPtr> >
    const wss::UserMap<wss::ConnectionMap<wss::WsConnectionPtr>> &get();

    /// \brief Callback function to handle connections for entire UserId
    /// \param id UserId
    /// \param handler callback function (void<WsConnectionPtr &>)
    void handle(UserId id, std::function<void(WsConnectionPtr &)> &&handler);

    /// \brief Mark connection as waiting for pong response from client.
    /// \param connection
    void markPongWait(const WsConnectionPtr &connection);

    /// \brief Mark connection as received pong response from client
    /// \param connection
    void markPongReceived(const WsConnectionPtr &connection);

    /// \brief Disconnect all queued connections, that waits for pong response but not received
    /// \return count of disconnected connections
    std::size_t disconnectWithoutPong();

    /// \brief Works like a map of connections, just pass UserId to square braces
    /// \param id UserId unsigned long
    /// \return map of user connections
    wss::ConnectionMap<wss::WsConnectionPtr> &operator[](wss::UserId id) noexcept;
};

}

#endif //WSSERVER_CONNECTIONSTORAGE_H
