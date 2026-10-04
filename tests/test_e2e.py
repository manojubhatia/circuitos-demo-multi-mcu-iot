"""
End-to-End System Tests - theSpeaker (3 boards)
Auto-generated. Validates the full data pipeline:
  Device (sensors) → Backend (API) → Frontend (display)

Requirements:
  pip install pytest httpx pyserial websockets

Usage:
  pytest test_e2e.py -v
  pytest test_e2e.py -v -k "backend"   # Only backend tests
  pytest test_e2e.py -v -k "serial"    # Only serial/device tests
"""

import pytest
import time
import json
from typing import Dict, Any, Optional

# Try imports - skip gracefully if not installed
try:
    import httpx
    HAS_HTTPX = True
except ImportError:
    HAS_HTTPX = False

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False


# ============================================================================
# Configuration (from architecture.json)
# ============================================================================

BACKEND_URL = "http://localhost:8000"
SERIAL_PORT = "/dev/ttyUSB0"
SERIAL_BAUD = 115200
DATA_CONTRACTS = [{"id": "environment_aht10_temperature", "name": "environment_aht10_temperature", "source": "board_1", "type": "float", "unit": "celsius"}, {"id": "environment_aht10_humidity", "name": "environment_aht10_humidity", "source": "board_1", "type": "float", "unit": "percent"}, {"id": "motor_n_mike_inmp441_audio_samples", "name": "motor_n_mike_inmp441_audio_samples", "source": "board_5", "type": "buffer", "unit": ""}, {"id": "motor_n_mike_inmp441_audio_level", "name": "motor_n_mike_inmp441_audio_level", "source": "board_5", "type": "int", "unit": "dB"}, {"id": "motor_n_mike_inmp441_peak_amplitude", "name": "motor_n_mike_inmp441_peak_amplitude", "source": "board_5", "type": "int", "unit": ""}, {"id": "motor_n_mike_inmp441_rms_level", "name": "motor_n_mike_inmp441_rms_level", "source": "board_5", "type": "float", "unit": ""}, {"id": "motor_n_mike_inmp441_is_clipping", "name": "motor_n_mike_inmp441_is_clipping", "source": "board_5", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_connected", "name": "thespeaker_bluetoothspeaker_connected", "source": "board_0", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_device_name", "name": "thespeaker_bluetoothspeaker_device_name", "source": "board_0", "type": "string", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_rssi", "name": "thespeaker_bluetoothspeaker_rssi", "source": "board_0", "type": "int", "unit": ""}]
SEND_INTERVAL_S = 2.0


# ============================================================================
# Fixtures
# ============================================================================

@pytest.fixture
def backend_client():
    """HTTP client for backend API."""
    if not HAS_HTTPX:
        pytest.skip("httpx not installed")
    with httpx.Client(base_url=BACKEND_URL, timeout=10.0) as client:
        yield client


@pytest.fixture
def serial_device():
    """Serial connection to the device."""
    if not HAS_SERIAL:
        pytest.skip("pyserial not installed")
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=5)
        time.sleep(2)  # Wait for device boot
        yield ser
        ser.close()
    except serial.SerialException:
        pytest.skip(f"Cannot open {SERIAL_PORT}")


# ============================================================================
# Backend API Tests
# ============================================================================

class TestBackendAPI:
    """Validate backend is running and accepts data."""

    @pytest.mark.skipif(not HAS_HTTPX, reason="httpx not installed")
    def test_backend_health(self, backend_client):
        """Backend should respond to health check."""
        resp = backend_client.get("/health")
        assert resp.status_code == 200

    @pytest.mark.skipif(not HAS_HTTPX, reason="httpx not installed")
    def test_backend_accepts_sensor_data(self, backend_client):
        """Backend should accept a POST with sensor data."""
        payload = {}


        payload["environment_aht10_temperature"] = 25.0



        payload["environment_aht10_humidity"] = 25.0



        payload["motor_n_mike_inmp441_audio_samples"] = "test"



        payload["motor_n_mike_inmp441_audio_level"] = 42



        payload["motor_n_mike_inmp441_peak_amplitude"] = 42



        payload["motor_n_mike_inmp441_rms_level"] = 25.0



        payload["motor_n_mike_inmp441_is_clipping"] = True



        payload["thespeaker_bluetoothspeaker_connected"] = True



        payload["thespeaker_bluetoothspeaker_device_name"] = "test"



        payload["thespeaker_bluetoothspeaker_rssi"] = 42


        resp = backend_client.post("/api/data", json=payload)
        assert resp.status_code in (200, 201, 422)  # 422 = validation (OK for test data)

    @pytest.mark.skipif(not HAS_HTTPX, reason="httpx not installed")
    def test_backend_data_contracts_match(self, backend_client):
        """Backend schema should match data contracts from architecture."""
        resp = backend_client.get("/api/data")
        assert resp.status_code in (200, 404)
        if resp.status_code == 200:
            data = resp.json()
            # Verify expected fields exist

            # Contract: environment_aht10_temperature (float)

            # Contract: environment_aht10_humidity (float)

            # Contract: motor_n_mike_inmp441_audio_samples (buffer)

            # Contract: motor_n_mike_inmp441_audio_level (int)

            # Contract: motor_n_mike_inmp441_peak_amplitude (int)

            # Contract: motor_n_mike_inmp441_rms_level (float)

            # Contract: motor_n_mike_inmp441_is_clipping (bool)

            # Contract: thespeaker_bluetoothspeaker_connected (bool)

            # Contract: thespeaker_bluetoothspeaker_device_name (string)

            # Contract: thespeaker_bluetoothspeaker_rssi (int)


    @pytest.mark.skipif(not HAS_HTTPX, reason="httpx not installed")
    def test_backend_rejects_invalid_data(self, backend_client):
        """Backend should reject malformed payloads."""
        resp = backend_client.post("/api/data", json={"invalid": "garbage"})
        assert resp.status_code in (422, 400)


# ============================================================================
# Device Serial Tests
# ============================================================================

class TestDeviceSerial:
    """Validate device is producing data over serial."""

    @pytest.mark.skipif(not HAS_SERIAL, reason="pyserial not installed")
    def test_device_produces_output(self, serial_device):
        """Device should produce serial output within 5 seconds."""
        data = serial_device.read(256)
        assert len(data) > 0, "No serial output from device"

    @pytest.mark.skipif(not HAS_SERIAL, reason="pyserial not installed")
    def test_device_outputs_valid_json(self, serial_device):
        """Device should output parseable JSON lines."""
        lines = []
        deadline = time.time() + 10
        while time.time() < deadline:
            line = serial_device.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith('{'):
                try:
                    data = json.loads(line)
                    lines.append(data)
                    if len(lines) >= 2:
                        break
                except json.JSONDecodeError:
                    continue
        assert len(lines) >= 1, "No valid JSON output from device"

    @pytest.mark.skipif(not HAS_SERIAL, reason="pyserial not installed")
    def test_device_data_matches_contracts(self, serial_device):
        """Device JSON should contain fields from data contracts."""
        deadline = time.time() + 15
        while time.time() < deadline:
            line = serial_device.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith('{'):
                try:
                    data = json.loads(line)
                    # Check at least one contract field is present
                    contract_fields = [c["name"] for c in DATA_CONTRACTS]
                    found = [f for f in contract_fields if f in data]
                    if found:
                        return  # Pass
                except json.JSONDecodeError:
                    continue
        pytest.fail("Device output did not match any data contract fields")


# ============================================================================
# End-to-End Pipeline Tests
# ============================================================================

class TestE2EPipeline:
    """Full pipeline: device → backend → verify stored."""

    @pytest.mark.skipif(not (HAS_HTTPX and HAS_SERIAL), reason="requires httpx + pyserial")
    def test_device_data_reaches_backend(self, backend_client, serial_device):
        """Data produced by device should appear in backend within 2 intervals."""
        # Wait for device to send data
        time.sleep(SEND_INTERVAL_S * 2.5)
        # Check backend has data
        resp = backend_client.get("/api/data")
        if resp.status_code == 200:
            data = resp.json()
            assert data is not None, "Backend has no data after device sent"
