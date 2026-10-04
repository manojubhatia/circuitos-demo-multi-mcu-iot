#!/bin/bash
# Run script for theSpeaker (3 boards)
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }
info() { echo -e "${CYAN}ℹ $1${NC}"; }

echo "=============================================="
echo " Running theSpeaker (3 boards)"
echo "=============================================="

cd "$PROJECT_ROOT"

if command -v docker &> /dev/null && [ -f "docker-compose.yml" ]; then
    docker-compose up -d
    success "Services started with Docker Compose"
else
    echo "Starting services locally..."
    
    # Backend
    if [ -d "backend" ]; then
        cd backend
        [ -d "venv" ] && source venv/bin/activate
        uvicorn main:app --host 0.0.0.0 --port 8500 &
        echo $! > "$PROJECT_ROOT/.backend.pid"
        cd "$PROJECT_ROOT"
    fi
    
    # Frontend
    if [ -d "frontend" ]; then
        cd frontend
        npm run dev &
        echo $! > "$PROJECT_ROOT/.frontend.pid"
        cd "$PROJECT_ROOT"
    fi
fi

echo ""
info "Backend API:  http://localhost:8500"
info "Frontend:     http://localhost:8501"
info "WebSocket:    ws://localhost:8500/ws"
echo ""
info "Stop with: $SCRIPT_DIR/stop_all.sh"
