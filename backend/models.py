"""
Pydantic Models for theSpeaker (3 boards)
Generated from dataflow configuration (data contracts)
"""

from pydantic import BaseModel, Field, model_validator
from typing import Optional, List, Union
from datetime import datetime


class SignalMetadata(BaseModel):
    """Metadata about a signal for frontend configuration"""
    name: str
    type: str
    unit: str
    source: str
    display_name: Optional[str] = None
    min_value: Optional[float] = None
    max_value: Optional[float] = None


class SensorReading(BaseModel):
    """Single sensor reading with all signals from data contracts"""
    timestamp: datetime = Field(default_factory=datetime.utcnow)
    mcu_id: Optional[str] = Field(default=None, description="Source MCU identifier")
    environment_aht10_temperature: Optional[float] = Field(
        default=None,
        description="Environment Aht10 Temperature ()"    )
    environment_aht10_humidity: Optional[float] = Field(
        default=None,
        description="Environment Aht10 Humidity ()"    )
    motor_n_mike_inmp441_audio_samples: Optional[float] = Field(
        default=None,
        description="Inmp441 Audio Samples ()"    )
    motor_n_mike_inmp441_audio_level: Optional[int] = Field(
        default=None,
        description="Inmp441 Audio Level ()"    )
    motor_n_mike_inmp441_peak_amplitude: Optional[int] = Field(
        default=None,
        description="Inmp441 Peak Amplitude ()"    )
    motor_n_mike_inmp441_rms_level: Optional[float] = Field(
        default=None,
        description="Inmp441 Rms Level ()"    )
    motor_n_mike_inmp441_is_clipping: Optional[bool] = Field(
        default=None,
        description="Inmp441 Clipping ()"    )
    thespeaker_bluetoothspeaker_connected: Optional[bool] = Field(
        default=None,
        description="Thespeaker Bluetoothspeaker Connected ()"    )
    thespeaker_bluetoothspeaker_device_name: Optional[str] = Field(
        default=None,
        description="Bluetoothspeaker Name ()"    )
    thespeaker_bluetoothspeaker_rssi: Optional[int] = Field(
        default=None,
        description="Thespeaker Bluetoothspeaker Rssi ()"    )

    model_config = {
        "json_encoders": {datetime: lambda v: v.isoformat() if v else None},
        "coerce_numbers_to_str": False,
        "populate_by_name": True,
    }

    @model_validator(mode='before')
    @classmethod
    def _coerce_floats_to_int(cls, values):
        """Coerce float values to int for int-typed fields to handle device imprecision."""
        if not isinstance(values, dict):
            return values
        for field_name, field_info in cls.model_fields.items():
            if field_name in values and values[field_name] is not None:
                anno = field_info.annotation
                origin = getattr(anno, '__origin__', None)
                args = getattr(anno, '__args__', ())
                if origin is Union and int in args and isinstance(values[field_name], float):
                    values[field_name] = int(values[field_name])
        return values


class SensorReadingCreate(BaseModel):
    """Schema for creating a new sensor reading (device POST)"""
    device_id: Optional[str] = Field(default=None, description="Device identifier")
    environment_aht10_temperature: Optional[float] = None
    environment_aht10_humidity: Optional[float] = None
    motor_n_mike_inmp441_audio_samples: Optional[float] = None
    motor_n_mike_inmp441_audio_level: Optional[int] = None
    motor_n_mike_inmp441_peak_amplitude: Optional[int] = None
    motor_n_mike_inmp441_rms_level: Optional[float] = None
    motor_n_mike_inmp441_is_clipping: Optional[bool] = None
    thespeaker_bluetoothspeaker_connected: Optional[bool] = None
    thespeaker_bluetoothspeaker_device_name: Optional[str] = None
    thespeaker_bluetoothspeaker_rssi: Optional[int] = None


class HistoryResponse(BaseModel):
    """Response for historical data queries"""
    data: List[SensorReading]
    count: int


class SignalHistoryResponse(BaseModel):
    """Response for single signal history"""
    signal: str
    data: List[dict]


class StatusResponse(BaseModel):
    """API status response"""
    status: str
    project: str
    signals_count: int
    buffer_size: int
    active_connections: int


# Signal metadata from architecture data contracts
SIGNAL_METADATA: List[SignalMetadata] = [
    SignalMetadata(
        name="environment_aht10_temperature",
        type="float",
        unit="",
        source="board_1",
        display_name="Environment Aht10 Temperature",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="environment_aht10_humidity",
        type="float",
        unit="",
        source="board_1",
        display_name="Environment Aht10 Humidity",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="motor_n_mike_inmp441_audio_samples",
        type="buffer",
        unit="",
        source="board_5",
        display_name="Inmp441 Audio Samples",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="motor_n_mike_inmp441_audio_level",
        type="int",
        unit="",
        source="board_5",
        display_name="Inmp441 Audio Level",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="motor_n_mike_inmp441_peak_amplitude",
        type="int",
        unit="",
        source="board_5",
        display_name="Inmp441 Peak Amplitude",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="motor_n_mike_inmp441_rms_level",
        type="float",
        unit="",
        source="board_5",
        display_name="Inmp441 Rms Level",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="motor_n_mike_inmp441_is_clipping",
        type="bool",
        unit="",
        source="board_5",
        display_name="Inmp441 Clipping",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="thespeaker_bluetoothspeaker_connected",
        type="bool",
        unit="",
        source="board_0",
        display_name="Thespeaker Bluetoothspeaker Connected",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="thespeaker_bluetoothspeaker_device_name",
        type="string",
        unit="",
        source="board_0",
        display_name="Bluetoothspeaker Name",
        min_value=None,
        max_value=None    ),
    SignalMetadata(
        name="thespeaker_bluetoothspeaker_rssi",
        type="int",
        unit="",
        source="board_0",
        display_name="Thespeaker Bluetoothspeaker Rssi",
        min_value=None,
        max_value=None    ),
]


# =============================================================================
# Parameter Models (for sending values TO MCUs)
# =============================================================================
