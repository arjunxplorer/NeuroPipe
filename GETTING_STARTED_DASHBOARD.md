# 🚀 Getting Started with Live Debugging Dashboard

**5-Minute Quick Start Guide**

---

## ✅ What You Have Now

A **complete real-time debugging system** that lets you see logs from all your applications in one place!

**Key Features**:
- 🔴 See errors instantly
- 📊 Monitor performance metrics
- 🟢 Track all log messages
- ⚡ Real-time updates (< 1ms)
- 🎨 Color-coded output

---

## 🎯 Try It Now (3 Commands)

### Terminal 1: Start the Message Broker
```bash
cd /Users/arjunsharma/Desktop/NeuroPipe
./build/broker
```

**You'll see**:
```
=== NeuroPipe Broker Running ===
Port: 9092
Backend: Standalone Asio
Ready for connections!
```

### Terminal 2: Start the Dashboard
```bash
./dashboards/view_all.sh
```

**You'll see**:
```
╔════════════════════════════════════════════════╗
║     NeuroPipe Live Debugging Dashboard        ║
║              Viewing: ALL LOGS                 ║
╚════════════════════════════════════════════════╝

Press Ctrl+C to stop
```

### Terminal 3: Run Example Application
```bash
./build/simple_app
```

**Now watch Terminal 2!** You'll see logs appearing in real-time with colors:
- 🟢 Green = Normal info
- 🟡 Yellow = Warnings
- 🔴 Red = Errors
- 🔵 Blue = Performance metrics

---

## 📸 What You'll See

**Terminal 2 (Dashboard)**:
```
[20:15:23.123] [INFO] simple_app: Application started          ← Green
[20:15:23.145] [INFO] simple_app: Processing order #101       ← Green
[20:15:23.268] order_processing_time_ms=125                   ← Blue
[20:15:25.456] [ERROR] simple_app: Payment failed #110        ← Red
[20:15:25.478] [WARN] simple_app: High memory usage: 85%      ← Yellow
[20:15:25.500] cpu_usage_percent=35                           ← Blue
```

Everything **updates in real-time** as your app runs!

---

## 🎨 Different Dashboard Views

Try these in separate terminals:

```bash
# See ALL logs (info, errors, warnings, metrics)
./dashboards/view_all.sh

# See ONLY errors and warnings
./dashboards/view_errors.sh

# See ONLY performance metrics
./dashboards/view_metrics.sh

# See logs from a SPECIFIC service
./dashboards/view_service.sh simple_app
```

You can run **multiple dashboards at once**!

---

## 💻 Use In Your Own Application

### Step 1: Include the logger

```cpp
#include "lib/debug_logger.hpp"
```

### Step 2: Create a logger

```cpp
int main() {
    DebugLogger logger("my_app_name");
    
    // Your code here...
}
```

### Step 3: Add logging

```cpp
// Normal messages
logger.info("User logged in successfully");
logger.debug("Database query took 45ms");

// Warnings
logger.warn("API rate limit approaching: 90%");

// Errors
logger.error("Failed to connect to database");

// Performance metrics
logger.metric("response_time_ms", 123);
logger.metric("memory_usage_mb", 512);
logger.metric("active_users", 42);
```

### Step 4: Compile

```bash
clang++ -std=c++20 -Ilib my_app.cpp build/libdebug_logger.a -o my_app -pthread
```

### Step 5: Run with dashboard

```bash
# Terminal 1
./build/broker

# Terminal 2
./dashboards/view_all.sh

# Terminal 3
./my_app
```

**Done!** See your logs in Terminal 2 in real-time!

---

## 🎯 Real Example

Here's a complete working example:

```cpp
#include "lib/debug_logger.hpp"
#include <thread>
#include <chrono>

int main() {
    // Create logger
    DebugLogger logger("payment_service");
    
    logger.info("Payment service starting...");
    
    // Simulate processing payments
    for (int i = 1; i <= 10; i++) {
        logger.info("Processing payment #" + std::to_string(i));
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Random error
        if (i == 5) {
            logger.error("Card declined for payment #5");
        } else {
            logger.info("Payment #" + std::to_string(i) + " successful");
            logger.metric("payment_amount", i * 50);
        }
    }
    
    logger.info("Payment service shutting down");
    
    return 0;
}
```

Save as `payment_service.cpp`, then:

```bash
# Compile
clang++ -std=c++20 -Ilib payment_service.cpp build/libdebug_logger.a -o payment_service -pthread

# Run with dashboard
./dashboards/view_all.sh &
./payment_service
```

---

## 🔥 Cool Use Cases

### 1. Debug Multiple Microservices

Run 3 services simultaneously:

```bash
# Terminal 1: Broker
./build/broker

# Terminal 2: Dashboard showing everything
./dashboards/view_all.sh

# Terminal 3, 4, 5: Your services
./user_service &
./order_service &
./payment_service &
```

See **all logs from all services in one place**!

### 2. Monitor Production Server

```bash
# On production server, save errors to file
./dashboards/view_errors.sh | tee production_errors.log
```

All errors are both **displayed and saved to file**.

### 3. Performance Testing

```bash
# Watch metrics during load test
./dashboards/view_metrics.sh &

# Run load test
for i in {1..100}; do
    curl http://localhost:8080/api/test
done
```

See **real-time performance** as requests are processed!

### 4. Team Debugging

Each team member runs their own view:

- **Developer A**: `./dashboards/view_service.sh frontend`
- **Developer B**: `./dashboards/view_service.sh backend`
- **QA**: `./dashboards/view_errors.sh`
- **DevOps**: `./dashboards/view_metrics.sh`

Everyone sees the **same live system** from different angles!

---

## 📚 What Logging Methods Are Available?

```cpp
DebugLogger logger("my_service");

// Log levels
logger.info("Normal information");           // Shows in all logs
logger.debug("Detailed debugging info");     // Shows in all logs
logger.warn("Warning about something");      // Shows in all + errors view
logger.error("Something went wrong!");       // Shows in all + errors view

// Performance metrics
logger.metric("response_time_ms", 123);      // Number metrics
logger.metric("cpu_usage_percent", 45.7);    // Decimal metrics
logger.metric("status", "healthy");          // Text metrics

// Custom topics
logger.publish("security", "Failed login from IP 1.2.3.4");
logger.publish("analytics", "Page view: /products/123");
```

---

## ⚙️ Configuration

### Change Broker Address

Default is `localhost:9092`. To use a different broker:

```cpp
// Default
DebugLogger logger("my_service");

// Custom broker
DebugLogger logger("my_service", "192.168.1.100", 9092);
```

### Check Connection

```cpp
DebugLogger logger("my_service");

if (!logger.is_connected()) {
    std::cerr << "Failed to connect to broker!" << std::endl;
    return 1;
}

// Connection is good, proceed...
```

### Reconnect if Needed

```cpp
if (!logger.is_connected()) {
    logger.reconnect();
}
```

---

## 🔧 Troubleshooting

### "Nothing shows in dashboard"

**Check**:
1. ✅ Is broker running? (`./build/broker`)
2. ✅ Is dashboard running? (`./dashboards/view_all.sh`)
3. ✅ Is your app calling logger methods? (`logger.info(...)`)
4. ✅ Did app connect successfully? (`logger.is_connected()`)

### "Failed to connect to broker"

**Solution**: Start the broker first!
```bash
./build/broker
```

Then run your app.

### "Permission denied" on dashboard scripts

**Solution**: Make them executable:
```bash
chmod +x dashboards/*.sh
```

### "Build errors"

**Solution**: Clean and rebuild:
```bash
make clean
make examples
```

---

## 📖 Documentation

| Document | What It Contains |
|----------|------------------|
| `README_DASHBOARD.md` | Detailed quick start + examples |
| `DASHBOARD_COMPLETE.md` | What was built + verification |
| `DASHBOARD_ARCHITECTURE.md` | Technical architecture details |
| `GETTING_STARTED_DASHBOARD.md` | This file - simplest guide |
| `LIVE_DASHBOARD_PLAN.md` | Original implementation plan |

---

## 💡 Pro Tips

1. **Keep dashboard running while developing**
   - Instantly see any logs/errors
   - No need to check multiple log files

2. **Use grep to filter**
   ```bash
   ./dashboards/view_all.sh | grep "order_123"
   ```

3. **Save logs to file**
   ```bash
   ./dashboards/view_all.sh | tee today_logs.txt
   ```

4. **Multiple views simultaneously**
   ```bash
   ./dashboards/view_all.sh &
   ./dashboards/view_errors.sh &
   ./dashboards/view_metrics.sh &
   ```

5. **Custom topics for organization**
   ```cpp
   logger.publish("database", "Query slow: 5.2s");
   logger.publish("network", "API call timeout");
   logger.publish("security", "Suspicious activity");
   ```

---

## 🎉 You're Ready!

You now have a **production-ready debugging dashboard**!

### Next Steps:

1. ✅ **Play with the example**: Run `./build/simple_app` and watch the logs
2. ✅ **Add to your app**: Include `lib/debug_logger.hpp` in your code
3. ✅ **Use it daily**: Keep the dashboard running while developing
4. ✅ **Share with team**: Multiple people can view simultaneously

---

## 🆘 Need Help?

**Quick commands**:
```bash
# Build everything
make examples

# See all available commands
make help

# Run example
./build/simple_app

# View all logs
./dashboards/view_all.sh
```

**Read documentation**:
- Start with `README_DASHBOARD.md`
- See architecture in `DASHBOARD_ARCHITECTURE.md`
- Check what was built in `DASHBOARD_COMPLETE.md`

---

## 📊 Summary

**What You Can Do Now**:
- ✅ See logs from all your apps in real-time
- ✅ Filter by errors, warnings, metrics
- ✅ Monitor multiple services simultaneously
- ✅ Debug issues as they happen
- ✅ Track performance metrics live
- ✅ Save logs to files
- ✅ Share views with your team

**No external dependencies, no complex setup. Just works!** 🚀

---

**Happy Debugging!** 🐛🔍

*Built with NeuroPipe - November 2025*

