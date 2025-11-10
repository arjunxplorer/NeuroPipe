/**
 * Simple Application with Debug Logging
 * 
 * Demonstrates how to use DebugLogger in your application.
 * Shows various logging levels, metrics, and error handling.
 */

#include "../lib/debug_logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>

// Simulate some application work
void process_order(DebugLogger& logger, int order_id) {
    logger.info("Processing order #" + std::to_string(order_id));
    
    auto start = std::chrono::steady_clock::now();
    
    // Simulate database query
    std::this_thread::sleep_for(std::chrono::milliseconds(10 + (rand() % 20)));
    logger.debug("Database query completed for order #" + std::to_string(order_id));
    
    // Simulate payment processing
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + (rand() % 100)));
    
    // Randomly fail some orders
    if (rand() % 10 == 0) {
        logger.error("Payment failed for order #" + std::to_string(order_id));
        return;
    }
    
    logger.info("Payment successful for order #" + std::to_string(order_id));
    
    // Send confirmation email
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    logger.debug("Confirmation email sent for order #" + std::to_string(order_id));
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    logger.metric("order_processing_time_ms", static_cast<int>(duration.count()));
    logger.info("Order #" + std::to_string(order_id) + " completed successfully");
}

void simulate_user_activity(DebugLogger& logger, int user_id) {
    logger.info("User " + std::to_string(user_id) + " logged in");
    
    // Simulate browsing
    for (int i = 0; i < 3; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        logger.debug("User " + std::to_string(user_id) + " viewing page");
    }
    
    // Maybe make a purchase
    if (rand() % 2 == 0) {
        int order_id = 1000 + user_id * 10 + (rand() % 10);
        process_order(logger, order_id);
    }
    
    logger.info("User " + std::to_string(user_id) + " logged out");
}

int main() {
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║   Simple Application with Debug Logging       ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Connecting to NeuroPipe broker...\n";
    
    // Create logger for this service
    DebugLogger logger("simple_app");
    
    if (!logger.is_connected()) {
        std::cerr << "❌ Failed to connect to broker!\n";
        std::cerr << "   Make sure the broker is running: ./build/broker\n";
        return 1;
    }
    
    std::cout << "✓ Connected to broker!\n\n";
    std::cout << "View logs in another terminal:\n";
    std::cout << "  ./dashboards/view_all.sh     - See all logs\n";
    std::cout << "  ./dashboards/view_errors.sh  - See errors only\n";
    std::cout << "  ./dashboards/view_metrics.sh - See metrics\n\n";
    std::cout << "Running simulation...\n";
    std::cout << "Press Ctrl+C to stop\n\n";
    
    // Log application startup
    logger.info("Application started");
    logger.metric("startup_time_ms", 123);
    
    // Simulate various activities
    int iteration = 0;
    while (true) {
        iteration++;
        
        logger.info("=== Iteration #" + std::to_string(iteration) + " ===");
        
        // Simulate some orders
        for (int i = 0; i < 3; i++) {
            int order_id = iteration * 100 + i;
            process_order(logger, order_id);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Simulate a warning condition
        if (iteration % 5 == 0) {
            logger.warn("High memory usage detected: 85%");
        }
        
        // Log some metrics
        logger.metric("active_connections", 10 + (rand() % 20));
        logger.metric("cpu_usage_percent", 20 + (rand() % 40));
        logger.metric("memory_usage_mb", 500 + (rand() % 200));
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    logger.info("Application shutting down");
    
    return 0;
}

