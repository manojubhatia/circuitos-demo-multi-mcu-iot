#!/usr/bin/env bash
# Local test runner for theSpeaker (3 boards)
# Runs all test suites locally without requiring external CI services.
# Usage: ./run_tests.sh [device|backend|frontend|integration|all]

set -e

PROJECT_NAME="thespeaker_(3_boards)"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="${SCRIPT_DIR}/test-results"
mkdir -p "${RESULTS_DIR}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

banner() {
    echo -e "\n${CYAN}╔══════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║  $1${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════╝${NC}\n"
}

pass() { echo -e "  ${GREEN}✓ PASS${NC}: $1"; PASS_COUNT=$((PASS_COUNT + 1)); }
fail() { echo -e "  ${RED}✗ FAIL${NC}: $1"; FAIL_COUNT=$((FAIL_COUNT + 1)); }
skip() { echo -e "  ${YELLOW}⊘ SKIP${NC}: $1"; SKIP_COUNT=$((SKIP_COUNT + 1)); }

# ============================================================================
# Device Tests
# ============================================================================
run_device_tests() {
    banner "Device Tests (ESP_IDF)"


    DEVICE_DIR="${SCRIPT_DIR}/device_thespeaker"
    if [ -d "${DEVICE_DIR}" ]; then
        echo "  Testing: theSpeaker"
        if command -v pio &>/dev/null; then
            cd "${DEVICE_DIR}" && pio test -e native 2>&1 | tee "${RESULTS_DIR}/device_thespeaker.log"
            if [ $? -eq 0 ]; then pass "theSpeaker"; else fail "theSpeaker"; fi
        else
            skip "theSpeaker (pio not found)"
        fi
    fi

    DEVICE_DIR="${SCRIPT_DIR}/device_environment"
    if [ -d "${DEVICE_DIR}" ]; then
        echo "  Testing: environment"
        if command -v pio &>/dev/null; then
            cd "${DEVICE_DIR}" && pio test -e native 2>&1 | tee "${RESULTS_DIR}/device_environment.log"
            if [ $? -eq 0 ]; then pass "environment"; else fail "environment"; fi
        else
            skip "environment (pio not found)"
        fi
    fi

    DEVICE_DIR="${SCRIPT_DIR}/device_motor_n_mike"
    if [ -d "${DEVICE_DIR}" ]; then
        echo "  Testing: motor_n_mike"
        if command -v pio &>/dev/null; then
            cd "${DEVICE_DIR}" && pio test -e native 2>&1 | tee "${RESULTS_DIR}/device_motor_n_mike.log"
            if [ $? -eq 0 ]; then pass "motor_n_mike"; else fail "motor_n_mike"; fi
        else
            skip "motor_n_mike (pio not found)"
        fi
    fi


}

# ============================================================================
# Backend Tests
# ============================================================================
run_backend_tests() {
    banner "Backend Tests (pytest)"

    BACKEND_DIR="${SCRIPT_DIR}/backend"
    if [ -d "${BACKEND_DIR}" ] && [ -f "${BACKEND_DIR}/requirements.txt" ]; then
        cd "${BACKEND_DIR}"
        pip install -r requirements.txt -q 2>/dev/null
        if [ -d "tests" ]; then
            pytest tests/ -v --tb=short --junitxml="${RESULTS_DIR}/backend.xml" 2>&1 | tee "${RESULTS_DIR}/backend.log"
            if [ $? -eq 0 ]; then pass "Backend tests"; else fail "Backend tests"; fi
        else
            skip "Backend (no tests/ directory)"
        fi
    else
        skip "Backend (directory not found)"
    fi
}

# ============================================================================
# Frontend Tests
# ============================================================================
run_frontend_tests() {
    banner "Frontend Tests (vitest)"

    FRONTEND_DIR="${SCRIPT_DIR}/frontend"
    if [ -d "${FRONTEND_DIR}" ] && [ -f "${FRONTEND_DIR}/package.json" ]; then
        cd "${FRONTEND_DIR}"
        npm install --silent 2>/dev/null
        npx vitest run --reporter=verbose 2>&1 | tee "${RESULTS_DIR}/frontend.log"
        if [ $? -eq 0 ]; then pass "Frontend tests"; else fail "Frontend tests"; fi
    else
        skip "Frontend (directory not found)"
    fi
}

# ============================================================================
# Integration Tests
# ============================================================================
run_integration_tests() {
    banner "Integration Tests (E2E)"

    if [ -d "${SCRIPT_DIR}/tests" ]; then
        cd "${SCRIPT_DIR}"
        if [ -f "tests/requirements-test.txt" ]; then
            pip install -r tests/requirements-test.txt -q 2>/dev/null
        fi

        # Start backend if available
        BACKEND_PID=""
        if [ -f "backend/main.py" ]; then
            echo "  Starting backend on port 8500..."
            cd backend && pip install -r requirements.txt -q 2>/dev/null
            python -m uvicorn main:app --host 127.0.0.1 --port 8500 &
            BACKEND_PID=$!
            sleep 2
            cd "${SCRIPT_DIR}"
        fi

        BACKEND_URL="http://127.0.0.1:8500" pytest tests/ -v --tb=short -k "not serial" 2>&1 | tee "${RESULTS_DIR}/integration.log"
        if [ $? -eq 0 ]; then pass "Integration tests"; else fail "Integration tests"; fi

        # Stop backend
        if [ -n "${BACKEND_PID}" ]; then
            kill "${BACKEND_PID}" 2>/dev/null || true
        fi
    else
        skip "Integration (no tests/ directory)"
    fi
}

# ============================================================================
# Summary
# ============================================================================
print_summary() {
    echo ""
    banner "Test Summary: ${PROJECT_NAME}"
    echo -e "  ${GREEN}Passed${NC}: ${PASS_COUNT}"
    echo -e "  ${RED}Failed${NC}: ${FAIL_COUNT}"
    echo -e "  ${YELLOW}Skipped${NC}: ${SKIP_COUNT}"
    echo -e "  Results in: ${RESULTS_DIR}/"
    echo ""

    if [ ${FAIL_COUNT} -gt 0 ]; then
        echo -e "  ${RED}OVERALL: FAILED${NC}"
        exit 1
    else
        echo -e "  ${GREEN}OVERALL: PASSED${NC}"
        exit 0
    fi
}

# ============================================================================
# Main
# ============================================================================
TARGET="${1:-all}"

case "${TARGET}" in
    device)     run_device_tests ;;
    backend)    run_backend_tests ;;
    frontend)   run_frontend_tests ;;
    integration) run_integration_tests ;;
    all)
        run_device_tests
        run_backend_tests
        run_frontend_tests
        run_integration_tests
        ;;
    *)
        echo "Usage: $0 [device|backend|frontend|integration|all]"
        exit 1
        ;;
esac

print_summary
