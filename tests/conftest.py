"""
Shared pytest fixtures for E2E/System tests.
Auto-generated from architecture.json.
"""

import pytest
import os
import json
from pathlib import Path


def pytest_configure(config):
    config.addinivalue_line("markers", "hardware: requires physical device connected")
    config.addinivalue_line("markers", "backend: requires backend running")
    config.addinivalue_line("markers", "frontend: requires frontend built")
    config.addinivalue_line("markers", "slow: marks slow tests")


@pytest.fixture(scope="session")
def project_config():
    """Load project architecture configuration."""
    cfg_path = Path(__file__).parent.parent / "architecture.json"
    if cfg_path.exists():
        return json.loads(cfg_path.read_text())
    return {}


@pytest.fixture(scope="session")
def data_contracts():
    """Data contracts from the architecture."""
    return [{"id": "environment_aht10_temperature", "name": "environment_aht10_temperature", "source": "board_1", "type": "float", "unit": "celsius"}, {"id": "environment_aht10_humidity", "name": "environment_aht10_humidity", "source": "board_1", "type": "float", "unit": "percent"}, {"id": "motor_n_mike_inmp441_audio_samples", "name": "motor_n_mike_inmp441_audio_samples", "source": "board_5", "type": "buffer", "unit": ""}, {"id": "motor_n_mike_inmp441_audio_level", "name": "motor_n_mike_inmp441_audio_level", "source": "board_5", "type": "int", "unit": "dB"}, {"id": "motor_n_mike_inmp441_peak_amplitude", "name": "motor_n_mike_inmp441_peak_amplitude", "source": "board_5", "type": "int", "unit": ""}, {"id": "motor_n_mike_inmp441_rms_level", "name": "motor_n_mike_inmp441_rms_level", "source": "board_5", "type": "float", "unit": ""}, {"id": "motor_n_mike_inmp441_is_clipping", "name": "motor_n_mike_inmp441_is_clipping", "source": "board_5", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_connected", "name": "thespeaker_bluetoothspeaker_connected", "source": "board_0", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_device_name", "name": "thespeaker_bluetoothspeaker_device_name", "source": "board_0", "type": "string", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_rssi", "name": "thespeaker_bluetoothspeaker_rssi", "source": "board_0", "type": "int", "unit": ""}]
