#!/bin/bash
# Start Frontend Dev Server for theSpeaker (3 boards)
# This script sets up Node.js environment and runs the Vite dev server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FRONTEND_DIR="$PROJECT_ROOT/frontend"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }
info() { echo -e "${CYAN}ℹ $1${NC}"; }
error() { echo -e "${RED}✗ $1${NC}"; exit 1; }

echo "=============================================="
echo " Starting Frontend Dev Server"
echo "=============================================="

if [ ! -d "$FRONTEND_DIR" ]; then
    error "Frontend directory not found: $FRONTEND_DIR"
fi

# Check for Node.js
if ! command -v node &> /dev/null; then
    error "Node.js not found. Please install Node.js 18+ from https://nodejs.org/"
fi

cd "$FRONTEND_DIR"

# Install dependencies (always run to ensure packages are up to date)
info "Checking npm dependencies..."
npm install --legacy-peer-deps
success "Dependencies ready"

# Set environment variables
export VITE_API_URL="http://localhost:8500/api"
export VITE_WS_URL="ws://localhost:8500/ws"

# Use polling mode for file watching to avoid EMFILE errors
# (inotify instances can be exhausted by IDEs and other watchers)
export CHOKIDAR_USEPOLLING=true

echo ""
info "Starting Vite dev server..."
info "Frontend:   http://localhost:8501"
info "Backend:    http://localhost:8500"
echo ""
info "Make sure the backend is running: ./start_backend.sh"
info "Press Ctrl+C to stop the server"
echo ""

npm run dev -- --port 8501
