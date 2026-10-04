"""
Protocol Frame Decoder for CircuitOS DataFlow

Auto-generated from data_flow_frames.json. Decodes binary protocol frames
received from MCUs into structured Python dicts.

Wire format (inside the length-prefixed TCP envelope):
  [FRAME_ID(2)] [SEQ(2)] [FRAME_TYPE(1)] [PAYLOAD(N)]

Frame types:
  0x01 = DATA, 0x02 = REQUEST, 0x03 = BEGIN, 0x04 = CONTINUE,
  0x05 = END, 0x06 = ACK, 0x07 = NACK, 0x08 = START_STREAM,
  0x09 = STOP_STREAM, 0x0A = HEARTBEAT

@note Auto-generated — DO NOT EDIT MANUALLY
"""

import struct
from typing import Dict, Any, Optional, List, Tuple
from loguru import logger


# ═══════════════════════════════════════════════════════════════════════
# Wire Protocol Constants (must match dataflow_protocol_defs.h)
# ═══════════════════════════════════════════════════════════════════════

WIRE_HEADER_SIZE = 5  # frame_id(2) + sequence(2) + frame_type(1)

# Frame types
FRAME_TYPE_DATA         = 0x01
FRAME_TYPE_REQUEST      = 0x02
FRAME_TYPE_BEGIN        = 0x03
FRAME_TYPE_CONTINUE     = 0x04
FRAME_TYPE_END          = 0x05
FRAME_TYPE_ACK          = 0x06
FRAME_TYPE_NACK         = 0x07
FRAME_TYPE_START_STREAM = 0x08
FRAME_TYPE_STOP_STREAM  = 0x09
FRAME_TYPE_HEARTBEAT    = 0x0A

FRAME_TYPE_NAMES = {
    FRAME_TYPE_DATA: "DATA",
    FRAME_TYPE_REQUEST: "REQUEST",
    FRAME_TYPE_BEGIN: "BEGIN",
    FRAME_TYPE_CONTINUE: "CONTINUE",
    FRAME_TYPE_END: "END",
    FRAME_TYPE_ACK: "ACK",
    FRAME_TYPE_NACK: "NACK",
    FRAME_TYPE_START_STREAM: "START_STREAM",
    FRAME_TYPE_STOP_STREAM: "STOP_STREAM",
    FRAME_TYPE_HEARTBEAT: "HEARTBEAT",
}


# ═══════════════════════════════════════════════════════════════════════
# Frame Descriptor Table (from data_flow_frames.json / frame_descriptors.h)
# Maps frame_id → { name, fields: [{ name, type, offset, size }] }
# ═══════════════════════════════════════════════════════════════════════

FRAME_TABLE: Dict[int, Dict[str, Any]] = {
    0x0100: {
        "name": "heartbeat_board_0_to_backend",
        "source_mcu_id": "board_0",
        "target_mcu_id": "system_backend",
        "frame_role": "heartbeat",
        "variable_ids": [],
        "fields": [
        ],
    },
    0x0101: {
        "name": "heartbeat_board_1_to_backend",
        "source_mcu_id": "board_1",
        "target_mcu_id": "system_backend",
        "frame_role": "heartbeat",
        "variable_ids": [],
        "fields": [
        ],
    },
    0x0102: {
        "name": "heartbeat_board_5_to_backend",
        "source_mcu_id": "board_5",
        "target_mcu_id": "system_backend",
        "frame_role": "heartbeat",
        "variable_ids": [],
        "fields": [
        ],
    },
}

# Field type unpacking format strings (for struct.unpack)
FIELD_TYPE_FORMAT = {
    "float": "<f",     # little-endian float
    "double": "<d",    # little-endian double
    "int32": "<i",     # little-endian signed int32
    "int32_t": "<i",
    "uint32": "<I",    # little-endian unsigned int32
    "uint32_t": "<I",
    "int16": "<h",
    "int16_t": "<h",
    "uint16": "<H",
    "uint16_t": "<H",
    "int8": "<b",
    "int8_t": "<b",
    "uint8": "<B",
    "uint8_t": "<B",
    "bool": "<?",
    "boolean": "<?",
}


# ═══════════════════════════════════════════════════════════════════════
# Decoder Functions
# ═══════════════════════════════════════════════════════════════════════

def decode_wire_header(raw: bytes) -> Optional[Dict[str, Any]]:
    """
    Decode the 5-byte wire header from raw bytes.
    
    Returns:
        Dict with frame_id, sequence, frame_type, frame_type_name
        or None if the buffer is too short
    """
    if len(raw) < WIRE_HEADER_SIZE:
        return None
    
    frame_id = struct.unpack_from("<H", raw, 0)[0]
    sequence = struct.unpack_from("<H", raw, 2)[0]
    frame_type = raw[4]
    
    return {
        "frame_id": frame_id,
        "sequence": sequence,
        "frame_type": frame_type,
        "frame_type_name": FRAME_TYPE_NAMES.get(frame_type, f"UNKNOWN(0x{frame_type:02X})"),
    }


def decode_frame(raw: bytes) -> Dict[str, Any]:
    """
    Decode a complete protocol frame (header + payload).
    
    Args:
        raw: Raw bytes starting with the wire header
        
    Returns:
        Dict with:
            - frame_id, sequence, frame_type, frame_type_name (from header)
            - frame_name (from FRAME_TABLE lookup)
            - values: Dict[variable_id, value] (decoded payload fields)
            - raw_payload: hex string of undecoded payload (for debugging)
    """
    header = decode_wire_header(raw)
    if not header:
        return {"error": "Buffer too short for wire header", "raw": raw.hex()}
    
    payload = raw[WIRE_HEADER_SIZE:]
    result = {**header, "payload_size": len(payload)}
    
    descriptor = FRAME_TABLE.get(header["frame_id"])
    if not descriptor:
        result["frame_name"] = f"unknown_0x{header['frame_id']:04X}"
        result["raw_payload"] = payload.hex() if payload else ""
        return result
    
    result["frame_name"] = descriptor["name"]
    result["source_mcu_id"] = descriptor.get("source_mcu_id", "")
    result["target_mcu_id"] = descriptor.get("target_mcu_id", "")
    result["frame_role"] = descriptor.get("frame_role", "data")
    result["variable_ids"] = descriptor.get("variable_ids", [])
    
    # Decode payload fields
    values = {}
    for field in descriptor.get("fields", []):
        fmt = FIELD_TYPE_FORMAT.get(field["type"], "<f")
        offset = field["offset"]
        size = field["size"]
        
        if offset + size <= len(payload):
            try:
                value = struct.unpack_from(fmt, payload, offset)[0]
                values[field["name"]] = value
            except struct.error as e:
                logger.warning(f"Failed to decode field {field['name']}: {e}")
                values[field["name"]] = None
        else:
            values[field["name"]] = None
    
    result["values"] = values
    return result


def decode_legacy_frame(raw: bytes) -> Optional[Dict[str, Any]]:
    """
    Decode a legacy raw-byte frame: [var_id_len(1)][var_id(N)][float(4)]
    
    Returns:
        Dict with variable_id and value, or None if not valid legacy format
    """
    if len(raw) < 6:
        return None
    
    var_id_len = raw[0]
    if var_id_len == 0 or var_id_len > 127:
        return None
    
    expected = 1 + var_id_len + 4  # len + id + float
    if len(raw) < expected:
        return None
    
    try:
        var_id = raw[1:1+var_id_len].decode("utf-8", errors="replace")
        value = struct.unpack_from("<f", raw, 1 + var_id_len)[0]
        return {
            "variable_id": var_id,
            "value": value,
            "format": "legacy",
        }
    except Exception:
        return None


def decode_auto(raw: bytes) -> Dict[str, Any]:
    """
    Auto-detect and decode a frame (protocol or legacy format).
    
    Protocol frames have frame_id >= 0x0001 in the first 2 bytes and
    frame_type in [0x01..0x0A] at byte 4.
    
    Legacy frames have var_id_len (0-127) at byte 0.
    """
    if len(raw) >= WIRE_HEADER_SIZE:
        frame_id = struct.unpack_from("<H", raw, 0)[0]
        if len(raw) >= 5:
            frame_type = raw[4]
            if frame_id > 0 and 0x01 <= frame_type <= 0x0A:
                return decode_frame(raw)
    
    legacy = decode_legacy_frame(raw)
    if legacy:
        return legacy
    
    return {"error": "Unrecognized format", "raw": raw.hex()}


def encode_control_frame(frame_id: int, frame_type: int, sequence: int = 0) -> bytes:
    """
    Encode a control frame (header only, no payload).
    
    Used by the backend to send START_STREAM, STOP_STREAM, etc. to devices.
    """
    return struct.pack("<HHB", frame_id, sequence, frame_type)


def get_frame_ids_for_variable(variable_id: str) -> List[int]:
    """
    Find all frame IDs that contain a given variable.
    
    Used when the frontend requests streaming for a variable —
    the backend needs to know which frame_id to send in the
    START_STREAM control frame.
    """
    matching = []
    for frame_id, desc in FRAME_TABLE.items():
        if variable_id in desc.get("variable_ids", []):
            matching.append(frame_id)
    return matching


def get_data_frame_for_mcu(source_mcu_id: str) -> Optional[int]:
    """
    Find the primary data frame ID for a given source MCU.
    
    Returns the first data-role frame where source matches the MCU.
    """
    for frame_id, desc in FRAME_TABLE.items():
        if desc.get("source_mcu_id") == source_mcu_id and desc.get("frame_role") in ("data", "stream_data"):
            return frame_id
    return None