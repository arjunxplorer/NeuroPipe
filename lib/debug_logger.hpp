#pragma once

#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <atomic>
#include <mutex>

/**
 * DebugLogger - Simple logging library for NeuroPipe
 * 
 * Publishes log messages to NeuroPipe broker for real-time debugging.
 * Thread-safe, non-blocking, automatic reconnection.
 * 
 * Usage:
 *   DebugLogger logger("my_service");
 *   logger.info("User logged in");
 *   logger.error("Database connection failed");
 *   logger.metric("response_time_ms", 123.45);
 */
class DebugLogger {
public:
    /**
     * Create logger for a service
     * @param service_name Name of your service (e.g., "order_service")
     * @param broker_host Broker hostname (default: "127.0.0.1")
     * @param broker_port Broker port (default: 9092)
     */
    DebugLogger(const std::string& service_name, 
                const std::string& broker_host = "127.0.0.1",
                int broker_port = 9092);
    
    ~DebugLogger();
    
    // Logging methods
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void debug(const std::string& message);
    
    // Metric logging
    void metric(const std::string& metric_name, double value);
    void metric(const std::string& metric_name, int value);
    void metric(const std::string& metric_name, const std::string& value);
    
    // Raw message publishing (for custom formats)
    void publish(const std::string& topic, const std::string& message);
    
    // Connection status
    bool is_connected() const { return connected_; }
    
    // Reconnect to broker
    bool reconnect();
    
private:
    void connect_to_broker();
    void send_message(const std::string& topic, const std::string& message);
    std::string get_timestamp();
    std::string format_log_message(const std::string& level, const std::string& message);
    std::string escape_message(const std::string& message);
    
    std::string service_name_;
    std::string broker_host_;
    int broker_port_;
    int socket_fd_;
    std::atomic<bool> connected_;
    std::mutex socket_mutex_;
};

/**
 * Helper function to get current timestamp
 */
inline std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

