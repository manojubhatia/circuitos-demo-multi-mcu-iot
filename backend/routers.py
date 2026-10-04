"""
API Routers for theSpeaker (3 boards)
Generated from dataflow configuration
"""

from fastapi import APIRouter, HTTPException, Request
from typing import List, Optional
from datetime import datetime

from models import (
    SensorReading,
    SignalMetadata,
    HistoryResponse,
    SignalHistoryResponse,
    SIGNAL_METADATA,
)

router = APIRouter()


@router.get("/signals", response_model=List[SignalMetadata])
async def get_signals():
    """Get metadata about all available signals for frontend configuration"""
    return SIGNAL_METADATA


@router.get("/signals/{signal_name}", response_model=SignalMetadata)
async def get_signal_metadata(signal_name: str):
    """Get metadata for a specific signal"""
    for signal in SIGNAL_METADATA:
        if signal.name == signal_name:
            return signal
    raise HTTPException(status_code=404, detail=f"Signal '{signal_name}' not found")


@router.post("/data", status_code=201)
@router.post("/telemetry", status_code=201)
async def receive_data(request: Request):
    """
    Receive sensor data from device (HTTP POST)

    This endpoint is called by the embedded device to send sensor readings.
    Accepts both /api/data and /api/telemetry for compatibility.
    Supports both protocol-aware frames (frame_role+values) and legacy flat payloads.
    """
    body = await request.json()

    # Protocol-aware frame: {"frame_role":"data","mcu_id":"...","values":{...},"sequences":{...}}
    frame_role = body.get("frame_role")
    if frame_role == "data" and "values" in body:
        values = body["values"]
        mcu_id = body.get("mcu_id")
    else:
        values = body
        mcu_id = body.get("device_id") or body.get("mcu_id")

    # Auto-register device IP for command dispatch
    if mcu_id and request.client:
        cmd_dispatcher = getattr(request.app.state, 'command_dispatcher', None)
        if cmd_dispatcher:
            cmd_dispatcher.register_from_telemetry(mcu_id, request.client.host)

    sensor_reading = SensorReading(
        timestamp=datetime.utcnow(),
        mcu_id=mcu_id,
        environment_aht10_temperature=values.get("environment_aht10_temperature"),
        environment_aht10_humidity=values.get("environment_aht10_humidity"),
        motor_n_mike_inmp441_audio_samples=values.get("motor_n_mike_inmp441_audio_samples"),
        motor_n_mike_inmp441_audio_level=values.get("motor_n_mike_inmp441_audio_level"),
        motor_n_mike_inmp441_peak_amplitude=values.get("motor_n_mike_inmp441_peak_amplitude"),
        motor_n_mike_inmp441_rms_level=values.get("motor_n_mike_inmp441_rms_level"),
        motor_n_mike_inmp441_is_clipping=values.get("motor_n_mike_inmp441_is_clipping"),
        thespeaker_bluetoothspeaker_connected=values.get("thespeaker_bluetoothspeaker_connected"),
        thespeaker_bluetoothspeaker_device_name=values.get("thespeaker_bluetoothspeaker_device_name"),
        thespeaker_bluetoothspeaker_rssi=values.get("thespeaker_bluetoothspeaker_rssi"),
    )

    # Store and broadcast
    request.app.state.store_reading(sensor_reading)
    await request.app.state.broadcast_reading(sensor_reading)

    # Capture in protocol sniffer for visibility
    sniffer = getattr(request.app.state, 'protocol_sniffer', None)
    if sniffer:
        sniffer.capture(
            direction="device_to_backend",
            frame_role="data",
            frame_name=body.get("frame_name", "telemetry_data"),
            source=mcu_id or "device",
            target="backend",
            payload_summary={
                "variables": list(values.keys()) if isinstance(values, dict) else [],
                "values": {k: v for k, v in (values if isinstance(values, dict) else {}).items() if v is not None},
            },
        )

    return {"status": "ok", "timestamp": sensor_reading.timestamp.isoformat()}


@router.get("/data/latest", response_model=Optional[SensorReading])
async def get_latest(request: Request):
    """Get the most recent sensor reading"""
    return request.app.state.get_latest()


@router.get("/data/history", response_model=HistoryResponse)
async def get_history(
    request: Request,
    limit: int = 100,
    signal: Optional[str] = None
):
    """
    Get historical sensor readings
    
    Args:
        limit: Maximum number of readings to return (default 100)
        signal: Optional signal name to filter
    """
    buffer = request.app.state.readings_buffer
    data = list(buffer)[-limit:]
    
    if signal:
        # Validate signal exists
        if not any(s.name == signal for s in SIGNAL_METADATA):
            raise HTTPException(status_code=404, detail=f"Signal '{signal}' not found")
    
    return HistoryResponse(data=data, count=len(data))


@router.get("/data/history/{signal_name}", response_model=SignalHistoryResponse)
async def get_signal_history(signal_name: str, request: Request, limit: int = 100):
    """Get historical data for a specific signal"""
    # Validate signal exists
    if not any(s.name == signal_name for s in SIGNAL_METADATA):
        raise HTTPException(status_code=404, detail=f"Signal '{signal_name}' not found")
    
    buffer = request.app.state.readings_buffer
    data = [
        {
            "timestamp": r.timestamp.isoformat() if r.timestamp else None,
            "value": getattr(r, signal_name, None)
        }
        for r in list(buffer)[-limit:]
        if getattr(r, signal_name, None) is not None
    ]
    
    return SignalHistoryResponse(signal=signal_name, data=data)


@router.delete("/data")
async def clear_data(request: Request):
    """Clear all stored data (for testing/debugging)"""
    request.app.state.readings_buffer.clear()
    return {"status": "cleared"}


# =============================================================================
# Parameter Endpoints (for sending values TO MCUs)
# =============================================================================

# =============================================================================
# Persistent History & Alerts (SQLite-backed)
# =============================================================================

try:
    from db import store_telemetry, query_history, get_statistics, cleanup_old_data, store_alert, get_alerts
    DB_AVAILABLE = True
except ImportError:
    DB_AVAILABLE = False


@router.get("/history/query")
async def query_persistent_history(
    device_id: Optional[str] = None,
    signal: Optional[str] = None,
    from_time: Optional[str] = None,
    to_time: Optional[str] = None,
    limit: int = 1000,
    aggregation: Optional[str] = None,
):
    """Query persistent historical data with optional aggregation."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    data = query_history(device_id, signal, from_time, to_time, limit, aggregation)
    return {"data": data, "count": len(data)}


@router.get("/history/stats")
async def get_history_stats(
    device_id: Optional[str] = None,
    signal: Optional[str] = None,
    from_time: Optional[str] = None,
    to_time: Optional[str] = None,
):
    """Get statistical summary (min, max, avg) for signals."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    stats = get_statistics(device_id, signal, from_time, to_time)
    return {"stats": stats}


@router.get("/history/export")
async def export_history(
    device_id: Optional[str] = None,
    signal: Optional[str] = None,
    from_time: Optional[str] = None,
    to_time: Optional[str] = None,
    format: str = "json",
    limit: int = 10000,
):
    """Export historical data as JSON or CSV."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    
    data = query_history(device_id, signal, from_time, to_time, limit)
    
    if format == "csv":
        from fastapi.responses import PlainTextResponse
        if not data:
            return PlainTextResponse("timestamp,device_id,signal_name,value\n", media_type="text/csv")
        headers = list(data[0].keys())
        lines = [",".join(headers)]
        for row in data:
            lines.append(",".join(str(row.get(h, "")) for h in headers))
        return PlainTextResponse("\n".join(lines), media_type="text/csv")
    
    return {"data": data, "count": len(data), "format": "json"}


@router.post("/history/cleanup")
async def cleanup_history(retention_days: int = 30):
    """Delete data older than retention period."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    deleted = cleanup_old_data(retention_days)
    return {"status": "ok", "deleted_rows": deleted, "retention_days": retention_days}


@router.get("/alerts/history")
async def get_alert_history(limit: int = 100, severity: Optional[str] = None, active_only: bool = False):
    """Get alert history from persistent storage."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    alerts = get_alerts(limit, severity, active_only)
    return {"alerts": alerts, "count": len(alerts)}


@router.post("/alerts")
async def create_alert_event(request: Request):
    """Store a new alert event (called by frontend or alert engine)."""
    if not DB_AVAILABLE:
        raise HTTPException(status_code=501, detail="Database not available")
    body = await request.json()
    store_alert(body)
    return {"status": "ok"}


