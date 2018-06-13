/**
 * wsserver
 * ConnectionStorage.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "ConnectionStorage.h"
#include <fmt/format.h>

wss::ConnectionStorage::~ConnectionStorage() {
    for (auto &kv: m_idMap) {
        for (const auto &c: kv.second) {
            try {
                c.second->sendClose(1000, "Server Gone Away");
            } catch (...) {

            }
        }
    }
    m_idMap.clear();
}
bool wss::ConnectionStorage::exists(wss::user_id_t id) const {
    // мы проверяем только наличие мапы, но есть ли такое содениение с уникальным айдишником - нет, тут баг
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    return m_idMap.find(id) != m_idMap.end();
}
bool wss::ConnectionStorage::verify(uint8_t pingFlag) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    for (const std::pair<user_id_t, std::unordered_map<conn_id_t, WsConnectionPtr>> &t: m_idMap) {
        for (const auto &conn: t.second) {
            if (!conn.second) {

                // haha deadlock? i hope no
                remove(t.first, conn.first);
                continue;
            }

            auto pingStream = std::make_shared<WsMessageStream>();
            *pingStream << ".";
            conn.second
                ->send(std::move(pingStream), [conn, this](const SimpleWeb::ErrorCode &err, std::size_t) {
                  if (err) {
                      // does not matter, what happens, anyway, this mean connection is bad, broken pipe, eof or something else
                      remove(conn.second);
                  } else {
                      // lazy ping pong
                      markPongWait(conn.second);
                  }
                }, pingFlag);
        }
    }
    return true;
}
std::size_t wss::ConnectionStorage::size() const {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    return m_idMap.size();
}
std::size_t wss::ConnectionStorage::size(wss::user_id_t id) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);

    if (m_idMap.find(id) == m_idMap.end()) {
        return 0;
    }

    return m_idMap[id].size();
}
void wss::ConnectionStorage::add(wss::user_id_t id, const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    connection->setId(id);
    m_idMap[id][connection->getUniqueId()] = connection;
    L_DEBUG_F("Connection::Add", "Adding connection for %lu. Now size: %lu", connection->getId(), m_idMap[id].size());
}
void wss::ConnectionStorage::remove(wss::user_id_t id) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    m_idMap.erase(id);
}
void wss::ConnectionStorage::remove(wss::user_id_t id, wss::conn_id_t connectionId) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    const auto &it = m_idMap.find(id);
    if (it != m_idMap.end()) {
        if (it->second.find(connectionId) != it->second.end()) {
            it->second.erase(connectionId);
        }
    }
}
void wss::ConnectionStorage::remove(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    const user_id_t id = connection->getId();
    const conn_id_t connId = connection->getUniqueId();

    const auto &userMapIt = m_idMap.find(id);
    if (userMapIt != m_idMap.end()) {
        if (userMapIt->second.find(connId) != userMapIt->second.end()) {
            userMapIt->second.erase(connId);
        }
    }

    L_DEBUG_F("Connection::Remove", "User %lu (%lu). Left connections: %lu",
              connection->getId(),
              connection->getUniqueId(),
              m_idMap[id].size());
}
wss::ConnectionMap<wss::WsConnectionPtr> &wss::ConnectionStorage::get(wss::user_id_t id) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    if (!exists(id)) {
        throw ConnectionNotFound();
    }
    return m_idMap[id];
}
const wss::UserMap<wss::ConnectionMap<wss::WsConnectionPtr>> &wss::ConnectionStorage::get() const {
    return m_idMap;
}
void wss::ConnectionStorage::handle(wss::user_id_t id, std::function<void(wss::WsConnectionPtr &)> &&handler) {
    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);
    for (auto &conn: get(id)) {
        handler(conn.second);
    }
}
void wss::ConnectionStorage::markPongWait(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::mutex> locker(m_pongMutex);
    m_waitForPong[connection->getUniqueId()] = {connection->getId(), false};
}
void wss::ConnectionStorage::markPongReceived(const wss::WsConnectionPtr &connection) {
    std::lock_guard<std::mutex> locker(m_pongMutex);
    m_waitForPong[connection->getUniqueId()].second = true;
}
std::size_t wss::ConnectionStorage::disconnectWithoutPong(int statusCode, const std::string &reason) {
    std::lock_guard<std::mutex> locker(m_pongMutex);
    std::size_t disconnected = 0;
    for (auto it = m_waitForPong.begin(); it != m_waitForPong.end();) {
        if (!it->second.second) {
            {
                std::lock_guard<std::recursive_mutex> sublock(m_connectionMutex);
                WsConnectionPtr &conn = m_idMap[it->second.first][it->first];
                if (conn) {
                    conn->sendClose(statusCode, reason);
                } else {
                    // by some reason, connection already nullptr
                    m_idMap[it->second.first].erase(it->first);
                }
            }

            disconnected++;
        }

        m_waitForPong.erase(it++);
    }

    return disconnected;
}


void wss::ConnectionStorage::forEach(
    wss::user_id_t recipient,
    const wss::ConnectionStorage::ItemHandler &handler,
    const wss::ConnectionStorage::ItemNotFoundHandler& notFoundHandler) {

    if(handler == nullptr) {
        return;
    }

    std::lock_guard<std::recursive_mutex> locker(m_connectionMutex);

    try {
        const auto &connections = get(recipient);
        size_t i = 0;

        for (const auto &connection: connections) {
            if (!connection.second) {
                if(notFoundHandler != nullptr) {
                    notFoundHandler(recipient, connection.first);
                }
                // removing invalid recipient connection
                remove(recipient, connection.first);
                continue;
            }

            handler(i, connection.second, connection.first, recipient);
        }

    } catch (const std::exception &e) {
        Logger::get().warning(__FILE__, __LINE__, "Connection::Handle", fmt::format("Unknown error: {0}", e.what()));
    } catch (...) {
        Logger::get().warning(__FILE__, __LINE__, "Connection::Handle", "Unknown error");
    }
}
