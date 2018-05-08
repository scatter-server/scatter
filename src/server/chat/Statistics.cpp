/**
 * wsserver
 * Statistics.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "Statistics.h"
wss::Statistics::Statistics(wss::user_id_t id) :
    m_id(id),
    m_lastConnectionTime(time(nullptr)),
    m_lastDisconnectionTime(0),
    m_connectedTimes(0),
    m_disconnectedTimes(0),
    m_bytesTransferred(0),
    m_sentMessages(0),
    m_receivedMessages(0),
    m_lastMessageTime(0) { }
wss::Statistics::Statistics() :
    m_id(0),
    m_lastConnectionTime(time(nullptr)),
    m_lastDisconnectionTime(0),
    m_connectedTimes(0),
    m_disconnectedTimes(0),
    m_bytesTransferred(0),
    m_sentMessages(0),
    m_receivedMessages(0),
    m_lastMessageTime(0) { }

wss::Statistics &wss::Statistics::setId(wss::user_id_t _id) {
    m_id = _id;
    return *this;
}
wss::Statistics &wss::Statistics::addConnection() {
    m_lastConnectionTime = time(nullptr);
    m_connectedTimes++;
    m_lastMessageTime = 0;
    return *this;
}
wss::Statistics &wss::Statistics::addDisconnection() {
    m_lastDisconnectionTime = time(nullptr);
    m_disconnectedTimes++;
    return *this;
}
wss::Statistics &wss::Statistics::addBytesTransferred(std::size_t bytes) {
    m_bytesTransferred += bytes;
    return *this;
}
wss::Statistics &wss::Statistics::addSentMessages(std::size_t sent) {
    m_sentMessages += sent;
    m_lastMessageTime = time(nullptr);
    return *this;
}
wss::Statistics &wss::Statistics::addSendMessage() {
    return addSentMessages(1);
}
wss::Statistics &wss::Statistics::addReceivedMessages(std::size_t received) {
    m_receivedMessages += received;
    return *this;
}
wss::Statistics &wss::Statistics::addReceivedMessage() {
    return addReceivedMessages(1);
}
std::size_t wss::Statistics::getConnectedTimes() {
    return m_connectedTimes;
}
std::size_t wss::Statistics::getDisconnectedTimes() {
    return m_disconnectedTimes;
}
time_t wss::Statistics::getOnlineTime() const {
    if (!isOnline()) {
        return 0;
    }
    return time(nullptr) - m_lastConnectionTime;
}

time_t wss::Statistics::getInactiveTime() const {
    if (getLastMessageTime() == 0) {
        return std::time(nullptr) - getConnectionTime();
    }

    return std::time(nullptr) - getLastMessageTime();

}
time_t wss::Statistics::getOfflineTime() const {
    if (isOnline()) {
        return 0;
    }

    return time(nullptr) - m_lastDisconnectionTime;
}
time_t wss::Statistics::getLastMessageTime() const {
    return m_lastMessageTime;
}
bool wss::Statistics::isOnline() const {
    return m_connectedTimes > m_disconnectedTimes;
}
time_t wss::Statistics::getConnectionTime() const {
    return m_lastConnectionTime;
}
std::size_t wss::Statistics::getBytesTransferred() const {
    return m_bytesTransferred;
}
std::size_t wss::Statistics::getSentMessages() const {
    return m_sentMessages;
}
std::size_t wss::Statistics::getReceivedMessages() const {
    return m_receivedMessages;
}
