#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

// Broker configuration with sensible defaults
struct BrokerConfig {
    uint16_t port = 9092;
    size_t max_message_size = 1048576;       // 1MB
    size_t max_subscribers_per_topic = 0;    // 0 = unlimited
    std::string log_level = "INFO";          // DEBUG, INFO, WARN, ERROR
    std::string log_format = "text";         // "text" or "json"
    std::string config_file = "neuropipe.conf";
};

// Trim whitespace from both ends of a string
inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Parse an INI-style config file into BrokerConfig
// Format: key = value, with [section] headers (sections are ignored for simplicity)
// Lines starting with # or ; are comments
inline BrokerConfig load_config(const std::string& path) {
    BrokerConfig config;
    std::ifstream file(path);

    if (!file.is_open()) {
        // No config file found — use defaults
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // Skip empty lines, comments, and section headers
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }

        // Parse key = value
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        // Map keys to config fields
        if (key == "port") {
            int p = std::stoi(value);
            if (p < 1 || p > 65535) {
                throw std::runtime_error("Invalid port number: " + value);
            }
            config.port = static_cast<uint16_t>(p);
        }
        else if (key == "max_message_size") {
            config.max_message_size = std::stoul(value);
        }
        else if (key == "max_subscribers_per_topic") {
            config.max_subscribers_per_topic = std::stoul(value);
        }
        else if (key == "log_level") {
            // Normalize to uppercase
            std::string upper = value;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "DEBUG" || upper == "INFO" || upper == "WARN" || upper == "ERROR") {
                config.log_level = upper;
            }
        }
        else if (key == "log_format") {
            std::string lower = value;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == "text" || lower == "json") {
                config.log_format = lower;
            }
        }
    }

    return config;
}
