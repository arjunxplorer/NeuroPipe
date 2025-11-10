#!/bin/bash

# View Specific Service - Shows logs from one service only

if [ -z "$1" ]; then
    echo "Usage: $0 <service_name>"
    echo ""
    echo "Example: $0 order_service"
    echo "         $0 simple_app"
    exit 1
fi

SERVICE=$1

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          NeuroPipe Live Debugging Dashboard               ║"
echo "║              Viewing Service: $SERVICE"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Press Ctrl+C to stop"
echo ""
echo "────────────────────────────────────────────────────────────"
echo ""

# Subscribe and filter for specific service
./build/consumer_client localhost 9092 debug errors warnings metrics 2>/dev/null | grep "$SERVICE" | while IFS= read -r line; do
    # Color code based on content
    if [[ $line =~ ERROR ]]; then
        echo -e "${RED}$line${NC}"
    elif [[ $line =~ WARN ]]; then
        echo -e "${YELLOW}$line${NC}"
    elif [[ $line =~ INFO ]]; then
        echo -e "${GREEN}$line${NC}"
    elif [[ $line =~ DEBUG ]]; then
        echo -e "${CYAN}$line${NC}"
    else
        echo "$line"
    fi
done

