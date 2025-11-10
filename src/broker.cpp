#define ASIO_STANDALONE
#include <asio.hpp>
#include "asio_server.hpp"
#include "utils.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

// Global flag for graceful shutdown
std::atomic<bool> running(true);
asio::io_context* global_io_context = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        log_info("Received shutdown signal, stopping broker...");
        running = false;
        if (global_io_context) {
            global_io_context->stop();
        }
    }
}

int main() {
    // Setup signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    log_info("Starting NeuroPipe Broker (Asio Edition)...");
    
    try {
        // Create io_context
        asio::io_context io_context;
        global_io_context = &io_context;
        
        // Create broker server on port 9092
        BrokerServer broker(io_context, 9092);
        broker.start();
        
        std::cout << "\n==================================" << std::endl;
        std::cout << "=== NeuroPipe Broker Running ===" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Port:       9092" << std::endl;
        std::cout << "Backend:    Standalone Asio" << std::endl;
        std::cout << "Protocol:   TCP" << std::endl;
        std::cout << "Commands:   PUBLISH, SUBSCRIBE, UNSUBSCRIBE" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Press Ctrl+C to stop\n" << std::endl;
        
        // Run the io_context in a separate thread
        std::thread io_thread([&io_context]() {
            io_context.run();
        });
        
        // Monitor thread - prints statistics periodically
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            if (running) {
                log_info("Stats - Active Sessions: " + std::to_string(broker.get_active_sessions()) + 
                        ", Topics: " + std::to_string(broker.get_topic_count()));
            }
        }
        
        log_info("Shutting down broker...");
        broker.stop();
        
        // Wait for io_context to finish
        if (io_thread.joinable()) {
            io_thread.join();
        }
        
        log_info("Broker stopped successfully");
        
    } catch (std::exception& e) {
        log_error("Exception in broker: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}
