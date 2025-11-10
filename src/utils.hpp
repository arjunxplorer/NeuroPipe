#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// Thread-safe queue for message buffering
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    
public:
    // Push an item to the queue
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        cond_.notify_one();
    }
    
    // Try to pop an item without blocking
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    // Wait and pop an item (blocking)
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            cond_.wait(lock);
        }
        value = std::move(queue_.front());
        queue_.pop();
    }
    
    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    // Get queue size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    // Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }
};

// Helper function to get current timestamp string
inline std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Simple logger functions
inline void log_info(const std::string& msg) {
    std::cout << "[" << get_timestamp() << "] [INFO] " << msg << std::endl;
}

inline void log_error(const std::string& msg) {
    std::cerr << "[" << get_timestamp() << "] [ERROR] " << msg << std::endl;
}

inline void log_debug(const std::string& msg) {
    std::cout << "[" << get_timestamp() << "] [DEBUG] " << msg << std::endl;
}

inline void log_warn(const std::string& msg) {
    std::cout << "[" << get_timestamp() << "] [WARN] " << msg << std::endl;
}
