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
#include "chat/Message.h"

namespace wss {

struct EmptyQueueError : std::exception {

  const char *what() const throw() override {
      return "Queue is empty";
  }
};

template<typename T>
class QueueThreadSafe {
 private:
    std::queue<T> data;
    mutable std::mutex m;
 public:
    QueueThreadSafe() = default;
    QueueThreadSafe(const QueueThreadSafe &other) {
        std::lock_guard<std::mutex> locker(m);
        data = other.data;
    }

    QueueThreadSafe &operator=(const QueueThreadSafe &other) = delete;

    void push(T value) {
        std::lock_guard<std::mutex> locker(m);
        data.push(value);
    }

    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> locker(m);
        if (data.empty()) {
            throw EmptyQueueError();
        }

        const std::shared_ptr<T> res = std::make_shared<T>(data.front());
        data.pop();

        return res;
    }

    void pop(T &value) {
        std::lock_guard<std::mutex> locker(m);
        if (data.empty()) throw EmptyQueueError();

        value = data.front();
        data.pop();
    }

    bool empty() {
        std::lock_guard<std::mutex> locker(m);
        return data.empty();
    }
};

}

#endif //WSSERVER_BLOCKINGQUEUE_H
