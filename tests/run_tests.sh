#!/bin/bash
# Run all tests for the generated project
set -e

echo "=== Running Backend Unit Tests ==="
cd backend && pip install -r requirements.txt -q && pytest tests/ -v && cd ..

echo ""
echo "=== Running Frontend Tests ==="
cd frontend && npm install -q && npx vitest run && cd ..

echo ""
echo "=== Running Integration Tests ==="
pip install -r tests/requirements-test.txt -q
pytest tests/integration_test.py -v

echo ""
echo "=== All Tests Passed ==="
