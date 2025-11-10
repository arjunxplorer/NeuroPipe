#include "../include/message.hpp"
#include "../src/utils.hpp"
#include <cassert>
#include <iostream>
#include <thread>
#include <functional>

void test_message_creation() {
    std::cout << "Testing Message creation..." << std::endl;
    
    Message msg("orders", "hello");
    assert(msg.topic == "orders");
    assert(msg.payload == "hello");
    assert(msg.sequence == 0);
    
    std::cout << "✓ Message creation test passed" << std::endl;
}

void test_thread_safe_queue() {
    std::cout << "Testing ThreadSafeQueue..." << std::endl;
    
    ThreadSafeQueue<int> queue;
    
    // Test push and try_pop
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    assert(queue.size() == 3);
    assert(!queue.empty());
    
    int value;
    assert(queue.try_pop(value));
    assert(value == 1);
    
    assert(queue.try_pop(value));
    assert(value == 2);
    
    assert(queue.size() == 1);
    
    // Test clear
    queue.clear();
    assert(queue.empty());
    assert(queue.size() == 0);
    
    std::cout << "✓ ThreadSafeQueue test passed" << std::endl;
}

void producer_func(ThreadSafeQueue<int>* queue, int num_items) {
    for (int i = 0; i < num_items; ++i) {
        queue->push(i);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void consumer_func(ThreadSafeQueue<int>* queue, int num_items) {
    int count = 0;
    int value;
    while (count < num_items) {
        if (queue->try_pop(value)) {
            count++;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
}

void test_thread_safe_queue_threading() {
    std::cout << "Testing ThreadSafeQueue with multiple threads..." << std::endl;
    
    ThreadSafeQueue<int> queue;
    const int NUM_ITEMS = 100;
    
    // Producer thread
    std::thread producer(std::bind(producer_func, &queue, NUM_ITEMS));
    
    // Consumer thread
    std::thread consumer(std::bind(consumer_func, &queue, NUM_ITEMS));
    
    producer.join();
    consumer.join();
    
    assert(queue.empty());
    
    std::cout << "✓ ThreadSafeQueue threading test passed" << std::endl;
}

void test_logging() {
    std::cout << "Testing logging functions..." << std::endl;
    
    log_info("This is an info message");
    log_error("This is an error message");
    log_debug("This is a debug message");
    log_warn("This is a warning message");
    
    std::cout << "✓ Logging test passed" << std::endl;
}

int main() {
    std::cout << "\n=== Running NeuroPipe Basic Tests ===" << std::endl;
    std::cout << std::endl;
    
    try {
        test_message_creation();
        test_thread_safe_queue();
        test_thread_safe_queue_threading();
        test_logging();
        
        std::cout << std::endl;
        std::cout << "=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
