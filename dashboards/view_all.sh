#!/bin/bash

# View All Logs - Shows everything from all services in real-time

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          NeuroPipe Live Debugging Dashboard               ║"
echo "║                    Viewing: ALL LOGS                       ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Topics: debug, errors, warnings, metrics"
echo "Press Ctrl+C to stop"
echo ""
echo "────────────────────────────────────────────────────────────"
echo ""

# Subscribe to all debug topics
./build/consumer_client localhost 9092 debug errors warnings metrics 2>/dev/null | while IFS= read -r line; do
    # Color code based on content
    if [[ $line =~ ERROR ]]; then
        echo -e "${RED}$line${NC}"
    elif [[ $line =~ WARN ]]; then
        echo -e "${YELLOW}$line${NC}"
    elif [[ $line =~ INFO ]]; then
        echo -e "${GREEN}$line${NC}"
    elif [[ $line =~ DEBUG ]]; then
        echo -e "${CYAN}$line${NC}"
    elif [[ $line =~ metrics ]]; then
        echo -e "${BLUE}$line${NC}"
    else
        echo "$line"
    fi
done

