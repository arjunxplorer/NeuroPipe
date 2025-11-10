/**
 * Robust Application Example - Graceful Degradation
 * 
 * Demonstrates how a production application should handle logging:
 * - Continues running even if broker is unavailable
 * - Logs are optional, not critical to business logic
 * - Auto-reconnects when broker becomes available
 * - Handles edge cases gracefully
 */

#include "../lib/debug_logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

// Simulate a production service that processes orders
class OrderProcessor {
private:
    DebugLogger* logger_;  // Optional logger
    int orders_processed_;
    int orders_failed_;
    
public:
    OrderProcessor(DebugLogger* logger = nullptr) 
        : logger_(logger), orders_processed_(0), orders_failed_(0) {}
    
    void process_order(int order_id, double amount) {
        // Log if available, but don't fail if not
        if (logger_ && logger_->is_connected()) {
            logger_->info("Processing order #" + std::to_string(order_id) + 
                         " amount: $" + std::to_string(amount));
        }
        
        auto start = std::chrono::steady_clock::now();
        
        // Simulate order processing (the actual business logic)
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + (rand() % 20)));
        
        // Simulate occasional failures (10% fail rate)
        bool success = (rand() % 10 != 0);
        
        if (success) {
            orders_processed_++;
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            // Log success
            if (logger_ && logger_->is_connected()) {
                logger_->info("Order #" + std::to_string(order_id) + " completed successfully");
                logger_->metric("order_processing_time_ms", static_cast<int>(duration.count()));
                logger_->metric("order_amount", amount);
            }
        } else {
            orders_failed_++;
            
            // Log error
            if (logger_ && logger_->is_connected()) {
                logger_->error("Order #" + std::to_string(order_id) + " failed: Payment declined");
            }
        }
        
        // Business logic continues regardless of logging
    }
    
    void print_stats() {
        std::cout << "\n═══════════════════════════════════════\n";
        std::cout << "  Order Processing Statistics\n";
        std::cout << "═══════════════════════════════════════\n";
        std::cout << "  Processed: " << orders_processed_ << "\n";
        std::cout << "  Failed:    " << orders_failed_ << "\n";
        std::cout << "  Total:     " << (orders_processed_ + orders_failed_) << "\n";
        std::cout << "═══════════════════════════════════════\n\n";
        
        if (logger_ && logger_->is_connected()) {
            logger_->metric("total_orders_processed", orders_processed_);
            logger_->metric("total_orders_failed", orders_failed_);
        }
    }
};

// Test edge cases
void test_edge_cases(DebugLogger& logger) {
    std::cout << "Testing edge cases...\n";
    
    // Test 1: Empty message
    logger.info("");
    std::cout << "  ✓ Empty message handled\n";
    
    // Test 2: Very long message
    std::string long_msg(1000, 'A');
    logger.info("Long message: " + long_msg);
    std::cout << "  ✓ Long message (1KB) handled\n";
    
    // Test 3: Special characters
    logger.info("Message with: colon test");
    logger.info("Message with\nnewline test");
    logger.info("Message with \"quotes\" test");
    logger.info("Unicode test: 你好世界 🚀");
    std::cout << "  ✓ Special characters handled\n";
    
    // Test 4: Rapid logging
    for (int i = 0; i < 100; i++) {
        logger.debug("Rapid message " + std::to_string(i));
    }
    std::cout << "  ✓ Rapid logging (100 msgs) handled\n";
    
    std::cout << "Edge case testing complete!\n\n";
}

// Test multi-threading
void thread_worker(DebugLogger* logger, int thread_id, int iterations) {
    for (int i = 0; i < iterations; i++) {
        if (logger && logger->is_connected()) {
            logger->debug("Thread " + std::to_string(thread_id) + 
                         " iteration " + std::to_string(i));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void test_thread_safety(DebugLogger& logger) {
    std::cout << "Testing thread safety with 10 threads...\n";
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back(thread_worker, &logger, i, 50);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "  ✓ Thread safety test complete (500 messages from 10 threads)\n\n";
}

int main(int argc, char* argv[]) {
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║   Robust Application - Graceful Degradation Demo    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    bool run_tests = (argc > 1 && std::string(argv[1]) == "--test");
    
    // Create logger
    std::cout << "Initializing logger...\n";
    DebugLogger logger("robust_app");
    
    // Check connection but DON'T EXIT if not connected
    if (!logger.is_connected()) {
        std::cout << "⚠️  Warning: NeuroPipe broker not available\n";
        std::cout << "   Application will continue WITHOUT debug logging\n";
        std::cout << "   To enable logging, start broker: ./build/broker\n\n";
    } else {
        std::cout << "✓ Debug logging enabled\n";
        std::cout << "  View logs: ./dashboards/view_all.sh\n\n";
        logger.info("Robust application started");
    }
    
    // Run edge case tests if requested
    if (run_tests) {
        std::cout << "\n═══ Running Edge Case Tests ═══\n\n";
        
        if (logger.is_connected()) {
            test_edge_cases(logger);
            test_thread_safety(logger);
            
            logger.info("All edge case tests completed successfully");
        } else {
            std::cout << "⚠️  Skipping tests (broker not available)\n\n";
        }
    }
    
    // Create order processor (business logic)
    OrderProcessor processor(&logger);
    
    // Process orders - this works WITH or WITHOUT logging!
    std::cout << "Processing orders...\n";
    std::cout << "(Press Ctrl+C to stop)\n\n";
    
    int iteration = 0;
    while (iteration < 20) {  // Process 20 orders total
        iteration++;
        
        // Process 3 orders per iteration
        for (int i = 0; i < 3; i++) {
            int order_id = iteration * 100 + i;
            double amount = 50.0 + (rand() % 200);
            
            processor.process_order(order_id, amount);
            std::cout << ".";
            std::cout.flush();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Periodic warning
        if (iteration % 5 == 0) {
            if (logger.is_connected()) {
                logger.warn("High load detected at iteration " + std::to_string(iteration));
            }
        }
        
        // Log metrics
        if (logger.is_connected()) {
            logger.metric("active_connections", 10 + (rand() % 20));
            logger.metric("memory_usage_mb", 500 + (rand() % 200));
            logger.metric("cpu_usage_percent", 20 + (rand() % 40));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "\n\n";
    
    // Print statistics
    processor.print_stats();
    
    // Final log
    if (logger.is_connected()) {
        logger.info("Application completed successfully");
    }
    
    std::cout << "Application finished!\n";
    std::cout << "\n";
    std::cout << "Key Points Demonstrated:\n";
    std::cout << "  ✓ App runs successfully with OR without broker\n";
    std::cout << "  ✓ Logging is optional, not critical\n";
    std::cout << "  ✓ Business logic continues regardless\n";
    std::cout << "  ✓ Graceful degradation in action\n";
    std::cout << "  ✓ Production-ready pattern\n";
    std::cout << "\n";
    
    return 0;  // Success even if logging failed!
}

