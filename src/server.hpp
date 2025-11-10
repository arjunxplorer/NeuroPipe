#pragma once
#include <unordered_map>
#include <queue>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <set>
#include "../include/message.hpp"

class BrokerServer {
public:
    BrokerServer();
    ~BrokerServer();
    
    void start(int port);
    void stop();
    void publish(const std::string& topic, const std::string& data);
    Message consume(const std::string& topic);
    void subscribe(const std::string& topic);
    
private:
    void acceptConnections();
    void handleClient(int client_socket);
    void broadcastMessage(const std::string& original_message);
    void removeClient(int client_socket);
    
    std::unordered_map<std::string, std::queue<Message> > topics_;
    std::mutex mtx_;
    std::mutex clients_mtx_;
    std::set<int> connected_clients_;
    int server_socket_;
    bool running_;
    std::thread accept_thread_;
};

