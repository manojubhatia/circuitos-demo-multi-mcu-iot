#!/bin/bash
# Stop script for theSpeaker (3 boards)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

GREEN='\033[0;32m'
NC='\033[0m'

success() { echo -e "${GREEN}✓ $1${NC}"; }

echo "Stopping theSpeaker (3 boards)..."

cd "$PROJECT_ROOT"

# Docker Compose
if [ -f "docker-compose.yml" ]; then
    docker-compose down 2>/dev/null && success "Docker services stopped"
fi

# Individual containers
docker stop thespeaker-(3-boards)-backend 2>/dev/null && docker rm thespeaker-(3-boards)-backend 2>/dev/null
docker stop thespeaker-(3-boards)-frontend 2>/dev/null && docker rm thespeaker-(3-boards)-frontend 2>/dev/null

# Local processes
[ -f ".backend.pid" ] && kill $(cat .backend.pid) 2>/dev/null && rm .backend.pid && success "Backend stopped"
[ -f ".frontend.pid" ] && kill $(cat .frontend.pid) 2>/dev/null && rm .frontend.pid && success "Frontend stopped"

echo "All services stopped."
