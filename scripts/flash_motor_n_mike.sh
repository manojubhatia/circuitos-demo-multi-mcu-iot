#!/bin/bash
# Flash script for motor_n_mike
# Usage: motor_n_mike.sh [PORT] [--no-erase|-n]
#   Default: Erases flash + NVS before uploading (clean start)
#   --no-erase, -n : Skip erase (incremental flash, preserves NVS/WiFi)
#   PORT: Serial port path (e.g., /dev/ttyACM0, /dev/ttyUSB0)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEVICE_DIR="$PROJECT_ROOT/device_motor_n_mike"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }
success() { echo -e "${GREEN}✓ $1${NC}"; }

# Parse arguments - erase is DEFAULT, use --no-erase to skip
ERASE_FLASH=true
USER_PORT=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-erase|-n)
            ERASE_FLASH=false
            shift
            ;;
        /dev/*|COM*)
            USER_PORT="$1"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

[ -d "$DEVICE_DIR" ] || error "Device directory not found: device_motor_n_mike"

cd "$DEVICE_DIR"

echo "=============================================="
echo " Flashing motor_n_mike"
echo "=============================================="
echo "Device: $DEVICE_DIR"
if [ "$ERASE_FLASH" = true ]; then
    echo "Mode: ERASE FLASH + NVS, then PROGRAM (clean start)"
else
    echo "Mode: INCREMENTAL FLASH (preserving NVS)"
fi

if [ -z "$IDF_PATH" ]; then
    [ -f "$HOME/esp/esp-idf/export.sh" ] && source "$HOME/esp/esp-idf/export.sh"
    [ -f "/opt/esp-idf/export.sh" ] && source "/opt/esp-idf/export.sh"
fi

PORT="${USER_PORT:-${ESP_PORT:-}}"
[ -z "$PORT" ] && for p in /dev/ttyUSB* /dev/ttyACM*; do [ -e "$p" ] && PORT="$p" && break; done
[ -z "$PORT" ] && error "No serial port found. Usage: $0 [/dev/ttyACM0] [--no-erase]"

# Erase flash if requested (clears NVS, WiFi credentials, triggers AP mode)
if [ "$ERASE_FLASH" = true ]; then
    echo "Erasing flash..."
    idf.py -p "$PORT" erase-flash
    echo "Flash erased. Waiting for device..."
    sleep 3
fi

echo "Using port: $PORT"
idf.py -p "$PORT" flash monitor

