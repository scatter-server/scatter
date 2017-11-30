/**
 * wsserver
 * Statistics.cpp
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#include "Statistics.h"
wss::Statistics::Statistics(wss::UserId id) :
    id(id),
    lastConnectionTime(time(nullptr)),
    lastDisconnectionTime(0),
    connectedTimes(0),
    disconnectedTimes(0),
    bytesTransferred(0),
    sentMessages(0),
    receivedMessages(0),
    lastMessageTime(0) { }
wss::Statistics::Statistics() :
    id(0),
    lastConnectionTime(time(nullptr)),
    lastDisconnectionTime(0),
    connectedTimes(0),
    disconnectedTimes(0),
    bytesTransferred(0),
    sentMessages(0),
    receivedMessages(0),
    lastMessageTime(0) { }

wss::Statistics &wss::Statistics::setId(wss::UserId _id) {
    id = _id;
    return *this;
}
wss::Statistics &wss::Statistics::addConnection() {
    lastConnectionTime = time(nullptr);
    connectedTimes++;
    return *this;
}
wss::Statistics &wss::Statistics::addDisconnection() {
    lastDisconnectionTime = time(nullptr);
    disconnectedTimes++;
    return *this;
}
wss::Statistics &wss::Statistics::addBytesTransferred(std::size_t bytes) {
    bytesTransferred += bytes;
    return *this;
}
wss::Statistics &wss::Statistics::addSentMessages(std::size_t sent) {
    sentMessages += sent;
    lastMessageTime = time(nullptr);
    return *this;
}
wss::Statistics &wss::Statistics::addSendMessage() {
    return addSentMessages(1);
}
wss::Statistics &wss::Statistics::addReceivedMessages(std::size_t received) {
    receivedMessages += received;
    return *this;
}
wss::Statistics &wss::Statistics::addReceivedMessage() {
    return addReceivedMessages(1);
}
std::size_t wss::Statistics::getConnectedTimes() {
    return connectedTimes;
}
std::size_t wss::Statistics::getDisconnectedTimes() {
    return disconnectedTimes;
}
time_t wss::Statistics::getOnlineTime() const {
    if (!isOnline()) {
        return 0;
    }
    return time(nullptr) - lastConnectionTime;
}
time_t wss::Statistics::getOfflineTime() const {
    if (isOnline()) {
        return 0;
    }

    return time(nullptr) - lastDisconnectionTime;
}
time_t wss::Statistics::getLastMessageSecondsAgo() const {
    return time(nullptr) - lastMessageTime;
}
bool wss::Statistics::isOnline() const {
    return connectedTimes > disconnectedTimes;
}
time_t wss::Statistics::getConnectionTime() const {
    return lastConnectionTime;
}
std::size_t wss::Statistics::getBytesTransferred() const {
    return bytesTransferred;
}
std::size_t wss::Statistics::getSentMessages() const {
    return sentMessages;
}
std::size_t wss::Statistics::getReceivedMessages() const {
    return receivedMessages;
}
