#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
echo "Desktop Simulation: theSpeaker (3 boards)"
mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$(nproc) && cd "$SCRIPT_DIR"
PIDS=()
cleanup() { for pid in "${PIDS[@]}"; do kill "$pid" 2>/dev/null || true; done; wait; }
trap cleanup EXIT INT TERM
"$BUILD_DIR/sim_thespeaker" & PIDS+=($!); sleep 0.3
"$BUILD_DIR/sim_environment" & PIDS+=($!); sleep 0.3
"$BUILD_DIR/sim_motor_n_mike" & PIDS+=($!); sleep 0.3
echo "All processes running. Ctrl+C to stop."
wait