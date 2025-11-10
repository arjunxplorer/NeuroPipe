# NeuroPipe

**A high-performance distributed message broker for real-time communication and debugging**

## Overview

NeuroPipe is a lightweight, production-ready TCP message broker built in C++20 that implements the publish-subscribe pattern for distributed systems. It enables real-time log aggregation, event streaming, and inter-service communication with automatic reconnection and graceful degradation.

## Key Features

- **Asynchronous I/O** - Built on standalone Asio for high-performance non-blocking operations
- **Pub/Sub Architecture** - Topic-based message routing with multiple subscribers
- **Language Agnostic** - Simple TCP protocol works with any programming language
- **Thread-Safe** - Concurrent operations with mutex-protected shared resources
- **Auto-Reconnect** - Automatic connection recovery on failure
- **Live Debugging Dashboard** - Real-time log visualization with severity filtering
- **Production Ready** - Comprehensive edge case handling and automated testing

## Quick Start

### Build

```bash
make all                # Build broker and clients
make examples           # Build example applications
```

### Run

```bash
# Terminal 1: Start broker
./build/broker

# Terminal 2: View live logs
./dashboards/view_all.sh

# Terminal 3: Run example app
./build/robust_app
```

## Architecture

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Publisher  │───────▶ │   Broker    │ ───────▶│ Subscriber  │
│   (Client)  │         │   (Server)  │         │   (Client)  │
└─────────────┘         └─────────────┘         └─────────────┘
                               │
                        Topic Routing
                     (debug, errors, metrics)
```

## Use Cases

- **Distributed Logging** - Centralized log aggregation from microservices
- **Real-Time Monitoring** - Live metrics and performance tracking
- **Event Streaming** - Inter-service communication and event broadcasting
- **Debugging** - Real-time debugging dashboard for production environments

## Technology Stack

- **Language**: C++20
- **Networking**: Standalone Asio (header-only)
- **Build System**: Make / CMake
- **Protocol**: Custom TCP-based pub/sub protocol
- **Testing**: Automated edge case test suite (11 tests)

## Project Structure

```
NeuroPipe/
├── src/                    # Core broker implementation
│   ├── broker.cpp         # Main broker entry point
│   ├── asio_server.cpp    # Async networking layer
│   ├── producer.cpp       # Publisher client
│   └── consumer.cpp       # Subscriber client
├── lib/                    # Client library
│   ├── debug_logger.hpp   # C++ logging API
│   └── debug_logger.cpp   # Logger implementation
├── examples/              # Example applications
│   ├── simple_app.cpp     # Basic usage example
│   └── robust_app.cpp     # Production pattern demo
├── dashboards/            # Live monitoring scripts
│   ├── view_all.sh        # View all logs
│   ├── view_errors.sh     # Error-only view
│   └── view_metrics.sh    # Metrics dashboard
└── tests/                 # Test suite
    └── test_edge_cases.sh # Comprehensive testing

```



### Protocol (Language Agnostic)

```bash
# Subscribe to topic
echo "SUBSCRIBE:debug" | nc localhost 9092

# Publish message
echo "PUBLISH:debug:Hello World" | nc localhost 9092

# Ping broker
echo "PING" | nc localhost 9092
```

## Building from Source

### Prerequisites

- C++20 compatible compiler (clang++ or g++)
- Make or CMake
- Standalone Asio (included in `third_party/`)

### Build Commands

```bash
# Using Make
make clean
make all
make examples

# Using CMake
mkdir build && cd build
cmake ..
make

# Run tests
make test-edge-cases
```

## Testing

Comprehensive edge case testing covering:
- Connection handling (timeout, reconnect, disconnect)
- Protocol validation (malformed commands, bounds checking)
- Concurrency (thread safety, race conditions)
- Special characters (escaping, delimiter handling)
- Performance (large messages, rapid connections)

```bash
# Run full test suite
./tests/test_edge_cases.sh

# Expected: All 11 tests pass
```

## Performance

- **Concurrent Connections**: 100+ simultaneous clients
- **Message Throughput**: 500+ messages/second
- **Thread Safety**: 10 threads, 500 messages verified
- **Latency**: Sub-millisecond message routing

## Contributing

This is a portfolio project demonstrating advanced C++ systems programming, distributed architecture, and production-ready software engineering practices.

## License

Educational/Portfolio Project

---

**Built with modern C++20 for high-performance distributed systems.**
