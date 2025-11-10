#!/bin/bash

# Test script to verify end-to-end message flow

echo "=== NeuroPipe Manual Flow Test ==="
echo ""

# Check if broker is running
if ! lsof -i :9092 > /dev/null 2>&1; then
    echo "❌ Broker is not running on port 9092"
    echo "Start it with: ./build/broker"
    exit 1
fi

echo "✓ Broker is running on port 9092"
echo ""

# Test 1: Consumer with command-line topic
echo "Test 1: Starting consumer with command-line topic..."
timeout 2 ./build/consumer_client 127.0.0.1 9092 test_topic > /tmp/consumer_test.log 2>&1 &
CONSUMER_PID=$!

sleep 0.5

echo "Test 2: Publishing message..."
echo "PUBLISH:test_topic:hello_from_test" | nc localhost 9092

sleep 0.5

# Check consumer log
if grep -q "hello_from_test" /tmp/consumer_test.log; then
    echo "✓ Consumer received message!"
    cat /tmp/consumer_test.log | grep "hello_from_test"
else
    echo "❌ Consumer did not receive message"
    echo "Consumer log:"
    cat /tmp/consumer_test.log
fi

# Cleanup
kill $CONSUMER_PID 2>/dev/null
rm -f /tmp/consumer_test.log

echo ""
echo "=== Test Complete ==="

