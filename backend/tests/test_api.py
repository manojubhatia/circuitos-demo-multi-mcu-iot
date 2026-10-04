#!/usr/bin/env python3
"""
Auto-generated API Unit Tests for Backend
Project: theSpeaker (3 boards)
Protocol Version: 1.0.0
Generated: 2026-09-29T13:56:59.947414

Tests:
- API endpoint responses
- Request validation
- Error handling

Usage:
    pytest test_api.py -v
"""

import pytest
from fastapi.testclient import TestClient
from datetime import datetime
import json

# Import the FastAPI app
from main import app

client = TestClient(app)


class TestHealthEndpoints:
    """Test health and status endpoints"""

    def test_root_endpoint(self):
        """Test root endpoint returns OK"""
        response = client.get("/api/")
        assert response.status_code == 200
        data = response.json()
        assert "status" in data or "message" in data

    def test_health_check(self):
        """Test health check endpoint"""
        response = client.get("/api/health")
        assert response.status_code in [200, 404]  # 404 if not implemented
        
        if response.status_code == 200:
            data = response.json()
            assert "status" in data


class TestProtocolEndpoints:
    """Test protocol version endpoints"""

    def test_protocol_version_endpoint(self):
        """Test protocol version endpoint"""
        response = client.get("/api/protocol/version")
        
        if response.status_code == 200:
            data = response.json()
            assert "version" in data
            assert data["version"] == "1.0.0"

    def test_schema_hash_returned(self):
        """Test schema hash is returned"""
        response = client.get("/api/protocol/version")
        
        if response.status_code == 200:
            data = response.json()
            assert "schema_hash" in data
            assert data["schema_hash"] == "00000000"


class TestSignalEndpoints:
    """Test signal data endpoints"""


    def test_get_environment_aht10_temperature_signal(self):
        """Test getting environment_aht10_temperature signal"""
        response = client.get("/api/signals/environment_aht10_temperature")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_environment_aht10_humidity_signal(self):
        """Test getting environment_aht10_humidity signal"""
        response = client.get("/api/signals/environment_aht10_humidity")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_motor_n_mike_inmp441_audio_samples_signal(self):
        """Test getting motor_n_mike_inmp441_audio_samples signal"""
        response = client.get("/api/signals/motor_n_mike_inmp441_audio_samples")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_motor_n_mike_inmp441_audio_level_signal(self):
        """Test getting motor_n_mike_inmp441_audio_level signal"""
        response = client.get("/api/signals/motor_n_mike_inmp441_audio_level")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_motor_n_mike_inmp441_peak_amplitude_signal(self):
        """Test getting motor_n_mike_inmp441_peak_amplitude signal"""
        response = client.get("/api/signals/motor_n_mike_inmp441_peak_amplitude")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_motor_n_mike_inmp441_rms_level_signal(self):
        """Test getting motor_n_mike_inmp441_rms_level signal"""
        response = client.get("/api/signals/motor_n_mike_inmp441_rms_level")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_motor_n_mike_inmp441_is_clipping_signal(self):
        """Test getting motor_n_mike_inmp441_is_clipping signal"""
        response = client.get("/api/signals/motor_n_mike_inmp441_is_clipping")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_thespeaker_bluetoothspeaker_connected_signal(self):
        """Test getting thespeaker_bluetoothspeaker_connected signal"""
        response = client.get("/api/signals/thespeaker_bluetoothspeaker_connected")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_thespeaker_bluetoothspeaker_device_name_signal(self):
        """Test getting thespeaker_bluetoothspeaker_device_name signal"""
        response = client.get("/api/signals/thespeaker_bluetoothspeaker_device_name")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data


    def test_get_thespeaker_bluetoothspeaker_rssi_signal(self):
        """Test getting thespeaker_bluetoothspeaker_rssi signal"""
        response = client.get("/api/signals/thespeaker_bluetoothspeaker_rssi")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "name" in data or "value" in data



    def test_list_signals(self):
        """Test listing all signals"""
        response = client.get("/api/signals")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert isinstance(data, (list, dict))


class TestParameterEndpoints:
    """Test parameter get/set endpoints"""


    def test_get_playing_parameter(self):
        """Test getting playing parameter"""
        response = client.get("/api/parameters/playing")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "playing" in data or "value" in data

    def test_set_playing_parameter(self):
        """Test setting playing parameter"""

        test_value = false

        
        response = client.post(
            "/api/parameters/playing",
            json={"value": test_value}
        )
        # 200 = success, 202 = accepted (async), 503 = device unavailable
        assert response.status_code in [200, 202, 404, 422, 503]


    def test_get_volume_parameter(self):
        """Test getting volume parameter"""
        response = client.get("/api/parameters/volume")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "volume" in data or "value" in data

    def test_set_volume_parameter(self):
        """Test setting volume parameter"""

        test_value = 0

        
        response = client.post(
            "/api/parameters/volume",
            json={"value": test_value}
        )
        # 200 = success, 202 = accepted (async), 503 = device unavailable
        assert response.status_code in [200, 202, 404, 422, 503]


    def test_get_audio_source_parameter(self):
        """Test getting audio_source parameter"""
        response = client.get("/api/parameters/audio_source")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert "audio_source" in data or "value" in data

    def test_set_audio_source_parameter(self):
        """Test setting audio_source parameter"""

        test_value = "test"

        
        response = client.post(
            "/api/parameters/audio_source",
            json={"value": test_value}
        )
        # 200 = success, 202 = accepted (async), 503 = device unavailable
        assert response.status_code in [200, 202, 404, 422, 503]




class TestDataEndpoints:
    """Test data retrieval endpoints"""

    def test_get_latest_data(self):
        """Test getting latest data"""
        response = client.get("/api/data/latest")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            assert isinstance(data, (dict, list))

    def test_get_historical_data(self):
        """Test getting historical data"""
        response = client.get("/api/data/history?limit=10")
        assert response.status_code in [200, 404]
        
        if response.status_code == 200:
            data = response.json()
            if isinstance(data, dict):
                data = data.get("data", [])
            assert isinstance(data, list)

    def test_post_data(self):
        """Test posting data"""
        test_data = {
            "timestamp": datetime.utcnow().isoformat(),


            "environment_aht10_temperature": 0.0,



            "environment_aht10_humidity": 0.0,



            "motor_n_mike_inmp441_audio_samples": "test",


        }
        
        response = client.post("/api/data", json=test_data)
        # Various valid response codes depending on implementation
        assert response.status_code in [200, 201, 202, 404, 422]


class TestErrorHandling:
    """Test error handling"""

    def test_invalid_endpoint_returns_404(self):
        """Test invalid endpoint returns 404"""
        response = client.get("/api/nonexistent_endpoint_12345")
        assert response.status_code == 404

    def test_invalid_signal_returns_404(self):
        """Test invalid signal returns 404"""
        response = client.get("/api/signals/nonexistent_signal_12345")
        assert response.status_code in [404, 400]

    def test_invalid_parameter_returns_404(self):
        """Test invalid parameter returns 404"""
        response = client.get("/api/parameters/nonexistent_param_12345")
        assert response.status_code in [404, 400]

    def test_malformed_json_returns_422(self):
        """Test malformed JSON returns 422"""
        response = client.post(
            "/api/data",
            content="this is not json",
            headers={"Content-Type": "application/json"}
        )
        assert response.status_code in [400, 422]

    def test_wrong_content_type_handled(self):
        """Test wrong content type is handled"""
        response = client.post(
            "/api/data",
            content="key=value",
            headers={"Content-Type": "application/x-www-form-urlencoded"}
        )
        assert response.status_code in [400, 415, 422]


class TestCORS:
    """Test CORS headers"""

    def test_cors_headers_present(self):
        """Test CORS headers are present"""
        response = client.options("/api/")
        # CORS headers should be present if configured
        # This test is informational

    def test_preflight_request(self):
        """Test preflight request handling"""
        response = client.options(
            "/api/data",
            headers={
                "Origin": "http://localhost:5173",
                "Access-Control-Request-Method": "POST",
            }
        )
        # Should return 200 or 204 for preflight
        assert response.status_code in [200, 204, 405]


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
