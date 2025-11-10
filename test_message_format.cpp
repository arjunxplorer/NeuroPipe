#include <iostream>
#include <string>

int main() {
    // Test what the producer creates
    std::string topic = "sensor_data";
    std::string payload = "temperature=23.5";
    
    std::string producer_message = "PUBLISH:" + topic + ":" + payload;
    std::cout << "Producer creates: '" << producer_message << "'" << std::endl;
    std::cout << "Has newline? " << (producer_message.back() == '\n' ? "YES" : "NO") << std::endl;
    std::cout << "Length: " << producer_message.length() << std::endl;
    
    std::cout << "\nShould be: 'PUBLISH:sensor_data:temperature=23.5\\n'" << std::endl;
    std::cout << "Length should be: " << (producer_message.length() + 1) << std::endl;
    
    return 0;
}

