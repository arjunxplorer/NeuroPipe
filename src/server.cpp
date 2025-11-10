#include "server.hpp"
#include "utils.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <functional>

BrokerServer::BrokerServer() : server_socket_(-1), running_(false) {}

BrokerServer::~BrokerServer() {
    stop();
}

void BrokerServer::start(int port) {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        log_error("Failed to create socket");
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_error("Failed to set socket options");
        close(server_socket_);
        return;
    }
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_error("Failed to bind socket to port " + std::to_string(port));
        close(server_socket_);
        return;
    }
    
    // Listen for connections
    if (listen(server_socket_, 10) < 0) {
        log_error("Failed to listen on socket");
        close(server_socket_);
        return;
    }
    
    running_ = true;
    log_info("Broker server started on port " + std::to_string(port));
    
    // Start accepting connections in a separate thread
    accept_thread_ = std::thread(std::bind(&BrokerServer::acceptConnections, this));
}

void BrokerServer::stop() {
    if (running_) {
        running_ = false;
        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        log_info("Broker server stopped");
    }
}

void BrokerServer::acceptConnections() {
    while (running_) {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket_, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running_) {
                log_error("Failed to accept client connection");
            }
            continue;
        }
        
        log_info("Client connected from " + 
                 std::string(inet_ntoa(client_addr.sin_addr)));
        
        // Add client to connected clients list
        {
            std::lock_guard<std::mutex> lock(clients_mtx_);
            connected_clients_.insert(client_socket);
        }
        
        // Handle each client in a separate thread
        std::thread(std::bind(&BrokerServer::handleClient, this, client_socket)).detach();
    }
}

void BrokerServer::handleClient(int client_socket) {
    char buffer[4096];
    
    while (running_) {
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_read <= 0) {
            // Client disconnected or error
            break;
        }
        
        buffer[bytes_read] = '\0';
        std::string data(buffer, bytes_read);
        
        log_info("Received message: " + data);
        
        // Simple protocol: PUBLISH:topic:payload
        // TODO: Implement proper protocol parsing
        if (data.find("PUBLISH:") == 0) {
            size_t first_colon = data.find(':', 8);
            if (first_colon != std::string::npos) {
                std::string topic = data.substr(8, first_colon - 8);
                std::string payload = data.substr(first_colon + 1);
                publish(topic, payload);
                
                // Broadcast the message to all other connected clients
                broadcastMessage(data);
            }
        }
    }
    
    // Remove client from connected clients list
    removeClient(client_socket);
    
    close(client_socket);
    log_info("Client disconnected");
}

void BrokerServer::publish(const std::string& topic, const std::string& data) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    static uint64_t sequence_counter = 0;
    Message msg(topic, data);
    msg.sequence = sequence_counter++;
    
    topics_[topic].push(msg);
    
    log_info("Published message to topic '" + topic + "': " + data);
}

Message BrokerServer::consume(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    if (topics_.find(topic) != topics_.end() && !topics_[topic].empty()) {
        Message msg = topics_[topic].front();
        topics_[topic].pop();
        log_info("Consumed message from topic '" + topic + "': " + msg.payload);
        return msg;
    }
    
    // Return empty message if topic doesn't exist or is empty
    return Message("", "");
}

void BrokerServer::subscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mtx_);
    
    // Create topic if it doesn't exist
    if (topics_.find(topic) == topics_.end()) {
        topics_[topic] = std::queue<Message>();
        log_info("Created new topic: " + topic);
    }
    
    log_info("Subscribed to topic: " + topic);
}

void BrokerServer::broadcastMessage(const std::string& original_message) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    
    int sent_count = 0;
    int failed_count = 0;
    
    for (int client_socket : connected_clients_) {
        ssize_t sent = send(client_socket, original_message.c_str(), original_message.length(), MSG_NOSIGNAL);
        if (sent > 0) {
            sent_count++;
        } else {
            failed_count++;
        }
    }
    
    if (sent_count > 0) {
        log_info("Broadcasted message to " + std::to_string(sent_count) + " client(s)");
    }
    if (failed_count > 0) {
        log_warn("Failed to send to " + std::to_string(failed_count) + " client(s)");
    }
}

void BrokerServer::removeClient(int client_socket) {
    std::lock_guard<std::mutex> lock(clients_mtx_);
    connected_clients_.erase(client_socket);
    log_info("Removed client socket " + std::to_string(client_socket) + " from connected clients");
}
