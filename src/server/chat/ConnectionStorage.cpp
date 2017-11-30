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
        for (auto &c: kv.second) {
            try {
                c->send_close(1000, "Server Gone Away");
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
    idMap[id].push_back(connection);
    L_DEBUG_F("SetConnection", "Adding connection for %lu. Now size: %lu", connection->getId(), idMap[id].size());
}
void wss::ConnectionStorage::remove(wss::UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    idMap.erase(id);
}
void wss::ConnectionStorage::remove(wss::WsConnectionPtr &connection) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    const UserId id = connection->getId();
    std::vector<WsConnectionPtr> &connections = idMap[connection->getId()];

    for (auto it = connections.begin(); it != connections.end();) {
        if ((*it)->getUniqueId() == connection->getUniqueId()) {
            it = connections.erase(it);
        } else {
            ++it;
        }
    }
    L_DEBUG_F("SetConnection", "Removing connection for %lu. Now size: %lu", connection->getId(), idMap[id].size());
}
std::vector<wss::WsConnectionPtr> &wss::ConnectionStorage::get(wss::UserId id) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    if (!exists(id)) {
        throw ConnectionNotFound();
    }
    return idMap[id];
}
const wss::UserMap<std::__1::vector<std::__1::shared_ptr<SimpleWeb::SocketServerBase<boost::asio::basic_stream_socket<
    boost::asio::ip::tcp>>::Connection>>> &wss::ConnectionStorage::get() {
    return idMap;
}
void wss::ConnectionStorage::handle(wss::UserId id, std::function<void(WsConnectionPtr &)> &&handler) {
    std::lock_guard<std::recursive_mutex> locker(connectionMutex);
    for (auto &conn: get(id)) {
        handler(conn);
    }
}
