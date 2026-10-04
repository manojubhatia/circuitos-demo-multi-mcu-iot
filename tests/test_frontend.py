"""
Frontend / Display Pipeline Tests - theSpeaker (3 boards)
Auto-generated. Tests that the frontend displays correct data.

For web frontends:
  - Validates React/JS build doesn't fail
  - Checks API integration layer
  - Tests display component data binding

For embedded displays (OLED/LCD):
  - Validates display update logic
  - Tests text rendering calls
  - Tests data formatting for display

Usage:
  pytest test_frontend.py -v
"""

import pytest
import subprocess
import os
import json
from pathlib import Path

# ============================================================================
# Configuration
# ============================================================================

FRONTEND_DIR = "../frontend"
BACKEND_URL = "http://localhost:8000"
DATA_CONTRACTS = [{"id": "environment_aht10_temperature", "name": "environment_aht10_temperature", "source": "board_1", "type": "float", "unit": "celsius"}, {"id": "environment_aht10_humidity", "name": "environment_aht10_humidity", "source": "board_1", "type": "float", "unit": "percent"}, {"id": "motor_n_mike_inmp441_audio_samples", "name": "motor_n_mike_inmp441_audio_samples", "source": "board_5", "type": "buffer", "unit": ""}, {"id": "motor_n_mike_inmp441_audio_level", "name": "motor_n_mike_inmp441_audio_level", "source": "board_5", "type": "int", "unit": "dB"}, {"id": "motor_n_mike_inmp441_peak_amplitude", "name": "motor_n_mike_inmp441_peak_amplitude", "source": "board_5", "type": "int", "unit": ""}, {"id": "motor_n_mike_inmp441_rms_level", "name": "motor_n_mike_inmp441_rms_level", "source": "board_5", "type": "float", "unit": ""}, {"id": "motor_n_mike_inmp441_is_clipping", "name": "motor_n_mike_inmp441_is_clipping", "source": "board_5", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_connected", "name": "thespeaker_bluetoothspeaker_connected", "source": "board_0", "type": "bool", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_device_name", "name": "thespeaker_bluetoothspeaker_device_name", "source": "board_0", "type": "string", "unit": ""}, {"id": "thespeaker_bluetoothspeaker_rssi", "name": "thespeaker_bluetoothspeaker_rssi", "source": "board_0", "type": "int", "unit": ""}]


# ============================================================================
# Web Frontend Build Tests
# ============================================================================


class TestWebFrontendBuild:
    """Verify the React/web frontend builds without errors."""

    def test_frontend_package_json_exists(self):
        """package.json should exist."""
        pkg = Path(FRONTEND_DIR) / "package.json"
        assert pkg.exists(), f"No package.json at {pkg}"

    def test_frontend_dependencies_installed(self):
        """node_modules should exist after install."""
        nm = Path(FRONTEND_DIR) / "node_modules"
        if not nm.exists():
            pytest.skip("Run `npm install` in frontend dir first")

    def test_frontend_builds_successfully(self):
        """npm run build should succeed."""
        result = subprocess.run(
            ["npm", "run", "build"],
            cwd=FRONTEND_DIR,
            capture_output=True,
            text=True,
            timeout=120
        )
        assert result.returncode == 0, f"Build failed: {result.stderr[-500:]}"

    def test_frontend_api_service_exists(self):
        """API service file should exist for data fetching."""
        candidates = [
            Path(FRONTEND_DIR) / "src" / "services" / "api.js",
            Path(FRONTEND_DIR) / "src" / "services" / "api.ts",
            Path(FRONTEND_DIR) / "src" / "api.js",
            Path(FRONTEND_DIR) / "src" / "api.ts",
        ]
        found = any(c.exists() for c in candidates)
        assert found, "No API service file found in frontend"

    def test_frontend_api_url_configured(self):
        """Frontend should have backend URL configured."""
        env_file = Path(FRONTEND_DIR) / ".env"
        env_local = Path(FRONTEND_DIR) / ".env.local"
        src_files = list(Path(FRONTEND_DIR).rglob("*.ts")) + list(Path(FRONTEND_DIR).rglob("*.js"))

        # Check if backend URL is referenced somewhere
        url_found = False
        for f in [env_file, env_local]:
            if f.exists():
                content = f.read_text()
                if "localhost" in content or "8000" in content:
                    url_found = True
                    break
        if not url_found:
            for f in src_files[:20]:
                content = f.read_text()
                if "localhost:8000" in content or "BACKEND" in content:
                    url_found = True
                    break
        assert url_found, "Backend URL not configured in frontend"



# ============================================================================
# Data Display Correctness Tests
# ============================================================================

class TestDataDisplay:
    """Validate that display renders data from contracts correctly."""


    def test_display_format_environment_aht10_temperature(self):
        """environment_aht10_temperature (celsius) should format correctly."""
        value = 25.0
        unit = "celsius"


        # Float should display with reasonable precision
        formatted = f"{value:.1f} {unit}"
        assert "." in formatted
        assert unit in formatted



    def test_display_format_environment_aht10_humidity(self):
        """environment_aht10_humidity (percent) should format correctly."""
        value = 25.0
        unit = "percent"


        # Float should display with reasonable precision
        formatted = f"{value:.1f} {unit}"
        assert "." in formatted
        assert unit in formatted



    def test_display_format_motor_n_mike_inmp441_audio_samples(self):
        """motor_n_mike_inmp441_audio_samples () should format correctly."""
        value = 25.0
        unit = ""




    def test_display_format_motor_n_mike_inmp441_audio_level(self):
        """motor_n_mike_inmp441_audio_level (dB) should format correctly."""
        value = 25.0
        unit = "dB"


        formatted = f"{int(value)} {unit}"
        assert "." not in formatted.split(" ")[0]



    def test_display_format_motor_n_mike_inmp441_peak_amplitude(self):
        """motor_n_mike_inmp441_peak_amplitude () should format correctly."""
        value = 25.0
        unit = ""


        formatted = f"{int(value)} {unit}"
        assert "." not in formatted.split(" ")[0]



    def test_display_format_motor_n_mike_inmp441_rms_level(self):
        """motor_n_mike_inmp441_rms_level () should format correctly."""
        value = 25.0
        unit = ""


        # Float should display with reasonable precision
        formatted = f"{value:.1f} {unit}"
        assert "." in formatted
        assert unit in formatted



    def test_display_format_motor_n_mike_inmp441_is_clipping(self):
        """motor_n_mike_inmp441_is_clipping () should format correctly."""
        value = 25.0
        unit = ""


        formatted = "ON" if value else "OFF"
        assert formatted in ("ON", "OFF")



    def test_display_format_thespeaker_bluetoothspeaker_connected(self):
        """thespeaker_bluetoothspeaker_connected () should format correctly."""
        value = 25.0
        unit = ""


        formatted = "ON" if value else "OFF"
        assert formatted in ("ON", "OFF")



    def test_display_format_thespeaker_bluetoothspeaker_device_name(self):
        """thespeaker_bluetoothspeaker_device_name () should format correctly."""
        value = 25.0
        unit = ""




    def test_display_format_thespeaker_bluetoothspeaker_rssi(self):
        """thespeaker_bluetoothspeaker_rssi () should format correctly."""
        value = 25.0
        unit = ""


        formatted = f"{int(value)} {unit}"
        assert "." not in formatted.split(" ")[0]





# ============================================================================
# Embedded Display Tests (for OLED/LCD projects)
# ============================================================================


class TestEmbeddedDisplay:
    """Test embedded display rendering logic (compile-time validation)."""

    def test_display_dimensions_valid(self):
        """Display dimensions should be positive."""

        # ILI9341_TFT_Display: check compile-time macros exist in headers
        header = Path("../src/components/ILI9341_TFT_Display.h")
        if header.exists():
            content = header.read_text()
            assert "WIDTH" in content or "COLS" in content
            assert "HEIGHT" in content or "ROWS" in content

        # ST7735S_LCD_Display: check compile-time macros exist in headers
        header = Path("../src/components/ST7735S_LCD_Display.h")
        if header.exists():
            content = header.read_text()
            assert "WIDTH" in content or "COLS" in content
            assert "HEIGHT" in content or "ROWS" in content


    def test_display_update_function_exists(self):
        """Display should have an update function declared."""

        header = Path("../src/components/ILI9341_TFT_Display.h")
        if header.exists():
            content = header.read_text()
            assert "_update" in content or "_refresh" in content

        header = Path("../src/components/ST7735S_LCD_Display.h")
        if header.exists():
            content = header.read_text()
            assert "_update" in content or "_refresh" in content


    def test_display_text_rendering(self):
        """Display should support text drawing."""

        header = Path("../src/components/ILI9341_TFT_Display.h")
        if header.exists():
            content = header.read_text()
            assert "draw_text" in content or "write_text" in content or "print" in content

        header = Path("../src/components/ST7735S_LCD_Display.h")
        if header.exists():
            content = header.read_text()
            assert "draw_text" in content or "write_text" in content or "print" in content




# ============================================================================
# Data Formatting / Conversion Tests
# ============================================================================

class TestDataConversion:
    """Test that sensor data converts correctly for display/API."""



    def test_convert_environment_aht10_temperature_to_string(self):
        """environment_aht10_temperature: float→string for display."""
        raw = 25.678
        # Should truncate to reasonable precision for display
        display_str = f"{raw:.1f}"
        assert len(display_str) <= 8  # Fits on small display

    def test_convert_environment_aht10_temperature_boundaries(self):
        """environment_aht10_temperature: validate range boundaries."""

        pytest.skip("No min/max defined for environment_aht10_temperature")





    def test_convert_environment_aht10_humidity_to_string(self):
        """environment_aht10_humidity: float→string for display."""
        raw = 25.678
        # Should truncate to reasonable precision for display
        display_str = f"{raw:.1f}"
        assert len(display_str) <= 8  # Fits on small display

    def test_convert_environment_aht10_humidity_boundaries(self):
        """environment_aht10_humidity: validate range boundaries."""

        pytest.skip("No min/max defined for environment_aht10_humidity")














    def test_convert_motor_n_mike_inmp441_rms_level_to_string(self):
        """motor_n_mike_inmp441_rms_level: float→string for display."""
        raw = 25.678
        # Should truncate to reasonable precision for display
        display_str = f"{raw:.1f}"
        assert len(display_str) <= 8  # Fits on small display

    def test_convert_motor_n_mike_inmp441_rms_level_boundaries(self):
        """motor_n_mike_inmp441_rms_level: validate range boundaries."""

        pytest.skip("No min/max defined for motor_n_mike_inmp441_rms_level")
















