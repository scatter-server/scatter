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
#include <toolboxpp.h>
#include "../defs.h"
namespace wss {

struct ConnectionNotFound : std::exception {
  const char *what() const throw() override {
      return "Connection not found or already disconnected";
  }
};

class ConnectionStorage {
 private:
    mutable std::recursive_mutex connectionMutex;
    wss::UserMap<std::vector<WsConnectionPtr>> idMap;

 public:
    ConnectionStorage() = default;
    ConnectionStorage(const ConnectionStorage &other) = delete;
    ConnectionStorage(ConnectionStorage &&other) = delete;
    ~ConnectionStorage();

    /// \brief Check for existing connection using UserId
    /// \param id UserId
    /// \return true if connection with UserId in map
    bool exists(wss::UserId id) const;

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

    /// \brief Remove from connection by unique connection id : std::stoul("ip address without dots" + "remove port num").
    /// \param connection SimpleWeb::Connection shared_ptr
    void remove(wss::WsConnectionPtr &connection);

    /// \brief Return from map connections for entire user
    /// \param id UserId
    /// \return  Vector of connections
    std::vector<wss::WsConnectionPtr> &get(wss::UserId id);

    /// \brief Return whole map by const reference
    /// \return unordered_map< UserId, vector<WsConnectionPtr> >
    const wss::UserMap<std::vector<wss::WsConnectionPtr>> &get();

    /// \brief Callback function to handle connections for entire UserId
    /// \param id UserId
    /// \param handler callback function (void<WsConnectionPtr &>)
    void handle(UserId id, std::function<void(WsConnectionPtr &)> &&handler);

    std::vector<wss::WsConnectionPtr> &operator[](wss::UserId id) noexcept {
        return get(id);
    }
};

}

#endif //WSSERVER_CONNECTIONSTORAGE_H
