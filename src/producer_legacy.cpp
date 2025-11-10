#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {
    std::cout << "\n=== NeuroPipe Producer Client ===" << std::endl;
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
    std::cout << "\nFormat: PUBLISH:topic:message" << std::endl;
    std::cout << "Example: PUBLISH:sensor_data:temperature=23.5" << std::endl;
    std::cout << "Type 'quit' to exit\n" << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        if (line == "quit" || line == "exit") {
            std::cout << "Disconnecting..." << std::endl;
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        // Send message to broker
        ssize_t sent = send(sock, line.c_str(), line.size(), 0);
        if (sent < 0) {
            std::cerr << "ERROR: Failed to send message" << std::endl;
            break;
        }
        
        std::cout << "✓ Sent " << sent << " bytes" << std::endl;
    }
    
    close(sock);
    std::cout << "Producer client stopped." << std::endl;
    return 0;
}
