#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    std::cout << "\n=== NeuroPipe Consumer Client ===" << std::endl;
    std::cout << "Connecting to broker at localhost:9092..." << std::endl;
    
    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "ERROR: Failed to create socket" << std::endl;
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9092);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << "ERROR: Invalid address" << std::endl;
        close(sock);
        return 1;
    }
    
    // Connect to broker
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "ERROR: Connection failed. Is the broker running?" << std::endl;
        std::cerr << "Start the broker with: ./build/broker" << std::endl;
        close(sock);
        return 1;
    }
    
    std::cout << "✓ Connected to broker successfully!" << std::endl;
    std::cout << "Waiting for messages... (Press Ctrl+C to stop)\n" << std::endl;
    
    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes < 0) {
            std::cerr << "\nERROR: Failed to receive message" << std::endl;
            break;
        } else if (bytes == 0) {
            std::cout << "\nBroker disconnected" << std::endl;
            break;
        }
        
        buffer[bytes] = '\0';
        std::string msg(buffer, bytes);
        std::cout << "[RECEIVED " << bytes << " bytes] " << msg << std::endl;
    }
    
    close(sock);
    std::cout << "Consumer client stopped." << std::endl;
    return 0;
}
