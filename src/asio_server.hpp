#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <functional>
#include "../include/message.hpp"
#include "utils.hpp"

// Forward declarations
class Session;
class BrokerServer;

// Connection session for each client
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::ip::tcp::socket socket, BrokerServer& broker);
    ~Session();
    
    void start();
    void deliver(const std::string& message);
    std::string get_client_id() const { return client_id_; }
    
private:
    void do_read();
    void do_write();
    void process_message(const std::string& message);
    
    asio::ip::tcp::socket socket_;
    BrokerServer& broker_;
    std::string client_id_;
    
    asio::streambuf read_buffer_;
    std::queue<std::string> write_queue_;
    std::mutex write_mutex_;
};

// Topic subscription manager
class TopicManager {
public:
    // Subscribe a session to a topic
    void subscribe(const std::string& topic, std::shared_ptr<Session> session);
    
    // Unsubscribe a session from a topic
    void unsubscribe(const std::string& topic, std::shared_ptr<Session> session);
    
    // Unsubscribe session from all topics
    void unsubscribe_all(std::shared_ptr<Session> session);
    
    // Publish message to a topic
    void publish(const std::string& topic, const std::string& payload);
    
    // Get all subscribers for a topic
    std::vector<std::shared_ptr<Session>> get_subscribers(const std::string& topic);
    
    // Store message in topic queue
    void store_message(const Message& msg);
    
    // Consume message from topic
    bool consume_message(const std::string& topic, Message& msg);
    
    // Get topic statistics
    size_t get_topic_count() const;
    size_t get_subscriber_count(const std::string& topic) const;
    
private:
    // Map: topic -> set of subscribed sessions
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<Session>>> subscriptions_;
    
    // Map: topic -> message queue
    std::unordered_map<std::string, std::queue<Message>> topic_queues_;
    
    mutable std::mutex mutex_;
    uint64_t sequence_counter_ = 0;
};

// Main broker server with Asio
class BrokerServer {
public:
    BrokerServer(asio::io_context& io_context, uint16_t port);
    ~BrokerServer();
    
    // Start accepting connections
    void start();
    
    // Stop the broker
    void stop();
    
    // Publish message to topic
    void publish(const std::string& topic, const std::string& payload);
    
    // Subscribe a session to a topic
    void subscribe(const std::string& topic, std::shared_ptr<Session> session);
    
    // Unsubscribe session from topic
    void unsubscribe(const std::string& topic, std::shared_ptr<Session> session);
    
    // Handle session disconnect
    void on_session_disconnect(std::shared_ptr<Session> session);
    
    // Get broker statistics
    size_t get_active_sessions() const;
    size_t get_topic_count() const;
    
    TopicManager& get_topic_manager() { return topic_manager_; }
    
private:
    void do_accept();
    
    asio::ip::tcp::acceptor acceptor_;
    TopicManager topic_manager_;
    
    std::unordered_set<std::shared_ptr<Session>> sessions_;
    mutable std::mutex sessions_mutex_;
    
    bool running_ = false;
};

