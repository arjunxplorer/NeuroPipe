#!/bin/bash

# View Errors Only - Shows only error messages with highlighting

# Colors
RED='\033[0;31m'
BOLD_RED='\033[1;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════╗"
echo "║          NeuroPipe Live Debugging Dashboard               ║"
echo "║                  Viewing: ERRORS ONLY                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Topic: errors, warnings"
echo "Press Ctrl+C to stop"
echo ""
echo "────────────────────────────────────────────────────────────"
echo ""

# Subscribe to errors and warnings
./build/consumer_client localhost 9092 errors warnings 2>/dev/null | while IFS= read -r line; do
    # Highlight errors in bold red, warnings in yellow
    if [[ $line =~ ERROR ]]; then
        echo -e "${BOLD_RED}🔴 $line${NC}"
        
        # Optional: System notification for critical errors
        if [[ $line =~ CRITICAL ]]; then
            # Uncomment to enable desktop notifications
            # osascript -e "display notification \"$line\" with title \"Critical Error\""
            echo -e "${BOLD_RED}⚠️  CRITICAL ERROR DETECTED ⚠️${NC}"
        fi
    elif [[ $line =~ WARN ]]; then
        echo -e "${YELLOW}⚠️  $line${NC}"
    else
        echo -e "${RED}$line${NC}"
    fi
done

