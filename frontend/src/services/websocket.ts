/**
 * WebSocket Service for real-time data streaming
 * Generated for theSpeaker (3 boards)
 *
 * MCU Sources:
 * - theSpeaker (board_0): http communication
 * - environment (board_1): http communication
 * - motor_n_mike (board_5): tcp communication
 */

import { SensorReading, SignalMetadata } from './api'

export const PROTOCOL_VERSION = '1.0.0'
export const SCHEMA_HASH = '00000000'

type MessageHandler = (data: SensorReading) => void
type MetadataHandler = (signals: SignalMetadata[]) => void
type ConnectionHandler = (connected: boolean) => void
type ParameterHandler = (variableId: string, value: any, success?: boolean) => void
type StreamEventHandler = (event: StreamEvent) => void

export interface MCUInfo {
  id: string
  name: string
}

export interface StreamEvent {
  type: 'stream_started' | 'stream_stopped'
  mcu_id: string
  frame_name?: string
  signals?: Array<{ variable_id: string; name: string; type: string; frequency_ms: number }>
  active_variables?: string[]
  protocol_version?: string
  schema_hash?: string
}

export interface VariableStreamInfo {
  variable_id: string
  name: string
  start_frame_name: string
  started_at: string
  last_data_at: string | null
  sequence: number
  frame_count: number
}

export interface StreamInfo {
  mcu_id: string
  signals: string[]
  started_at: string
  last_data_at: string | null
  frame_count: number
  sequences: Record<string, number>
  protocol_version: string | null
  schema_hash: string | null
  frame_name: string | null
  variable_streams: Map<string, VariableStreamInfo>
}


interface WebSocketMessage {
  type: 'data' | 'metadata' | 'history' | 'subscribed' | 'set_response' | 'get_response' | 'parameter_update' | 'stream_started' | 'stream_stopped' | 'stream_requested' | 'stream_stop_requested' | 'set_ack' | 'set_pending' | 'simulation' | 'pong'
  data?: SensorReading | SensorReading[]
  signals?: SignalMetadata[] | Array<{ variable_id: string; name: string; type: string; frequency_ms: number }>
  signal?: string
  mcus?: MCUInfo[]
  mcu_id?: string
  frame_name?: string
  active_variables?: string[]
  variable_id?: string
  parameter?: string
  value?: any
  success?: boolean
  request_id?: string
  protocol_version?: string
  schema_hash?: string
}

class WebSocketService {
  private ws: WebSocket | null = null
  private url: string
  private reconnectTimeout: number = 3000
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null
  private pingTimer: ReturnType<typeof setInterval> | null = null
  private messageHandlers: MessageHandler[] = []
  private metadataHandlers: MetadataHandler[] = []
  private connectionHandlers: ConnectionHandler[] = []
  private parameterHandlers: ParameterHandler[] = []
  private streamEventHandlers: StreamEventHandler[] = []
  private pendingStreamRequests: Array<{ variables: string[]; mcuId?: string }> = []
  private requestedVariables: Set<string> = new Set()
  private historyBuffer: SensorReading[] = []
  private mcuList: MCUInfo[] = []
  private parameterValues: Record<string, any> = {}
  private activeStreams: Map<string, StreamInfo> = new Map()

  readonly protocolVersion = PROTOCOL_VERSION
  readonly schemaHash = SCHEMA_HASH

  constructor(url?: string) {
    this.url = url || `${window.location.protocol === 'https:' ? 'wss:' : 'ws:'}//${window.location.host}/ws`
  }

  connect(): void {
    if (this.ws?.readyState === WebSocket.OPEN) return

    try {
      this.ws = new WebSocket(this.url)

      this.ws.onopen = () => {
        console.log('WebSocket connected')
        this.connectionHandlers.forEach(h => h(true))
        this.ws?.send(JSON.stringify({ type: 'get_history', limit: 100 }))
        this.startPing()
        // Notify any pending stream subscribers that connection is ready
        this.pendingStreamRequests.forEach(req => this.requestStream(req.variables, req.mcuId))
        this.pendingStreamRequests = []
      }

      this.ws.onmessage = (event) => {
        try {
          const msg: WebSocketMessage = JSON.parse(event.data)
          this.handleMessage(msg)
        } catch (e) {
          console.error('Failed to parse WebSocket message:', e)
        }
      }

      this.ws.onclose = () => {
        console.log('WebSocket disconnected')
        this.stopPing()
        this.connectionHandlers.forEach(h => h(false))
        this.scheduleReconnect()
      }

      this.ws.onerror = (error) => {
        console.error('WebSocket error:', error)
      }
    } catch (e) {
      console.error('Failed to connect WebSocket:', e)
      this.scheduleReconnect()
    }
  }

  private handleMessage(msg: WebSocketMessage): void {
    switch (msg.type) {
      case 'data':
        if (msg.data && !Array.isArray(msg.data)) {
          // Add MCU ID to the reading if available
          const reading = { ...msg.data as SensorReading }
          if (msg.mcu_id) {
            (reading as any).mcu_id = msg.mcu_id
          }
          this.messageHandlers.forEach(h => h(reading))
        }
        break
      case 'metadata':
        if (msg.signals) {
          this.metadataHandlers.forEach(h => h(msg.signals!))
        }
        if (msg.mcus) {
          this.mcuList = msg.mcus
        }
        break
      case 'history':
        if (Array.isArray(msg.data)) {
          this.historyBuffer = msg.data
        }
        break
      case 'set_response':
        if (msg.variable_id !== undefined) {
          const varId = msg.variable_id
          if (msg.success) {
            this.parameterValues[varId] = msg.value
          }
          this.parameterHandlers.forEach(h => h(varId, msg.value, msg.success))
        }
        break
      case 'get_response':
        if (msg.variable_id !== undefined && msg.value !== undefined) {
          const varId = msg.variable_id
          this.parameterValues[varId] = msg.value
          this.parameterHandlers.forEach(h => h(varId, msg.value, true))
        }
        break
      case 'parameter_update':
        const paramName = msg.parameter || msg.variable_id
        if (paramName !== undefined && msg.value !== undefined) {
          this.parameterValues[paramName] = msg.value
          this.parameterHandlers.forEach(h => h(paramName, msg.value, true))
        }
        break
      case 'set_ack':
        // Device confirmed a set_output command — notify parameter handlers
        if (msg.signal) {
          this.parameterValues[msg.signal] = msg.value
          this.parameterHandlers.forEach(h => h(msg.signal!, msg.value, true))
        }
        break
      case 'set_pending':
        // Command sent to device, waiting for ACK
        if (msg.signal) {
          this.parameterHandlers.forEach(h => h(msg.signal!, msg.value, false))
        }
        break
      case 'simulation':
        // Legacy simulation broadcast — treat as parameter update
        if (msg.signal) {
          this.parameterValues[msg.signal] = msg.value
          this.parameterHandlers.forEach(h => h(msg.signal!, msg.value, true))
        }
        break
      case 'stream_started':
        if (msg.mcu_id) {
          let streamInfo = this.activeStreams.get(msg.mcu_id)
          if (!streamInfo) {
            streamInfo = {
              mcu_id: msg.mcu_id,
              signals: [],
              started_at: new Date().toISOString(),
              last_data_at: null,
              frame_count: 0,
              sequences: {},
              protocol_version: msg.protocol_version || null,
              schema_hash: msg.schema_hash || null,
              frame_name: msg.frame_name || null,
              variable_streams: new Map(),
            }
            this.activeStreams.set(msg.mcu_id, streamInfo)
          }
          if (msg.signals) {
            for (const sig of (msg.signals as any[])) {
              const varId = sig.variable_id || sig.name
              streamInfo.signals = [...new Set([...streamInfo.signals, varId])]
              streamInfo.variable_streams.set(varId, {
                variable_id: varId,
                name: sig.name || varId,
                start_frame_name: msg.frame_name || 'start_stream',
                started_at: new Date().toISOString(),
                last_data_at: null,
                sequence: 0,
                frame_count: 0,
              })
            }
          }
          if (msg.active_variables) {
            streamInfo.signals = msg.active_variables
          }
          const startEvent: StreamEvent = {
            type: 'stream_started',
            mcu_id: msg.mcu_id,
            frame_name: msg.frame_name,
            signals: msg.signals as any,
            active_variables: msg.active_variables,
            protocol_version: msg.protocol_version,
            schema_hash: msg.schema_hash,
          }
          this.streamEventHandlers.forEach(h => h(startEvent))
          console.log(`Stream frame: ${msg.frame_name || 'start_stream'} from ${msg.mcu_id} (${streamInfo.variable_streams.size} variables)`)
        }
        break
      case 'stream_stopped':
        if (msg.mcu_id) {
          const stream = this.activeStreams.get(msg.mcu_id)
          if (stream && msg.signals) {
            for (const varId of (msg.signals as any[])) {
              stream.variable_streams.delete(typeof varId === 'string' ? varId : varId.variable_id)
            }
            if (stream.variable_streams.size === 0) {
              this.activeStreams.delete(msg.mcu_id)
            }
          } else {
            this.activeStreams.delete(msg.mcu_id)
          }
          const stopEvent: StreamEvent = {
            type: 'stream_stopped',
            mcu_id: msg.mcu_id,
            frame_name: msg.frame_name,
          }
          this.streamEventHandlers.forEach(h => h(stopEvent))
          console.log(`Stream frame: ${msg.frame_name || 'stop_stream'} from ${msg.mcu_id}`)
        }
        break
      case 'stream_requested':
        console.log(`Stream request acknowledged: ${(msg as any).status} for ${(msg as any).mcu_id || 'unknown'}`)
        break
      case 'stream_stop_requested':
        console.log(`Stream stop acknowledged: ${(msg as any).status} for ${(msg as any).mcu_id || 'unknown'}`)
        break
      case 'pong':
        break
    }
  }

  private startPing(): void {
    this.stopPing()
    this.pingTimer = setInterval(() => {
      if (this.ws?.readyState === WebSocket.OPEN) {
        this.ws.send(JSON.stringify({ type: 'ping' }))
      }
    }, 15000)
  }

  private stopPing(): void {
    if (this.pingTimer) {
      clearInterval(this.pingTimer)
      this.pingTimer = null
    }
  }

  private scheduleReconnect(): void {
    if (this.reconnectTimer) return
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null
      this.connect()
    }, this.reconnectTimeout)
  }

  disconnect(): void {
    this.stopPing()
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }
    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
  }

  onMessage(handler: MessageHandler): () => void {
    this.messageHandlers.push(handler)
    return () => {
      this.messageHandlers = this.messageHandlers.filter(h => h !== handler)
    }
  }

  onMetadata(handler: MetadataHandler): () => void {
    this.metadataHandlers.push(handler)
    return () => {
      this.metadataHandlers = this.metadataHandlers.filter(h => h !== handler)
    }
  }

  onConnectionChange(handler: ConnectionHandler): () => void {
    this.connectionHandlers.push(handler)
    return () => {
      this.connectionHandlers = this.connectionHandlers.filter(h => h !== handler)
    }
  }

  onParameterUpdate(handler: ParameterHandler): () => void {
    this.parameterHandlers.push(handler)
    return () => {
      this.parameterHandlers = this.parameterHandlers.filter(h => h !== handler)
    }
  }

  getHistory(): SensorReading[] {
    return this.historyBuffer
  }

  getParameterValue(variableId: string): any {
    return this.parameterValues[variableId]
  }

  getMCUs(): MCUInfo[] {
    return this.mcuList
  }

  subscribeToMCUs(mcuIds: string[]): void {
    this.send({ type: 'subscribe', mcu_ids: mcuIds })
  }

  requestHistoryForMCU(mcuId: string, limit: number = 100): void {
    this.send({ type: 'get_history', mcu_id: mcuId, limit })
  }

  /**
   * Request the backend to start streaming specific variables from a device.
   * If not yet connected, queues the request for when the connection opens.
   */
  requestStream(variables: string[], mcuId?: string): void {
    const newVars = variables.filter(v => !this.requestedVariables.has(v))
    if (newVars.length === 0) return
    newVars.forEach(v => this.requestedVariables.add(v))
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.send({ type: 'request_stream', variables: newVars, mcu_id: mcuId })
      console.log(`[start] Stream requested: ${newVars.join(', ')}`)
    } else {
      this.pendingStreamRequests.push({ variables: newVars, mcuId })
      console.log(`[start] Stream queued (not connected): ${newVars.join(', ')}`)
    }
  }

  /**
   * Stop streaming specific variables. Call when a view unmounts.
   */
  stopStream(variables: string[], mcuId?: string): void {
    variables.forEach(v => this.requestedVariables.delete(v))
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.send({ type: 'stop_stream', variables, mcu_id: mcuId })
      console.log(`[stop] Stream stopped: ${variables.join(', ')}`)
    }
  }

  onStreamEvent(handler: StreamEventHandler): () => void {
    this.streamEventHandlers.push(handler)
    return () => {
      this.streamEventHandlers = this.streamEventHandlers.filter(h => h !== handler)
    }
  }

  getActiveStreams(): Map<string, StreamInfo> {
    return this.activeStreams
  }

  isStreamActive(mcuId: string): boolean {
    return this.activeStreams.has(mcuId)
  }

  getVariableStreams(mcuId: string): Map<string, VariableStreamInfo> | undefined {
    return this.activeStreams.get(mcuId)?.variable_streams
  }

  send(message: object): void {
    if (this.ws?.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(message))
    }
  }
}

export const wsService = new WebSocketService()
export default wsService