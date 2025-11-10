#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

class ConsumerClient {
public:
    ConsumerClient(asio::io_context& io_context, 
                   const std::string& host, 
                   const std::string& port)
        : socket_(io_context),
          resolver_(io_context),
          host_(host),
          port_(port),
          running_(true) {
    }
    
    void connect() {
        try {
            auto endpoints = resolver_.resolve(host_, port_);
            asio::connect(socket_, endpoints);
            std::cout << "✓ Connected to broker at " << host_ << ":" << port_ << std::endl;
        } catch (std::exception& e) {
            throw std::runtime_error("Connection failed: " + std::string(e.what()));
        }
    }
    
    void subscribe(const std::string& topic) {
        std::string command = "SUBSCRIBE:" + topic + "\n";
        try {
            asio::write(socket_, asio::buffer(command));
            
            // Read response
            asio::streambuf response;
            asio::read_until(socket_, response, '\n');
            
            std::istream response_stream(&response);
            std::string response_line;
            std::getline(response_stream, response_line);
            
            if (response_line.find("OK:SUBSCRIBED") == 0) {
                std::cout << "✓ Subscribed to topic: " << topic << std::endl;
            } else {
                std::cout << "✗ Subscription failed: " << response_line << std::endl;
            }
        } catch (std::exception& e) {
            std::cerr << "✗ Failed to subscribe: " << e.what() << std::endl;
        }
    }
    
    void start_listening() {
        std::cout << "\n=== Listening for messages ===" << std::endl;
        std::cout << "Press Ctrl+C to stop\n" << std::endl;
        
        asio::streambuf buffer;
        std::string line;
        
        while (running_) {
            try {
                asio::read_until(socket_, buffer, '\n');
                
                std::istream is(&buffer);
                std::getline(is, line);
                
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                char time_str[100];
                std::strftime(time_str, sizeof(time_str), "%H:%M:%S", std::localtime(&time));
                
                // Parse message
                if (line.find("MESSAGE:") == 0) {
                    size_t first_colon = line.find(':', 8);
                    if (first_colon != std::string::npos) {
                        std::string topic = line.substr(8, first_colon - 8);
                        std::string payload = line.substr(first_colon + 1);
                        
                        std::cout << "[" << time_str << "] "
                                  << "📨 [" << topic << "] " << payload << std::endl;
                    }
                } else if (line.find("OK:") == 0 || line.find("ERROR:") == 0) {
                    std::cout << "[" << time_str << "] ℹ️  " << line << std::endl;
                } else if (line == "PONG") {
                    std::cout << "[" << time_str << "] 🏓 PONG received" << std::endl;
                } else if (!line.empty()) {
                    std::cout << "[" << time_str << "] " << line << std::endl;
                }
                
            } catch (std::exception& e) {
                if (running_) {
                    std::cerr << "\n✗ Connection lost: " << e.what() << std::endl;
                }
                break;
            }
        }
    }
    
    void stop() {
        running_ = false;
        if (socket_.is_open()) {
            socket_.close();
        }
    }
    
    void disconnect() {
        stop();
        std::cout << "Disconnected from broker" << std::endl;
    }
    
private:
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string port_;
    std::atomic<bool> running_;
};

void print_usage(const char* program_name) {
    std::cout << "\nUsage: " << program_name << " [host] [port] [topic1] [topic2] ..." << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << " 127.0.0.1 9092 sensor_data" << std::endl;
    std::cout << "  " << program_name << " localhost 9092 events logs alerts" << std::endl;
    std::cout << "\nDefaults:" << std::endl;
    std::cout << "  host: 127.0.0.1" << std::endl;
    std::cout << "  port: 9092" << std::endl;
    std::cout << "  topics: (none - will prompt interactively)\n" << std::endl;
}

std::atomic<bool> g_running(true);
ConsumerClient* g_client = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nReceived shutdown signal..." << std::endl;
        g_running = false;
        if (g_client) {
            g_client->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    std::string port = "9092";
    std::vector<std::string> topics;
    
    // Parse command line arguments
    if (argc >= 2) {
        if (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        host = argv[1];
    }
    if (argc >= 3) {
        port = argv[2];
    }
    if (argc >= 4) {
        for (int i = 3; i < argc; ++i) {
            topics.push_back(argv[i]);
        }
    }
    
    std::cout << "\n=========================================" << std::endl;
    std::cout << "=== NeuroPipe Consumer Client (Asio) ===" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "=========================================\n" << std::endl;
    
    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        asio::io_context io_context;
        ConsumerClient client(io_context, host, port);
        g_client = &client;
        
        std::cout << "Connecting to broker..." << std::endl;
        client.connect();
        
        // If no topics provided via command line, ask interactively
        if (topics.empty()) {
            std::cout << "\nEnter topics to subscribe (one per line, empty line to finish):" << std::endl;
            std::string topic;
            while (true) {
                std::cout << "Topic: ";
                if (!std::getline(std::cin, topic)) {
                    break;
                }
                topic.erase(0, topic.find_first_not_of(" \t\n\r"));
                topic.erase(topic.find_last_not_of(" \t\n\r") + 1);
                
                if (topic.empty()) {
                    break;
                }
                topics.push_back(topic);
            }
            
            if (topics.empty()) {
                std::cout << "\n✗ No topics specified. Exiting." << std::endl;
                return 1;
            }
        }
        
        // Subscribe to all topics
        std::cout << "\nSubscribing to topics..." << std::endl;
        for (const auto& topic : topics) {
            client.subscribe(topic);
        }
        
        // Start listening for messages
        client.start_listening();
        
        client.disconnect();
        
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << "\nIs the broker running? Start it with: ./build/broker" << std::endl;
        return 1;
    }
    
    std::cout << "Consumer client stopped." << std::endl;
    return 0;
}
