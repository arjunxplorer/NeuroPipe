#define ASIO_STANDALONE
#include <asio.hpp>
#include <iostream>
#include <string>
#include <atomic>

class ProducerClient {
public:
    ProducerClient(asio::io_context& io_context,
                   const std::string& host,
                   const std::string& port)
        : socket_(io_context),
          resolver_(io_context),
          host_(host),
          port_(port),
          connected_(false) {
    }

    bool connect() {
        try {
            auto endpoints = resolver_.resolve(host_, port_);
            asio::connect(socket_, endpoints);
            connected_ = true;
            std::cout << "✓ Connected to broker at " << host_ << ":" << port_ << std::endl;
            return true;
        } catch (std::exception& e) {
            connected_ = false;
            std::cerr << "✗ Connection failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool reconnect() {
        std::cout << "Attempting to reconnect..." << std::endl;
        if (socket_.is_open()) {
            asio::error_code ec;
            socket_.close(ec);
        }
        connected_ = false;
        return connect();
    }

    bool publish(const std::string& topic, const std::string& payload) {
        if (!connected_) {
            std::cerr << "✗ Not connected to broker" << std::endl;
            return false;
        }

        std::string message = "PUBLISH:" + topic + ":" + payload + "\n";

        try {
            asio::write(socket_, asio::buffer(message));

            asio::streambuf response;
            asio::read_until(socket_, response, '\n');

            std::istream response_stream(&response);
            std::string response_line;
            std::getline(response_stream, response_line);

            if (response_line.find("OK:PUBLISHED") == 0) {
                std::cout << "✓ Published to topic '" << topic << "'" << std::endl;
                return true;
            } else if (response_line.find("ERROR:SERVER_SHUTDOWN") == 0) {
                std::cout << "✗ Broker is shutting down" << std::endl;
                connected_ = false;
                return false;
            } else {
                std::cout << "✗ Broker response: " << response_line << std::endl;
                return false;
            }
        } catch (std::exception& e) {
            std::cerr << "✗ Connection lost: " << e.what() << std::endl;
            connected_ = false;
            return false;
        }
    }

    bool send_command(const std::string& command) {
        if (!connected_) {
            std::cerr << "✗ Not connected to broker" << std::endl;
            return false;
        }

        try {
            asio::write(socket_, asio::buffer(command));

            asio::streambuf response;
            asio::read_until(socket_, response, '\n');

            std::istream response_stream(&response);
            std::string response_line;
            std::getline(response_stream, response_line);

            if (response_line.find("ERROR:SERVER_SHUTDOWN") == 0) {
                std::cout << "✗ Broker is shutting down" << std::endl;
                connected_ = false;
                return false;
            }

            std::cout << "← " << response_line << std::endl;
            return true;
        } catch (std::exception& e) {
            std::cerr << "✗ Connection lost: " << e.what() << std::endl;
            connected_ = false;
            return false;
        }
    }

    void disconnect() {
        if (socket_.is_open()) {
            asio::error_code ec;
            socket_.close(ec);
        }
        connected_ = false;
        std::cout << "Disconnected from broker" << std::endl;
    }

    bool is_connected() const { return connected_; }

private:
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::string host_;
    std::string port_;
    bool connected_;
};

void print_help() {
    std::cout << "\nAvailable commands:" << std::endl;
    std::cout << "  PUBLISH:topic:payload   - Publish a message to a topic" << std::endl;
    std::cout << "  SUBSCRIBE:topic         - Subscribe to a topic" << std::endl;
    std::cout << "  UNSUBSCRIBE:topic       - Unsubscribe from a topic" << std::endl;
    std::cout << "  PING                    - Ping the broker" << std::endl;
    std::cout << "  help                    - Show this help" << std::endl;
    std::cout << "  quit                    - Exit the producer\n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    std::string port = "9092";

    if (argc >= 2) {
        host = argv[1];
    }
    if (argc >= 3) {
        port = argv[2];
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "=== NeuroPipe Producer Client (Asio) ===" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "========================================\n" << std::endl;

    try {
        asio::io_context io_context;
        ProducerClient client(io_context, host, port);

        std::cout << "Connecting to broker..." << std::endl;
        if (!client.connect()) {
            std::cerr << "\nIs the broker running? Start it with: ./build/broker" << std::endl;
            return 1;
        }

        print_help();

        std::string line;
        while (true) {
            std::cout << "> ";
            if (!std::getline(std::cin, line)) {
                break;
            }

            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t\n\r"));
            line.erase(line.find_last_not_of(" \t\n\r") + 1);

            if (line.empty()) {
                continue;
            }

            if (line == "quit" || line == "exit") {
                std::cout << "Disconnecting..." << std::endl;
                break;
            }

            if (line == "help" || line == "?") {
                print_help();
                continue;
            }

            // Check connection before sending
            if (!client.is_connected()) {
                std::cerr << "✗ Not connected to broker. Type 'reconnect' to try again, or 'quit' to exit." << std::endl;
                continue;
            }

            if (line == "reconnect") {
                if (!client.reconnect()) {
                    std::cerr << "✗ Reconnection failed. Is the broker running?" << std::endl;
                }
                continue;
            }

            bool success = false;
            if (line.find("PUBLISH:") == 0) {
                size_t first_colon = line.find(':', 8);
                if (first_colon != std::string::npos) {
                    std::string topic = line.substr(8, first_colon - 8);
                    std::string payload = line.substr(first_colon + 1);
                    success = client.publish(topic, payload);
                } else {
                    std::cerr << "✗ Invalid format. Use: PUBLISH:topic:payload" << std::endl;
                }
            } else if (line.find("SUBSCRIBE:") == 0 ||
                       line.find("UNSUBSCRIBE:") == 0 ||
                       line.find("PING") == 0) {
                success = client.send_command(line + "\n");
            } else {
                std::cerr << "✗ Unknown command. Type 'help' for available commands." << std::endl;
            }

            // If command failed due to disconnect, offer reconnection
            if (!success && !client.is_connected()) {
                std::cerr << "Connection lost. Type 'reconnect' to try again, or 'quit' to exit." << std::endl;
            }
        }

        client.disconnect();

    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::cerr << "\nIs the broker running? Start it with: ./build/broker" << std::endl;
        return 1;
    }

    std::cout << "Producer client stopped." << std::endl;
    return 0;
}
