"""
HTTP Receiver for MCU Communication

Receives data from MCUs via HTTP POST requests.
Implements the frame-driven dataflow protocol:
  - start_stream: MCU sends per-variable named frames (e.g. start_aht10_data_humidity_esp32_to_backend)
  - data: MCU sends periodic sensor readings with sequences
  - stop_stream: MCU sends per-variable stop frames

Frame names come from data_flow_frames.json defined in the Protocol Definition UI.

This is the default communication method for WiFi-enabled MCUs.
"""

from fastapi import APIRouter, Request, HTTPException
from typing import Optional, Dict, Any, List
from datetime import datetime
from loguru import logger

from .base_receiver import BaseReceiver
from models import SensorReading


class VariableStreamState:
    """Tracks per-variable stream state using named frames from protocol definition"""

    def __init__(self, variable_id: str, signal_info: Dict[str, Any], frame_name: str):
        self.variable_id = variable_id
        self.name = signal_info.get("name", variable_id)
        self.type = signal_info.get("type", "float")
        self.frequency_ms = signal_info.get("frequency_ms", 1000)
        self.start_frame_name = frame_name
        self.stop_frame_name: Optional[str] = None
        self.started_at = datetime.utcnow()
        self.last_data_at: Optional[datetime] = None
        self.sequence = 0
        self.frame_count = 0


class StreamState:
    """Tracks the state of a data stream from an MCU"""

    def __init__(self, mcu_id: str, signals: List[Dict[str, Any]], frame_name: Optional[str] = None):
        self.mcu_id = mcu_id
        self.signals = {s.get("variable_id", s.get("name", "")): s for s in signals}
        self.started_at = datetime.utcnow()
        self.last_data_at: Optional[datetime] = None
        self.frame_count = 0
        self.sequences: Dict[str, int] = {}
        self.protocol_version: Optional[str] = None
        self.schema_hash: Optional[str] = None
        self.frame_name = frame_name
        self.variable_streams: Dict[str, VariableStreamState] = {}

    def register_variable(self, variable_id: str, signal_info: Dict[str, Any], frame_name: str):
        """Register a per-variable stream from a named start_stream frame"""
        self.variable_streams[variable_id] = VariableStreamState(variable_id, signal_info, frame_name)
        self.signals[variable_id] = signal_info

    def unregister_variable(self, variable_id: str, frame_name: str):
        """Unregister a variable stream from a named stop_stream frame"""
        if variable_id in self.variable_streams:
            self.variable_streams[variable_id].stop_frame_name = frame_name
            del self.variable_streams[variable_id]
        self.signals.pop(variable_id, None)

    def update(self, sequences: Dict[str, int]):
        self.last_data_at = datetime.utcnow()
        self.frame_count += 1
        self.sequences.update(sequences)
        for var_id, seq in sequences.items():
            if var_id in self.variable_streams:
                self.variable_streams[var_id].last_data_at = self.last_data_at
                self.variable_streams[var_id].sequence = seq
                self.variable_streams[var_id].frame_count += 1


class DeviceInfo:
    """Tracks a registered MCU device from heartbeat announcements"""

    def __init__(self, mcu_id: str, ip: str, port: int = 0):
        self.mcu_id = mcu_id
        self.ip = ip
        self.port = port
        self.last_heartbeat = datetime.utcnow()
        self.uptime_ms: int = 0
        self.firmware_version: str = "unknown"
        self.signal_count: int = 0
        self.active_streams: int = 0
        self.stream_state: str = "idle"
        self.signals: List[Dict[str, Any]] = []
        self.heartbeat_count: int = 0

    def update(self, data: Dict[str, Any]):
        self.last_heartbeat = datetime.utcnow()
        self.ip = data.get("ip", self.ip)
        self.port = data.get("port", self.port)
        self.uptime_ms = data.get("uptime_ms", self.uptime_ms)
        self.firmware_version = data.get("firmware_version", self.firmware_version)
        self.signal_count = data.get("signal_count", self.signal_count)
        self.active_streams = data.get("active_streams", self.active_streams)
        self.stream_state = data.get("stream_state", self.stream_state)
        self.signals = data.get("signals", self.signals)
        self.heartbeat_count += 1

    def to_dict(self) -> Dict[str, Any]:
        return {
            "mcu_id": self.mcu_id,
            "ip": self.ip,
            "port": self.port,
            "last_heartbeat": self.last_heartbeat.isoformat(),
            "uptime_ms": self.uptime_ms,
            "firmware_version": self.firmware_version,
            "signal_count": self.signal_count,
            "active_streams": self.active_streams,
            "stream_state": self.stream_state,
            "signals": self.signals,
            "heartbeat_count": self.heartbeat_count,
        }


class PendingCommand:
    """A command queued for delivery to a device via heartbeat response"""

    def __init__(self, action: str, variable_id: str, frame_name: Optional[str] = None,
                 value: Any = None):
        self.action = action
        self.variable_id = variable_id
        self.frame_name = frame_name
        self.value = value
        self.created_at = datetime.utcnow()

    def to_dict(self) -> Dict[str, Any]:
        d = {
            "action": self.action,
            "variable_id": self.variable_id,
            "frame_name": self.frame_name,
        }
        if self.value is not None:
            d["value"] = self.value
        return d


class HTTPReceiver(BaseReceiver):
    """
    HTTP-based receiver for MCU data with frame-driven protocol support.

    Protocol:
      POST /api/mcu/heartbeat - device announces presence, receives pending commands
      POST /api/mcu/stream    - start_stream / stop_stream frames (per-variable, named)
      POST /api/mcu/data      - data frames (periodic readings)
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.router = APIRouter(prefix="/mcu", tags=["MCU Data"])
        self.active_streams: Dict[str, StreamState] = {}
        self.device_registry: Dict[str, DeviceInfo] = {}
        self.pending_commands: Dict[str, List[PendingCommand]] = {}
        self._setup_routes()

    def queue_command(self, mcu_id: str, action: str, variable_id: str,
                      frame_name: Optional[str] = None, value: Any = None):
        """Queue a command for delivery to a device on next heartbeat"""
        if mcu_id not in self.pending_commands:
            self.pending_commands[mcu_id] = []
        cmd = PendingCommand(action=action, variable_id=variable_id,
                             frame_name=frame_name, value=value)
        self.pending_commands[mcu_id].append(cmd)
        logger.info(f"Queued command: {action} {variable_id}={value} for {mcu_id}")

    def get_device(self, mcu_id: str) -> Optional[DeviceInfo]:
        """Get device info by MCU ID"""
        return self.device_registry.get(mcu_id)

    def _setup_routes(self):
        """Set up HTTP routes for MCU data reception"""

        @self.router.post("/heartbeat")
        async def handle_heartbeat(request: Request):
            """
            Handle device heartbeat announcements.

            Device sends periodic heartbeats with its status. Backend responds
            with any pending commands (begin/stop stream).
            """
            try:
                data = await request.json()
                mcu_id = data.get("mcu_id", "unknown")

                if mcu_id not in self.device_registry:
                    device = DeviceInfo(
                        mcu_id=mcu_id,
                        ip=data.get("ip", request.client.host if request.client else "unknown"),
                        port=data.get("port", 0),
                    )
                    self.device_registry[mcu_id] = device
                    logger.info(f"New device registered: {mcu_id} at {device.ip}")
                else:
                    device = self.device_registry[mcu_id]

                device.update(data)

                # Collect and clear pending commands for this device
                commands = []
                if mcu_id in self.pending_commands:
                    commands = [cmd.to_dict() for cmd in self.pending_commands[mcu_id]]
                    del self.pending_commands[mcu_id]

                # Notify protocol sniffer if available
                if self.on_stream_event:
                    await self.on_stream_event({
                        "type": "heartbeat",
                        "mcu_id": mcu_id,
                        "ip": device.ip,
                        "stream_state": device.stream_state,
                        "active_streams": device.active_streams,
                        "commands_delivered": len(commands),
                    })

                return {
                    "status": "ok",
                    "mcu_id": mcu_id,
                    "commands": commands,
                    "timestamp": datetime.utcnow().isoformat(),
                }

            except Exception as e:
                logger.error(f"Heartbeat error: {e}")
                raise HTTPException(status_code=400, detail=str(e))

        @self.router.get("/devices")
        async def get_registered_devices():
            """Get all registered MCU devices with their status"""
            return {
                "devices": {
                    mcu_id: device.to_dict()
                    for mcu_id, device in self.device_registry.items()
                },
                "count": len(self.device_registry),
            }

        @self.router.get("/devices/{mcu_id}")
        async def get_device_info(mcu_id: str):
            """Get info for a specific device"""
            if mcu_id not in self.device_registry:
                raise HTTPException(status_code=404, detail=f"Device '{mcu_id}' not found")
            return self.device_registry[mcu_id].to_dict()

        @self.router.get("/devices/{mcu_id}/commands")
        async def get_pending_commands(mcu_id: str):
            """Get pending stream-control commands for a device (debug view).
            Actuator commands are pushed over the device WebSocket, not polled."""
            commands = self.pending_commands.get(mcu_id, [])
            return {
                "mcu_id": mcu_id,
                "commands": [cmd.to_dict() for cmd in commands],
                "count": len(commands),
            }

        @self.router.post("/stream")
        async def handle_stream_control(request: Request):
            """
            Handle stream control frames (start_stream / stop_stream).

            Device sends per-variable named frames from data_flow_frames.json.
            Each frame has a 'frame_name' field identifying which protocol frame it is.
            """
            try:
                data = await request.json()
                frame_role = data.get("frame_role")
                mcu_id = data.get("mcu_id", "unknown")
                frame_name = data.get("frame_name")

                if frame_role == "start_stream":
                    signals = data.get("signals", [])
                    protocol_version = data.get("protocol_version")
                    schema_hash = data.get("schema_hash")

                    if mcu_id not in self.active_streams:
                        stream = StreamState(mcu_id, [], frame_name)
                        stream.protocol_version = protocol_version
                        stream.schema_hash = schema_hash
                        self.active_streams[mcu_id] = stream
                    else:
                        stream = self.active_streams[mcu_id]

                    for sig in signals:
                        var_id = sig.get("variable_id", sig.get("name", ""))
                        stream.register_variable(var_id, sig, frame_name or "start_stream")

                    logger.info(
                        f"Stream frame: {frame_name or 'start_stream'} from {mcu_id} "
                        f"({len(signals)} signals, total active: {len(stream.variable_streams)})"
                    )

                    if self.on_stream_event:
                        await self.on_stream_event({
                            "type": "stream_started",
                            "mcu_id": mcu_id,
                            "frame_name": frame_name,
                            "signals": signals,
                            "protocol_version": protocol_version,
                            "schema_hash": schema_hash,
                            "active_variables": list(stream.variable_streams.keys()),
                        })

                    return {
                        "status": "ok",
                        "frame_role": "begin",
                        "frame_name": frame_name,
                        "mcu_id": mcu_id,
                        "signals_registered": len(stream.variable_streams),
                        "timestamp": datetime.utcnow().isoformat(),
                    }

                elif frame_role == "stop_stream":
                    signals = data.get("signals", [])

                    if mcu_id in self.active_streams:
                        stream = self.active_streams[mcu_id]
                        for var_id in signals:
                            stream.unregister_variable(var_id, frame_name or "stop_stream")
                        if not stream.variable_streams:
                            del self.active_streams[mcu_id]

                    logger.info(f"Stream frame: {frame_name or 'stop_stream'} from {mcu_id}")

                    if self.on_stream_event:
                        await self.on_stream_event({
                            "type": "stream_stopped",
                            "mcu_id": mcu_id,
                            "frame_name": frame_name,
                            "signals": signals,
                        })

                    return {
                        "status": "ok",
                        "frame_role": "ack",
                        "frame_name": frame_name,
                        "mcu_id": mcu_id,
                    }

                else:
                    raise HTTPException(
                        status_code=400,
                        detail=f"Unknown frame_role: {frame_role}"
                    )

            except HTTPException:
                raise
            except Exception as e:
                logger.error(f"Stream control error: {e}")
                raise HTTPException(status_code=400, detail=str(e))

        @self.router.post("/data")
        async def receive_data(request: Request):
            """
            Receive data frames from MCUs.

            Supports:
            1. Binary wire-protocol frames (application/octet-stream with wire header)
            2. Protocol-aware JSON frames (frame_role: data)
            3. Legacy JSON payloads (backward compatibility)
            """
            content_type = request.headers.get("content-type", "")
            mcu_id = request.headers.get("x-mcu-id") or request.query_params.get("mcu_id")

            try:
                if "application/octet-stream" in content_type:
                    raw = await request.body()
                    reading = None
                    try:
                        from protocol_decoder import decode_auto
                        decoded = decode_auto(raw)
                        if "error" not in decoded and "values" in decoded:
                            reading = self.process_data(
                                decoded["values"],
                                mcu_id or decoded.get("source_mcu_id", "unknown")
                            )
                    except ImportError:
                        pass
                    if reading is None:
                        reading = self.process_data(raw, mcu_id)
                else:
                    data = await request.json()

                    frame_role = data.get("frame_role")
                    if frame_role == "data":
                        reading = self._process_protocol_frame(data)
                    else:
                        reading = self.process_data(data, mcu_id)

                if reading:
                    await self.handle_reading(reading)
                    return {
                        "status": "ok",
                        "timestamp": reading.timestamp.isoformat(),
                        "signals_received": sum(
                            1 for k, v in reading.model_dump().items()
                            if v is not None and k not in ('timestamp', 'mcu_id')
                        ),
                    }
                else:
                    raise HTTPException(status_code=400, detail="Failed to decode data")

            except HTTPException:
                raise
            except Exception as e:
                logger.error(f"HTTP receive error: {e}")
                raise HTTPException(status_code=400, detail=str(e))

        @self.router.post("/{mcu_id}/data")
        async def receive_data_from_mcu(mcu_id: str, request: Request):
            """Receive data from a specific MCU"""
            content_type = request.headers.get("content-type", "")

            try:
                if "application/octet-stream" in content_type:
                    raw = await request.body()
                    reading = None
                    try:
                        from protocol_decoder import decode_auto
                        decoded = decode_auto(raw)
                        if "error" not in decoded and "values" in decoded:
                            reading = self.process_data(decoded["values"], mcu_id)
                    except ImportError:
                        pass
                    if reading is None:
                        reading = self.process_data(raw, mcu_id)
                else:
                    data = await request.json()
                    frame_role = data.get("frame_role")
                    if frame_role == "data":
                        reading = self._process_protocol_frame(data)
                    else:
                        reading = self.process_data(data, mcu_id)

                if reading:
                    await self.handle_reading(reading)
                    return {
                        "status": "ok",
                        "mcu_id": mcu_id,
                        "timestamp": reading.timestamp.isoformat(),
                    }
                else:
                    raise HTTPException(status_code=400, detail="Failed to decode data")

            except HTTPException:
                raise
            except Exception as e:
                logger.error(f"HTTP receive error from {mcu_id}: {e}")
                raise HTTPException(status_code=400, detail=str(e))

        @self.router.get("/streams")
        async def get_active_streams():
            """Get information about all active MCU streams with per-variable detail"""
            return {
                "streams": {
                    mcu_id: {
                        "mcu_id": stream.mcu_id,
                        "signals": list(stream.signals.keys()),
                        "started_at": stream.started_at.isoformat(),
                        "last_data_at": stream.last_data_at.isoformat() if stream.last_data_at else None,
                        "frame_count": stream.frame_count,
                        "sequences": stream.sequences,
                        "protocol_version": stream.protocol_version,
                        "schema_hash": stream.schema_hash,
                        "variable_streams": {
                            var_id: {
                                "name": vs.name,
                                "start_frame_name": vs.start_frame_name,
                                "started_at": vs.started_at.isoformat(),
                                "last_data_at": vs.last_data_at.isoformat() if vs.last_data_at else None,
                                "sequence": vs.sequence,
                                "frame_count": vs.frame_count,
                            }
                            for var_id, vs in stream.variable_streams.items()
                        },
                    }
                    for mcu_id, stream in self.active_streams.items()
                },
                "count": len(self.active_streams),
            }

        @self.router.post("/frame/{frame_id}")
        async def receive_frame(frame_id: int, request: Request):
            """Receive a specific frame type from any MCU (binary protocol)"""
            mcu_id = request.headers.get("x-mcu-id")

            try:
                raw = await request.body()
                
                # Try protocol decoder first (wire header format)
                try:
                    from protocol_decoder import decode_auto, FRAME_TYPE_NAMES
                    decoded = decode_auto(raw)
                    
                    if "error" not in decoded and "values" in decoded:
                        frame_type_name = decoded.get("frame_type_name", "DATA")
                        logger.info(
                            f"Protocol frame 0x{decoded.get('frame_id', 0):04X} "
                            f"({decoded.get('frame_name', '?')}) type={frame_type_name} "
                            f"seq={decoded.get('sequence', 0)} from {mcu_id}: {decoded['values']}"
                        )
                        
                        # Convert decoded values into a SensorReading
                        reading = self.process_data(decoded["values"], mcu_id or decoded.get("source_mcu_id", "unknown"))
                        
                        if reading:
                            await self.handle_reading(reading)
                            
                            # Notify protocol sniffer
                            if self.on_stream_event:
                                await self.on_stream_event({
                                    "type": "protocol_frame",
                                    "frame_id": decoded.get("frame_id"),
                                    "frame_name": decoded.get("frame_name"),
                                    "frame_type": frame_type_name,
                                    "sequence": decoded.get("sequence"),
                                    "mcu_id": mcu_id,
                                    "values": decoded["values"],
                                })
                            
                            return {
                                "status": "ok",
                                "frame_id": decoded.get("frame_id", frame_id),
                                "frame_name": decoded.get("frame_name"),
                                "frame_type": frame_type_name,
                                "signals_decoded": len(decoded["values"]),
                            }
                except ImportError:
                    pass
                
                # Fallback: legacy binary format
                if raw[0] != 0xAA:
                    import struct
                    header = struct.pack('<BBI', 0xAA, frame_id, 0)
                    raw = header + raw + b'\x00\x00'

                reading = self.process_data(raw, mcu_id)

                if reading:
                    await self.handle_reading(reading)
                    return {"status": "ok", "frame_id": frame_id}
                else:
                    raise HTTPException(status_code=400, detail="Failed to decode frame")

            except HTTPException:
                raise
            except Exception as e:
                logger.error(f"Frame receive error: {e}")
                raise HTTPException(status_code=400, detail=str(e))

    def _process_protocol_frame(self, data: Dict[str, Any]) -> Optional[SensorReading]:
        """
        Process a protocol-aware data frame.

        Expected format:
        {
            "frame_role": "data",
            "mcu_id": "board_0",
            "timestamp": 12345,
            "values": {"aht10_data_temperature": 25.3, "aht10_data_humidity": 60.1},
            "sequences": {"aht10_data_temperature": 42, "aht10_data_humidity": 42}
        }
        """
        mcu_id = data.get("mcu_id", "unknown")
        values = data.get("values", {})
        sequences = data.get("sequences", {})

        if mcu_id in self.active_streams:
            seq_map = {
                f"{mcu_id}::{k}": v for k, v in sequences.items()
            }
            self.active_streams[mcu_id].update(seq_map)

        # Use decode_json for proper signal name mapping (fuzzy matching)
        return self.process_data(values, mcu_id)

    async def start(self):
        """HTTP receiver doesn't need explicit start - routes are registered"""
        self._running = True
        logger.info("HTTP Receiver started (frame-driven protocol enabled)")

    async def stop(self):
        """HTTP receiver doesn't need explicit stop"""
        self._running = False
        self.active_streams.clear()
        logger.info("HTTP Receiver stopped")