#pragma once
#include <string>
#include <chrono>
#include <cstdint>

struct Message {
    std::string topic;
    std::string payload;
    uint64_t sequence;
    std::chrono::system_clock::time_point timestamp;
    
    // Constructor
    Message(const std::string& t, const std::string& p)
        : topic(t), payload(p), sequence(0), 
          timestamp(std::chrono::system_clock::now()) {}
};