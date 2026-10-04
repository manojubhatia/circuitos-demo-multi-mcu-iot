#!/bin/bash
# Start Backend Server for theSpeaker (3 boards)
# This script sets up the environment and runs the FastAPI backend

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BACKEND_DIR="$PROJECT_ROOT/backend"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }
info() { echo -e "${CYAN}ℹ $1${NC}"; }
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }

echo "=============================================="
echo " Starting Backend Server"
echo "=============================================="

if [ ! -d "$BACKEND_DIR" ]; then
    error "Backend directory not found: $BACKEND_DIR"
fi

cd "$BACKEND_DIR"

# Create virtual environment if it doesn't exist
if [ ! -d "venv" ]; then
    info "Creating virtual environment..."
    python3 -m venv venv
    success "Virtual environment created"
fi

# Activate virtual environment
source venv/bin/activate
success "Virtual environment activated"

# Install dependencies
if [ -f "requirements.txt" ]; then
    info "Installing dependencies..."
    pip install -q -r requirements.txt
    success "Dependencies installed"
fi

# Start the server
echo ""
info "Starting FastAPI server on port 8500..."
info "API docs:     http://localhost:8500/docs"
info "Health check: http://localhost:8500/health"
info "WebSocket:    ws://localhost:8500/ws"
echo ""
info "Press Ctrl+C to stop the server"
echo ""

# Use the venv's python explicitly to avoid picking up a system/other-venv uvicorn.
# If watchfiles fails (EMFILE: too many open files), fall back to running without --reload.
python -m uvicorn main:app --host 0.0.0.0 --port 8500 --reload --reload-dir . --reload-exclude '*.pyc' --reload-exclude '__pycache__' 2>&1 || {
    echo ""
    info "File watcher failed (EMFILE). Restarting without --reload..."
    python -m uvicorn main:app --host 0.0.0.0 --port 8500
}
