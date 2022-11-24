// Copyright [2021] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_THREADSAFE_QUEUE_H_
#define SRC_PRIMIHUB_UTIL_THREADSAFE_QUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace primihub {
template<typename T>
class ThreadSafeQueue {
 public:
  void push(const T& item) {
    emplace(item);
  }

  void push(T&& item) {
    emplace(std::move(item));
  }

  template<typename... Args>
  void emplace(Args&&... args) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue.emplace(std::forward<Args>(args)...);
    lock.unlock();
    m_cv.notify_one();
  }

  bool empty() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
  }

  bool try_pop(T& popped_value) {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_queue.empty()) {
      return false;
    }

    popped_value = std::move(m_queue.front());
    m_queue.pop();
    return true;
  }

  void wait_and_pop(T& popped_value) {
    std::unique_lock<std::mutex> lock(m_mutex);
    // while (m_queue.empty()) {
    //   m_cv.wait(lock);
    // }
    m_cv.wait(lock, [&]() {return stop_.load() || !m_queue.empty();});
    if (stop_.load()) {
      return;
    }
    popped_value = std::move(m_queue.front());
    m_queue.pop();
  }

  // Provides only basic exception safety guarantee when RVO is not applied.
  T pop() {
    std::unique_lock<std::mutex> lock(m_mutex);
    // while (m_queue.empty()) {
    //   m_cv.wait(lock);
    // }
    m_cv.wait(lock, [&]() {return stop_.load() || !m_queue.empty();});
    if (stop_.load()) {
      return T();
    }
    auto item = std::move(m_queue.front());
    m_queue.pop();
    return item;
  }

  void shutdown() {
    stop_.store(true);
    m_cv.notify_one();
  }

 private:
  std::queue<T> m_queue;
  mutable std::mutex m_mutex;
  std::condition_variable m_cv;
  std::atomic<bool> stop_{false};
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_UTIL_THREADSAFE_QUEUE_H_
