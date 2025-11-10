#!/bin/bash

echo "=== NeuroPipe Connection Debugger ==="
echo ""

# Test 1: Can we connect?
echo "Test 1: Testing connection to broker..."
if echo "PING" | nc -w 1 localhost 9092 | grep -q "PONG"; then
    echo "✓ Broker responds to PING"
else
    echo "❌ Broker not responding"
    exit 1
fi

echo ""

# Test 2: Can we subscribe?
echo "Test 2: Testing SUBSCRIBE..."
{
    echo "SUBSCRIBE:test_debug"
    sleep 0.5
} | nc -w 2 localhost 9092 > /tmp/broker_response.txt 2>&1

if grep -q "OK:SUBSCRIBED" /tmp/broker_response.txt; then
    echo "✓ Subscription successful"
    cat /tmp/broker_response.txt
else
    echo "❌ Subscription failed"
    echo "Response:"
    cat /tmp/broker_response.txt
fi

echo ""

# Test 3: Can we publish and receive?
echo "Test 3: Testing full pub/sub flow..."
{
    echo "SUBSCRIBE:test_flow"
    sleep 0.3
    echo "PUBLISH:test_flow:test_message"
    sleep 0.3
} | nc -w 2 localhost 9092 > /tmp/flow_test.txt 2>&1

echo "Messages received:"
cat /tmp/flow_test.txt

if grep -q "test_message" /tmp/flow_test.txt; then
    echo "✓ End-to-end flow works!"
else
    echo "❌ Message not received"
fi

# Cleanup
rm -f /tmp/broker_response.txt /tmp/flow_test.txt

echo ""
echo "=== Debug Complete ==="

