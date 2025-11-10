# 🎯 NeuroPipe Live Debugging Dashboard - Quick Start

Welcome to the NeuroPipe Live Debugging Dashboard! This guide will have you up and running in **5 minutes**.

---

## 🚀 Quick Start (3 Steps)

### Step 1: Build Everything

```bash
cd /Users/arjunsharma/Desktop/NeuroPipe
make examples
```

**Output**:
```
clang++ ... building debug_logger library
clang++ ... building simple_app
✓ Build complete!
```

### Step 2: Start the Broker

**Terminal 1**:
```bash
./build/broker
```

**You should see**:
```
=== NeuroPipe Broker Running ===
Port: 9092
Backend: Standalone Asio
```

### Step 3: Run Example App + Dashboard

**Terminal 2** (View logs):
```bash
./dashboards/view_all.sh
```

**Terminal 3** (Run example):
```bash
./build/simple_app
```

**🎉 You should see logs appearing in Terminal 2 in real-time!**

---

## 📊 Dashboard Views

### View All Logs
```bash
./dashboards/view_all.sh
```

Shows everything:
- 🟢 INFO messages (green)
- ⚠️ WARN messages (yellow)
- 🔴 ERROR messages (red)
- 📊 METRICS (blue)

### View Errors Only
```bash
./dashboards/view_errors.sh
```

Shows only:
- 🔴 Errors
- ⚠️ Warnings

### View Metrics Only
```bash
./dashboards/view_metrics.sh
```

Shows performance metrics:
- ⚡ Fast operations (< 50ms)
- ⏱️ Slow operations (> 200ms)
- 📈 Statistics

### View Specific Service
```bash
./dashboards/view_service.sh simple_app
```

Shows logs from one service only.

---

## 💻 Using in Your Application

### 1. Include the Library

Add to your `.cpp` file:
```cpp
#include "lib/debug_logger.hpp"
```

### 2. Create Logger Instance

```cpp
// In your main() or class
DebugLogger logger("my_service");
```

### 3. Add Logging

```cpp
// Info logs
logger.info("Application started");
logger.info("Processing request #123");

// Debug logs
logger.debug("Cache hit for key: user_456");

// Warnings
logger.warn("High memory usage: 85%");

// Errors
logger.error("Database connection failed");

// Metrics
logger.metric("response_time_ms", 123.45);
logger.metric("active_users", 42);
logger.metric("cpu_usage_percent", 35.2);
```

### 4. Compile Your App

**With Makefile**:
```makefile
my_app: my_app.cpp
	$(CXX) $(CXXFLAGS) my_app.cpp build/libdebug_logger.a -o my_app
```

**With CMake**:
```cmake
add_executable(my_app my_app.cpp)
target_link_libraries(my_app PRIVATE debug_logger)
```

**Direct compilation**:
```bash
clang++ -std=c++20 -Ilib my_app.cpp build/libdebug_logger.a -o my_app -pthread
```

---

## 🎬 Real-World Example

### Complete Application

```cpp
#include "lib/debug_logger.hpp"
#include <thread>
#include <chrono>

void process_order(DebugLogger& logger, int order_id) {
    logger.info("Processing order #" + std::to_string(order_id));
    
    auto start = std::chrono::steady_clock::now();
    
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check for errors
    if (order_id % 10 == 0) {
        logger.error("Payment failed for order #" + std::to_string(order_id));
        return;
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    logger.metric("order_processing_ms", duration.count());
    logger.info("Order completed #" + std::to_string(order_id));
}

int main() {
    DebugLogger logger("order_service");
    
    if (!logger.is_connected()) {
        std::cerr << "Failed to connect to broker!" << std::endl;
        return 1;
    }
    
    logger.info("Order service started");
    
    for (int i = 1; i <= 20; i++) {
        process_order(logger, i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    logger.info("Order service shutting down");
    
    return 0;
}
```

### Running It

**Terminal 1**: `./build/broker`
**Terminal 2**: `./dashboards/view_all.sh`
**Terminal 3**: `./my_order_service`

**Terminal 2 shows live**:
```
[14:23:45] [INFO] order_service: Order service started
[14:23:45] [INFO] order_service: Processing order #1
[14:23:45] order_processing_ms=102
[14:23:45] [INFO] order_service: Order completed #1
[14:23:46] [INFO] order_service: Processing order #2
...
[14:23:55] [ERROR] order_service: Payment failed for order #10
```

---

## 🎯 Use Cases

### 1. Debugging Production Issues
```bash
# Leave running on production server
./dashboards/view_errors.sh | tee -a production_errors.log
```

### 2. Performance Monitoring
```bash
# Watch metrics in real-time
./dashboards/view_metrics.sh
```

### 3. Multi-Service Development
```bash
# Terminal 1: All logs
./dashboards/view_all.sh

# Terminal 2: Only service A
./dashboards/view_service.sh service_a

# Terminal 3: Only service B
./dashboards/view_service.sh service_b
```

### 4. Load Testing
```bash
# Watch metrics during load test
./dashboards/view_metrics.sh &
ab -n 1000 -c 10 http://localhost:8080/
```

---

## ⚙️ Configuration

### Change Broker Location

```cpp
// Default: localhost:9092
DebugLogger logger("my_service");

// Custom broker
DebugLogger logger("my_service", "192.168.1.100", 9092);
```

### Logging Levels

```cpp
logger.debug("Detailed info");      // Development only
logger.info("Normal operation");    // Standard
logger.warn("Potential problem");   // Attention needed
logger.error("Something failed");   // Action required
```

### Custom Topics

```cpp
// Publish to custom topics
logger.publish("security", "Login attempt from suspicious IP");
logger.publish("analytics", "Page view: /products/123");
```

Then view with:
```bash
./build/consumer_client localhost 9092 security analytics
```

---

## 🔧 Troubleshooting

### "Failed to connect to broker"

**Solution**: Make sure broker is running
```bash
./build/broker
```

### "No logs appearing"

**Checklist**:
1. ✅ Broker running?
2. ✅ Dashboard script executable? (`chmod +x dashboards/*.sh`)
3. ✅ App connected successfully?
4. ✅ Actually calling `logger.info()` etc?

### "Build failed"

```bash
# Clean and rebuild
make clean
make examples
```

---

## 📋 API Reference

### DebugLogger Methods

| Method | Purpose | Example |
|--------|---------|---------|
| `info(msg)` | Normal log | `logger.info("User logged in")` |
| `debug(msg)` | Debug details | `logger.debug("Cache hit")` |
| `warn(msg)` | Warnings | `logger.warn("High CPU")` |
| `error(msg)` | Errors | `logger.error("DB failed")` |
| `metric(name, val)` | Metrics | `logger.metric("latency_ms", 123)` |
| `publish(topic, msg)` | Custom | `logger.publish("alerts", "...")` |
| `is_connected()` | Check status | `if (!logger.is_connected())` |
| `reconnect()` | Reconnect | `logger.reconnect()` |

---

## 🎓 Best Practices

### ✅ DO

- Log important events and state changes
- Use appropriate log levels
- Include context (IDs, values) in messages
- Log metrics for performance tracking
- Use descriptive service names

### ❌ DON'T

- Log sensitive data (passwords, keys)
- Log in tight loops (causes spam)
- Log without context ("Error!" vs "DB connection failed: timeout")
- Mix log levels incorrectly

---

## 🚀 Next Steps

1. ✅ **You've set up the dashboard!**
2. 📝 **Add logging to your apps**
3. 🔍 **Use it to debug real issues**
4. 📊 **Monitor your application health**
5. 🎯 **Share with your team**

---

## 📚 More Examples

Check out:
- `examples/simple_app.cpp` - Basic example
- `LIVE_DASHBOARD_PLAN.md` - Full plan and roadmap
- `ASIO_QUICKSTART.md` - NeuroPipe broker guide

---

## 💡 Pro Tips

1. **Keep dashboard running during development** - catch issues immediately
2. **Use metrics for optimization** - see what's slow
3. **Filter with grep** - `./dashboards/view_all.sh | grep "order_123"`
4. **Save logs** - `./dashboards/view_errors.sh | tee errors.log`
5. **Multiple viewers** - run several dashboards simultaneously

---

## 🎉 You're Ready!

You now have a **production-grade debugging dashboard** at your fingertips!

**Questions? Issues?** Check `LIVE_DASHBOARD_PLAN.md` for more details.

---

*Happy Debugging!* 🐛🔍

