#include "debug_logger.hpp"
#include <iostream>

DebugLogger::DebugLogger(const std::string& service_name,
                         const std::string& broker_host,
                         int broker_port)
    : service_name_(service_name),
      broker_host_(broker_host),
      broker_port_(broker_port),
      socket_fd_(-1),
      connected_(false) {
    
    connect_to_broker();
}

DebugLogger::~DebugLogger() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

void DebugLogger::connect_to_broker() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    
    // Close existing connection if any
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    
    // Create socket
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        std::cerr << "[DebugLogger] Failed to create socket" << std::endl;
        connected_ = false;
        return;
    }
    
    // Set socket to non-blocking for connection timeout
    struct timeval timeout;
    timeout.tv_sec = 2;  // 2 second timeout
    timeout.tv_usec = 0;
    setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Setup server address
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(broker_port_);
    
    if (inet_pton(AF_INET, broker_host_.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[DebugLogger] Invalid broker address: " << broker_host_ << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        return;
    }
    
    // Connect to broker
    if (connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[DebugLogger] Failed to connect to broker at " 
                  << broker_host_ << ":" << broker_port_ << std::endl;
        close(socket_fd_);
        socket_fd_ = -1;
        connected_ = false;
        return;
    }
    
    connected_ = true;
}

bool DebugLogger::reconnect() {
    connect_to_broker();
    return connected_;
}

std::string DebugLogger::get_timestamp() {
    return current_timestamp();
}

std::string DebugLogger::escape_message(const std::string& message) {
    // Escape special characters that could break the protocol
    // Protocol format: PUBLISH:topic:payload\n
    // So we need to escape: \n (newline) and : (colon)
    std::string result;
    result.reserve(message.size());
    
    for (char c : message) {
        if (c == '\n') {
            result += "\\n";  // Escape newline
        } else if (c == '\r') {
            result += "\\r";  // Escape carriage return
        } else if (c == ':') {
            result += "\\:";  // Escape colon (protocol delimiter)
        } else if (c == '\\') {
            result += "\\\\"; // Escape backslash itself
        } else {
            result.push_back(c);
        }
    }
    return result;
}

std::string DebugLogger::format_log_message(const std::string& level, 
                                            const std::string& message) {
    std::stringstream ss;
    ss << "[" << get_timestamp() << "] "
       << "[" << level << "] "
       << service_name_ << ": " << escape_message(message);
    return ss.str();
}

void DebugLogger::send_message(const std::string& topic, const std::string& message) {
    if (!connected_) {
        // Try to reconnect
        connect_to_broker();
        if (!connected_) {
            return;  // Still not connected, skip
        }
    }
    
    // Format as NeuroPipe protocol: PUBLISH:topic:payload\n
    std::string protocol_msg = "PUBLISH:" + topic + ":" + message + "\n";
    
    std::lock_guard<std::mutex> lock(socket_mutex_);
    
    ssize_t sent = send(socket_fd_, protocol_msg.c_str(), protocol_msg.length(), MSG_NOSIGNAL);
    if (sent < 0) {
        // Connection lost, mark as disconnected
        connected_ = false;
        std::cerr << "[DebugLogger] Send failed, connection lost" << std::endl;
    }
}

void DebugLogger::info(const std::string& message) {
    std::string formatted = format_log_message("INFO", message);
    send_message("debug", formatted);
}

void DebugLogger::warn(const std::string& message) {
    std::string formatted = format_log_message("WARN", message);
    send_message("debug", formatted);
    send_message("warnings", formatted);  // Also send to warnings topic
}

void DebugLogger::error(const std::string& message) {
    std::string formatted = format_log_message("ERROR", message);
    send_message("debug", formatted);
    send_message("errors", formatted);  // Also send to errors topic
}

void DebugLogger::debug(const std::string& message) {
    std::string formatted = format_log_message("DEBUG", message);
    send_message("debug", formatted);
}

void DebugLogger::metric(const std::string& metric_name, double value) {
    std::stringstream ss;
    ss << metric_name << "=" << std::fixed << std::setprecision(2) << value;
    send_message("metrics", ss.str());
}

void DebugLogger::metric(const std::string& metric_name, int value) {
    std::stringstream ss;
    ss << metric_name << "=" << value;
    send_message("metrics", ss.str());
}

void DebugLogger::metric(const std::string& metric_name, const std::string& value) {
    std::string msg = metric_name + "=" + value;
    send_message("metrics", msg);
}

void DebugLogger::publish(const std::string& topic, const std::string& message) {
    send_message(topic, message);
}

