#!/bin/bash
# Build script for theSpeaker (3 boards)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }
warning() { echo -e "${YELLOW}⚠ $1${NC}"; }
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }

echo "=============================================="
echo " Building theSpeaker (3 boards)"
echo "=============================================="

echo "Building: theSpeaker (ESP-IDF)"
DEVICE_DIR="$PROJECT_ROOT/device_thespeaker"
if [ -d "$DEVICE_DIR" ]; then
    cd "$DEVICE_DIR"
    if [ -n "$IDF_PATH" ]; then
        idf.py build && success "theSpeaker built" || error "theSpeaker build failed"
    else
        warning "ESP-IDF not found, skipping theSpeaker"
    fi
fi

echo "Building: environment (ESP-IDF)"
DEVICE_DIR="$PROJECT_ROOT/device_environment"
if [ -d "$DEVICE_DIR" ]; then
    cd "$DEVICE_DIR"
    if [ -n "$IDF_PATH" ]; then
        idf.py build && success "environment built" || error "environment build failed"
    else
        warning "ESP-IDF not found, skipping environment"
    fi
fi

echo "Building: motor_n_mike (ESP-IDF)"
DEVICE_DIR="$PROJECT_ROOT/device_motor_n_mike"
if [ -d "$DEVICE_DIR" ]; then
    cd "$DEVICE_DIR"
    if [ -n "$IDF_PATH" ]; then
        idf.py build && success "motor_n_mike built" || error "motor_n_mike build failed"
    else
        warning "ESP-IDF not found, skipping motor_n_mike"
    fi
fi

echo "Building: Backend (Docker)"
cd "$PROJECT_ROOT/backend"
if command -v docker &> /dev/null; then
    docker build -t thespeaker-(3-boards)-backend:latest . && success "Backend built" || error "Backend build failed"
else
    pip install -r requirements.txt && success "Backend dependencies installed"
fi

echo "Building: Frontend (Docker)"
cd "$PROJECT_ROOT/frontend"
if command -v docker &> /dev/null; then
    docker build -t thespeaker-(3-boards)-frontend:latest . && success "Frontend built" || error "Frontend build failed"
else
    npm install && npm run build && success "Frontend built"
fi

echo ""
echo "Build complete!"
