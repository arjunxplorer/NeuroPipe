#define ASIO_STANDALONE
#include <asio.hpp>
#include "asio_server.hpp"
#include "config.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>

// Parse simple CLI arguments: --port N, --config path
BrokerConfig parse_args(int argc, char* argv[]) {
    BrokerConfig config;

    // Try to load config file first (default or specified)
    std::string config_path = "neuropipe.conf";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }

    // Load config file
    try {
        config = load_config(config_path);
    } catch (const std::exception& e) {
        log_error("Failed to load config: " + std::string(e.what()));
        throw;
    }

    // CLI flags override config file
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            int p = std::stoi(argv[++i]);
            if (p < 1 || p > 65535) {
                throw std::runtime_error("Invalid port: " + std::to_string(p));
            }
            config.port = static_cast<uint16_t>(p);
        }
        else if (arg == "--log-format" && i + 1 < argc) {
            std::string fmt = argv[++i];
            std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);
            if (fmt == "text" || fmt == "json") {
                config.log_format = fmt;
            }
        }
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: broker [OPTIONS]\n\n"
                      << "Options:\n"
                      << "  --port N          Override listen port (default: 9092)\n"
                      << "  --config PATH     Path to config file (default: neuropipe.conf)\n"
                      << "  --log-format FMT  Log output format: text or json (default: text)\n"
                      << "  --help, -h        Show this help message\n";
            exit(0);
        }
    }

    return config;
}

int main(int argc, char* argv[]) {
    BrokerConfig config;

    try {
        config = parse_args(argc, argv);
    } catch (const std::exception& e) {
        log_error("Startup failed: " + std::string(e.what()));
        return 1;
    }

    // Apply log settings from config
    set_log_level(config.log_level);
    set_log_format(config.log_format);

    log_info("Starting NeuroPipe Broker...");

    try {
        // Create io_context
        asio::io_context io_context;

        // Register signal handlers via Asio (proper async signal handling)
        asio::signal_set signals(io_context, SIGINT, SIGTERM);

        // Create broker server
        BrokerServer broker(io_context, config.port, config);
        broker.start();

        std::cout << "\n==================================" << std::endl;
        std::cout << "=== NeuroPipe Broker Running ===" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Port:       " << config.port << std::endl;
        std::cout << "Backend:    Standalone Asio" << std::endl;
        std::cout << "Protocol:   TCP" << std::endl;
        std::cout << "Log Level:  " << config.log_level << std::endl;
        std::cout << "Log Format: " << config.log_format << std::endl;
        std::cout << "Max Msg:    " << config.max_message_size << " bytes" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << "Press Ctrl+C to stop\n" << std::endl;

        // Async signal handler: on SIGINT/SIGTERM, stop the broker gracefully
        signals.async_wait([&](const std::error_code& ec, int signal_number) {
            if (!ec) {
                std::string sig_name = (signal_number == SIGINT) ? "SIGINT" : "SIGTERM";
                log_info("Received " + sig_name + ", shutting down gracefully...");
                broker.stop();
            }
        });

        // Stats timer — prints stats every 10 seconds
        asio::steady_timer stats_timer(io_context);
        std::function<void(const std::error_code&)> print_stats;
        print_stats = [&](const std::error_code& ec) {
            if (!ec && broker.get_active_sessions() > 0) {
                log_info("Stats - Active Sessions: " + std::to_string(broker.get_active_sessions()) +
                        ", Topics: " + std::to_string(broker.get_topic_count()));
            }
            stats_timer.expires_after(std::chrono::seconds(10));
            stats_timer.async_wait(print_stats);
        };
        stats_timer.expires_after(std::chrono::seconds(10));
        stats_timer.async_wait(print_stats);

        // Run the io_context on the main thread
        // This blocks until there's no more work (i.e., after broker.stop() is called)
        io_context.run();

        log_info("Broker stopped successfully");

    } catch (std::exception& e) {
        log_error("Exception in broker: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
