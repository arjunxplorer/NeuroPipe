#include "asio_server.hpp"
#include <sstream>
#include <algorithm>

// ============================================================================
// Session Implementation
// ============================================================================

Session::Session(asio::ip::tcp::socket socket, BrokerServer& broker)
    : socket_(std::move(socket)), broker_(broker) {
    // Generate unique client ID from endpoint
    std::ostringstream oss;
    oss << socket_.remote_endpoint();
    client_id_ = oss.str();
}

Session::~Session() {
    log_info("Session destroyed: " + client_id_);
}

void Session::start() {
    log_info("New session started: " + client_id_);
    do_read();
}

void Session::deliver(const std::string& message) {
    bool write_in_progress = false;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_in_progress = !write_queue_.empty();
        write_queue_.push(message);
    }
    
    if (!write_in_progress) {
        do_write();
    }
}

void Session::do_read() {
    auto self(shared_from_this());
    asio::async_read_until(
        socket_,
        read_buffer_,
        '\n',
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::istream is(&read_buffer_);
                std::string message;
                std::getline(is, message);
                
                log_debug("Received from " + client_id_ + ": " + message);
                process_message(message);
                do_read(); // Continue reading
            } else {
                log_info("Session disconnected: " + client_id_ + " (" + ec.message() + ")");
                broker_.on_session_disconnect(self);
            }
        });
}

void Session::do_write() {
    auto self(shared_from_this());
    
    std::string message;
    {
        std::lock_guard<std::mutex> lock(write_mutex_);
        if (write_queue_.empty()) {
            return;
        }
        message = write_queue_.front();
    }
    
    asio::async_write(
        socket_,
        asio::buffer(message.data(), message.length()),
        [this, self](std::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                {
                    std::lock_guard<std::mutex> lock(write_mutex_);
                    write_queue_.pop();
                }
                do_write(); // Write next message
            } else {
                log_error("Write failed for " + client_id_ + ": " + ec.message());
                broker_.on_session_disconnect(self);
            }
        });
}

void Session::process_message(const std::string& message) {
    // Protocol format:
    // PUBLISH:topic:payload
    // SUBSCRIBE:topic
    // UNSUBSCRIBE:topic
    
    // Handle empty messages
    if (message.empty()) {
        deliver("ERROR:EMPTY_MESSAGE\n");
        return;
    }
    
    if (message.find("PUBLISH:") == 0) {
        // Bounds check: need at least "PUBLISH:t:p" (11 chars minimum)
        if (message.length() < 10) {
            deliver("ERROR:INVALID_FORMAT\n");
            return;
        }
        
        size_t first_colon = message.find(':', 8);
        if (first_colon != std::string::npos && first_colon > 8) {
            std::string topic = message.substr(8, first_colon - 8);
            std::string payload = message.substr(first_colon + 1);
            
            // Validate topic is not empty
            if (topic.empty()) {
                deliver("ERROR:EMPTY_TOPIC\n");
                return;
            }
            
            broker_.publish(topic, payload);
            deliver("OK:PUBLISHED\n");
        } else {
            deliver("ERROR:INVALID_FORMAT\n");
        }
    }
    else if (message.find("SUBSCRIBE:") == 0) {
        // Bounds check: need at least "SUBSCRIBE:t" (11 chars minimum)
        if (message.length() <= 10) {
            deliver("ERROR:INVALID_FORMAT\n");
            return;
        }
        
        std::string topic = message.substr(10);
        
        // Validate topic is not empty
        if (topic.empty()) {
            deliver("ERROR:EMPTY_TOPIC\n");
            return;
        }
        
        broker_.subscribe(topic, shared_from_this());
        deliver("OK:SUBSCRIBED:" + topic + "\n");
    }
    else if (message.find("UNSUBSCRIBE:") == 0) {
        // Bounds check: need at least "UNSUBSCRIBE:t" (13 chars minimum)
        if (message.length() <= 12) {
            deliver("ERROR:INVALID_FORMAT\n");
            return;
        }
        
        std::string topic = message.substr(12);
        
        // Validate topic is not empty
        if (topic.empty()) {
            deliver("ERROR:EMPTY_TOPIC\n");
            return;
        }
        
        broker_.unsubscribe(topic, shared_from_this());
        deliver("OK:UNSUBSCRIBED:" + topic + "\n");
    }
    else if (message.find("PING") == 0) {
        deliver("PONG\n");
    }
    else {
        deliver("ERROR:UNKNOWN_COMMAND\n");
    }
}

// ============================================================================
// TopicManager Implementation
// ============================================================================

void TopicManager::subscribe(const std::string& topic, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscriptions_[topic].insert(session);
    log_info("Session " + session->get_client_id() + " subscribed to topic: " + topic);
}

void TopicManager::unsubscribe(const std::string& topic, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(topic);
    if (it != subscriptions_.end()) {
        it->second.erase(session);
        if (it->second.empty()) {
            subscriptions_.erase(it);
        }
        log_info("Session " + session->get_client_id() + " unsubscribed from topic: " + topic);
    }
}

void TopicManager::unsubscribe_all(std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [topic, subscribers] : subscriptions_) {
        subscribers.erase(session);
    }
    log_info("Session " + session->get_client_id() + " unsubscribed from all topics");
}

void TopicManager::publish(const std::string& topic, const std::string& payload) {
    Message msg(topic, payload);
    
    std::vector<std::shared_ptr<Session>> subscribers;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Assign sequence number
        msg.sequence = sequence_counter_++;
        
        // Store message in queue
        topic_queues_[topic].push(msg);
        
        // Get subscribers
        auto it = subscriptions_.find(topic);
        if (it != subscriptions_.end()) {
            subscribers.assign(it->second.begin(), it->second.end());
        }
    }
    
    // Broadcast to subscribers (outside lock to avoid deadlock)
    if (!subscribers.empty()) {
        std::string notification = "MESSAGE:" + topic + ":" + payload + "\n";
        for (auto& subscriber : subscribers) {
            subscriber->deliver(notification);
        }
        log_info("Published to topic '" + topic + "' (" + std::to_string(subscribers.size()) + " subscribers)");
    } else {
        log_info("Published to topic '" + topic + "' (no subscribers)");
    }
}

std::vector<std::shared_ptr<Session>> TopicManager::get_subscribers(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(topic);
    if (it != subscriptions_.end()) {
        return std::vector<std::shared_ptr<Session>>(it->second.begin(), it->second.end());
    }
    return {};
}

void TopicManager::store_message(const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    topic_queues_[msg.topic].push(msg);
}

bool TopicManager::consume_message(const std::string& topic, Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = topic_queues_.find(topic);
    if (it != topic_queues_.end() && !it->second.empty()) {
        msg = it->second.front();
        it->second.pop();
        return true;
    }
    return false;
}

size_t TopicManager::get_topic_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.size();
}

size_t TopicManager::get_subscriber_count(const std::string& topic) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscriptions_.find(topic);
    return it != subscriptions_.end() ? it->second.size() : 0;
}

// ============================================================================
// BrokerServer Implementation
// ============================================================================

BrokerServer::BrokerServer(asio::io_context& io_context, uint16_t port)
    : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    log_info("BrokerServer initialized on port " + std::to_string(port));
}

BrokerServer::~BrokerServer() {
    stop();
}

void BrokerServer::start() {
    if (running_) {
        log_warn("BrokerServer already running");
        return;
    }
    
    running_ = true;
    log_info("BrokerServer started, accepting connections...");
    do_accept();
}

void BrokerServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Close acceptor
    if (acceptor_.is_open()) {
        acceptor_.close();
    }
    
    // Close all sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.clear();
    }
    
    log_info("BrokerServer stopped");
}

void BrokerServer::do_accept() {
    acceptor_.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket) {
            if (!ec) {
                auto session = std::make_shared<Session>(std::move(socket), *this);
                {
                    std::lock_guard<std::mutex> lock(sessions_mutex_);
                    sessions_.insert(session);
                }
                session->start();
            } else {
                log_error("Accept failed: " + ec.message());
            }
            
            if (running_) {
                do_accept();
            }
        });
}

void BrokerServer::publish(const std::string& topic, const std::string& payload) {
    topic_manager_.publish(topic, payload);
}

void BrokerServer::subscribe(const std::string& topic, std::shared_ptr<Session> session) {
    topic_manager_.subscribe(topic, session);
}

void BrokerServer::unsubscribe(const std::string& topic, std::shared_ptr<Session> session) {
    topic_manager_.unsubscribe(topic, session);
}

void BrokerServer::on_session_disconnect(std::shared_ptr<Session> session) {
    // Remove from all subscriptions
    topic_manager_.unsubscribe_all(session);
    
    // Remove from active sessions
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_.erase(session);
    }
    
    log_info("Session removed: " + session->get_client_id());
}

size_t BrokerServer::get_active_sessions() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

size_t BrokerServer::get_topic_count() const {
    return topic_manager_.get_topic_count();
}

