"""
TCP/UDP Receiver for MCU Communication

Receives data from MCUs via raw TCP or UDP sockets.
Useful for direct network communication without HTTP overhead.
"""

import asyncio
import json
import struct
from typing import Optional, Dict
from loguru import logger

from .base_receiver import BaseReceiver

# TCP/UDP Configuration from dataflow
TCP_HOST = "0.0.0.0"
TCP_PORT = 9001
UDP_HOST = "0.0.0.0"
UDP_PORT = 8002

# Protocol selection
USE_TCP = True
USE_UDP = False


class TCPReceiver(BaseReceiver):
    """
    TCP/UDP-based receiver for MCU data.
    
    TCP: Reliable, connection-oriented - good for critical data
    UDP: Fast, connectionless - good for streaming sensor data
    
    Frame format:
    - Length (2 bytes, uint16, big-endian) - only for TCP
    - Payload (JSON or binary dataframe)
    """
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._tcp_server = None
        self._udp_transport = None
        self._connections: Dict[str, asyncio.StreamWriter] = {}
    
    async def start(self):
        """Start TCP and/or UDP receivers"""
        self._running = True
        
        if USE_TCP:
            self._tcp_server = await asyncio.start_server(
                self._handle_tcp_connection,
                TCP_HOST,
                TCP_PORT
            )
            logger.info(f"TCP Receiver started on {TCP_HOST}:{TCP_PORT}")
        
        if USE_UDP:
            loop = asyncio.get_event_loop()
            self._udp_transport, _ = await loop.create_datagram_endpoint(
                lambda: UDPProtocol(self._handle_udp_data),
                local_addr=(UDP_HOST, UDP_PORT)
            )
            logger.info(f"UDP Receiver started on {UDP_HOST}:{UDP_PORT}")
    
    async def stop(self):
        """Stop TCP and UDP receivers"""
        self._running = False
        
        # Close all TCP connections
        for writer in self._connections.values():
            writer.close()
            await writer.wait_closed()
        self._connections.clear()
        
        # Stop TCP server
        if self._tcp_server:
            self._tcp_server.close()
            await self._tcp_server.wait_closed()
        
        # Stop UDP
        if self._udp_transport:
            self._udp_transport.close()
        
        logger.info("TCP/UDP Receiver stopped")
    
    async def _handle_tcp_connection(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        """Handle a new TCP connection from an MCU"""
        addr = writer.get_extra_info('peername')
        conn_id = f"{addr[0]}:{addr[1]}"
        self._connections[conn_id] = writer
        
        logger.info(f"TCP connection from {conn_id}")
        
        try:
            while self._running:
                # Read length prefix (2 bytes)
                length_data = await reader.readexactly(2)
                length = struct.unpack('>H', length_data)[0]
                
                if length == 0 or length > 65535:
                    logger.warning(f"Invalid frame length: {length}")
                    continue
                
                # Read payload
                payload = await reader.readexactly(length)
                
                # Process data
                await self._process_payload(payload, conn_id)
                
        except asyncio.IncompleteReadError:
            logger.info(f"TCP connection closed: {conn_id}")
        except Exception as e:
            logger.error(f"TCP error from {conn_id}: {e}")
        finally:
            del self._connections[conn_id]
            writer.close()
            await writer.wait_closed()
    
    async def _handle_udp_data(self, data: bytes, addr: tuple):
        """Handle incoming UDP datagram"""
        conn_id = f"{addr[0]}:{addr[1]}"
        await self._process_payload(data, conn_id)
    
    async def _process_payload(self, payload: bytes, source_id: str):
        """Process a received payload"""
        try:
            # Extract MCU ID if embedded in payload
            mcu_id = source_id
            
            # Try JSON first
            try:
                data = json.loads(payload.decode('utf-8'))
                mcu_id = data.get('device_id') or data.get('mcu_id') or source_id
                reading = self.process_data(data, mcu_id)
            except (json.JSONDecodeError, UnicodeDecodeError):
                # Try binary dataframe
                reading = self.process_data(payload, mcu_id)
            
            if reading:
                await self.handle_reading(reading)
                logger.debug(f"TCP/UDP received from {source_id}")
            else:
                logger.warning(f"Failed to decode payload from {source_id}")
                
        except Exception as e:
            logger.error(f"Payload processing error: {e}")
    
    async def send_to_mcu(self, mcu_id: str, data: bytes):
        """Send data back to an MCU via TCP (if connected)"""
        for conn_id, writer in self._connections.items():
            if mcu_id in conn_id or mcu_id == conn_id:
                try:
                    # Send with length prefix
                    length = struct.pack('>H', len(data))
                    writer.write(length + data)
                    await writer.drain()
                    return True
                except Exception as e:
                    logger.error(f"Failed to send to {mcu_id}: {e}")
        return False


class UDPProtocol(asyncio.DatagramProtocol):
    """Simple UDP protocol handler"""
    
    def __init__(self, callback):
        self.callback = callback
    
    def datagram_received(self, data: bytes, addr: tuple):
        asyncio.create_task(self.callback(data, addr))
    
    def error_received(self, exc):
        logger.error(f"UDP error: {exc}")