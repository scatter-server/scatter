/**
 * scatter
 * Statistics.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef SCATTER_STATISTICS_H
#define SCATTER_STATISTICS_H

#include <atomic>
#include <ctime>
#include "../ServerCore.h"

namespace wss {

/// \brief User statistics storage
class Statistics {
 private:
    std::atomic<user_id_t> m_id;
    std::atomic<time_t> m_lastConnectionTime;
    std::atomic<time_t> m_lastDisconnectionTime;
    std::atomic_size_t m_connectedTimes;
    std::atomic_size_t m_disconnectedTimes;
    std::atomic_size_t m_bytesTransferred;
    std::atomic_size_t m_sentMessages;
    std::atomic_size_t m_receivedMessages;
    std::atomic<time_t> m_lastMessageTime;

 public:
    explicit Statistics(user_id_t id);
    Statistics();

    /// \brief Set to statistic user id
    /// \param _id UserId
    /// \return self
    Statistics &setId(user_id_t _id);

    /// \brief Add 1 to connections count
    /// \return self
    Statistics &addConnection();

    /// \brief Add 1 to disconnections count
    /// \return self
    Statistics &addDisconnection();

    /// \brief Add bytes transferred to or by user
    /// \param bytes
    /// \return self
    Statistics &addBytesTransferred(std::size_t bytes);

    /// \brief add n to sent messages count
    /// \param sent
    /// \return self
    Statistics &addSentMessages(std::size_t sent);

    /// \brief add 1 to sent messages count
    /// \return self
    Statistics &addSendMessage();

    /// \brief add n to received messages count
    /// \param received
    /// \return self
    Statistics &addReceivedMessages(std::size_t received);

    /// \brief add 1 to received messages count
    /// \return self
    Statistics &addReceivedMessage();

    /// \brief Summary connections count
    /// \return total count
    std::size_t getConnectedTimes();

    /// \brief Summary disconnections count
    /// \return total count
    std::size_t getDisconnectedTimes();

    /// \brief Statistics of last online time. Counts from the last connection.
    /// \return seconds ago
    time_t getOnlineTime() const;

    /// \brief Time of inactivity. Calculated from last message time if it was, or from last connection time
    /// \return Seconds ago
    time_t getInactiveTime() const;

    /// \brief Statistics of last offline time. Counts from the last disconnection.
    /// \return seconds ago
    time_t getOfflineTime() const;

    /// \brief Statistics of last message time.
    /// \return Seconds between now and last message
    time_t getLastMessageTime() const;

    /// \brief Check user is online
    /// \return true if connections > disconnections
    bool isOnline() const;

    /// \brief Last connection time
    /// \return timestamp
    time_t getConnectionTime() const;

    /// \brief Summary bytes transferred by or to user
    /// \return bytes
    std::size_t getBytesTransferred() const;

    /// \brief Count of sent messages
    /// \return total count
    std::size_t getSentMessages() const;

    /// \brief Count of received messages
    /// \return total count
    std::size_t getReceivedMessages() const;
};
}

#endif //SCATTER_STATISTICS_H
