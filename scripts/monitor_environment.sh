#!/bin/bash
# Serial monitor for environment
# Usage: monitor_environment.sh [PORT] [BAUD]
#   PORT: Serial port (auto-detected if not specified)
#   BAUD: Baud rate (default: 115200)
#   Press Ctrl+] (ESP-IDF) or Ctrl+C to exit

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEVICE_DIR="$PROJECT_ROOT/device_environment"

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }
success() { echo -e "${GREEN}✓ $1${NC}"; }

USER_PORT=""
BAUD="115200"
for arg in "$@"; do
    case "$arg" in
        /dev/*|COM*) USER_PORT="$arg" ;;
        [0-9]*) BAUD="$arg" ;;
    esac
done

PORT="${USER_PORT:-${ESP_PORT:-}}"
if [ -z "$PORT" ]; then
    if [ -e "/dev/luatos-esp32c3" ]; then PORT="/dev/luatos-esp32c3"; else
    for p in /dev/ttyACM* /dev/ttyUSB*; do [ -e "$p" ] && PORT="$p" && break; done
    fi
fi
[ -z "$PORT" ] && error "No serial port found. Usage: $0 [/dev/ttyACM0]"

echo -e "${CYAN}=============================================="
echo " Serial Monitor: environment"
echo " Port: $PORT"
echo " Baud: $BAUD"
echo "==============================================${NC}"
echo ""

[ -d "$DEVICE_DIR" ] && cd "$DEVICE_DIR"

if [ -z "$IDF_PATH" ]; then
    [ -f "$HOME/esp/esp-idf/export.sh" ] && source "$HOME/esp/esp-idf/export.sh"
    [ -f "/opt/esp-idf/export.sh" ] && source "/opt/esp-idf/export.sh"
fi
[ -z "$IDF_PATH" ] && error "ESP-IDF not found. Install it or set IDF_PATH."

idf.py -p "$PORT" -b "$BAUD" monitor

