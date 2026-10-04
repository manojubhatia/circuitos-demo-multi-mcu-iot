#!/bin/bash
# Build script for theSpeaker
# Usage: build_thespeaker.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEVICE_DIR="$PROJECT_ROOT/device_thespeaker"

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }
success() { echo -e "${GREEN}✓ $1${NC}"; }

[ -d "$DEVICE_DIR" ] || error "Device directory not found: device_thespeaker"




cd "$DEVICE_DIR"

echo "=============================================="
echo " Building theSpeaker"
echo "=============================================="

if [ -z "$IDF_PATH" ]; then
    [ -f "$HOME/esp/esp-idf/export.sh" ] && source "$HOME/esp/esp-idf/export.sh"
    [ -f "/opt/esp-idf/export.sh" ] && source "/opt/esp-idf/export.sh"
fi
[ -z "$IDF_PATH" ] && error "ESP-IDF not found. Install it or set IDF_PATH."

idf.py set-target esp32 2>/dev/null || true
idf.py build
success "Build complete! Binary: build/thespeaker.bin"

