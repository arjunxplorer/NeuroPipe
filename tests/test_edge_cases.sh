#!/bin/bash

# NeuroPipe Edge Case Test Suite
# Tests various edge cases to ensure robustness

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

BROKER_PID=""
TEST_COUNT=0
PASS_COUNT=0
FAIL_COUNT=0

# Cleanup function
cleanup() {
    if [ ! -z "$BROKER_PID" ]; then
        echo -e "\n${YELLOW}Cleaning up...${NC}"
        kill $BROKER_PID 2>/dev/null || true
        wait $BROKER_PID 2>/dev/null || true
    fi
}

trap cleanup EXIT

# Helper functions
test_start() {
    TEST_COUNT=$((TEST_COUNT + 1))
    echo -e "\n${BLUE}[Test $TEST_COUNT]${NC} $1"
}

test_pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    echo -e "${GREEN}  ✓ PASS${NC}: $1"
}

test_fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    echo -e "${RED}  ✗ FAIL${NC}: $1"
}

start_broker() {
    echo -e "${YELLOW}Starting broker...${NC}"
    ./build/broker > /tmp/neuropipe_test_broker.log 2>&1 &
    BROKER_PID=$!
    sleep 1
    
    if kill -0 $BROKER_PID 2>/dev/null; then
        echo -e "${GREEN}  ✓ Broker started (PID: $BROKER_PID)${NC}"
    else
        echo -e "${RED}  ✗ Failed to start broker${NC}"
        exit 1
    fi
}

stop_broker() {
    if [ ! -z "$BROKER_PID" ]; then
        echo -e "${YELLOW}Stopping broker...${NC}"
        kill $BROKER_PID 2>/dev/null || true
        wait $BROKER_PID 2>/dev/null || true
        BROKER_PID=""
        sleep 1
    fi
}

echo "╔═══════════════════════════════════════════════════════════╗"
echo "║        NeuroPipe Edge Case Test Suite                    ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${RED}Error: build directory not found. Run 'make' first.${NC}"
    exit 1
fi

# ============================================================================
# Test 1: Broker Not Running
# ============================================================================
test_start "Application handles broker not running"

OUTPUT=$(./build/simple_app 2>&1 || true)
if echo "$OUTPUT" | grep -q "Failed to connect"; then
    test_pass "App detects broker unavailable"
else
    test_fail "App should fail gracefully when broker not running"
fi

# ============================================================================
# Test 2: Empty Messages
# ============================================================================
test_start "Broker handles empty messages"

start_broker

RESPONSE=$(echo "" | nc -w 1 localhost 9092 2>/dev/null || echo "TIMEOUT")
if [ "$RESPONSE" != "TIMEOUT" ]; then
    test_pass "Broker handles empty message without crashing"
else
    test_fail "Broker should respond to empty messages"
fi

# ============================================================================
# Test 3: Malformed Commands
# ============================================================================
test_start "Broker handles malformed commands"

RESPONSE=$(echo "INVALID_COMMAND" | nc -w 1 localhost 9092)
if echo "$RESPONSE" | grep -q "ERROR"; then
    test_pass "Broker returns error for invalid command"
else
    test_fail "Broker should return ERROR for invalid commands"
fi

# Test short SUBSCRIBE command
RESPONSE=$(echo "SUBSCRIBE" | nc -w 1 localhost 9092)
if echo "$RESPONSE" | grep -q "ERROR"; then
    test_pass "Broker handles short SUBSCRIBE command"
else
    test_fail "Broker should return ERROR for incomplete SUBSCRIBE"
fi

# Test PUBLISH without payload
RESPONSE=$(echo "PUBLISH:test" | nc -w 1 localhost 9092)
if echo "$RESPONSE" | grep -q "ERROR"; then
    test_pass "Broker handles PUBLISH without payload"
else
    test_fail "Broker should return ERROR for PUBLISH without payload"
fi

# ============================================================================
# Test 4: Special Characters
# ============================================================================
test_start "Protocol handles special characters"

# Test colon in message
(echo "SUBSCRIBE:test_special"; sleep 0.1) | nc localhost 9092 > /tmp/nc_special.txt &
NC_PID=$!
sleep 0.5

echo "PUBLISH:test_special:Message with:colons:here" | nc localhost 9092 > /dev/null
sleep 0.5

kill $NC_PID 2>/dev/null || true
wait $NC_PID 2>/dev/null || true

if grep -q "test_special" /tmp/nc_special.txt; then
    test_pass "Messages with colons handled correctly"
else
    test_fail "Messages with colons not handled properly"
fi

rm -f /tmp/nc_special.txt

# ============================================================================
# Test 5: Multiple Subscribers
# ============================================================================
test_start "Multiple subscribers receive messages"

# Start 3 subscribers
(echo "SUBSCRIBE:multi_test"; sleep 2) | nc localhost 9092 > /tmp/nc_sub1.txt &
SUB1=$!
(echo "SUBSCRIBE:multi_test"; sleep 2) | nc localhost 9092 > /tmp/nc_sub2.txt &
SUB2=$!
(echo "SUBSCRIBE:multi_test"; sleep 2) | nc localhost 9092 > /tmp/nc_sub3.txt &
SUB3=$!

sleep 0.5

# Publish message
echo "PUBLISH:multi_test:test_message" | nc localhost 9092 > /dev/null

sleep 0.5

# Kill subscribers
kill $SUB1 $SUB2 $SUB3 2>/dev/null || true
wait $SUB1 $SUB2 $SUB3 2>/dev/null || true

# Check all received
RECEIVED=0
grep -q "test_message" /tmp/nc_sub1.txt && RECEIVED=$((RECEIVED + 1))
grep -q "test_message" /tmp/nc_sub2.txt && RECEIVED=$((RECEIVED + 1))
grep -q "test_message" /tmp/nc_sub3.txt && RECEIVED=$((RECEIVED + 1))

if [ $RECEIVED -eq 3 ]; then
    test_pass "All 3 subscribers received message"
elif [ $RECEIVED -gt 0 ]; then
    test_fail "Only $RECEIVED/3 subscribers received message"
else
    test_fail "No subscribers received message"
fi

rm -f /tmp/nc_sub*.txt

# ============================================================================
# Test 6: Subscriber Disconnect
# ============================================================================
test_start "Broker handles subscriber disconnect"

(echo "SUBSCRIBE:disconnect_test"; sleep 0.5) | nc localhost 9092 &
SUB_PID=$!

sleep 0.3

# Kill subscriber
kill $SUB_PID 2>/dev/null || true
wait $SUB_PID 2>/dev/null || true

sleep 0.2

# Broker should still be running
if kill -0 $BROKER_PID 2>/dev/null; then
    test_pass "Broker continues after subscriber disconnect"
else
    test_fail "Broker crashed after subscriber disconnect"
fi

# ============================================================================
# Test 7: Rapid Connections
# ============================================================================
test_start "Broker handles rapid connections"

for i in {1..20}; do
    echo "PING" | nc -w 1 localhost 9092 > /dev/null &
done

sleep 2

if kill -0 $BROKER_PID 2>/dev/null; then
    test_pass "Broker handles 20 rapid connections"
else
    test_fail "Broker crashed under connection load"
fi

# ============================================================================
# Test 8: Large Messages
# ============================================================================
test_start "System handles large messages"

LARGE_MSG=$(head -c 5000 /dev/urandom | base64 | tr -d '\n')
RESPONSE=$(echo "PUBLISH:large_test:$LARGE_MSG" | nc -w 2 localhost 9092)

if echo "$RESPONSE" | grep -q "OK"; then
    test_pass "5KB message handled successfully"
else
    test_fail "Large message not handled properly"
fi

# ============================================================================
# Test 9: Broker Restart
# ============================================================================
test_start "Application handles broker restart"

# Start robust app in background
./build/robust_app > /tmp/robust_test.log 2>&1 &
APP_PID=$!

sleep 1

# Stop broker
stop_broker

sleep 1

# Restart broker
start_broker

sleep 1

# Check if app is still running
if kill -0 $APP_PID 2>/dev/null; then
    test_pass "Application survived broker restart"
    kill $APP_PID 2>/dev/null || true
    wait $APP_PID 2>/dev/null || true
else
    test_fail "Application crashed during broker restart"
fi

rm -f /tmp/robust_test.log

# ============================================================================
# Test 10: No Subscribers
# ============================================================================
test_start "Broker handles publish with no subscribers"

RESPONSE=$(echo "PUBLISH:no_sub_topic:test_message" | nc -w 1 localhost 9092)

if echo "$RESPONSE" | grep -q "OK"; then
    test_pass "Broker accepts publish with no subscribers"
else
    test_fail "Broker should accept publish even without subscribers"
fi

# Check broker logs
if grep -q "no subscribers" /tmp/neuropipe_test_broker.log; then
    test_pass "Broker logs 'no subscribers' appropriately"
fi

# ============================================================================
# Test 11: Thread Safety (using robust_app)
# ============================================================================
test_start "Thread safety test"

timeout 5 ./build/robust_app --test > /tmp/thread_test.log 2>&1 || true

if grep -q "Thread safety test complete" /tmp/thread_test.log; then
    test_pass "Multi-threaded logging successful (500 msgs, 10 threads)"
else
    test_fail "Thread safety test did not complete"
fi

rm -f /tmp/thread_test.log

# ============================================================================
# Summary
# ============================================================================
stop_broker

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║                    Test Summary                           ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "  Total Tests:  $TEST_COUNT"
echo -e "  ${GREEN}Passed:       $PASS_COUNT${NC}"
if [ $FAIL_COUNT -gt 0 ]; then
    echo -e "  ${RED}Failed:       $FAIL_COUNT${NC}"
else
    echo -e "  Failed:       $FAIL_COUNT"
fi
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║              ✓ ALL TESTS PASSED ✓                        ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
    exit 0
else
    echo -e "${RED}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║              ✗ SOME TESTS FAILED ✗                       ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi

