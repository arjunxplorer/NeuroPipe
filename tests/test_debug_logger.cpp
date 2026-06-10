/**
 * Unit and integration tests for DebugLogger
 *
 * Tests: message escaping, connection, log levels, metrics, thread safety
 * Spins up a real broker and verifies what DebugLogger sends over the wire.
 */

#define ASIO_STANDALONE
#include <asio.hpp>
#include "../lib/debug_logger.hpp"
#include "../src/asio_server.hpp"
#include "../src/utils.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <atomic>
#include <vector>
#include <sstream>

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

// Test subscriber that captures messages
class TestSubscriber {
public:
    TestSubscriber(asio::io_context& io_context, const std::string& host,
                   uint16_t port, const std::string& topic)
        : socket_(io_context), resolver_(io_context) {
        auto endpoints = resolver_.resolve(host, std::to_string(port));
        asio::connect(socket_, endpoints);

        // Subscribe to topic
        std::string sub_cmd = "SUBSCRIBE:" + topic + "\n";
        asio::write(socket_, asio::buffer(sub_cmd));

        // Read OK response
        asio::streambuf resp;
        asio::read_until(socket_, resp, '\n');
    }

    // Read a message with timeout (returns empty string on timeout)
    std::string read_message(int timeout_ms = 1000) {
        asio::error_code ec;
        asio::streambuf buffer;

        // Set a deadline
        socket_.non_blocking(true);
        auto start = std::chrono::steady_clock::now();

        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > timeout_ms) {
                socket_.non_blocking(false);
                return "";
            }

            size_t bytes = socket_.read_some(buffer.prepare(4096), ec);
            if (ec == asio::error::would_block) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (ec) {
                socket_.non_blocking(false);
                return "";
            }

            buffer.commit(bytes);
            socket_.non_blocking(false);

            std::istream is(&buffer);
            std::string line;
            std::getline(is, line);
            return line;
        }
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

// Broker infrastructure
std::unique_ptr<BrokerServer> g_broker;
std::unique_ptr<asio::io_context> g_io_context;
std::unique_ptr<std::thread> g_io_thread;
const uint16_t TEST_PORT = 9094;

void setup_broker() {
    g_io_context = std::make_unique<asio::io_context>();
    g_broker = std::make_unique<BrokerServer>(*g_io_context, TEST_PORT);
    g_broker->start();

    g_io_thread = std::make_unique<std::thread>([]() {
        g_io_context->run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void teardown_broker() {
    if (g_broker) g_broker->stop();
    if (g_io_context) g_io_context->stop();
    if (g_io_thread && g_io_thread->joinable()) g_io_thread->join();
    g_broker.reset();
    g_io_context.reset();
    g_io_thread.reset();
}

// ============================================================================
// Test Cases
// ============================================================================

TEST(test_connection) {
    DebugLogger logger("test_service", "127.0.0.1", TEST_PORT);
    ASSERT(logger.is_connected(), "Should be connected to broker");
}

TEST(test_connection_failure) {
    DebugLogger logger("test_service", "127.0.0.1", 19999);  // Wrong port
    ASSERT(!logger.is_connected(), "Should not be connected to non-existent broker");
}

TEST(test_info_message) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.info("hello world");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive a message");
    ASSERT(msg.find("MESSAGE:debug:") == 0, "Should be on 'debug' topic");
    ASSERT(msg.find("hello world") != std::string::npos, "Should contain the message text");
    ASSERT(msg.find("[INFO]") != std::string::npos, "Should contain [INFO] level");
    ASSERT(msg.find("test_svc:") != std::string::npos, "Should contain service name");

    sub.close();
}

TEST(test_warn_message) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber debug_sub(sub_io, "127.0.0.1", TEST_PORT, "debug");
    TestSubscriber warn_sub(sub_io, "127.0.0.1", TEST_PORT, "warnings");

    logger.warn("disk full");
    std::string debug_msg = debug_sub.read_message(2000);
    std::string warn_msg = warn_sub.read_message(2000);

    ASSERT(!debug_msg.empty(), "debug topic should receive warn message");
    ASSERT(!warn_msg.empty(), "warnings topic should receive warn message");
    ASSERT(warn_msg.find("[WARN]") != std::string::npos, "Should contain [WARN] level");

    debug_sub.close();
    warn_sub.close();
}

TEST(test_error_message) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber debug_sub(sub_io, "127.0.0.1", TEST_PORT, "debug");
    TestSubscriber err_sub(sub_io, "127.0.0.1", TEST_PORT, "errors");

    logger.error("connection failed");
    std::string debug_msg = debug_sub.read_message(2000);
    std::string err_msg = err_sub.read_message(2000);

    ASSERT(!debug_msg.empty(), "debug topic should receive error message");
    ASSERT(!err_msg.empty(), "errors topic should receive error message");
    ASSERT(err_msg.find("[ERROR]") != std::string::npos, "Should contain [ERROR] level");

    debug_sub.close();
    err_sub.close();
}

TEST(test_debug_message) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.debug("trace info");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive debug message");
    ASSERT(msg.find("[DEBUG]") != std::string::npos, "Should contain [DEBUG] level");

    sub.close();
}

TEST(test_metric_double) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "metrics");

    logger.metric("latency_ms", 42.5);
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive metric message");
    ASSERT(msg.find("MESSAGE:metrics:") == 0, "Should be on 'metrics' topic");
    ASSERT(msg.find("latency_ms=42.50") != std::string::npos, "Should contain metric name=value");

    sub.close();
}

TEST(test_metric_int) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "metrics");

    logger.metric("connections", 15);
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive metric message");
    ASSERT(msg.find("connections=15") != std::string::npos, "Should contain metric name=value");

    sub.close();
}

TEST(test_metric_string) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "metrics");

    logger.metric("status", "healthy");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive metric message");
    ASSERT(msg.find("status=healthy") != std::string::npos, "Should contain metric name=value");

    sub.close();
}

TEST(test_escape_newlines) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.info("line1\nline2\nline3");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive message");
    // The escaped newlines should appear as literal \n in the message
    ASSERT(msg.find("line1\\nline2\\nline3") != std::string::npos,
           "Newlines should be escaped. Got: " + msg);

    sub.close();
}

TEST(test_escape_colons) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.info("key:value:pair");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive message");
    // Colons should be escaped to avoid breaking protocol parsing
    ASSERT(msg.find("key\\:value\\:pair") != std::string::npos,
           "Colons should be escaped. Got: " + msg);

    sub.close();
}

TEST(test_escape_backslashes) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.info("path\\to\\file");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive message");
    // Backslashes should be escaped
    ASSERT(msg.find("path\\\\to\\\\file") != std::string::npos,
           "Backslashes should be escaped. Got: " + msg);

    sub.close();
}

TEST(test_reconnect) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);
    ASSERT(logger.is_connected(), "Should be connected initially");

    // Reconnect should succeed while broker is running
    bool ok = logger.reconnect();
    ASSERT(ok, "Reconnect should succeed");
    ASSERT(logger.is_connected(), "Should be connected after reconnect");

    // Should still be able to send messages
    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    logger.info("after reconnect");
    std::string msg = sub.read_message(2000);
    ASSERT(!msg.empty(), "Should receive message after reconnect");

    sub.close();
}

TEST(test_publish_raw) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "custom_topic");

    logger.publish("custom_topic", "raw payload");
    std::string msg = sub.read_message(2000);

    ASSERT(!msg.empty(), "Should receive message");
    ASSERT(msg.find("MESSAGE:custom_topic:") == 0, "Should be on custom topic");
    ASSERT(msg.find("raw payload") != std::string::npos, "Should contain raw payload");

    sub.close();
}

TEST(test_thread_safety) {
    DebugLogger logger("test_svc", "127.0.0.1", TEST_PORT);

    asio::io_context sub_io;
    TestSubscriber sub(sub_io, "127.0.0.1", TEST_PORT, "debug");

    const int num_threads = 5;
    const int messages_per_thread = 20;
    std::atomic<int> sent_count(0);

    // Launch threads that all log concurrently
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&logger, &sent_count, t]() {
            for (int i = 0; i < messages_per_thread; i++) {
                logger.info("thread=" + std::to_string(t) + " msg=" + std::to_string(i));
                sent_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    ASSERT(sent_count == num_threads * messages_per_thread,
           "All messages should be sent without crashes");

    // Verify we can receive at least some messages (they may come on the same connection)
    int received = 0;
    for (int i = 0; i < 50; i++) {
        std::string msg = sub.read_message(200);
        if (!msg.empty()) received++;
        else break;
    }

    ASSERT(received > 0, "Should receive at least some messages from concurrent senders");

    sub.close();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "=== NeuroPipe DebugLogger Test Suite ===" << std::endl;
    std::cout << "============================================" << std::endl;

    try {
        std::cout << "\n[SETUP] Starting test broker on port " << TEST_PORT << "..." << std::endl;
        setup_broker();
        std::cout << "[SETUP] Broker started\n" << std::endl;

        run_test_connection();
        run_test_connection_failure();
        run_test_info_message();
        run_test_warn_message();
        run_test_error_message();
        run_test_debug_message();
        run_test_metric_double();
        run_test_metric_int();
        run_test_metric_string();
        run_test_escape_newlines();
        run_test_escape_colons();
        run_test_escape_backslashes();
        run_test_reconnect();
        run_test_publish_raw();
        run_test_thread_safety();

        std::cout << "\n[TEARDOWN] Stopping broker..." << std::endl;
        teardown_broker();
        std::cout << "[TEARDOWN] Done\n" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL] Setup/teardown failed: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "============================================" << std::endl;
    std::cout << "=== Test Summary ===" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Tests Passed: " << tests_passed << std::endl;
    std::cout << "Tests Failed: " << tests_failed << std::endl;
    std::cout << "Total Tests:  " << (tests_passed + tests_failed) << std::endl;
    std::cout << "============================================" << std::endl;

    if (tests_failed == 0) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
