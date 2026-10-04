# Auto-generated Makefile for theSpeaker (3 boards)
# Orchestrates device, backend, frontend, and integration tests.

.PHONY: all test-device test-backend test-frontend test-integration clean

PROJECT_NAME := thespeaker_(3_boards)

# Configuration
BACKEND_DIR := backend
FRONTEND_DIR := frontend

DEVICE_1_DIR := device_thespeaker

DEVICE_2_DIR := device_environment

DEVICE_3_DIR := device_motor_n_mike


# Default target: run all tests
all: test-device test-backend test-frontend test-integration

# ============================================================================
# Device Tests
# ============================================================================

test-device:
	@echo "╔══════════════════════════════════════╗"
	@echo "║   Device Tests (PlatformIO)          ║"
	@echo "╚══════════════════════════════════════╝"

	cd $(DEVICE_1_DIR) && pio test -e native || true

	cd $(DEVICE_2_DIR) && pio test -e native || true

	cd $(DEVICE_3_DIR) && pio test -e native || true



# ============================================================================
# Backend Tests
# ============================================================================
test-backend:
	@echo "╔══════════════════════════════════════╗"
	@echo "║   Backend Tests (pytest)             ║"
	@echo "╚══════════════════════════════════════╝"
	cd $(BACKEND_DIR) && pip install -r requirements.txt -q && pytest tests/ -v --tb=short

# ============================================================================
# Frontend Tests
# ============================================================================
test-frontend:
	@echo "╔══════════════════════════════════════╗"
	@echo "║   Frontend Tests (vitest)            ║"
	@echo "╚══════════════════════════════════════╝"
	cd $(FRONTEND_DIR) && npm install --silent && npx vitest run --reporter=verbose

# ============================================================================
# Integration / E2E Tests
# ============================================================================
test-integration:
	@echo "╔══════════════════════════════════════╗"
	@echo "║   Integration Tests (pytest E2E)     ║"
	@echo "╚══════════════════════════════════════╝"
	pip install -r tests/requirements-test.txt -q
	pytest tests/ -v --tb=short -k "not serial"

# ============================================================================
# Clean
# ============================================================================
clean:
	@echo "Cleaning test artifacts..."

	rm -rf $(DEVICE_1_DIR)/build

	rm -rf $(DEVICE_2_DIR)/build

	rm -rf $(DEVICE_3_DIR)/build

	rm -rf $(FRONTEND_DIR)/node_modules
	find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
	find . -name "*.pyc" -delete 2>/dev/null || true
