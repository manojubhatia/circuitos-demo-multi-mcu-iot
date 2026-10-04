"""
Protocol Frame Sniffer / Capture

Captures all protocol frames (heartbeat, start_stream, stop_stream, data, begin, stop)
into a ring buffer for analysis and debugging.

Endpoints:
  GET  /api/protocol/capture         - Get last N captured frames
  GET  /api/protocol/capture/stream  - SSE stream for real-time monitoring
  DELETE /api/protocol/capture       - Clear capture buffer
  GET  /api/protocol/stats           - Get protocol statistics
"""

from fastapi import APIRouter, Request
from fastapi.responses import StreamingResponse
from typing import List, Dict, Any, Optional
from collections import deque
from datetime import datetime
import asyncio
import json
from loguru import logger


MAX_CAPTURE_SIZE = 1000


class CapturedFrame:
    """A single captured protocol frame"""

    def __init__(
        self,
        direction: str,
        frame_role: str,
        frame_name: Optional[str],
        source: str,
        target: str,
        payload_summary: Dict[str, Any],
    ):
        self.timestamp = datetime.utcnow()
        self.direction = direction
        self.frame_role = frame_role
        self.frame_name = frame_name
        self.source = source
        self.target = target
        self.payload_summary = payload_summary
        self.sequence_id = 0

    def to_dict(self) -> Dict[str, Any]:
        return {
            "id": self.sequence_id,
            "timestamp": self.timestamp.isoformat(),
            "direction": self.direction,
            "frame_role": self.frame_role,
            "frame_name": self.frame_name,
            "source": self.source,
            "target": self.target,
            "payload_summary": self.payload_summary,
        }


class ProtocolSniffer:
    """
    Captures and stores protocol frames in a ring buffer.
    
    Attach to the HTTP receiver's on_stream_event callback to capture
    all protocol events automatically.
    """

    def __init__(self, max_size: int = MAX_CAPTURE_SIZE):
        self.buffer: deque = deque(maxlen=max_size)
        self.sequence_counter: int = 0
        self.stats: Dict[str, int] = {
            "heartbeat": 0,
            "start_stream": 0,
            "stop_stream": 0,
            "data": 0,
            "begin": 0,
            "continue": 0,
            "end": 0,
            "request": 0,
            "ack": 0,
            "nack": 0,
            "stop": 0,
            "request_stream": 0,
            "stream_started": 0,
            "stream_stopped": 0,
            "unknown": 0,
        }
        self.started_at = datetime.utcnow()
        self._sse_queues: List[asyncio.Queue] = []
        self.router = APIRouter(prefix="/protocol", tags=["Protocol Sniffer"])
        self._setup_routes()

    def capture(
        self,
        direction: str,
        frame_role: str,
        frame_name: Optional[str] = None,
        source: str = "unknown",
        target: str = "unknown",
        payload_summary: Optional[Dict[str, Any]] = None,
    ):
        """Capture a protocol frame into the ring buffer"""
        frame = CapturedFrame(
            direction=direction,
            frame_role=frame_role,
            frame_name=frame_name,
            source=source,
            target=target,
            payload_summary=payload_summary or {},
        )
        self.sequence_counter += 1
        frame.sequence_id = self.sequence_counter
        self.buffer.append(frame)

        # Update stats
        role_key = frame_role if frame_role in self.stats else "unknown"
        self.stats[role_key] = self.stats.get(role_key, 0) + 1

        # Notify SSE subscribers
        frame_dict = frame.to_dict()
        for queue in self._sse_queues:
            try:
                queue.put_nowait(frame_dict)
            except asyncio.QueueFull:
                pass

    async def capture_from_event(self, event: Dict[str, Any]):
        """
        Callback for HTTP receiver's on_stream_event.
        Automatically classifies and captures protocol events.
        """
        event_type = event.get("type", "unknown")
        mcu_id = event.get("mcu_id", "unknown")

        if event_type == "heartbeat":
            self.capture(
                direction="device_to_backend",
                frame_role="heartbeat",
                frame_name=f"heartbeat_{mcu_id}",
                source=mcu_id,
                target="backend",
                payload_summary={
                    "ip": event.get("ip"),
                    "stream_state": event.get("stream_state"),
                    "active_streams": event.get("active_streams"),
                    "commands_delivered": event.get("commands_delivered"),
                },
            )
        elif event_type == "stream_started":
            self.capture(
                direction="device_to_backend",
                frame_role="start_stream",
                frame_name=event.get("frame_name"),
                source=mcu_id,
                target="backend",
                payload_summary={
                    "signals": event.get("signals"),
                    "active_variables": event.get("active_variables"),
                    "protocol_version": event.get("protocol_version"),
                },
            )
        elif event_type == "stream_stopped":
            self.capture(
                direction="device_to_backend",
                frame_role="stop_stream",
                frame_name=event.get("frame_name"),
                source=mcu_id,
                target="backend",
                payload_summary={
                    "signals": event.get("signals"),
                },
            )
        elif event_type == "protocol_frame":
            # Protocol-aware frame with decoded wire header
            self.capture(
                direction="device_to_backend",
                frame_role=event.get("frame_type", "data").lower(),
                frame_name=event.get("frame_name"),
                source=mcu_id,
                target="backend",
                payload_summary={
                    "frame_id": event.get("frame_id"),
                    "frame_type": event.get("frame_type"),
                    "sequence": event.get("sequence"),
                    "values": event.get("values", {}),
                },
            )
        elif event_type == "data":
            self.capture(
                direction="device_to_backend",
                frame_role="data",
                frame_name=None,
                source=mcu_id,
                target="backend",
                payload_summary={
                    "signal_count": event.get("signal_count", 0),
                },
            )
        else:
            self.capture(
                direction="unknown",
                frame_role=event_type,
                source=mcu_id,
                target="backend",
                payload_summary=event,
            )

    def capture_frontend_request(self, action: str, variables: List[str], mcu_id: str):
        """Capture a frontend stream request/stop"""
        self.capture(
            direction="frontend_to_backend",
            frame_role=action,
            frame_name=f"{action}_request",
            source="frontend",
            target=mcu_id or "backend",
            payload_summary={"variables": variables},
        )

    def _setup_routes(self):
        """Set up REST endpoints for the sniffer"""

        @self.router.get("/capture")
        async def get_capture(limit: int = 100, offset: int = 0, role: Optional[str] = None):
            """Get captured protocol frames"""
            frames = list(self.buffer)
            
            if role:
                frames = [f for f in frames if f.frame_role == role]
            
            total = len(frames)
            frames = frames[-(limit + offset):]
            if offset > 0:
                frames = frames[:-offset]
            else:
                frames = frames[-limit:]

            return {
                "frames": [f.to_dict() for f in frames],
                "total": total,
                "buffer_size": len(self.buffer),
                "max_size": self.buffer.maxlen,
            }

        @self.router.get("/capture/stream")
        async def capture_stream(request: Request):
            """SSE stream for real-time protocol frame monitoring"""
            queue: asyncio.Queue = asyncio.Queue(maxsize=100)
            self._sse_queues.append(queue)

            async def event_generator():
                try:
                    # Send initial connection event
                    yield f"data: {json.dumps({'type': 'connected', 'buffer_size': len(self.buffer)})}\n\n"
                    
                    while True:
                        if await request.is_disconnected():
                            break
                        try:
                            frame = await asyncio.wait_for(queue.get(), timeout=30.0)
                            yield f"data: {json.dumps(frame)}\n\n"
                        except asyncio.TimeoutError:
                            yield f"data: {json.dumps({'type': 'keepalive'})}\n\n"
                finally:
                    self._sse_queues.remove(queue)

            return StreamingResponse(
                event_generator(),
                media_type="text/event-stream",
                headers={
                    "Cache-Control": "no-cache",
                    "Connection": "keep-alive",
                    "X-Accel-Buffering": "no",
                },
            )

        @self.router.delete("/capture")
        async def clear_capture():
            """Clear the capture buffer"""
            count = len(self.buffer)
            self.buffer.clear()
            self.sequence_counter = 0
            return {"status": "cleared", "frames_removed": count}

        @self.router.get("/stats")
        async def get_stats():
            """Get protocol statistics"""
            return {
                "stats": self.stats,
                "total_captured": self.sequence_counter,
                "buffer_size": len(self.buffer),
                "max_buffer_size": self.buffer.maxlen,
                "started_at": self.started_at.isoformat(),
                "uptime_seconds": (datetime.utcnow() - self.started_at).total_seconds(),
                "sse_subscribers": len(self._sse_queues),
            }