"""
Protocol Exchange Test Harness for theSpeaker (3 boards)

Validates that the message exchange between components works as designed
by simulating device→backend→frontend data flow using the generated
protocol model (frames, transactions, operations).

Run with: python -m pytest tests/test_protocol_exchange.py -v
"""

import struct
import json
import pytest
import asyncio
from pathlib import Path
from unittest.mock import AsyncMock, MagicMock, patch
from typing import Dict, List, Any, Optional

# ═══════════════════════════════════════════════════════════════════
# PROTOCOL MODEL — Loaded from generated data_flow_*.json files
# ═══════════════════════════════════════════════════════════════════

PROJECT_DIR = Path(__file__).parent.parent

def load_json(filename: str) -> Any:
    """Load a JSON file from the project directory."""
    path = PROJECT_DIR / filename
    if path.exists():
        with open(path) as f:
            return json.load(f)
    return []

FRAMES = load_json("data_flow_frames.json")
TRANSACTIONS = load_json("data_flow_transactions.json")
OPERATIONS = load_json("data_flow_operations.json")
_bindings_raw = load_json("data_flow_bindings.json")
BINDINGS = _bindings_raw.get("bindings", _bindings_raw) if isinstance(_bindings_raw, dict) else _bindings_raw
CIRCUIT_DESIGN = load_json("circuit_design.json")

# ═══════════════════════════════════════════════════════════════════
# WIRE PROTOCOL CONSTANTS (must match dataflow_protocol_defs.h)
# ═══════════════════════════════════════════════════════════════════

WIRE_HEADER_SIZE = 5  # frame_id(2) + sequence(2) + frame_type(1)

class FrameType:
    DATA = 0x01
    REQUEST = 0x02
    BEGIN = 0x03
    CONTINUE = 0x04
    END = 0x05
    ACK = 0x06
    NACK = 0x07
    START_STREAM = 0x08
    STOP_STREAM = 0x09
    HEARTBEAT = 0x0A
    FRAME_DESCRIPTOR = 0x0B

FRAME_TYPE_NAMES = {
    0x01: "DATA", 0x02: "REQUEST", 0x03: "BEGIN", 0x04: "CONTINUE",
    0x05: "END", 0x06: "ACK", 0x07: "NACK", 0x08: "START_STREAM",
    0x09: "STOP_STREAM", 0x0A: "HEARTBEAT", 0x0B: "FRAME_DESCRIPTOR",
}

ROLE_TO_FRAME_TYPE = {
    "data": FrameType.DATA,
    "request": FrameType.REQUEST,
    "response": FrameType.DATA,
    "ack": FrameType.ACK,
    "nack": FrameType.NACK,
    "start_stream": FrameType.START_STREAM,
    "stop_stream": FrameType.STOP_STREAM,
    "heartbeat": FrameType.HEARTBEAT,
    "begin": FrameType.BEGIN,
    "continue": FrameType.CONTINUE,
    "end": FrameType.END,
}


def encode_wire_frame(frame_id: int, frame_type: int, sequence: int, payload: bytes = b"") -> bytes:
    """Encode a wire protocol frame: [frame_id(2)][sequence(2)][frame_type(1)][payload]"""
    header = struct.pack("<HHB", frame_id, sequence, frame_type)
    return header + payload


def decode_wire_frame(data: bytes) -> Dict[str, Any]:
    """Decode a wire protocol frame."""
    if len(data) < WIRE_HEADER_SIZE:
        raise ValueError(f"Frame too short: {len(data)} < {WIRE_HEADER_SIZE}")
    frame_id, sequence, frame_type = struct.unpack("<HHB", data[:WIRE_HEADER_SIZE])
    payload = data[WIRE_HEADER_SIZE:]
    return {
        "frame_id": frame_id,
        "sequence": sequence,
        "frame_type": frame_type,
        "frame_type_name": FRAME_TYPE_NAMES.get(frame_type, f"UNKNOWN(0x{frame_type:02X})"),
        "payload": payload,
        "payload_len": len(payload),
    }


# ═══════════════════════════════════════════════════════════════════
# SIMULATED COMPONENTS
# ═══════════════════════════════════════════════════════════════════

class SimulatedDevice:
    """Simulates an MCU device with protocol support."""

    def __init__(self, mcu_id: str, name: str = ""):
        self.mcu_id = mcu_id
        self.name = name or mcu_id
        self.sequence = 0
        self.active_streams: set = set()  # frame_ids that are active
        self.sent_frames: List[Dict] = []
        self.received_frames: List[Dict] = []
        self.peers: Dict[str, "SimulatedDevice"] = {}  # peer_id -> device

    def connect_peer(self, peer: "SimulatedDevice"):
        """Connect to a peer device (bidirectional)."""
        self.peers[peer.mcu_id] = peer
        peer.peers[self.mcu_id] = self

    def send_frame(self, frame_id: int, frame_type: int, payload: bytes = b"") -> bytes:
        """Send a wire protocol frame."""
        self.sequence += 1
        raw = encode_wire_frame(frame_id, frame_type, self.sequence, payload)
        record = {
            "frame_id": frame_id,
            "frame_type": frame_type,
            "sequence": self.sequence,
            "payload": payload,
            "raw": raw,
        }
        self.sent_frames.append(record)
        return raw

    def receive_frame(self, data: bytes) -> Dict[str, Any]:
        """Process a received wire protocol frame."""
        decoded = decode_wire_frame(data)
        self.received_frames.append(decoded)

        # Handle stream control
        if decoded["frame_type"] == FrameType.START_STREAM:
            self.active_streams.add(decoded["frame_id"])
            return {"action": "ack", "frame_id": decoded["frame_id"]}
        elif decoded["frame_type"] == FrameType.STOP_STREAM:
            self.active_streams.discard(decoded["frame_id"])
            return {"action": "ack", "frame_id": decoded["frame_id"]}
        elif decoded["frame_type"] == FrameType.ACK:
            return {"action": "none"}
        elif decoded["frame_type"] == FrameType.DATA:
            return {"action": "data_received", "frame_id": decoded["frame_id"], "payload": decoded["payload"]}

        return {"action": "unknown"}

    def is_stream_active(self, frame_id: int) -> bool:
        return frame_id in self.active_streams


class SimulatedBackend:
    """Simulates the FastAPI backend with protocol decoder."""

    def __init__(self):
        self.received_readings: List[Dict] = []
        self.websocket_messages: List[Dict] = []
        self.frame_table: Dict[int, Dict] = {}  # frame_id -> frame info
        self.dispatched_commands: List[Dict] = []

    def load_frame_table(self, protocol_frames: List[Dict]):
        """Load FRAME_TABLE from protocol_frames (as generated by _build_protocol_frames_with_ids)."""
        for pf in protocol_frames:
            self.frame_table[pf["frame_id"]] = pf

    def receive_telemetry_json(self, mcu_id: str, data: Dict) -> Dict:
        """Simulate receiving telemetry via HTTP POST JSON."""
        reading = {"mcu_id": mcu_id, "data": data, "method": "http_json"}
        self.received_readings.append(reading)
        ws_msg = {"type": "sensor_reading", "mcu_id": mcu_id, "data": data}
        self.websocket_messages.append(ws_msg)
        return reading

    def receive_binary_frame(self, mcu_id: str, data: bytes) -> Optional[Dict]:
        """Simulate receiving a binary wire protocol frame."""
        if len(data) < WIRE_HEADER_SIZE:
            return None
        decoded = decode_wire_frame(data)
        frame_info = self.frame_table.get(decoded["frame_id"])
        if frame_info:
            decoded["frame_name"] = frame_info.get("name", "")
            decoded["source_mcu_id"] = frame_info.get("source_mcu_id", mcu_id)
            decoded["target_mcu_id"] = frame_info.get("target_mcu_id", "")
            decoded["variable_ids"] = frame_info.get("variable_ids", [])
        reading = {"mcu_id": mcu_id, "decoded": decoded, "method": "binary_protocol"}
        self.received_readings.append(reading)
        return decoded

    def dispatch_stream_control(self, mcu_id: str, variable_id: str, action: str) -> Optional[int]:
        """Simulate dispatch_stream_control — resolve variable to frame_id."""
        for fid, info in self.frame_table.items():
            if variable_id in info.get("variable_ids", []):
                cmd = {"mcu_id": mcu_id, "variable_id": variable_id, "action": action, "frame_id": fid}
                self.dispatched_commands.append(cmd)
                return fid
        return None


class SimulatedFrontend:
    """Simulates the React frontend."""

    def __init__(self):
        self.received_messages: List[Dict] = []
        self.requested_streams: List[Dict] = []

    def receive_ws_message(self, msg: Dict):
        self.received_messages.append(msg)

    def request_stream(self, variables: List[str], mcu_id: str = ""):
        self.requested_streams.append({"variables": variables, "mcu_id": mcu_id})


# ═══════════════════════════════════════════════════════════════════
# FIXTURES
# ═══════════════════════════════════════════════════════════════════

@pytest.fixture
def circuit_nodes():
    """Extract board/system nodes from circuit design."""
    nodes = CIRCUIT_DESIGN.get("nodes", [])
    boards = [n for n in nodes if n.get("type") == "board"]
    systems = [n for n in nodes if n.get("type") == "system"]
    return {"boards": boards, "systems": systems, "all": nodes}


@pytest.fixture
def circuit_edges():
    """Extract edges from circuit design."""
    return CIRCUIT_DESIGN.get("edges", [])


@pytest.fixture
def data_frames():
    """All data frames from the protocol model."""
    return [f for f in FRAMES if f.get("frame_role") == "data"]


@pytest.fixture
def control_frames():
    """All control frames (start/stop stream, ack, etc.)."""
    return [f for f in FRAMES if f.get("frame_role") in
            ("start_stream", "stop_stream", "ack", "heartbeat")]


@pytest.fixture
def request_response_txns():
    """All request-response transactions."""
    return [t for t in TRANSACTIONS if t.get("transaction_type") == "request_response"]


@pytest.fixture
def stream_data_txns():
    """All stream-data transactions."""
    return [t for t in TRANSACTIONS if t.get("transaction_type") == "stream_data"]


# ═══════════════════════════════════════════════════════════════════
# TEST CASES
# ═══════════════════════════════════════════════════════════════════

class TestProtocolModelConsistency:
    """Verify the protocol model files are self-consistent."""

    def test_frames_exist(self):
        """At least some frames should be generated."""
        assert len(FRAMES) > 0, "No protocol frames found in data_flow_frames.json"

    def test_transactions_exist(self):
        """At least some transactions should be generated."""
        assert len(TRANSACTIONS) > 0, "No transactions found in data_flow_transactions.json"

    def test_every_data_frame_has_variables(self, data_frames):
        """Every data frame must reference at least one variable."""
        for f in data_frames:
            assert len(f.get("variable_ids", [])) > 0, \
                f"Data frame '{f.get('name')}' has no variable_ids"

    def test_every_frame_has_source_and_target(self):
        """Every frame must specify source and target."""
        for f in FRAMES:
            assert f.get("source_mcu_id"), f"Frame '{f.get('name')}' missing source_mcu_id"
            assert f.get("target_mcu_id"), f"Frame '{f.get('name')}' missing target_mcu_id"

    def test_request_response_txns_have_both_frames(self, request_response_txns):
        """Request-response transactions must have both request and response frame IDs."""
        for t in request_response_txns:
            assert t.get("request_frame_id"), \
                f"RR txn '{t.get('name')}' missing request_frame_id"
            assert t.get("response_frame_id"), \
                f"RR txn '{t.get('name')}' missing response_frame_id"

    def test_transaction_frame_references_valid(self):
        """All frame references in transactions must point to existing frames."""
        frame_ids = {f.get("id") for f in FRAMES}
        for t in TRANSACTIONS:
            req_fid = t.get("request_frame_id")
            if req_fid and not req_fid.startswith("grouped_"):
                assert req_fid in frame_ids, \
                    f"Txn '{t.get('name')}' references non-existent frame: {req_fid}"

    def test_no_self_loop_frames(self):
        """No frame should have the same source and target."""
        for f in FRAMES:
            src = f.get("source_mcu_id", "")
            tgt = f.get("target_mcu_id", "")
            assert src != tgt, \
                f"Frame '{f.get('name')}' is a self-loop: {src} → {tgt}"


class TestWireProtocol:
    """Test the wire protocol encoding/decoding."""

    def test_encode_decode_roundtrip(self):
        """Encoding then decoding a frame should preserve all fields."""
        payload = struct.pack("<ff", 25.3, 62.1)
        raw = encode_wire_frame(0x0100, FrameType.DATA, 42, payload)
        decoded = decode_wire_frame(raw)
        assert decoded["frame_id"] == 0x0100
        assert decoded["sequence"] == 42
        assert decoded["frame_type"] == FrameType.DATA
        assert decoded["payload"] == payload
        assert decoded["payload_len"] == 8

    def test_control_frame_no_payload(self):
        """Control frames (ACK, START_STREAM) work without payload."""
        raw = encode_wire_frame(0x0100, FrameType.ACK, 1)
        decoded = decode_wire_frame(raw)
        assert decoded["frame_type"] == FrameType.ACK
        assert decoded["payload_len"] == 0

    def test_header_size(self):
        """Wire header must be exactly 5 bytes."""
        raw = encode_wire_frame(0x0100, FrameType.DATA, 1, b"")
        assert len(raw) == WIRE_HEADER_SIZE

    def test_frame_too_short_raises(self):
        """Frames shorter than header should raise."""
        with pytest.raises(ValueError):
            decode_wire_frame(b"\x00\x01")


class TestStreamControl:
    """Test the stream start/stop/ACK exchange pattern."""

    def test_start_stream_activates(self):
        """Sending START_STREAM should activate the stream on the device."""
        device = SimulatedDevice("board_env")
        raw = encode_wire_frame(0x0100, FrameType.START_STREAM, 1)
        result = device.receive_frame(raw)
        assert result["action"] == "ack"
        assert device.is_stream_active(0x0100)

    def test_stop_stream_deactivates(self):
        """Sending STOP_STREAM should deactivate the stream."""
        device = SimulatedDevice("board_env")
        device.active_streams.add(0x0100)
        raw = encode_wire_frame(0x0100, FrameType.STOP_STREAM, 1)
        result = device.receive_frame(raw)
        assert result["action"] == "ack"
        assert not device.is_stream_active(0x0100)

    def test_data_only_when_active(self):
        """Device should only send data when stream is active."""
        device = SimulatedDevice("board_env")
        assert not device.is_stream_active(0x0100)
        # Activate
        device.receive_frame(encode_wire_frame(0x0100, FrameType.START_STREAM, 1))
        assert device.is_stream_active(0x0100)
        # Send data
        payload = struct.pack("<ff", 25.0, 60.0)
        raw = device.send_frame(0x0100, FrameType.DATA, payload)
        assert len(device.sent_frames) == 1


class TestEndToEndDataFlow:
    """Test the full data flow path: device → backend → frontend."""

    def test_telemetry_json_path(self):
        """Sensor data reaches frontend via HTTP JSON telemetry path."""
        backend = SimulatedBackend()
        frontend = SimulatedFrontend()

        # Device sends telemetry
        reading = backend.receive_telemetry_json("board_env", {
            "temperature": 25.3, "humidity": 62.1
        })
        assert reading["method"] == "http_json"
        assert len(backend.websocket_messages) == 1

        # Backend relays to frontend
        frontend.receive_ws_message(backend.websocket_messages[0])
        assert len(frontend.received_messages) == 1
        assert frontend.received_messages[0]["data"]["temperature"] == 25.3

    def test_binary_protocol_path(self):
        """Binary wire protocol frame is decoded by backend."""
        backend = SimulatedBackend()
        backend.load_frame_table([{
            "frame_id": 0x0100,
            "name": "env_data_frame",
            "source_mcu_id": "board_env",
            "target_mcu_id": "backend",
            "frame_role": "data",
            "variable_ids": ["temperature", "humidity"],
            "fields": [
                {"name": "temperature", "type": "float", "offset": 0, "size": 4},
                {"name": "humidity", "type": "float", "offset": 4, "size": 4},
            ]
        }])

        payload = struct.pack("<ff", 25.3, 62.1)
        raw = encode_wire_frame(0x0100, FrameType.DATA, 1, payload)
        decoded = backend.receive_binary_frame("board_env", raw)

        assert decoded is not None
        assert decoded["frame_id"] == 0x0100
        assert decoded["frame_name"] == "env_data_frame"
        assert "temperature" in decoded["variable_ids"]

    def test_frontend_stream_request_dispatches_command(self):
        """Frontend requesting a stream triggers command dispatch with correct frame_id."""
        backend = SimulatedBackend()
        backend.load_frame_table([{
            "frame_id": 0x0100,
            "name": "env_data_frame",
            "source_mcu_id": "board_env",
            "target_mcu_id": "backend",
            "frame_role": "data",
            "variable_ids": ["temperature"],
            "fields": [],
        }])

        fid = backend.dispatch_stream_control("board_env", "temperature", "begin_stream")
        assert fid == 0x0100
        assert len(backend.dispatched_commands) == 1
        assert backend.dispatched_commands[0]["action"] == "begin_stream"


class TestInterMCUExchange:
    """Test inter-MCU protocol exchange over TCP."""

    def test_two_device_data_exchange(self):
        """Two connected devices can exchange data frames."""
        env = SimulatedDevice("board_env", "environment")
        display = SimulatedDevice("board_display", "display")
        env.connect_peer(display)

        # Environment sends data to display
        payload = struct.pack("<ff", 24.5, 58.0)
        raw = env.send_frame(0x0100, FrameType.DATA, payload)

        # Display receives it
        result = display.receive_frame(raw)
        assert result["action"] == "data_received"
        assert result["frame_id"] == 0x0100

        # Verify payload
        temp, hum = struct.unpack("<ff", result["payload"])
        assert abs(temp - 24.5) < 0.01
        assert abs(hum - 58.0) < 0.01

    def test_start_stream_ack_exchange(self):
        """START_STREAM → ACK handshake between two devices."""
        display = SimulatedDevice("board_display")
        env = SimulatedDevice("board_env")
        display.connect_peer(env)

        # Display sends START_STREAM to env
        raw_start = display.send_frame(0x0100, FrameType.START_STREAM)
        result = env.receive_frame(raw_start)
        assert result["action"] == "ack"
        assert env.is_stream_active(0x0100)

        # Env sends ACK back
        raw_ack = env.send_frame(0x0100, FrameType.ACK)
        ack_result = display.receive_frame(raw_ack)
        assert ack_result["action"] == "none"  # ACK is informational

    def test_full_stream_lifecycle(self):
        """Complete lifecycle: START → DATA × N → STOP."""
        env = SimulatedDevice("board_env")
        display = SimulatedDevice("board_display")

        # Start stream
        env.receive_frame(encode_wire_frame(0x0100, FrameType.START_STREAM, 1))
        assert env.is_stream_active(0x0100)

        # Send 3 data frames
        for i in range(3):
            payload = struct.pack("<ff", 20.0 + i, 50.0 + i)
            raw = env.send_frame(0x0100, FrameType.DATA, payload)
            display.receive_frame(raw)

        assert len(display.received_frames) == 3

        # Stop stream
        env.receive_frame(encode_wire_frame(0x0100, FrameType.STOP_STREAM, 2))
        assert not env.is_stream_active(0x0100)


class TestCircuitDesignAlignment:
    """Verify protocol model matches the circuit design topology."""

    def test_frames_cover_all_data_bindings(self):
        """Every data binding should have at least one corresponding frame."""
        if not BINDINGS:
            pytest.skip("No bindings found")
        binding_vars = {b.get("variable_id") for b in BINDINGS if b.get("variable_id")}
        frame_vars = set()
        for f in FRAMES:
            frame_vars.update(f.get("variable_ids", []))
        uncovered = binding_vars - frame_vars
        assert len(uncovered) == 0, f"Bindings without frames: {uncovered}"

    def test_frame_hops_match_circuit_edges(self, circuit_edges):
        """Frame source→target pairs should correspond to circuit design edges."""
        if not circuit_edges:
            pytest.skip("No edges in circuit design")
        edge_pairs = set()
        for e in circuit_edges:
            edge_pairs.add((e.get("source"), e.get("target")))
            edge_pairs.add((e.get("target"), e.get("source")))  # bidirectional

        frame_hops = set()
        for f in FRAMES:
            src = f.get("source_mcu_id", "")
            tgt = f.get("target_mcu_id", "")
            if src and tgt:
                frame_hops.add((src, tgt))

        # System nodes (backend/frontend) aren't in circuit edges, so filter them
        board_ids = {n.get("id") for n in CIRCUIT_DESIGN.get("nodes", []) if n.get("type") == "board"}
        board_frame_hops = {(s, t) for s, t in frame_hops if s in board_ids and t in board_ids}

        for hop in board_frame_hops:
            assert hop in edge_pairs, \
                f"Frame hop {hop[0]}→{hop[1]} has no matching circuit edge"

    def test_board_count_matches(self, circuit_nodes):
        """Number of MCUs in frames should match circuit design boards."""
        board_ids = {n.get("id") for n in circuit_nodes["boards"]}
        frame_mcus = set()
        for f in FRAMES:
            frame_mcus.add(f.get("source_mcu_id", ""))
            frame_mcus.add(f.get("target_mcu_id", ""))
        frame_board_mcus = frame_mcus & board_ids
        assert len(frame_board_mcus) > 0, "No board MCUs found in protocol frames"


class TestProtocolDecoder:
    """Test that the generated protocol_decoder.py works correctly."""

    def test_import_protocol_decoder(self):
        """The generated protocol_decoder module should be importable."""
        import sys
        backend_dir = str(PROJECT_DIR / "backend")
        if backend_dir not in sys.path:
            sys.path.insert(0, backend_dir)
        try:
            import protocol_decoder
            assert hasattr(protocol_decoder, "decode_auto")
            assert hasattr(protocol_decoder, "FRAME_TABLE")
            assert hasattr(protocol_decoder, "get_frame_ids_for_variable")
        except ImportError:
            pytest.skip("protocol_decoder.py not generated for this project")

    def test_frame_table_populated(self):
        """FRAME_TABLE should have entries matching protocol_frames."""
        import sys
        backend_dir = str(PROJECT_DIR / "backend")
        if backend_dir not in sys.path:
            sys.path.insert(0, backend_dir)
        try:
            import protocol_decoder
            assert len(protocol_decoder.FRAME_TABLE) > 0, "FRAME_TABLE is empty"
        except ImportError:
            pytest.skip("protocol_decoder.py not generated")
