/**
 * wsserver
 * ConnectionStorage.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ConnectionStorage.h"

wss::ConnectionStorage::~ConnectionStorage() {
    for (auto &kv: idMap) {
        for (const auto &c: kv.second) {
            try {
                c.second->send_close(1000, "Server Gone Away");
            } catch (...) {

            }
        }
    }
    idMap.clear();
}
bool wss::ConnectionStorage::exists(wss::UserId id) const {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    return idMap.find(id) != idMap.end();
}
bool wss::ConnectionStorage::verify(wss::UserId userId, wss::ConnectionId connectionId) {

    const auto &it = idMap.find(userId);
    if (it == idMap.end()) {
        return false;
    }

    if (it->second.find(connectionId) != it->second.end()) {
        const WsConnectionPtr &ptr = it->second[connectionId];
        if (!ptr) {
            it->second.erase(connectionId);
            return false;
        }

        return true;
    } else {
        return false;
    }
}
std::size_t wss::ConnectionStorage::size() const {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    return idMap.size();
}
std::size_t wss::ConnectionStorage::size(wss::UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    return idMap[id].size();
}
void wss::ConnectionStorage::add(wss::UserId id, const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    connection->setId(id);
    idMap[id][connection->getUniqueId()] = connection;
    L_DEBUG_F("SetConnection", "Adding connection for %lu. Now size: %lu", connection->getId(), idMap[id].size());
}
void wss::ConnectionStorage::remove(wss::UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    idMap.erase(id);
}
void wss::ConnectionStorage::remove(wss::UserId id, wss::ConnectionId connectionId) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    const auto &it = idMap.find(id);
    if (it != idMap.end()) {
        if (it->second.find(connectionId) != it->second.end()) {
            it->second.erase(connectionId);
        }
    }
}
void wss::ConnectionStorage::remove(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    const UserId id = connection->getId();
    const ConnectionId connId = connection->getUniqueId();

    const auto &userMapIt = idMap.find(id);
    if (userMapIt != idMap.end()) {
        if (userMapIt->second.find(connId) != userMapIt->second.end()) {
            userMapIt->second.erase(connId);
        }
    }

    L_DEBUG_F("RemoveConnection", "User %lu (%lu). Left connections: %lu",
              connection->getId(),
              connection->getUniqueId(),
              idMap[id].size());
}
wss::ConnectionMap<wss::WsConnectionPtr> &wss::ConnectionStorage::get(wss::UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    if (!exists(id)) {
        throw ConnectionNotFound();
    }
    return idMap[id];
}
const wss::UserMap<wss::ConnectionMap<wss::WsConnectionPtr>> &wss::ConnectionStorage::get() {
    return idMap;
}
void wss::ConnectionStorage::handle(wss::UserId id, std::function<void(wss::WsConnectionPtr &)> &&handler) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    for (auto &conn: get(id)) {
        handler(conn.second);
    }
}
void wss::ConnectionStorage::markPongWait(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::mutex> locker(pongMutex);
    waitForPong[connection->getUniqueId()] = {connection->getId(), false};
}
void wss::ConnectionStorage::markPongReceived(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::mutex> locker(pongMutex);
    waitForPong[connection->getUniqueId()].second = true;
}
std::size_t wss::ConnectionStorage::disconnectWithoutPong() {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    std::size_t disconnected = 0;
    for (auto it = waitForPong.begin(); it != waitForPong.end();) {
        if (!it->second.second) {
            WsConnectionPtr &conn = idMap[it->second.first][it->first];
            if (conn) {
                conn->send_close(1000, "Dangling connection");
            } else {
                // by some reason, connection already nullptr
                idMap[it->second.first].erase(it->first);
            }
            disconnected++;
        }

        waitForPong.erase(it);

        ++it;
    }

    return disconnected;
}
