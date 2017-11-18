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

template<typename T>
class BlockingQueue {
 public:
    BlockingQueue() :
        queue(),
        mutex(),
        conditionVariable() {

    }

    ~BlockingQueue() {
        conditionVariable.notify_all();
        mutex.unlock();
    }

    BlockingQueue(const wss::BlockingQueue<T> &another) {
        *this = another;
    }

    BlockingQueue(wss::BlockingQueue<T> &another) {
        *this = another;
    }

    BlockingQueue(wss::BlockingQueue<T> &&another) noexcept :
        mutex(), conditionVariable() {
        while (!another.queue.empty()) {
            queue.push(another.queue.front());
            another.queue.pop();
        }
    }

    BlockingQueue<T> &operator=(const wss::BlockingQueue<T> &another) {
        queue = another.queue;
        return *this;
    }
    void push(T t) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(t);
        conditionVariable.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex);
        while (queue.empty()) {
            conditionVariable.wait(lock);
            //wait for values, cause we can't give another result
        }

        T result = queue.front();
        queue.pop();

        return result;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(mutex);
        bool result = queue.empty();
        conditionVariable.notify_one();
        return result;
    }

    std::size_t size() const {
        return queue.size();
    }
 private:
    std::queue<T> queue;
    mutable std::mutex mutex;
    std::condition_variable conditionVariable;
};

};

#endif //WSSERVER_BLOCKINGQUEUE_H
