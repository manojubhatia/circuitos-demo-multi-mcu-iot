"""
Backend Integration Tests - theSpeaker (3 boards)
Auto-generated. Tests API endpoints, data models, and storage.

Usage:
  pytest test_backend.py -v
"""

import pytest
import time
import json
from typing import Dict

try:
    import httpx
    HAS_HTTPX = True
except ImportError:
    HAS_HTTPX = False

BACKEND_URL = "http://localhost:8000"
DATA_CONTRACTS = [{"id": "environment_aht10_temperature", "name": "environment_aht10_temperature", "source": "board_1", "type": "float", "unit": "celsius"}, {"id": "environment_aht10_humidity", "name": "environment_aht10_humidity", "source": "board_1", "type": "float", "unit": "percent"}, {"id": "motor_n_mike_inmp441_audio_samples", "name": "motor_n_mike_inmp441_audio_samples", "source": "board_5", "type": "buffer", "unit": ""}, {"id": "motor_n_mike_inmp441_audio_level", "name": "motor_n_mike_inmp441_audio_level", "source": "board_5", "type": "int", "unit": "dB"}, {"id": "motor_n_mike_inmp441_peak_amplitude", "name": "motor_n_mike_inmp441_peak_amplitude", "source": "board_5", "type": "int", "unit": ""}, {"id": "motor_n_mike_inmp441_rms_level", "name": "motor_n_mike_inmp441_rms_level", "source": "board_5", "type": "float", "unit": ""}, {"id": "motor_n_mike_inmp441_is_clipping", "name": "motor_n_mike_inmp441_is_clipping", "source": "board_5", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_connected", "name": "thespeaker_bluetoothspeaker_connected", "source": "board_0", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_device_name", "name": "thespeaker_bluetoothspeaker_device_name", "source": "board_0", "type": "string", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_rssi", "name": "thespeaker_bluetoothspeaker_rssi", "source": "board_0", "type": "int", "unit": ""}]


@pytest.fixture
def client():
    if not HAS_HTTPX:
        pytest.skip("httpx not installed")
    with httpx.Client(base_url=BACKEND_URL, timeout=10.0) as c:
        yield c


# ============================================================================
# API Structure Tests
# ============================================================================

class TestAPIStructure:
    """Verify API routes and documentation exist."""

    def test_openapi_docs(self, client):
        """FastAPI should serve OpenAPI docs."""
        resp = client.get("/docs")
        assert resp.status_code == 200

    def test_openapi_json(self, client):
        """OpenAPI schema should be valid JSON."""
        resp = client.get("/openapi.json")
        assert resp.status_code == 200
        schema = resp.json()
        assert "paths" in schema
        assert "info" in schema

    def test_health_endpoint(self, client):
        resp = client.get("/health")
        assert resp.status_code == 200
        body = resp.json()
        assert body.get("status") in ("ok", "healthy")


# ============================================================================
# Data Model Tests
# ============================================================================

class TestDataModels:
    """Verify Pydantic models match data contracts."""

    def test_post_valid_data(self, client):
        """POST with valid contract data should succeed."""
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


        resp = client.post("/api/data", json=payload)
        assert resp.status_code in (200, 201)

    def test_post_missing_required_field(self, client):
        """POST missing a required field should return 422."""
        resp = client.post("/api/data", json={})
        assert resp.status_code == 422

    def test_post_invalid_type(self, client):
        """POST with wrong type should return 422."""



        resp = client.post("/api/data", json={"environment_aht10_temperature": "not_a_number"})
        assert resp.status_code == 422




# ============================================================================
# Data Retrieval Tests
# ============================================================================

class TestDataRetrieval:
    """Verify stored data can be retrieved."""

    def test_get_latest(self, client):
        """GET latest should return stored data."""
        resp = client.get("/api/data")
        assert resp.status_code in (200, 404)

    def test_get_history(self, client):
        """GET history should return list."""
        resp = client.get("/api/data/history")
        if resp.status_code == 200:
            data = resp.json()
            assert isinstance(data, list)

    def test_data_round_trip(self, client):
        """POST then GET should return same values."""
        payload = {}


        payload["environment_aht10_temperature"] = 42.5



        payload["environment_aht10_humidity"] = 42.5



        payload["motor_n_mike_inmp441_audio_samples"] = "roundtrip"



        payload["motor_n_mike_inmp441_audio_level"] = 100



        payload["motor_n_mike_inmp441_peak_amplitude"] = 100



        payload["motor_n_mike_inmp441_rms_level"] = 42.5



        payload["motor_n_mike_inmp441_is_clipping"] = True



        payload["thespeaker_bluetoothspeaker_connected"] = True



        payload["thespeaker_bluetoothspeaker_device_name"] = "roundtrip"



        payload["thespeaker_bluetoothspeaker_rssi"] = 100


        post_resp = client.post("/api/data", json=payload)
        if post_resp.status_code in (200, 201):
            get_resp = client.get("/api/data")
            if get_resp.status_code == 200:
                stored = get_resp.json()


                assert abs(stored.get("environment_aht10_temperature", 0) - payload["environment_aht10_temperature"]) < 0.01



                assert abs(stored.get("environment_aht10_humidity", 0) - payload["environment_aht10_humidity"]) < 0.01





                assert stored.get("motor_n_mike_inmp441_audio_level") == payload["motor_n_mike_inmp441_audio_level"]



                assert stored.get("motor_n_mike_inmp441_peak_amplitude") == payload["motor_n_mike_inmp441_peak_amplitude"]



                assert abs(stored.get("motor_n_mike_inmp441_rms_level", 0) - payload["motor_n_mike_inmp441_rms_level"]) < 0.01









                assert stored.get("thespeaker_bluetoothspeaker_rssi") == payload["thespeaker_bluetoothspeaker_rssi"]




# ============================================================================
# WebSocket Tests
# ============================================================================


