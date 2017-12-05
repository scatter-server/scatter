/**
 * wsserver
 * BlockingQueue.h
 *
 * @author Eduard Maximovich <edward.vstock@gmail.com>
 * @link https://github.com/edwardstock
 */

#ifndef WSSERVER_BLOCKINGQUEUE_H
#define WSSERVER_BLOCKINGQUEUE_H

#include <queue>
#include <mutex>

namespace wss {

struct EmptyQueueError : std::exception {

  const char *what() const throw() override {
      return "Queue is empty";
  }
};

/// \brief Thread safe queue uses simple mutexes and doesn't have seprate pop() and front() methods
/// \tparam T Type of value
/// \tparam mutex_t Type of mutex. Default: std::mutex. Must have support with std::lock_guard<mutex_t>
template<typename T, class mutex_t = std::mutex>
class QueueThreadSafe {
 private:
    std::queue<T> data;
    mutable mutex_t m;
 public:
    QueueThreadSafe() = default;

    /// \brief Copy and lock internal mutex while copying
    /// \param other Another instance of queue
    QueueThreadSafe(const QueueThreadSafe &other) {
        std::lock_guard<mutex_t> locker(m);
        data = other.data;
    }

    QueueThreadSafe &operator=(const QueueThreadSafe &other) = delete;

    /// \brief Push value with lock_guard
    /// \param value
    void push(T value) {
        std::lock_guard<mutex_t> locker(m);
        data.push(value);
    }

    /// \brief Pop and return shared_ptr to value
    /// \return shared_ptr<T>
    std::shared_ptr<T> pop() {
        std::lock_guard<mutex_t> locker(m);
        if (data.empty()) {
            throw EmptyQueueError();
        }

        const std::shared_ptr<T> res = std::make_shared<T>(data.front());
        data.pop();

        return res;
    }

    /// \brief Pop and set value to passed reference
    /// \param value Result value reference
    void pop(T &value) {
        std::lock_guard<mutex_t> locker(m);
        if (data.empty()) throw EmptyQueueError();

        value = data.front();
        data.pop();
    }

    /// \brief Check for emptiness. Locks lock_guard mutex.
    /// \return true whether not empty, otherwise - false
    bool empty() {
        std::lock_guard<mutex_t> locker(m);
        return data.empty();
    }
};

}

#endif //WSSERVER_BLOCKINGQUEUE_H
