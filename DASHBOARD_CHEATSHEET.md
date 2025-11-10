# 📋 Live Debugging Dashboard - Quick Reference

**One-page cheat sheet for daily use**

---

## 🚀 Start System (3 Commands)

```bash
./build/broker                # Terminal 1: Start broker
./dashboards/view_all.sh      # Terminal 2: View logs
./build/simple_app            # Terminal 3: Run app
```

---

## 🎨 Dashboard Views

| Command | Shows | Color |
|---------|-------|-------|
| `./dashboards/view_all.sh` | Everything | All colors |
| `./dashboards/view_errors.sh` | Errors + warnings | Red/Yellow |
| `./dashboards/view_metrics.sh` | Metrics only | Blue |
| `./dashboards/view_service.sh <name>` | One service | All colors |

---

## 💻 Code Integration

### Include & Create
```cpp
#include "lib/debug_logger.hpp"

DebugLogger logger("my_service");
```

### Log Messages
```cpp
logger.info("User logged in");           // 🟢 Green
logger.debug("Cache hit for key=123");   // 🔵 Cyan
logger.warn("Memory usage high: 85%");   // 🟡 Yellow
logger.error("Database connection lost"); // 🔴 Red
```

### Log Metrics
```cpp
logger.metric("response_time_ms", 123);   // 📊 Numbers
logger.metric("cpu_percent", 45.7);       // 📊 Decimals
logger.metric("status", "healthy");       // 📊 Text
```

### Compile
```bash
clang++ -std=c++20 -Ilib my_app.cpp build/libdebug_logger.a -o my_app -pthread
```

---

## 🔧 Common Operations

### Save Logs to File
```bash
./dashboards/view_all.sh | tee logs.txt
./dashboards/view_errors.sh | tee errors.txt
```

### Filter Logs
```bash
./dashboards/view_all.sh | grep "order_123"
./dashboards/view_all.sh | grep ERROR
```

### Multiple Views at Once
```bash
./dashboards/view_all.sh &
./dashboards/view_errors.sh &
./dashboards/view_metrics.sh &
```

---

## 🛠️ Build Commands

```bash
make examples          # Build example apps
make clean            # Clean build artifacts
make rebuild          # Clean + rebuild
make help             # Show all targets
```

---

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| Nothing shows | Check broker running: `./build/broker` |
| Connection failed | Start broker first |
| Permission denied | `chmod +x dashboards/*.sh` |
| Build error | `make clean && make examples` |

---

## 📊 Logger API

| Method | Purpose | Example |
|--------|---------|---------|
| `info(msg)` | Normal logs | `logger.info("Starting")` |
| `debug(msg)` | Debug details | `logger.debug("Cache hit")` |
| `warn(msg)` | Warnings | `logger.warn("High CPU")` |
| `error(msg)` | Errors | `logger.error("Failed")` |
| `metric(name, val)` | Metrics | `logger.metric("latency", 50)` |
| `is_connected()` | Check status | `if (logger.is_connected())` |
| `reconnect()` | Reconnect | `logger.reconnect()` |

---

## 🎯 Common Patterns

### Check Connection
```cpp
DebugLogger logger("my_app");
if (!logger.is_connected()) {
    std::cerr << "Broker not available!\n";
    return 1;
}
```

### Measure Performance
```cpp
auto start = std::chrono::steady_clock::now();
// ... do work ...
auto end = std::chrono::steady_clock::now();
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
logger.metric("operation_time_ms", static_cast<int>(ms.count()));
```

### Error Handling
```cpp
try {
    do_something();
    logger.info("Operation successful");
} catch (const std::exception& e) {
    logger.error("Operation failed: " + std::string(e.what()));
}
```

---

## 📁 File Locations

```
lib/debug_logger.hpp          # Include this
lib/debug_logger.cpp          # Implementation
build/libdebug_logger.a       # Link against this
examples/simple_app.cpp       # Example code
dashboards/*.sh               # Dashboard scripts
README_DASHBOARD.md           # Full documentation
```

---

## 🌐 Default Settings

| Setting | Value | Change in Code |
|---------|-------|----------------|
| Broker host | `127.0.0.1` | Constructor arg 2 |
| Broker port | `9092` | Constructor arg 3 |
| Topics | `debug, errors, warnings, metrics` | Auto-created |
| Latency | `<1ms` | N/A |

---

## 💡 Pro Tips

1. **Keep dashboard running** → Catch issues instantly
2. **Use grep for search** → `view_all.sh | grep "error"`
3. **Save important logs** → `view_errors.sh | tee errors.log`
4. **Multiple terminals** → Everyone can have their view
5. **Add context to logs** → Include IDs, values, timestamps

---

## 🎓 Example: Complete App

```cpp
#include "lib/debug_logger.hpp"
#include <thread>

int main() {
    DebugLogger logger("payment_svc");
    
    logger.info("Service starting");
    
    for (int i = 1; i <= 5; i++) {
        logger.info("Processing payment #" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (i == 3) {
            logger.error("Payment #3 failed");
        } else {
            logger.metric("amount", i * 100);
        }
    }
    
    logger.info("Service stopping");
    return 0;
}
```

**Compile**: `clang++ -std=c++20 -Ilib app.cpp build/libdebug_logger.a -o app -pthread`  
**Run**: `./app` (with broker and dashboard already running)

---

## 📚 Documentation

| Need | Read |
|------|------|
| Quick start | `GETTING_STARTED_DASHBOARD.md` |
| Full guide | `README_DASHBOARD.md` |
| Architecture | `DASHBOARD_ARCHITECTURE.md` |
| What's built | `DASHBOARD_COMPLETE.md` |
| This cheat sheet | `DASHBOARD_CHEATSHEET.md` |

---

## ✅ Quick Check

**System Working?**
```bash
# 1. Broker running?
ps aux | grep broker

# 2. Test connection
echo "PING" | nc localhost 9092
# Should respond: PONG

# 3. Run example
./build/simple_app

# 4. See logs?
./dashboards/view_all.sh
```

---

**Keep this handy!** Print or bookmark for daily reference. 📌

*NeuroPipe Dashboard - November 2025*

