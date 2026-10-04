#!/usr/bin/env python3
"""
ESP32 Hardware-in-the-Loop (HIL) Simulator for theSpeaker (3 boards)

This script simulates the ESP32 device behavior for testing the backend and frontend
without actual hardware. It:

1. Connects to the backend via WebSocket
2. Responds to START_STREAM / STOP_STREAM commands
3. Streams sensor data when streaming is active
4. Handles GET/SET commands for parameters
5. Sends data via HTTP POST (mimicking the actual ESP32 HTTP client)

Usage:
    # Start backend first:
    ./scripts/start_backend.sh
    
    # Then run the HIL simulator:
    python scripts/esp32_hil_simulator.py
    
    # Run with custom interval (default 1000ms):
    python scripts/esp32_hil_simulator.py --interval 500
    
    # Run in verbose mode:
    python scripts/esp32_hil_simulator.py --verbose
"""

import asyncio
import aiohttp
import json
import sys
import signal
import argparse
import random
from datetime import datetime
from typing import Optional, Dict, Any, Set
from dataclasses import dataclass, field


# Configuration
BACKEND_PORT = 8500
BACKEND_URL = f"http://localhost:{BACKEND_PORT}"
WS_URL = f"ws://localhost:{BACKEND_PORT}/ws"


class Colors:
    GREEN = '\033[0;32m'
    CYAN = '\033[0;36m'
    RED = '\033[0;31m'
    YELLOW = '\033[0;33m'
    MAGENTA = '\033[0;35m'
    NC = '\033[0m'
    BOLD = '\033[1m'
    DIM = '\033[2m'


def log_send(msg): print(f"{Colors.GREEN}↑ TX{Colors.NC} {msg}")
def log_recv(msg): print(f"{Colors.CYAN}↓ RX{Colors.NC} {msg}")
def log_info(msg): print(f"{Colors.DIM}ℹ {msg}{Colors.NC}")
def log_warn(msg): print(f"{Colors.YELLOW}⚠ {msg}{Colors.NC}")
def log_error(msg): print(f"{Colors.RED}✗ {msg}{Colors.NC}")
def log_state(msg): print(f"{Colors.MAGENTA}● {msg}{Colors.NC}")


@dataclass
class ESP32State:
    """Simulated ESP32 device state."""
    mcu_id: str = "board_0"
    device_name: str = "theSpeaker (3 boards)"
    
    # Sensor base values (will vary around these)
    base_temperature: float = 25.0
    base_humidity: float = 50.0
    
    # Parameters (can be changed via SET commands)
    read_interval: int = 1000  # ms
    
    # Streaming state
    streaming_variables: Set[str] = field(default_factory=set)
    
    # Statistics
    data_sent_count: int = 0
    commands_received: int = 0
    
    def is_streaming(self, variable: str = None) -> bool:
        if variable:
            return variable in self.streaming_variables
        return len(self.streaming_variables) > 0
    
    def start_streaming(self, variable: str):
        self.streaming_variables.add(variable)
        log_state(f"Started streaming: {variable}")
    
    def stop_streaming(self, variable: str):
        self.streaming_variables.discard(variable)
        log_state(f"Stopped streaming: {variable}")
    
    def generate_sensor_reading(self) -> Dict[str, float]:
        return {
            "temperature": round(self.base_temperature + random.uniform(-2.0, 2.0), 2),
            "humidity": round(self.base_humidity + random.uniform(-5.0, 5.0), 2),
        }


class ESP32HILSimulator:
    """Hardware-in-the-Loop simulator for ESP32."""
    
    def __init__(self, backend_url: str, ws_url: str, verbose: bool = False):
        self.backend_url = backend_url
        self.ws_url = ws_url
        self.verbose = verbose
        self.state = ESP32State()
        self.http_session: Optional[aiohttp.ClientSession] = None
        self.ws_connection = None
        self.running = False
        self._stream_task: Optional[asyncio.Task] = None
        self._ws_task: Optional[asyncio.Task] = None
    
    async def start(self):
        self.running = True
        self.http_session = aiohttp.ClientSession()
        
        log_info(f"ESP32 HIL Simulator starting...")
        log_info(f"  MCU ID: {self.state.mcu_id}")
        log_info(f"  Device: {self.state.device_name}")
        log_info(f"  Backend: {self.backend_url}")
        log_info(f"  Read Interval: {self.state.read_interval}ms")
        
        self._ws_task = asyncio.create_task(self._websocket_listener())
        self._stream_task = asyncio.create_task(self._data_streamer())
        
        log_state("Simulator running. Press Ctrl+C to stop.")
        
        try:
            await asyncio.gather(self._ws_task, self._stream_task)
        except asyncio.CancelledError:
            pass
    
    async def stop(self):
        self.running = False
        
        if self._stream_task:
            self._stream_task.cancel()
        if self._ws_task:
            self._ws_task.cancel()
        
        if self.ws_connection:
            await self.ws_connection.close()
        if self.http_session:
            await self.http_session.close()
        
        log_info(f"Simulator stopped. Sent {self.state.data_sent_count} readings.")
    
    async def _websocket_listener(self):
        try:
            import websockets
        except ImportError:
            log_warn("Installing websockets...")
            import subprocess
            subprocess.run([sys.executable, "-m", "pip", "install", "-q", "websockets"], check=True)
            import websockets
        
        while self.running:
            try:
                async with websockets.connect(self.ws_url) as ws:
                    self.ws_connection = ws
                    log_info("WebSocket connected to backend")
                    
                    register_msg = {
                        "type": "register",
                        "client_type": "mcu",
                        "mcu_id": self.state.mcu_id,
                        "device_name": self.state.device_name
                    }
                    await ws.send(json.dumps(register_msg))
                    log_send(f"Registered as MCU: {self.state.mcu_id}")
                    
                    async for message in ws:
                        await self._handle_ws_message(message, ws)
                        
            except asyncio.CancelledError:
                break
            except Exception as e:
                if self.running:
                    log_warn(f"WebSocket disconnected: {e}. Reconnecting in 3s...")
                    await asyncio.sleep(3)
    
    async def _handle_ws_message(self, message: str, ws):
        try:
            msg = json.loads(message)
            msg_type = msg.get("type", "")
            
            if self.verbose:
                log_recv(f"{msg_type}: {json.dumps(msg)[:200]}")
            
            self.state.commands_received += 1
            
            if msg_type == "get":
                await self._handle_get(msg, ws)
            elif msg_type == "set":
                await self._handle_set(msg, ws)
            elif msg_type == "start_stream":
                variable = msg.get("variable_id", "all")
                self.state.start_streaming(variable)
            elif msg_type == "stop_stream":
                variable = msg.get("variable_id", "all")
                if variable == "all":
                    self.state.streaming_variables.clear()
                else:
                    self.state.stop_streaming(variable)
                    
        except json.JSONDecodeError:
            log_warn(f"Invalid JSON: {message[:100]}")
        except Exception as e:
            log_error(f"Error handling message: {e}")
    
    async def _handle_get(self, msg: dict, ws):
        variable = msg.get("variable_id", msg.get("parameter", ""))
        request_id = msg.get("request_id", "")
        
        value = None
        if "interval" in variable.lower():
            value = self.state.read_interval
        
        response = {
            "type": "get_response",
            "variable_id": variable,
            "value": value,
            "mcu_id": self.state.mcu_id,
            "request_id": request_id,
            "timestamp": datetime.now().isoformat()
        }
        await ws.send(json.dumps(response))
        log_send(f"GET response: {variable}={value}")
    
    async def _handle_set(self, msg: dict, ws):
        variable = msg.get("variable_id", msg.get("parameter", ""))
        value = msg.get("value")
        request_id = msg.get("request_id", "")
        
        success = False
        if "interval" in variable.lower():
            try:
                self.state.read_interval = int(value)
                success = True
                log_state(f"Read interval changed to {self.state.read_interval}ms")
            except (ValueError, TypeError):
                pass
        
        response = {
            "type": "set_response",
            "variable_id": variable,
            "value": value,
            "success": success,
            "mcu_id": self.state.mcu_id,
            "request_id": request_id
        }
        await ws.send(json.dumps(response))
        log_send(f"SET response: {variable}={value} (success={success})")
    
    async def _data_streamer(self):
        # Auto-start streaming
        for sig in ['temperature', 'humidity']:
            self.state.start_streaming(sig)
        
        while self.running:
            try:
                if self.state.is_streaming():
                    await self._send_sensor_data()
                await asyncio.sleep(self.state.read_interval / 1000.0)
            except asyncio.CancelledError:
                break
            except Exception as e:
                log_error(f"Streaming error: {e}")
                await asyncio.sleep(1)
    
    async def _send_sensor_data(self):
        if not self.http_session:
            return
        
        reading = self.state.generate_sensor_reading()
        
        payload = {
            "mcu_id": self.state.mcu_id,
            "device_name": self.state.device_name,
            "temperature": reading["temperature"],
            "humidity": reading["humidity"],
            "read_interval": self.state.read_interval,
        }
        
        try:
            async with self.http_session.post(
                f"{self.backend_url}/api/mcu/data",
                json=payload,
                headers={"Content-Type": "application/json"}
            ) as response:
                self.state.data_sent_count += 1
                if response.status not in (200, 201):
                    log_warn(f"HTTP {response.status}")
                elif self.verbose or self.state.data_sent_count % 10 == 0:
                    log_send(f"Sent {self.state.data_sent_count} readings")
        except Exception as e:
            log_error(f"Failed to send data: {e}")


async def main(args):
    print(f"\n{Colors.BOLD}{Colors.CYAN}{'='*50}{Colors.NC}")
    print(f"{Colors.BOLD}{Colors.CYAN} ESP32 HIL Simulator - theSpeaker (3 boards){Colors.NC}")
    print(f"{Colors.BOLD}{Colors.CYAN}{'='*50}{Colors.NC}\n")
    
    simulator = ESP32HILSimulator(
        backend_url=BACKEND_URL,
        ws_url=WS_URL,
        verbose=args.verbose
    )
    
    if args.interval:
        simulator.state.read_interval = args.interval
    
    loop = asyncio.get_event_loop()
    
    def signal_handler():
        log_info("Shutting down...")
        asyncio.create_task(simulator.stop())
    
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, signal_handler)
    
    try:
        await simulator.start()
    except KeyboardInterrupt:
        pass
    finally:
        await simulator.stop()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ESP32 HIL Simulator")
    parser.add_argument("--interval", "-i", type=int, help="Data send interval in ms (default: 1000)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    args = parser.parse_args()
    
    asyncio.run(main(args))
