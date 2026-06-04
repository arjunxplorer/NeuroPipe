<div align="center">

# NeuroPipe

**A high-performance distributed message broker for real-time communication and debugging**

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus)
![License](https://img.shields.io/badge/License-Educational-green?style=flat-square)
![Build](https://img.shields.io/badge/Build-CMake%20%7C%20Make-orange?style=flat-square)
![Tests](https://img.shields.io/badge/Tests-11%20passing-brightgreen?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey?style=flat-square)

[Features](#features) · [Quick Start](#quick-start) · [Architecture](#architecture) · [Protocol](#protocol-reference) · [API](#debuglogger-api) · [Build](#building-from-source) · [Testing](#testing)

</div>

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Protocol Reference](#protocol-reference)
- [DebugLogger API](#debuglogger-api)
- [Live Dashboards](#live-dashboards)
- [Building from Source](#building-from-source)
- [Testing](#testing)
- [Performance](#performance)
- [Use Cases](#use-cases)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

NeuroPipe is a lightweight, production-ready TCP message broker built in C++20 that implements the publish-subscribe pattern for distributed systems. It enables real-time log aggregation, event streaming, and inter-service communication with automatic reconnection and graceful degradation.

The broker uses a simple, text-based protocol that is completely language-agnostic — any language capable of opening a TCP socket can publish and subscribe to messages. This makes NeuroPipe an ideal backbone for polyglot microservice architectures.

---

## Features

| Feature | Description |
|---------|-------------|
| **Asynchronous I/O** | Built on standalone Asio for high-performance non-blocking operations |
| **Pub/Sub Architecture** | Topic-based message routing with multiple concurrent subscribers |
| **Language Agnostic** | Simple newline-delimited TCP protocol — use from any language |
| **Thread-Safe** | Concurrent operations with mutex-protected shared resources |
| **Auto-Reconnect** | Client library handles connection recovery transparently |
| **Live Dashboards** | Real-time log visualization shell scripts with severity filtering |
| **Multi-Topic Routing** | Messages routed to `debug`, `errors`, `warnings`, and `metrics` topics |
| **Graceful Degradation** | Applications work with or without the broker running |
| **Comprehensive Testing** | Unit, integration, and edge case test suites |

---

## Quick Start

### 1. Build

```bash
make all         # Build broker and clients
make examples    # Build example applications
```

### 2. Run

Open three terminals:

```bash
# Terminal 1: Start the broker
./build/broker

# Terminal 2: Start a live dashboard
./dashboards/view_all.sh

# Terminal 3: Run an example application
./build/robust_app
```

You will see real-time logs, errors, warnings, and metrics flowing through the dashboard as the example application publishes messages.

### 3. Try the protocol directly

```bash
# Subscribe to all messages (in one terminal)
echo "SUBSCRIBE:debug" | nc localhost 9092

# Publish a message (in another terminal)
echo "PUBLISH:debug:Hello from NeuroPipe" | nc localhost 9092
```

---

## Architecture

```
┌────────────────┐                          ┌────────────────┐
│   Publisher    │──── PUBLISH:topic:msg ───▶│                │
│   (Client)     │◀─── OK:PUBLISHED ────────│                │
└────────────────┘                          │                │
                                            │     Broker     │
                                            │    (Server)    │
┌────────────────┐                          │                │
│  Subscriber    │◀─── MESSAGE:topic:msg ───│                │
│   (Client)     │──── SUBSCRIBE:topic ────▶│                │
└────────────────┘                          └────────────────┘
                                                     │
                                              Topic Routing
                                         ┌───────────┼───────────┐
                                         ▼           ▼           ▼
                                      debug      errors      metrics
```

### Core Components

| Component | Location | Role |
|-----------|----------|------|
| **BrokerServer** | `src/asio_server.cpp` | Async TCP server. Accepts connections, manages sessions, delegates to TopicManager |
| **Session** | `src/asio_server.cpp` | Per-client connection handler. Async read/write with a thread-safe write queue |
| **TopicManager** | `src/asio_server.cpp` | Manages topic-subscriber mappings, message queues, and sequence numbering |
| **Message** | `include/message.hpp` | Data structure: `{topic, payload, sequence, timestamp}` |
| **ThreadSafeQueue** | `src/utils.hpp` | Template queue with mutex + condition variable for producer/consumer patterns |
| **DebugLogger** | `lib/debug_logger.cpp` | Client library with auto-reconnect, message escaping, and multi-level logging |

### Data Flow

1. A **publisher** sends `PUBLISH:topic:payload\n` to the broker
2. The broker's **Session** parses the command and forwards it to **TopicManager**
3. TopicManager assigns a sequence number, timestamps the message, and queues it
4. The message is broadcast to all **subscribers** of that topic as `MESSAGE:topic:payload\n`
5. Each subscriber's **Session** writes the message asynchronously to its TCP socket

---

## Project Structure

```
NeuroPipe/
├── src/                        # Core broker implementation
│   ├── broker.cpp              # Main broker entry point (signal handling, stats)
│   ├── asio_server.hpp         # Session, TopicManager, BrokerServer declarations
│   ├── asio_server.cpp         # Async networking and protocol implementation
│   ├── producer.cpp            # Interactive publisher client
│   ├── consumer.cpp            # Subscriber client
│   ├── utils.hpp               # ThreadSafeQueue, logging utilities
│   └── server.hpp / server.cpp # Legacy POSIX socket implementation
│
├── include/
│   └── message.hpp             # Message struct definition
│
├── lib/                        # Reusable client library
│   ├── debug_logger.hpp        # DebugLogger API header
│   └── debug_logger.cpp        # Logger implementation (auto-reconnect, escaping)
│
├── examples/                   # Example applications
│   ├── simple_app.cpp          # E-commerce simulation with logging
│   └── robust_app.cpp          # Production patterns: graceful degradation + edge cases
│
├── dashboards/                 # Live monitoring shell scripts
│   ├── view_all.sh             # All topics, color-coded by severity
│   ├── view_errors.sh          # Errors and warnings only
│   ├── view_metrics.sh         # Metrics with performance threshold coloring
│   └── view_service.sh         # Filter by service name
│
├── tests/                      # Test suite
│   ├── test_basic.cpp          # Unit tests (Message, ThreadSafeQueue, logging)
│   ├── test_asio_broker.cpp    # Integration tests (11 broker tests)
│   └── test_edge_cases.sh      # End-to-end edge case suite (11 tests via netcat)
│
├── third_party/                # Vendored dependencies
│   └── include/asio/           # Standalone Asio (header-only)
│
├── CMakeLists.txt              # CMake build configuration
├── Makefile                    # Make build configuration
└── README.md
```

---

## Protocol Reference

NeuroPipe uses a simple text-based protocol. All messages are newline-delimited (`\n`).

### Commands

| Command | Format | Response |
|---------|--------|----------|
| **Publish** | `PUBLISH:<topic>:<payload>\n` | `OK:PUBLISHED\n` |
| **Subscribe** | `SUBSCRIBE:<topic>\n` | `OK:SUBSCRIBED:<topic>\n` |
| **Unsubscribe** | `UNSUBSCRIBE:<topic>\n` | `OK:UNSUBSCRIBED:<topic>\n` |
| **Ping** | `PING\n` | `PONG\n` |

### Server-to-Client Messages

| Type | Format |
|------|--------|
| **Message delivery** | `MESSAGE:<topic>:<payload>\n` |

### Error Responses

| Error | Condition |
|-------|-----------|
| `ERROR:EMPTY_MESSAGE\n` | Empty payload in PUBLISH command |
| `ERROR:INVALID_FORMAT\n` | Malformed command |
| `ERROR:EMPTY_TOPIC\n` | Missing topic in SUBSCRIBE/PUBLISH |
| `ERROR:UNKNOWN_COMMAND\n` | Unrecognized command |

### Example Session

```bash
# Connect and subscribe
$ nc localhost 9092
SUBSCRIBE:debug
OK:SUBSCRIBED:debug

# Messages arrive as they are published
MESSAGE:debug:Application started
MESSAGE:debug:Processing request #1234
MESSAGE:errors:Connection timeout to database
```

### Using from Other Languages

Since the protocol is plain TCP with text commands, integration is straightforward in any language:

```python
# Python example
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('localhost', 9092))
sock.send(b'SUBSCRIBE:debug\n')
print(sock.recv(1024).decode())  # OK:SUBSCRIBED:debug

sock.send(b'PUBLISH:debug:Hello from Python\n')
print(sock.recv(1024).decode())  # OK:PUBLISHED
```

```javascript
// Node.js example
const net = require('net');
const client = net.createConnection({ port: 9092 }, () => {
  client.write('SUBSCRIBE:debug\n');
});
client.on('data', (data) => console.log(data.toString()));
```

---

## DebugLogger API

The `DebugLogger` class provides a high-level C++ API for applications to publish logs and metrics to the broker.

### Header

```cpp
#include "debug_logger.hpp"
```

### Constructor

```cpp
DebugLogger logger("localhost", 9092, "my-service");
```

| Parameter | Description |
|-----------|-------------|
| `host` | Broker hostname |
| `port` | Broker port |
| `service_name` | Identifier for this service (included in all messages) |

### Logging Methods

```cpp
logger.info("Processing request", "order-1234");
logger.warn("Slow response from cache");
logger.error("Database connection failed");
logger.debug("Payload size: " + std::to_string(size));
```

| Method | Default Topic | Additional Topic |
|--------|---------------|------------------|
| `info()` | `debug` | — |
| `debug()` | `debug` | — |
| `warn()` | `debug` | `warnings` |
| `error()` | `debug` | `errors` |

### Metrics

```cpp
logger.metric("response_time", 42.5);    // Publishes to "metrics" topic
logger.metric("queue_depth", 128);
```

### Key Behaviors

- **Auto-Reconnect**: If the broker connection drops, the logger automatically reconnects on the next send
- **Message Escaping**: Colons and newlines in payloads are escaped to prevent protocol corruption
- **Thread-Safe**: All methods can be called concurrently from multiple threads
- **Graceful Degradation**: If the broker is unreachable, messages are silently dropped — the application continues running

---

## Live Dashboards

NeuroPipe includes shell-based dashboard scripts for real-time monitoring. All scripts use `nc` (netcat) as a subscriber client.

### Available Dashboards

| Script | Purpose | Usage |
|--------|---------|-------|
| `view_all.sh` | All topics, color-coded by severity | `./dashboards/view_all.sh` |
| `view_errors.sh` | Errors and warnings only | `./dashboards/view_errors.sh` |
| `view_metrics.sh` | Metrics with latency threshold colors | `./dashboards/view_metrics.sh` |
| `view_service.sh` | Filter by service name | `./dashboards/view_service.sh my-service` |

### Metrics Color Coding

The metrics dashboard color-codes output based on response time thresholds:

| Latency | Color | Meaning |
|---------|-------|---------|
| < 50ms | Green | Excellent |
| < 200ms | Cyan | Good |
| < 500ms | Yellow | Warning |
| > 500ms | Red | Critical |

---

## Building from Source

### Prerequisites

- **Compiler**: C++20 compatible (clang++ 13+ or g++ 10+)
- **Build System**: CMake 3.15+ or GNU Make
- **Platform**: Linux or macOS
- **Dependencies**: Standalone Asio (included in `third_party/`)

### Using Make

```bash
# Build everything
make all

# Build with specific compiler
make all CXX=g++

# Build examples only
make examples

# Clean build artifacts
make clean

# Full rebuild
make rebuild

# See all targets
make help
```

### Using CMake

```bash
mkdir build && cd build
cmake ..
make

# Build specific targets
make broker
make producer_client
make consumer_client
make simple_app
make robust_app
```

### Makefile Convenience Targets

| Target | Description |
|--------|-------------|
| `make run-broker` | Build and run the broker |
| `make run-producer` | Build and run the producer client |
| `make run-consumer` | Build and run the consumer client |
| `make run-simple-app` | Build and run the example app |
| `make run-robust-app` | Build and run the robust example app |
| `make test-edge-cases` | Run the edge case test suite |

---

## Testing

NeuroPipe includes three layers of testing:

### Unit Tests

Tests for core data structures and utilities:

```bash
# Build and run
make test_basic
./build/test_basic
```

Covers: `Message` creation, `ThreadSafeQueue` (single and multi-threaded), logging functions.

### Integration Tests

Tests the broker's protocol handling with real TCP connections:

```bash
# Build and run (uses CTest)
make test_asio_broker
./build/test_asio_broker
```

Covers (11 tests): broker startup, client connection, ping/pong, subscribe, publish, publish-and-receive, multiple subscribers, unsubscribe, multiple topics, invalid commands, session disconnect.

### Edge Case Tests

End-to-end tests using netcat against a running broker:

```bash
# Start broker first, then:
make test-edge-cases
# or
./tests/test_edge_cases.sh
```

Covers (11 tests): broker not running, empty messages, malformed commands, special characters, multiple subscribers, subscriber disconnect, rapid connections (20), large messages (5KB), broker restart resilience, publish with no subscribers, thread safety.

### Full Test Suite

```bash
# Run all tests
make test_basic && ./build/test_basic
make test_asio_broker && ./build/test_asio_broker
./tests/test_edge_cases.sh
```

---

## Performance

| Metric | Value |
|--------|-------|
| Concurrent connections | 100+ simultaneous clients |
| Message throughput | 500+ messages/second |
| Thread safety | Verified: 10 threads, 500 messages |
| Routing latency | Sub-millisecond |
| Protocol overhead | Minimal (text-based, newline-delimited) |

---

## Use Cases

- **Distributed Logging** — Centralized log aggregation from microservices across languages
- **Real-Time Monitoring** — Live metrics and performance tracking with dashboard scripts
- **Event Streaming** — Inter-service communication and event broadcasting
- **Debugging** — Real-time debugging dashboard for development and production environments
- **IoT Data Collection** — Lightweight message routing for device telemetry
- **Prototyping** — Quick pub/sub infrastructure without external dependencies

---

## Contributing

This is a portfolio project demonstrating advanced C++ systems programming, distributed architecture, and production-ready software engineering practices.

If you'd like to contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes with clear messages
4. Push to your branch and open a Pull Request

Please ensure all tests pass before submitting:

```bash
make test-edge-cases
```

---

## License

This project is an educational/portfolio project. See repository for details.

---

<div align="center">

**Built with modern C++20 for high-performance distributed systems.**

</div>
