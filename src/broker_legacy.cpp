#include "server.hpp"
#include "utils.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

// Global flag for graceful shutdown
std::atomic<bool> running(true);

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        log_info("Received shutdown signal, stopping broker...");
        running = false;
    }
}

int main() {
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    log_info("Starting NeuroPipe Broker...");
    
    BrokerServer broker;
    broker.start(9092);
    
    std::cout << "\n=== NeuroPipe Broker Running ===" << std::endl;
    std::cout << "Port: 9092" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "================================\n" << std::endl;
    
    // Keep the broker running until shutdown signal
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    log_info("Shutting down broker...");
    broker.stop();
    log_info("Broker stopped successfully");
    
    return 0;
}
