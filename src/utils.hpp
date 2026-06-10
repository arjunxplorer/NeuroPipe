#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Log level filtering
enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };
enum class LogFormat { TEXT = 0, JSON = 1 };

inline LogLevel& get_log_level() {
    static LogLevel level = LogLevel::INFO;
    return level;
}

inline LogFormat& get_log_format() {
    static LogFormat format = LogFormat::TEXT;
    return format;
}

inline void set_log_level(const std::string& level_str) {
    std::string upper = level_str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "DEBUG") get_log_level() = LogLevel::DEBUG;
    else if (upper == "INFO") get_log_level() = LogLevel::INFO;
    else if (upper == "WARN") get_log_level() = LogLevel::WARN;
    else if (upper == "ERROR") get_log_level() = LogLevel::ERROR;
}

inline void set_log_format(const std::string& format_str) {
    std::string lower = format_str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "json") get_log_format() = LogFormat::JSON;
    else get_log_format() = LogFormat::TEXT;
}

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

// Escape a string for JSON (handles quotes, backslashes, control chars)
inline std::string json_escape(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 10);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // Control characters → \u00XX
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                } else {
                    result.push_back(c);
                }
        }
    }
    return result;
}

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

// Format a log line as JSON
inline std::string format_log_json(const std::string& level, const std::string& msg) {
    return "{\"timestamp\":\"" + get_timestamp() + "\","
           "\"level\":\"" + level + "\","
           "\"message\":\"" + json_escape(msg) + "\"}";
}

// Logger functions (filtered by log level, supports text and JSON formats)
inline void log_info(const std::string& msg) {
    if (get_log_level() > LogLevel::INFO) return;
    if (get_log_format() == LogFormat::JSON)
        std::cout << format_log_json("INFO", msg) << std::endl;
    else
        std::cout << "[" << get_timestamp() << "] [INFO] " << msg << std::endl;
}

inline void log_error(const std::string& msg) {
    if (get_log_level() > LogLevel::ERROR) return;
    if (get_log_format() == LogFormat::JSON)
        std::cerr << format_log_json("ERROR", msg) << std::endl;
    else
        std::cerr << "[" << get_timestamp() << "] [ERROR] " << msg << std::endl;
}

inline void log_debug(const std::string& msg) {
    if (get_log_level() > LogLevel::DEBUG) return;
    if (get_log_format() == LogFormat::JSON)
        std::cout << format_log_json("DEBUG", msg) << std::endl;
    else
        std::cout << "[" << get_timestamp() << "] [DEBUG] " << msg << std::endl;
}

inline void log_warn(const std::string& msg) {
    if (get_log_level() > LogLevel::WARN) return;
    if (get_log_format() == LogFormat::JSON)
        std::cout << format_log_json("WARN", msg) << std::endl;
    else
        std::cout << "[" << get_timestamp() << "] [WARN] " << msg << std::endl;
}
