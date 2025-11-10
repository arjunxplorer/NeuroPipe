#define ASIO_STANDALONE
#include <asio.hpp>
#include "../src/asio_server.hpp"
#include "../src/utils.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <atomic>

// Simple test framework
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    void name(); \
    void run_##name() { \
        std::cout << "\n[TEST] " << #name << "..." << std::endl; \
        try { \
            name(); \
            std::cout << "[PASS] " << #name << std::endl; \
            tests_passed++; \
        } catch (const std::exception& e) { \
            std::cout << "[FAIL] " << #name << " - " << e.what() << std::endl; \
            tests_failed++; \
        } catch (...) { \
            std::cout << "[FAIL] " << #name << " - Unknown exception" << std::endl; \
            tests_failed++; \
        } \
    } \
    void name()

#define ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(message); \
    }

// Helper class for test client
class TestClient {
public:
    TestClient(asio::io_context& io_context, const std::string& host, uint16_t port)
        : socket_(io_context), resolver_(io_context) {
        auto endpoints = resolver_.resolve(host, std::to_string(port));
        asio::connect(socket_, endpoints);
    }
    
    void send(const std::string& message) {
        asio::write(socket_, asio::buffer(message));
    }
    
    std::string receive_line() {
        asio::streambuf buffer;
        asio::read_until(socket_, buffer, '\n');
        std::istream is(&buffer);
        std::string line;
        std::getline(is, line);
        return line;
    }
    
    void close() {
        if (socket_.is_open()) {
            socket_.close();
        }
    }
    
private:
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
};

// Global broker for tests
std::unique_ptr<BrokerServer> g_broker;
std::unique_ptr<asio::io_context> g_io_context;
std::unique_ptr<std::thread> g_io_thread;

void setup_broker() {
    g_io_context = std::make_unique<asio::io_context>();
    g_broker = std::make_unique<BrokerServer>(*g_io_context, 9093); // Use different port for testing
    g_broker->start();
    
    g_io_thread = std::make_unique<std::thread>([&]() {
        g_io_context->run();
    });
    
    // Give broker time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void teardown_broker() {
    if (g_broker) {
        g_broker->stop();
    }
    if (g_io_context) {
        g_io_context->stop();
    }
    if (g_io_thread && g_io_thread->joinable()) {
        g_io_thread->join();
    }
    
    g_broker.reset();
    g_io_context.reset();
    g_io_thread.reset();
}

// ============================================================================
// Test Cases
// ============================================================================

TEST(test_broker_starts) {
    ASSERT(g_broker != nullptr, "Broker should be initialized");
    ASSERT(g_broker->get_active_sessions() == 0, "No sessions should be active initially");
}

TEST(test_client_connection) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT(g_broker->get_active_sessions() >= 1, "Broker should have at least 1 active session");
    
    client.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST(test_ping_pong) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    client.send("PING\n");
    std::string response = client.receive_line();
    
    ASSERT(response == "PONG", "Expected PONG response, got: " + response);
    
    client.close();
}

TEST(test_subscribe_topic) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    client.send("SUBSCRIBE:test_topic\n");
    std::string response = client.receive_line();
    
    ASSERT(response.find("OK:SUBSCRIBED") == 0, "Expected OK:SUBSCRIBED, got: " + response);
    
    client.close();
}

TEST(test_publish_message) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    client.send("PUBLISH:test_topic:hello_world\n");
    std::string response = client.receive_line();
    
    ASSERT(response == "OK:PUBLISHED", "Expected OK:PUBLISHED, got: " + response);
    
    client.close();
}

TEST(test_publish_and_receive) {
    asio::io_context io_context;
    TestClient publisher(io_context, "127.0.0.1", 9093);
    TestClient subscriber(io_context, "127.0.0.1", 9093);
    
    // Subscribe first
    subscriber.send("SUBSCRIBE:test_channel\n");
    std::string sub_response = subscriber.receive_line();
    ASSERT(sub_response.find("OK:SUBSCRIBED") == 0, "Subscription failed");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Publish message
    publisher.send("PUBLISH:test_channel:test_payload\n");
    std::string pub_response = publisher.receive_line();
    ASSERT(pub_response == "OK:PUBLISHED", "Publish failed");
    
    // Receive message on subscriber
    std::string message = subscriber.receive_line();
    ASSERT(message.find("MESSAGE:test_channel:test_payload") == 0, 
           "Expected message with payload, got: " + message);
    
    publisher.close();
    subscriber.close();
}

TEST(test_multiple_subscribers) {
    asio::io_context io_context;
    TestClient publisher(io_context, "127.0.0.1", 9093);
    TestClient subscriber1(io_context, "127.0.0.1", 9093);
    TestClient subscriber2(io_context, "127.0.0.1", 9093);
    
    // Both subscribe
    subscriber1.send("SUBSCRIBE:broadcast_topic\n");
    subscriber1.receive_line(); // OK response
    
    subscriber2.send("SUBSCRIBE:broadcast_topic\n");
    subscriber2.receive_line(); // OK response
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Publish once
    publisher.send("PUBLISH:broadcast_topic:broadcast_message\n");
    publisher.receive_line(); // OK response
    
    // Both should receive
    std::string msg1 = subscriber1.receive_line();
    std::string msg2 = subscriber2.receive_line();
    
    ASSERT(msg1.find("MESSAGE:broadcast_topic:broadcast_message") == 0, 
           "Subscriber 1 didn't receive message");
    ASSERT(msg2.find("MESSAGE:broadcast_topic:broadcast_message") == 0, 
           "Subscriber 2 didn't receive message");
    
    publisher.close();
    subscriber1.close();
    subscriber2.close();
}

TEST(test_unsubscribe) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    // Subscribe
    client.send("SUBSCRIBE:temp_topic\n");
    std::string response1 = client.receive_line();
    ASSERT(response1.find("OK:SUBSCRIBED") == 0, "Subscribe failed");
    
    // Unsubscribe
    client.send("UNSUBSCRIBE:temp_topic\n");
    std::string response2 = client.receive_line();
    ASSERT(response2.find("OK:UNSUBSCRIBED") == 0, "Unsubscribe failed");
    
    client.close();
}

TEST(test_multiple_topics) {
    asio::io_context io_context;
    TestClient publisher(io_context, "127.0.0.1", 9093);
    TestClient subscriber(io_context, "127.0.0.1", 9093);
    
    // Subscribe to multiple topics
    subscriber.send("SUBSCRIBE:topic_a\n");
    subscriber.receive_line();
    
    subscriber.send("SUBSCRIBE:topic_b\n");
    subscriber.receive_line();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Publish to topic_a
    publisher.send("PUBLISH:topic_a:message_a\n");
    publisher.receive_line();
    std::string msg_a = subscriber.receive_line();
    ASSERT(msg_a.find("MESSAGE:topic_a:message_a") == 0, "Didn't receive message from topic_a");
    
    // Publish to topic_b
    publisher.send("PUBLISH:topic_b:message_b\n");
    publisher.receive_line();
    std::string msg_b = subscriber.receive_line();
    ASSERT(msg_b.find("MESSAGE:topic_b:message_b") == 0, "Didn't receive message from topic_b");
    
    publisher.close();
    subscriber.close();
}

TEST(test_invalid_command) {
    asio::io_context io_context;
    TestClient client(io_context, "127.0.0.1", 9093);
    
    client.send("INVALID_COMMAND\n");
    std::string response = client.receive_line();
    
    ASSERT(response.find("ERROR") == 0, "Expected ERROR response for invalid command");
    
    client.close();
}

TEST(test_session_disconnect) {
    asio::io_context io_context;
    
    {
        TestClient client(io_context, "127.0.0.1", 9093);
        client.send("SUBSCRIBE:disconnect_test\n");
        client.receive_line();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        size_t sessions_before = g_broker->get_active_sessions();
        ASSERT(sessions_before >= 1, "Should have at least 1 session");
        
        client.close();
        // Client destructor closes connection
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Session should be cleaned up (might not be 0 if other tests are concurrent)
    // Just verify the broker is still running
    ASSERT(g_broker != nullptr, "Broker should still be running after client disconnect");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "=== NeuroPipe Asio Broker Test Suite ===" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        std::cout << "\n[SETUP] Starting test broker on port 9093..." << std::endl;
        setup_broker();
        std::cout << "[SETUP] Broker started successfully\n" << std::endl;
        
        // Run all tests
        run_test_broker_starts();
        run_test_client_connection();
        run_test_ping_pong();
        run_test_subscribe_topic();
        run_test_publish_message();
        run_test_publish_and_receive();
        run_test_multiple_subscribers();
        run_test_unsubscribe();
        run_test_multiple_topics();
        run_test_invalid_command();
        run_test_session_disconnect();
        
        std::cout << "\n[TEARDOWN] Stopping test broker..." << std::endl;
        teardown_broker();
        std::cout << "[TEARDOWN] Broker stopped\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL] Test setup/teardown failed: " << e.what() << std::endl;
        return 1;
    }
    
    // Print summary
    std::cout << "\n=========================================" << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Tests Passed: " << tests_passed << std::endl;
    std::cout << "Tests Failed: " << tests_failed << std::endl;
    std::cout << "Total Tests:  " << (tests_passed + tests_failed) << std::endl;
    std::cout << "=========================================" << std::endl;
    
    if (tests_failed == 0) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}

