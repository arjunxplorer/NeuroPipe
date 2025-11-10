# Makefile for NeuroPipe
# Alternative to CMake for quick builds

CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Iinclude -Isrc -Ilib -Ithird_party/include
LDFLAGS = -pthread

BUILD_DIR = build
SRC_DIR = src
TEST_DIR = tests
INCLUDE_DIR = include

# Targets
BROKER = $(BUILD_DIR)/broker
BROKER_LEGACY = $(BUILD_DIR)/broker_legacy
PRODUCER = $(BUILD_DIR)/producer_client
CONSUMER = $(BUILD_DIR)/consumer_client
TEST_BASIC = $(BUILD_DIR)/test_basic
TEST_ASIO = $(BUILD_DIR)/test_asio_broker
SIMPLE_APP = $(BUILD_DIR)/simple_app
ROBUST_APP = $(BUILD_DIR)/robust_app
DEBUG_LOGGER_LIB = $(BUILD_DIR)/libdebug_logger.a

# Source files (Asio-based)
BROKER_SRCS = $(SRC_DIR)/broker.cpp $(SRC_DIR)/asio_server.cpp
BROKER_LEGACY_SRCS = $(SRC_DIR)/broker_legacy.cpp $(SRC_DIR)/server.cpp
PRODUCER_SRCS = $(SRC_DIR)/producer.cpp
CONSUMER_SRCS = $(SRC_DIR)/consumer.cpp
TEST_BASIC_SRCS = $(TEST_DIR)/test_basic.cpp
TEST_ASIO_SRCS = $(TEST_DIR)/test_asio_broker.cpp $(SRC_DIR)/asio_server.cpp
DEBUG_LOGGER_SRCS = lib/debug_logger.cpp
SIMPLE_APP_SRCS = examples/simple_app.cpp
ROBUST_APP_SRCS = examples/robust_app.cpp

.PHONY: all clean test run-broker run-producer run-consumer legacy examples dashboard

# Default target (Asio version)
all: $(BUILD_DIR) $(BROKER) $(PRODUCER) $(CONSUMER) $(TEST_BASIC) $(TEST_ASIO)

# Build examples
examples: $(BUILD_DIR) $(DEBUG_LOGGER_LIB) $(SIMPLE_APP) $(ROBUST_APP)

# Build legacy version
legacy: $(BUILD_DIR) $(BROKER_LEGACY)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build Asio broker
$(BROKER): $(BROKER_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(BROKER_SRCS) -o $(BROKER)

# Build legacy broker
$(BROKER_LEGACY): $(BROKER_LEGACY_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(BROKER_LEGACY_SRCS) -o $(BROKER_LEGACY)

# Build producer client
$(PRODUCER): $(PRODUCER_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(PRODUCER_SRCS) -o $(PRODUCER)

# Build consumer client
$(CONSUMER): $(CONSUMER_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(CONSUMER_SRCS) -o $(CONSUMER)

# Build basic test
$(TEST_BASIC): $(TEST_BASIC_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TEST_BASIC_SRCS) -o $(TEST_BASIC)

# Build Asio test
$(TEST_ASIO): $(TEST_ASIO_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(TEST_ASIO_SRCS) -o $(TEST_ASIO)

# Build debug logger library
$(DEBUG_LOGGER_LIB): $(DEBUG_LOGGER_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $(DEBUG_LOGGER_SRCS) -o $(BUILD_DIR)/debug_logger.o
	ar rcs $(DEBUG_LOGGER_LIB) $(BUILD_DIR)/debug_logger.o

# Build simple app example
$(SIMPLE_APP): $(SIMPLE_APP_SRCS) $(DEBUG_LOGGER_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SIMPLE_APP_SRCS) $(DEBUG_LOGGER_LIB) -o $(SIMPLE_APP)

# Build robust app example
$(ROBUST_APP): $(ROBUST_APP_SRCS) $(DEBUG_LOGGER_LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(ROBUST_APP_SRCS) $(DEBUG_LOGGER_LIB) -o $(ROBUST_APP)

# Run tests
test: $(TEST_BASIC) $(TEST_ASIO)
	@echo "Running basic tests..."
	@./$(TEST_BASIC)
	@echo ""
	@echo "Running Asio broker tests..."
	@./$(TEST_ASIO)

# Run broker
run-broker: $(BROKER)
	@echo "Starting NeuroPipe broker on port 9092..."
	@./$(BROKER)

# Run producer
run-producer: $(PRODUCER)
	@echo "Starting producer client..."
	@./$(PRODUCER)

# Run consumer
run-consumer: $(CONSUMER)
	@echo "Starting consumer client..."
	@./$(CONSUMER)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Run simple app example
run-simple-app: $(SIMPLE_APP)
	@echo "Starting simple app with debug logging..."
	@echo "View logs with: ./dashboards/view_all.sh"
	@./$(SIMPLE_APP)

# Run robust app example
run-robust-app: $(ROBUST_APP)
	@echo "Starting robust app (graceful degradation)..."
	@echo "View logs with: ./dashboards/view_all.sh"
	@./$(ROBUST_APP)

# Run edge case tests
test-edge-cases: all examples
	@echo "Running edge case test suite..."
	@./tests/test_edge_cases.sh

# Help
help:
	@echo "NeuroPipe Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all             - Build all executables (default)"
	@echo "  examples        - Build example applications"
	@echo "  test            - Build and run tests"
	@echo "  clean           - Remove build artifacts"
	@echo "  rebuild         - Clean and rebuild everything"
	@echo "  run-broker      - Run the broker server"
	@echo "  run-producer    - Run the producer client"
	@echo "  run-consumer    - Run the consumer client"
	@echo "  run-simple-app     - Run example with debug logging"
	@echo "  run-robust-app     - Run robust example (graceful degradation)"
	@echo "  test-edge-cases    - Run comprehensive edge case tests"
	@echo "  help               - Show this help message"

