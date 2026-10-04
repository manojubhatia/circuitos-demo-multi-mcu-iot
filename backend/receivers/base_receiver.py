"""
Base Receiver for MCU Communication

Provides common functionality for decoding incoming data from MCUs.
Generated from dataflow configuration with 10 signals.
"""

import struct
from abc import ABC, abstractmethod
from datetime import datetime
from typing import Dict, Any, Optional, Callable, List
from loguru import logger

from models import SensorReading, SIGNAL_METADATA


class DataFrameDecoder:
    """
    Decodes incoming binary dataframes from MCUs.
    
    Frame format matches the C struct generated for device code:
    - Magic byte (0xAA) 
    - Frame ID (1 byte)
    - Timestamp (4 bytes, uint32)
    - Payload (variable, based on signals)
    - CRC16 (2 bytes)
    """
    
    # Signal type to struct format mapping
    TYPE_TO_FORMAT = {
        'float': 'f',    # 4 bytes
        'double': 'd',   # 8 bytes
        'int': 'i',      # 4 bytes signed
        'int8': 'b',     # 1 byte signed
        'int16': 'h',    # 2 bytes signed
        'int32': 'i',    # 4 bytes signed
        'uint8': 'B',    # 1 byte unsigned
        'uint16': 'H',   # 2 bytes unsigned
        'uint32': 'I',   # 4 bytes unsigned
        'bool': '?',     # 1 byte
        'boolean': '?',
    }
    
    # Frame definitions from dataflow
    # Default frame with all signals
    FRAME_DEFINITIONS = {
        0x01: {
            'name': 'sensor_data',
            'signals': [
                {'name': 'environment_aht10_temperature', 'type': 'float'},
                {'name': 'environment_aht10_humidity', 'type': 'float'},
                {'name': 'motor_n_mike_inmp441_audio_samples', 'type': 'buffer'},
                {'name': 'motor_n_mike_inmp441_audio_level', 'type': 'int'},
                {'name': 'motor_n_mike_inmp441_peak_amplitude', 'type': 'int'},
                {'name': 'motor_n_mike_inmp441_rms_level', 'type': 'float'},
                {'name': 'motor_n_mike_inmp441_is_clipping', 'type': 'bool'},
                {'name': 'thespeaker_bluetoothspeaker_connected', 'type': 'bool'},
                {'name': 'thespeaker_bluetoothspeaker_device_name', 'type': 'string'},
                {'name': 'thespeaker_bluetoothspeaker_rssi', 'type': 'int'},
            ],
        },
    }
    
    # Per-MCU signal mapping from dataflow bindings
    MCU_SIGNALS = {
        'board_0': [
            {'name': 'thespeaker_bluetoothspeaker_connected', 'type': 'bool', 'unit': ''},
            {'name': 'thespeaker_bluetoothspeaker_device_name', 'type': 'string', 'unit': ''},
            {'name': 'thespeaker_bluetoothspeaker_rssi', 'type': 'int', 'unit': ''},
        ],
        'board_1': [
            {'name': 'environment_aht10_temperature', 'type': 'float', 'unit': ''},
            {'name': 'environment_aht10_humidity', 'type': 'float', 'unit': ''},
        ],
        'board_5': [
            {'name': 'motor_n_mike_inmp441_audio_samples', 'type': 'buffer', 'unit': ''},
            {'name': 'motor_n_mike_inmp441_audio_level', 'type': 'int', 'unit': ''},
            {'name': 'motor_n_mike_inmp441_peak_amplitude', 'type': 'int', 'unit': ''},
            {'name': 'motor_n_mike_inmp441_rms_level', 'type': 'float', 'unit': ''},
            {'name': 'motor_n_mike_inmp441_is_clipping', 'type': 'bool', 'unit': ''},
        ],
    }
    
    # Short name -> full signal name mapping
    # Allows devices to send compact field names (temperature, humidity) 
    # while backend expects full names (sensor_readTemperature, etc.)
    SHORT_NAME_MAP = {
        'inmp441_audio_samples': 'motor_n_mike_inmp441_audio_samples',
        'inmp441_audio_level': 'motor_n_mike_inmp441_audio_level',
        'inmp441_peak_amplitude': 'motor_n_mike_inmp441_peak_amplitude',
        'inmp441_rms_level': 'motor_n_mike_inmp441_rms_level',
        'inmp441_clipping': 'motor_n_mike_inmp441_is_clipping',
        'bluetoothspeaker_name': 'thespeaker_bluetoothspeaker_device_name',
    }
    
    @classmethod
    def _build_short_name_map(cls) -> Dict[str, str]:
        """Build short name mapping dynamically from MCU_SIGNALS"""
        mapping = dict(cls.SHORT_NAME_MAP)  # Start with static map
        
        # Add dynamic mappings based on signal names
        for mcu_id, signals in cls.MCU_SIGNALS.items():
            for signal in signals:
                full_name = signal['name']
                parts = full_name.lower().split('_')
                
                # Map common patterns
                if 'temperature' in parts or 'readtemperature' in parts:
                    mapping.setdefault('temperature', full_name)
                    mapping.setdefault('temp', full_name)
                if 'humidity' in parts or 'readhumidity' in parts:
                    mapping.setdefault('humidity', full_name)
                    mapping.setdefault('hum', full_name)
                if 'pressure' in parts:
                    mapping.setdefault('pressure', full_name)
                if 'altitude' in parts:
                    mapping.setdefault('altitude', full_name)
                if 'light' in parts or 'ldr' in parts:
                    mapping.setdefault('light', full_name)
                    mapping.setdefault('ldr', full_name)
                    mapping.setdefault('light_level', full_name)
                if 'audio' in parts:
                    mapping.setdefault('audio_level', full_name)
                    mapping.setdefault('audio', full_name)
                
                # Also map partial suffixes: strip board prefix and
                # map the component+signal portion for flexible matching.
                # e.g. "board0_aht10_temperature" -> map "aht10_temperature"
                for i in range(1, len(parts)):
                    suffix = '_'.join(parts[i:])
                    mapping.setdefault(suffix, full_name)
        
        return mapping
    
    @classmethod
    def decode_frame(cls, data: bytes, mcu_id: Optional[str] = None) -> Optional[Dict[str, Any]]:
        """
        Decode a binary dataframe from an MCU.
        
        Args:
            data: Raw bytes received from MCU
            mcu_id: Optional MCU identifier for context-aware decoding
        
        Returns:
            Dict with decoded signal values, or None if decode fails
        """
        if len(data) < 8:  # Minimum: magic + frame_id + timestamp + crc
            logger.warning(f"Frame too short: {len(data)} bytes")
            return None
        
        try:
            # Parse header
            magic = data[0]
            if magic != 0xAA:
                logger.warning(f"Invalid magic byte: {hex(magic)}")
                return None
            
            frame_id = data[1]
            timestamp = struct.unpack('<I', data[2:6])[0]  # Little-endian uint32
            
            # Get frame definition
            frame_def = cls.FRAME_DEFINITIONS.get(frame_id)
            if not frame_def:
                logger.warning(f"Unknown frame ID: {frame_id}")
                return None
            
            # Decode payload
            offset = 6
            result = {
                'timestamp': datetime.utcnow(),
                'mcu_id': mcu_id,
                'frame_id': frame_id,
                'frame_name': frame_def['name'],
            }
            
            for signal in frame_def['signals']:
                signal_name = signal['name']
                signal_type = signal['type'].lower()
                
                fmt = cls.TYPE_TO_FORMAT.get(signal_type, 'f')
                size = struct.calcsize(fmt)
                
                if offset + size > len(data) - 2:  # -2 for CRC
                    logger.warning(f"Not enough data for signal {signal_name}")
                    break
                
                value = struct.unpack(f'<{fmt}', data[offset:offset + size])[0]
                result[signal_name] = value
                offset += size
            
            # Verify CRC (optional)
            # received_crc = struct.unpack('<H', data[-2:])[0]
            # calculated_crc = cls._calculate_crc(data[:-2])
            
            return result
            
        except Exception as e:
            logger.error(f"Frame decode error: {e}")
            return None
    
    @classmethod
    def decode_json(cls, data: Dict[str, Any], mcu_id: Optional[str] = None) -> Dict[str, Any]:
        """
        Decode JSON data from an MCU.
        
        Handles both flat JSON and nested structures.
        Maps short field names (temperature, humidity) to full signal names.
        """
        resolved_mcu_id = mcu_id or data.get('device_id') or data.get('mcu_id') or next(iter(cls.MCU_SIGNALS.keys()), None)
        result = {
            'timestamp': datetime.utcnow(),
            'mcu_id': resolved_mcu_id,
        }
        
        # Get expected signals for this MCU (use resolved mcu_id from data if header was empty)
        expected_signals = cls.MCU_SIGNALS.get(resolved_mcu_id, [])
        expected_names = {s['name'] for s in expected_signals} if expected_signals else None
        
        # Build short name mapping
        name_map = cls._build_short_name_map()
        
        # Build case-insensitive lookup for expected signal names
        expected_names_lower = {s.lower(): s for s in expected_names} if expected_names else None
        
        # Build underscore-stripped lookup for fuzzy matching
        # Maps "ldrlightsensor" -> "esp32_cheap_yellow_display__cyd__ldrlightsensor_light_level"
        expected_names_norm = {}
        if expected_names:
            for en in expected_names:
                en_norm = en.lower().replace('_', '')
                expected_names_norm[en_norm] = en
        
        # Extract signal values from data
        for key, value in data.items():
            if key in ('device_id', 'mcu_id', 'timestamp', 'frame_id'):
                continue
            
            # Strip mcu_id prefix (e.g., "board_0::temperature" -> "temperature")
            stripped_key = key.split('::')[-1] if '::' in key else key
            lookup_key = stripped_key.lower()
            
            # Try direct match first (preserves case), then short name map
            if expected_names and stripped_key in expected_names:
                result[stripped_key] = value
            else:
                # Map short name to full name
                mapped_key = name_map.get(lookup_key, None)
                if mapped_key:
                    result[mapped_key] = value
                elif expected_names_lower and lookup_key in expected_names_lower:
                    result[expected_names_lower[lookup_key]] = value
                elif expected_names is None:
                    result[stripped_key] = value
                elif expected_names_norm:
                    # Fuzzy: strip underscores from incoming key and check if it
                    # appears as a substring of any expected signal name
                    key_norm = lookup_key.replace('_', '')
                    for en_norm, en_full in expected_names_norm.items():
                        if key_norm in en_norm:
                            result[en_full] = value
                            break
        
        def _fuzzy_resolve(k: str) -> Optional[str]:
            """Try to resolve a key to a known signal via substring matching."""
            k_norm = k.lower().replace('_', '')
            for en_norm, en_full in expected_names_norm.items():
                if k_norm in en_norm:
                    return en_full
            return None
        
        # Handle nested 'data' or 'signals' fields
        for nested_key in ('data', 'signals'):
            nested = data.get(nested_key)
            if not isinstance(nested, dict):
                continue
            for key, value in nested.items():
                mapped_key = name_map.get(key.lower(), None)
                if mapped_key:
                    result[mapped_key] = value
                elif expected_names and key in expected_names:
                    result[key] = value
                elif expected_names_lower and key.lower() in expected_names_lower:
                    result[expected_names_lower[key.lower()]] = value
                elif expected_names is None:
                    result[key] = value
                elif expected_names_norm:
                    fuzzy = _fuzzy_resolve(key)
                    if fuzzy:
                        result[fuzzy] = value
        
        return result


class BaseReceiver(ABC):
    """
    Base class for MCU communication receivers.
    
    Subclasses implement specific protocols (HTTP, MQTT, TCP, etc.)
    """
    
    def __init__(
        self,
        on_data_received: Callable[[SensorReading], None],
        on_data_received_async: Optional[Callable[[SensorReading], Any]] = None,
        on_stream_event: Optional[Callable[[Dict[str, Any]], Any]] = None,
    ):
        """
        Initialize the receiver.
        
        Args:
            on_data_received: Sync callback when data is received
            on_data_received_async: Async callback when data is received
            on_stream_event: Async callback for stream lifecycle events (start/stop)
        """
        self.on_data_received = on_data_received
        self.on_data_received_async = on_data_received_async
        self.on_stream_event = on_stream_event
        self.decoder = DataFrameDecoder()
        self._running = False
    
    @abstractmethod
    async def start(self):
        """Start the receiver"""
        pass
    
    @abstractmethod
    async def stop(self):
        """Stop the receiver"""
        pass
    
    def process_data(self, data: Any, mcu_id: Optional[str] = None) -> Optional[SensorReading]:
        """
        Process incoming data and convert to SensorReading.
        
        Args:
            data: Raw data (bytes, dict, or str)
            mcu_id: Source MCU identifier
        
        Returns:
            SensorReading if successful, None otherwise
        """
        try:
            if isinstance(data, bytes):
                # Binary dataframe
                decoded = self.decoder.decode_frame(data, mcu_id)
            elif isinstance(data, str):
                # JSON string
                import json
                decoded = self.decoder.decode_json(json.loads(data), mcu_id)
            elif isinstance(data, dict):
                # Already parsed dict
                decoded = self.decoder.decode_json(data, mcu_id)
            else:
                logger.warning(f"Unknown data type: {type(data)}")
                return None
            
            if decoded:
                # Convert to SensorReading
                # Include mcu_id and timestamp explicitly, plus all signal names
                allowed_fields = {'mcu_id', 'timestamp'}
                allowed_fields.update([
                    'environment_aht10_temperature',
                    'environment_aht10_humidity',
                    'motor_n_mike_inmp441_audio_samples',
                    'motor_n_mike_inmp441_audio_level',
                    'motor_n_mike_inmp441_peak_amplitude',
                    'motor_n_mike_inmp441_rms_level',
                    'motor_n_mike_inmp441_is_clipping',
                    'thespeaker_bluetoothspeaker_connected',
                    'thespeaker_bluetoothspeaker_device_name',
                    'thespeaker_bluetoothspeaker_rssi',
                ])
                reading = SensorReading(**{
                    k: v for k, v in decoded.items() 
                    if hasattr(SensorReading, k) or k in allowed_fields
                })
                return reading
            
        except Exception as e:
            logger.error(f"Error processing data: {e}")
        
        return None
    
    async def handle_reading(self, reading: SensorReading):
        """Handle a processed reading - store and broadcast"""
        self.on_data_received(reading)
        if self.on_data_received_async:
            await self.on_data_received_async(reading)