#!/bin/bash

# View Metrics - Shows performance metrics with analysis

# Colors
BLUE='\033[0;34m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          NeuroPipe Live Debugging Dashboard               ║"
echo "║                 Viewing: METRICS ONLY                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Topic: metrics"
echo "Press Ctrl+C to stop"
echo ""
echo "────────────────────────────────────────────────────────────"
echo ""

# Subscribe to metrics
./build/consumer_client localhost 9092 metrics 2>/dev/null | while IFS= read -r line; do
    # Extract metric value if present
    if [[ $line =~ ([0-9]+)ms ]]; then
        value="${BASH_REMATCH[1]}"
        
        # Color code based on performance
        if [ "$value" -lt 50 ]; then
            echo -e "${GREEN}⚡ $line${NC}"  # Fast
        elif [ "$value" -lt 200 ]; then
            echo -e "${CYAN}📊 $line${NC}"   # Normal
        elif [ "$value" -lt 500 ]; then
            echo -e "${YELLOW}⏱️  $line${NC}"  # Slow
        else
            echo -e "${RED}🐌 $line (SLOW!)${NC}"  # Very slow
        fi
    elif [[ $line =~ percent|% ]]; then
        # Handle percentage metrics
        echo -e "${BLUE}📈 $line${NC}"
    elif [[ $line =~ mb|MB|memory ]]; then
        # Handle memory metrics
        echo -e "${CYAN}💾 $line${NC}"
    else
        echo -e "${BLUE}📊 $line${NC}"
    fi
done

