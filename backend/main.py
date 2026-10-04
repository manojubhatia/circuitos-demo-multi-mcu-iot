"""
FastAPI Backend for theSpeaker (3 boards)
Generated from dataflow configuration

Communication Methods:
- HTTP: HTTP POST from WiFi-enabled MCUs
- TCP: Raw TCP socket connection

MCU Data Sources:
- theSpeaker (board_0): 3 signals
- environment (board_1): 2 signals
- motor_n_mike (board_5): 5 signals
"""

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException, Request
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
import asyncio
import json
from datetime import datetime
from collections import deque
from typing import List, Optional, Dict, Any

from models import SensorReading, SignalMetadata, SIGNAL_METADATA
from routers import router

# SQLite persistence (best-effort, non-blocking)
try:
    from db import store_telemetry, init_db
    init_db()
    DB_AVAILABLE = True
except ImportError:
    DB_AVAILABLE = False

# Communication receivers based on project configuration
# Receivers are ONLY generated when there is dataflow targeting the backend
from receivers.http_receiver import HTTPReceiver
from receivers.tcp_receiver import TCPReceiver
from protocol_sniffer import ProtocolSniffer
from command_dispatcher import CommandDispatcher

# Configuration
MAX_BUFFER_SIZE = 1000

# In-memory storage (per-MCU and aggregated)
readings_buffer: deque = deque(maxlen=MAX_BUFFER_SIZE)
latest_reading: Optional[SensorReading] = None

# Per-MCU storage for tracking individual device data
mcu_buffers: Dict[str, deque] = {
    'board_0': deque(maxlen=MAX_BUFFER_SIZE),
    'board_1': deque(maxlen=MAX_BUFFER_SIZE),
    'board_5': deque(maxlen=MAX_BUFFER_SIZE),
}
mcu_latest: Dict[str, Optional[SensorReading]] = {
    'board_0': None,
    'board_1': None,
    'board_5': None,
}

# Active receivers
receivers = []


class ConnectionManager:
    """Manages WebSocket connections for real-time data streaming to frontend"""
    
    def __init__(self):
        self.active_connections: List[WebSocket] = []
        self.subscriptions: Dict[str, List[WebSocket]] = {}  # mcu_id -> websockets

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active_connections.append(websocket)

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
        # Remove from subscriptions
        for subs in self.subscriptions.values():
            if websocket in subs:
                subs.remove(websocket)

    async def broadcast(self, data: dict, mcu_id: Optional[str] = None):
        """Broadcast to all connections or only those subscribed to mcu_id"""
        disconnected = []
        
        # Determine target connections
        if mcu_id and mcu_id in self.subscriptions:
            targets = self.subscriptions[mcu_id]
        else:
            targets = self.active_connections
        
        for connection in targets:
            try:
                await connection.send_json(data)
            except Exception:
                disconnected.append(connection)
        
        for conn in disconnected:
            self.disconnect(conn)
    
    def subscribe(self, websocket: WebSocket, mcu_id: str):
        """Subscribe a websocket to a specific MCU's data"""
        if mcu_id not in self.subscriptions:
            self.subscriptions[mcu_id] = []
        if websocket not in self.subscriptions[mcu_id]:
            self.subscriptions[mcu_id].append(websocket)


manager = ConnectionManager()


class DeviceConnectionManager:
    """Manages WebSocket connections from MCU devices (telemetry ingest only).

    Each device connects to ``/ws/device/{mcu_id}`` for **telemetry**.
    Commands are dispatched via the CommandDispatcher (HTTP POST to device).
    """

    def __init__(self):
        self.connections: Dict[str, WebSocket] = {}

    async def connect(self, mcu_id: str, websocket: WebSocket):
        await websocket.accept()
        old = self.connections.get(mcu_id)
        if old:
            try:
                await old.close()
            except Exception:
                pass
        self.connections[mcu_id] = websocket
        print(f"  🔌 Device WS connected: {mcu_id} (telemetry ingest)")

    def disconnect(self, mcu_id: str):
        self.connections.pop(mcu_id, None)
        print(f"  ⛓️‍💥 Device WS disconnected: {mcu_id}")


device_manager = DeviceConnectionManager()

# Command dispatcher — routes commands to devices via HTTP POST, serial, or BLE
command_dispatcher = CommandDispatcher(default_cmd_port=8501)


def store_reading(reading: SensorReading):
    """Store a reading in the buffer, update latest, persist to SQLite"""
    global latest_reading
    latest_reading = reading
    readings_buffer.append(reading)
    
    # Store per-MCU if mcu_id available
    mcu_id = getattr(reading, 'mcu_id', None)
    if mcu_id and mcu_id in mcu_buffers:
        mcu_buffers[mcu_id].append(reading)
        mcu_latest[mcu_id] = reading
    
    # Persist to SQLite (best-effort)
    if DB_AVAILABLE:
        try:
            fields = {}
            for key, value in reading.model_dump(exclude={'timestamp', 'mcu_id'}).items():
                if isinstance(value, (int, float)):
                    fields[key] = value
            if fields:
                store_telemetry(mcu_id or "unknown", fields, reading.timestamp.isoformat() if reading.timestamp else None)
        except Exception:
            pass
    
    # Write to InfluxDB (non-blocking, best-effort)
    try:
        from influxdb_service import get_influxdb_service
        influx = get_influxdb_service()
        if influx.is_connected():
            fields = {}
            for key, value in reading.model_dump(exclude={'timestamp', 'mcu_id'}).items():
                if isinstance(value, (int, float)):
                    fields[key] = value
            if fields:
                influx.write_point(
                    measurement="sensor_data",
                    tags={"device": mcu_id or "unknown"},
                    fields=fields,
                )
    except (ImportError, Exception):
        pass


async def broadcast_reading(reading: SensorReading):
    """Broadcast a new reading to all connected WebSocket clients.
    
    Also checks for ACKs: if a telemetry field matches a pending simulation command,
    broadcast a 'set_ack' message so the frontend can confirm the state change.
    """
    mcu_id = getattr(reading, 'mcu_id', None)
    await manager.broadcast({
        "type": "data",
        "mcu_id": mcu_id,
        "data": reading.model_dump(mode='json', by_alias=True)
    }, mcu_id)
    
    # Check for ACKs from pending simulation commands
    reading_data = reading.model_dump(exclude={'timestamp', 'mcu_id'})
    for signal_name, sim_info in list(simulation_state.items()):
        if not sim_info.get('pending'):
            continue
        if signal_name in reading_data and reading_data[signal_name] is not None:
            sim_info['pending'] = False
            await manager.broadcast({
                "type": "set_ack",
                "signal": signal_name,
                "value": reading_data[signal_name],
                "mcu_id": mcu_id,
            })


async def broadcast_stream_event(event: dict):
    """Broadcast stream lifecycle events (start/stop) to WebSocket clients"""
    await manager.broadcast(event)


# Protocol sniffer instance (captures all protocol frames for analysis)
protocol_sniffer = ProtocolSniffer()


async def broadcast_stream_event_with_capture(event: dict):
    """Broadcast stream events AND capture them in the protocol sniffer"""
    await protocol_sniffer.capture_from_event(event)
    await manager.broadcast(event)


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Application lifespan handler - start/stop receivers"""
    print(f"🚀 theSpeaker (3 boards) Backend starting...")
    print(f"📊 Tracking 10 signals from 3 MCUs")
    
    # Register protocol sniffer routes
    app.include_router(protocol_sniffer.router, prefix="/api")
    app.state.protocol_sniffer = protocol_sniffer
    print("  ✓ Protocol sniffer enabled (GET /api/protocol/capture)")
    
    # Initialize receivers based on configuration
    # Receivers are only initialized when dataflow bindings target the backend
    http_receiver = HTTPReceiver(
        on_data_received=store_reading,
        on_data_received_async=broadcast_reading,
        on_stream_event=broadcast_stream_event_with_capture
    )
    app.include_router(http_receiver.router, prefix="/api")
    receivers.append(http_receiver)
    app.state.http_receiver = http_receiver
    print("  ✓ HTTP receiver enabled (stream protocol)")



    tcp_receiver = TCPReceiver(
        on_data_received=store_reading,
        on_data_received_async=broadcast_reading
    )
    await tcp_receiver.start()
    receivers.append(tcp_receiver)
    print("  ✓ TCP/UDP receiver enabled")



    # Auto-discover devices via mDNS and register for command dispatch
    import socket as _socket
    try:
        _ip = _socket.gethostbyname("thespeaker.local")
        command_dispatcher.register_device("board_0", "http", _ip, 8501)
        print(f"  ✓ Discovered theSpeaker at {_ip}:8501 via mDNS")
    except _socket.gaierror:
        print(f"  ⚠ Could not resolve thespeaker.local — theSpeaker not on network")
    try:
        _ip = _socket.gethostbyname("environment.local")
        command_dispatcher.register_device("board_1", "http", _ip, 8501)
        print(f"  ✓ Discovered environment at {_ip}:8501 via mDNS")
    except _socket.gaierror:
        print(f"  ⚠ Could not resolve environment.local — environment not on network")

    print(f"📡 Communication endpoints ready")
    
    yield
    
    # Stop all receivers and command dispatcher
    print("👋 Backend shutting down...")
    for receiver in receivers:
        try:
            await receiver.stop()
        except Exception as e:
            print(f"  Error stopping receiver: {e}")
    await command_dispatcher.close()


app = FastAPI(
    title="theSpeaker (3 boards) API",
    description="Project with 3 boards",
    version="1.0.0",
    lifespan=lifespan
)

# CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Include API routers
app.include_router(router, prefix="/api")


@app.middleware("http")
async def auto_register_device_ip(request: Request, call_next):
    """Auto-register device IP in command dispatcher when device sends data."""
    response = await call_next(request)
    path = request.url.path
    if path.startswith("/api/mcu/") and request.client:
        mcu_id = request.headers.get("x-mcu-id") or request.query_params.get("mcu_id")
        if not mcu_id:
            parts = path.strip("/").split("/")
            if len(parts) >= 4 and parts[0] == "api" and parts[1] == "mcu":
                candidate = parts[2]
                if candidate not in ("data", "stream", "heartbeat", "devices", "streams"):
                    mcu_id = candidate
        if mcu_id:
            command_dispatcher.register_from_telemetry(mcu_id, request.client.host)
    return response


@app.get("/")
async def root():
    """API health check and status"""
    return {
        "status": "running",
        "project": "theSpeaker (3 boards)",
        "signals_count": 10,
        "mcus": 3,
        "buffer_size": len(readings_buffer),
        "active_connections": len(manager.active_connections),
        "communication_methods": [
            "http",
            "tcp",
        ]
    }


@app.get("/health")
async def health():
    """Health check endpoint"""
    return {"status": "healthy"}


@app.post("/api/cause-action/ping")
@app.get("/api/cause-action/ping")
async def cause_action_ping():
    """Cause-action ping/pong for ESP32 HTTPS probe."""
    return {"type": "pong"}


@app.post("/api/cause-action/data")
async def cause_action_data(request: Request):
    """Cause-action data endpoint for HTTPS probe -- ingest same as /api/mcu/data."""
    body = await request.json()
    mcu_id = body.get("mcu_id")
    
    # Auto-register device IP for command dispatch
    if mcu_id and request.client:
        command_dispatcher.register_from_telemetry(mcu_id, request.client.host)
    
    if body.get("type") == "header":
        signals = body.get("signals", [])
        print(f"CA header from {mcu_id}: {len(signals)} signals")
        return {"type": "pong", "status": "header_received"}
    values = body.get("values", body)
    try:
        reading = SensorReading(mcu_id=mcu_id, **{k: v for k, v in values.items() if k != "mcu_id"})
        store_reading(reading)
        await broadcast_reading(reading)
    except Exception as e:
        print(f"CA data parse: {e}, storing raw")
        if DB_AVAILABLE and mcu_id:
            store_telemetry(mcu_id, {k: v for k, v in values.items() if isinstance(v, (int, float))})
    
    # Check for ACKs from pending simulation commands (works with raw values
    # even when the SensorReading model doesn't have the field)
    if isinstance(values, dict):
        for signal_name, sim_info in list(simulation_state.items()):
            if not sim_info.get('pending'):
                continue
            if signal_name in values and values[signal_name] is not None:
                sim_info['pending'] = False
                await manager.broadcast({
                    "type": "set_ack",
                    "signal": signal_name,
                    "value": values[signal_name],
                    "mcu_id": mcu_id,
                })
    
    return {"status": "ok"}


@app.get("/metrics")
async def metrics():
    """Prometheus-compatible metrics endpoint"""
    from fastapi.responses import PlainTextResponse
    try:
        from prometheus_client import (
            generate_latest, CONTENT_TYPE_LATEST,
            Counter, Gauge, REGISTRY
        )
        
        if not hasattr(metrics, '_initialized'):
            metrics._readings_total = Counter(
                'sensor_readings_total',
                'Total sensor readings received',
                ['mcu_id', 'signal'],
                registry=REGISTRY
            )
            metrics._connected_mcus = Gauge(
                'connected_mcus',
                'Number of MCUs with recent data',
                registry=REGISTRY
            )
            metrics._websocket_clients = Gauge(
                'websocket_clients',
                'Active WebSocket connections',
                registry=REGISTRY
            )
            metrics._initialized = True
        
        metrics._connected_mcus.set(sum(1 for v in mcu_latest.values() if v is not None))
        metrics._websocket_clients.set(len(manager.active_connections))
        
        return PlainTextResponse(
            content=generate_latest(REGISTRY).decode('utf-8'),
            media_type=CONTENT_TYPE_LATEST
        )
    except ImportError:
        return PlainTextResponse(
            content="# prometheus-client not installed\n",
            media_type="text/plain"
        )


@app.get("/api/mcus")
async def get_mcus():
    """Get list of configured MCUs and their signals"""
    return {
        "mcus": [
            {
                "id": "board_0",
                "name": "theSpeaker",
                "signals": [
                    {"name": "thespeaker_bluetoothspeaker_connected", "type": "bool", "unit": ""},
                    {"name": "thespeaker_bluetoothspeaker_device_name", "type": "string", "unit": ""},
                    {"name": "thespeaker_bluetoothspeaker_rssi", "type": "int", "unit": ""},
                ],
                "communication": "http",
                "last_seen": mcu_latest.get("board_0", {}).timestamp.isoformat() if mcu_latest.get("board_0") else None,
            },
            {
                "id": "board_1",
                "name": "environment",
                "signals": [
                    {"name": "environment_aht10_temperature", "type": "float", "unit": ""},
                    {"name": "environment_aht10_humidity", "type": "float", "unit": ""},
                ],
                "communication": "http",
                "last_seen": mcu_latest.get("board_1", {}).timestamp.isoformat() if mcu_latest.get("board_1") else None,
            },
            {
                "id": "board_5",
                "name": "motor_n_mike",
                "signals": [
                    {"name": "motor_n_mike_inmp441_audio_samples", "type": "buffer", "unit": ""},
                    {"name": "motor_n_mike_inmp441_audio_level", "type": "int", "unit": ""},
                    {"name": "motor_n_mike_inmp441_peak_amplitude", "type": "int", "unit": ""},
                    {"name": "motor_n_mike_inmp441_rms_level", "type": "float", "unit": ""},
                    {"name": "motor_n_mike_inmp441_is_clipping", "type": "bool", "unit": ""},
                ],
                "communication": "tcp",
                "last_seen": mcu_latest.get("board_5", {}).timestamp.isoformat() if mcu_latest.get("board_5") else None,
            },
        ]
    }


@app.get("/api/mcu/{mcu_id}/data")
async def get_mcu_data(mcu_id: str, limit: int = 100):
    """Get data from a specific MCU"""
    if mcu_id not in mcu_buffers:
        raise HTTPException(status_code=404, detail=f"MCU {mcu_id} not found")
    
    data = list(mcu_buffers[mcu_id])[-limit:]
    return {
        "mcu_id": mcu_id,
        "latest": mcu_latest.get(mcu_id).model_dump(mode='json', by_alias=True) if mcu_latest.get(mcu_id) else None,
        "history": [r.model_dump(mode='json', by_alias=True) for r in data],
        "count": len(data)
    }


# =============================================================================
# BIDIRECTIONAL IO SIMULATION
# Allows frontend to override/simulate input values for testing
# Implements "last value wins" logic - simulated values are sent to device
# =============================================================================

# Simulation state storage
simulation_state: Dict[str, Dict[str, Any]] = {}

@app.post("/api/simulate/{signal_name}")
async def simulate_signal(signal_name: str, body: Dict[str, Any]):
    """
    Send a set_output command to a device for an input variable (bidirectional IO).

    The command is delivered via:
    1. HTTP POST to device /cmd endpoint (if device IP is known via CommandDispatcher)
    2. Queued in HTTP receiver pending_commands (delivered on next heartbeat)

    The frontend toggle should NOT change state until the device ACKs by sending
    the updated value back via telemetry. The backend broadcasts a 'set_ack' message
    when the telemetry confirms the value change.
    """
    value = body.get("value")
    simulation_active = body.get("simulation_active", True)
    
    simulation_state[signal_name] = {
        "active": simulation_active,
        "value": value,
        "pending": True,
        "timestamp": datetime.now().isoformat()
    }
    
    # Broadcast a PENDING state to frontends (toggle shows loading, not confirmed)
    pending_msg = {
        "type": "set_pending",
        "signal": signal_name,
        "value": value,
    }
    await manager.broadcast(pending_msg)

    # Push command to device(s) via the command dispatcher (HTTP POST to device /cmd)
    device_cmd = {
        "type": "set_output",
        "variable_id": signal_name,
        "value": 1 if value else 0,
    }
    delivered = await command_dispatcher.dispatch_broadcast(device_cmd)

    if delivered > 0:
        # Device responded with HTTP 200 — treat as ACK
        simulation_state[signal_name]['pending'] = False
        await manager.broadcast({
            "type": "set_ack",
            "signal": signal_name,
            "value": value,
        })
    else:
        # Queue command in HTTP receiver for delivery on next heartbeat
        http_receiver = getattr(app.state, 'http_receiver', None)
        if http_receiver:
            for mcu_id in list(http_receiver.device_registry.keys()):
                http_receiver.queue_command(
                    mcu_id, "set_output", signal_name, value=1 if value else 0
                )

    return {
        "status": "ok" if delivered > 0 else "pending",
        "signal": signal_name,
        "value": value,
        "active": simulation_active,
        "delivered_to_devices": delivered,
    }


@app.post("/api/simulate/{signal_name}/clear")
async def clear_simulation(signal_name: str):
    """Clear simulation for a signal, returning to real hardware input."""
    if signal_name in simulation_state:
        del simulation_state[signal_name]
    
    # Broadcast clear command
    clear_msg = {
        "type": "simulation_clear",
        "signal": signal_name
    }
    await manager.broadcast(clear_msg)
    
    return {"status": "ok", "signal": signal_name, "simulation_cleared": True}


@app.get("/api/simulation/status")
async def get_simulation_status():
    """Get current simulation state for all signals."""
    return {
        "simulations": simulation_state,
        "count": len([s for s in simulation_state.values() if s.get("active")])
    }


@app.websocket("/ws/device/{mcu_id}")
async def device_websocket(websocket: WebSocket, mcu_id: str):
    """WebSocket connection for MCU device telemetry ingest.

    Upstream (device -> backend): telemetry data frames.
    Commands flow via the separate cmd_channel (HTTP POST to device /cmd).
    """
    await device_manager.connect(mcu_id, websocket)

    # Auto-register device IP for command dispatch
    device_ip = websocket.client.host if websocket.client else ""
    if device_ip:
        command_dispatcher.register_from_telemetry(mcu_id, device_ip)

    try:
        while True:
            raw = await websocket.receive_text()
            data = json.loads(raw)
            msg_type = data.get("type", data.get("frame_role", "data"))

            if msg_type == "heartbeat":
                await websocket.send_json({
                    "type": "heartbeat_ack",
                    "timestamp": datetime.utcnow().isoformat(),
                })

            elif msg_type in ("data", "telemetry"):
                values = data.get("values", data)
                mcu = data.get("mcu_id", mcu_id)
                try:
                    reading = SensorReading(mcu_id=mcu, **{
                        k: v for k, v in values.items()
                        if k not in ("type", "mcu_id", "frame_role", "timestamp")
                    })
                    store_reading(reading)
                    await broadcast_reading(reading)
                except Exception as e:
                    print(f"Device WS data parse: {e}")

            elif msg_type == "set_response":
                await manager.broadcast(data)

    except WebSocketDisconnect:
        device_manager.disconnect(mcu_id)
    except Exception as e:
        print(f"Device WS error ({mcu_id}): {e}")
        device_manager.disconnect(mcu_id)


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """WebSocket endpoint for real-time data streaming to frontend"""
    await manager.connect(websocket)
    
    # Send signal metadata on connect
    await websocket.send_json({
        "type": "metadata",
        "signals": [s.model_dump() for s in SIGNAL_METADATA],
        "mcus": [
            {"id": "board_0", "name": "theSpeaker"},
            {"id": "board_1", "name": "environment"},
            {"id": "board_5", "name": "motor_n_mike"},
        ]
    })
    
    # Send latest reading if available
    if latest_reading:
        await websocket.send_json({
            "type": "data",
            "data": latest_reading.model_dump(mode='json', by_alias=True)
        })
    
    try:
        while True:
            data = await websocket.receive_text()
            msg = json.loads(data)
            
            if msg.get("type") == "subscribe":
                # Subscribe to specific MCU(s)
                mcu_ids = msg.get("mcu_ids", [])
                for mcu_id in mcu_ids:
                    manager.subscribe(websocket, mcu_id)
                await websocket.send_json({
                    "type": "subscribed",
                    "mcu_ids": mcu_ids
                })
            
            elif msg.get("type") == "get_history":
                limit = msg.get("limit", 100)
                mcu_id = msg.get("mcu_id")
                
                if mcu_id and mcu_id in mcu_buffers:
                    history = list(mcu_buffers[mcu_id])[-limit:]
                else:
                    history = list(readings_buffer)[-limit:]
                
                await websocket.send_json({
                    "type": "history",
                    "mcu_id": mcu_id,
                    "data": [r.model_dump(mode='json', by_alias=True) for r in history]
                })
            
            elif msg.get("type") == "ping":
                await websocket.send_json({"type": "pong"})

            elif msg.get("type") == "set":
                # Handle SET command for parameters — dispatch to devices and frontends
                variable_id = msg.get("variable_id", "")
                value = msg.get("value")
                request_id = msg.get("request_id", "")
                
                # Store the parameter value
                app.state.parameter_values[variable_id] = value
                
                # Dispatch command to devices via HTTP POST (cmd_channel)
                set_cmd = {
                    "type": "set_output",
                    "variable_id": variable_id,
                    "value": value,
                }
                await command_dispatcher.dispatch_broadcast(set_cmd)
                
                # Also broadcast parameter update to other frontends
                await broadcast_parameter_update(variable_id, value)

            elif msg.get("type") == "get":
                # Handle GET command for parameters
                variable_id = msg.get("variable_id", "")
                request_id = msg.get("request_id", "")
                
                # Get stored value or broadcast GET request to MCU
                stored_value = app.state.parameter_values.get(variable_id)
                if stored_value is not None:
                    await websocket.send_json({
                        "type": "get_response",
                        "variable_id": variable_id,
                        "value": stored_value,
                        "request_id": request_id,
                    })
                else:
                    # Broadcast GET request to MCUs
                    get_msg = {
                        "type": "get",
                        "variable_id": variable_id,
                        "request_id": request_id,
                    }
                    await manager.broadcast(get_msg)

            elif msg.get("type") == "request_stream":
                # Frontend requests device to start streaming specific variables
                variables = msg.get("variables", [])
                mcu_id = msg.get("mcu_id")
                
                # Capture in protocol sniffer
                sniffer = getattr(app.state, 'protocol_sniffer', None)
                if sniffer:
                    sniffer.capture_frontend_request("request_stream", variables, mcu_id or "auto")
                
                # Dispatch stream-start command to device(s) via cmd_channel.
                # Use dispatch_stream_control to resolve variable -> frame_id
                # for protocol-aware devices.
                if variables:
                    delivered = 0
                    for var_id in variables:
                        if mcu_id:
                            ok = await command_dispatcher.dispatch_stream_control(
                                mcu_id, var_id, "begin_stream"
                            )
                            delivered += int(ok)
                        else:
                            # Broadcast: send to all known devices
                            for did in list(command_dispatcher.known_devices.keys()):
                                ok = await command_dispatcher.dispatch_stream_control(
                                    did, var_id, "begin_stream"
                                )
                                delivered += int(ok)

                    await websocket.send_json({
                        "type": "stream_requested",
                        "mcu_id": mcu_id or "broadcast",
                        "variables": variables,
                        "status": "pushed" if delivered else "no_device",
                    })

            elif msg.get("type") == "stop_stream":
                # Frontend requests device to stop streaming specific variables
                variables = msg.get("variables", [])
                mcu_id = msg.get("mcu_id")
                
                # Capture in protocol sniffer
                sniffer = getattr(app.state, 'protocol_sniffer', None)
                if sniffer:
                    sniffer.capture_frontend_request("stop_stream", variables, mcu_id or "auto")
                
                if variables:
                    delivered = 0
                    for var_id in variables:
                        if mcu_id:
                            ok = await command_dispatcher.dispatch_stream_control(
                                mcu_id, var_id, "stop_stream"
                            )
                            delivered += int(ok)
                        else:
                            for did in list(command_dispatcher.known_devices.keys()):
                                ok = await command_dispatcher.dispatch_stream_control(
                                    did, var_id, "stop_stream"
                                )
                                delivered += int(ok)

                    await websocket.send_json({
                        "type": "stream_stop_requested",
                        "mcu_id": mcu_id or "broadcast",
                        "variables": variables,
                        "status": "pushed" if delivered else "no_device",
                    })

            elif msg.get("type") == "set_response":
                # MCU acknowledged SET - broadcast to frontends
                await manager.broadcast(msg)

            elif msg.get("type") == "get_response":
                # MCU responded to GET - store and broadcast to frontends
                variable_id = msg.get("variable_id", "")
                value = msg.get("value")
                if value is not None:
                    app.state.parameter_values[variable_id] = value
                await manager.broadcast(msg)
                
    except WebSocketDisconnect:
        manager.disconnect(websocket)


# Parameter broadcast function
async def broadcast_parameter_update(param_name: str, value):
    """Broadcast a parameter update to all connected WebSocket clients"""
    await manager.broadcast({
        "type": "parameter_update",
        "parameter": param_name,
        "value": value,
        "timestamp": datetime.utcnow().isoformat()
    })


# Export for use in routers
app.state.store_reading = store_reading
app.state.broadcast_reading = broadcast_reading
app.state.broadcast_parameter_update = broadcast_parameter_update
app.state.readings_buffer = readings_buffer
app.state.mcu_buffers = mcu_buffers
app.state.mcu_latest = mcu_latest
app.state.get_latest = lambda: latest_reading
app.state.parameter_values = {}
app.state.command_dispatcher = command_dispatcher


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8500)