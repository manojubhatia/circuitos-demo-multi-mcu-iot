/**
 * Protocol Frame Viewer for theSpeaker (3 boards)
 * Real-time view of protocol communication (heartbeat, start/stop stream, data)
 * Includes architecture and message-flow diagrams generated from the protocol model.
 * Connects to backend protocol sniffer via SSE and REST
 */

import { useState, useEffect, useRef, useCallback } from 'react'
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { cn } from '@/lib/utils'
import { Play, Pause, Trash2, Download, Filter, Radio, LayoutDashboard, GitBranch, MessageSquare, Network } from 'lucide-react'
import { Link } from 'react-router-dom'

const API_BASE = import.meta.env.VITE_API_URL || '/api'

interface CapturedFrame {
  id: string
  timestamp: string
  frame_role: string
  frame_name: string
  source: string
  target: string
  mcu_id: string
  direction: string
  variables: string[]
  payload_size: number
  payload_preview: string
  sequence?: number
}

interface ProtocolStats {
  total_captured: number
  by_role: Record<string, number>
  by_source: Record<string, number>
  oldest_timestamp: string | null
  newest_timestamp: string | null
  buffer_capacity: number
  buffer_usage_pct: number
}

const ROLE_COLORS: Record<string, string> = {
  heartbeat: 'bg-blue-500/20 text-blue-400 border-blue-500/30',
  start_stream: 'bg-green-500/20 text-green-400 border-green-500/30',
  stop_stream: 'bg-red-500/20 text-red-400 border-red-500/30',
  data: 'bg-amber-500/20 text-amber-400 border-amber-500/30',
  begin: 'bg-emerald-500/20 text-emerald-400 border-emerald-500/30',
  continue: 'bg-teal-500/20 text-teal-400 border-teal-500/30',
  end: 'bg-lime-500/20 text-lime-400 border-lime-500/30',
  request: 'bg-purple-500/20 text-purple-400 border-purple-500/30',
  response: 'bg-indigo-500/20 text-indigo-400 border-indigo-500/30',
  ack: 'bg-green-600/20 text-green-300 border-green-600/30',
  nack: 'bg-red-600/20 text-red-300 border-red-600/30',
  stream_requested: 'bg-cyan-500/20 text-cyan-400 border-cyan-500/30',
  stream_stopped: 'bg-orange-500/20 text-orange-400 border-orange-500/30',
}

const DIRECTION_ICONS: Record<string, string> = {
  upstream: '↑',
  downstream: '↓',
  frontend_to_backend: '→',
  backend_to_device: '↓',
}

// ═══ Generated Protocol Diagrams ═══
// These Mermaid diagrams are generated at code-gen time from the protocol model.
const SEQUENCE_DIAGRAM = `sequenceDiagram
    autonumber
    participant FE as Frontend
    participant BE as Backend
    participant theSpeaker as theSpeaker
    participant environment as environment<br/>AHT10 Temperature/Humidity Sensor
    participant motor_n_mike as motor_n_mike<br/>INMP441 I2S MEMS Microphone

    note over FE,BE: Telemetry Path (HTTP)
    environment->>BE: HTTP POST /api/receive_data (JSON)
    environment->>BE: HTTP POST /api/receive_data (JSON)
    BE->>BE: decode_auto() / handle_reading()
    BE->>FE: WebSocket sensor_reading

    note over BE: Inter-MCU Wire Protocol (TCP)

    note over BE: Inter-MCU SPI Bus
    environment->>motor_n_mike: SPI data transfer`;
const ARCHITECTURE_DIAGRAM = `graph LR
    subgraph Frontend["Frontend (React)"]
        DASH[Dashboard]
        PV[Protocol Viewer]
        WS_FE[WebSocket Client]
    end
    subgraph Backend["Backend (FastAPI)"]
        HTTP_RX[HTTP Receiver]
        PD[Protocol Decoder]
        CD[Command Dispatcher]
        WS_BE[WebSocket Server]
    end
    subgraph board_0["theSpeaker"]
        TEL_board_0[Telemetry]
        TCP_board_0[Transport]
    end
    subgraph board_1["environment"]
        C_board_1_0["AHT10 Temperature/Humidity Sensor"]
        TEL_board_1[Telemetry]
        TCP_board_1[Transport]
    end
    subgraph board_5["motor_n_mike"]
        C_board_5_0["INMP441 I2S MEMS Microphone"]
        TEL_board_5[Telemetry]
        TCP_board_5[Transport]
    end
    TEL_board_1 -->|HTTP JSON| HTTP_RX
    TEL_board_1 -->|HTTP JSON| HTTP_RX
    HTTP_RX --> PD
    PD --> WS_BE
    WS_BE --> WS_FE
    WS_FE --> DASH
    TCP_board_1 -->|"TCP Wire Protocol"| TCP_board_0
    TCP_board_1 -->|SPI Bus| TCP_board_5
    WS_FE -->|request_stream| WS_BE
    WS_BE --> CD`;
const PROTOCOL_SUMMARY = {
  totalFrames: 3,
  totalTransactions: 0,
  frameStats: {"heartbeat": 3},
  txnStats: {},
  boards: [{"board_type": "esp32-2432s028r", "id": "board_0", "name": "theSpeaker"}, {"board_type": "luatos-esp32-c3-air101-lcd-stack", "id": "board_1", "name": "environment"}, {"board_type": "zephyr-esp32c3-devkitm", "id": "board_5", "name": "motor_n_mike"}],
  tcpHops: [["board_1", "board_0"]],
  httpBoards: ["board_1", "board_1"],
};

type ViewTab = 'live' | 'sequence' | 'architecture';



// Mermaid rendering helper: dynamically imports mermaid and renders SVG
function MermaidDiagram({ chart, className }: { chart: string; className?: string }) {
  const containerRef = useRef<HTMLDivElement>(null)
  const [svg, setSvg] = useState<string>('')
  const [error, setError] = useState<string>('')

  useEffect(() => {
    if (!chart) return
    let cancelled = false
    ;(async () => {
      try {
        const mermaid = (await import('mermaid')).default
        mermaid.initialize({ startOnLoad: false, theme: 'dark', themeVariables: {
          primaryColor: '#06b6d4', primaryTextColor: '#e2e8f0',
          primaryBorderColor: '#0891b2', lineColor: '#94a3b8',
          secondaryColor: '#1e293b', tertiaryColor: '#0f172a',
          noteBkgColor: '#1e293b', noteTextColor: '#e2e8f0', noteBorderColor: '#334155',
        }})
        const { svg: rendered } = await mermaid.render('mermaid-' + Math.random().toString(36).slice(2), chart)
        if (!cancelled) setSvg(rendered)
      } catch (e: any) {
        if (!cancelled) setError(e?.message || 'Failed to render diagram')
      }
    })()
    return () => { cancelled = true }
  }, [chart])

  if (error) return <div className="text-red-400 p-4 text-sm">Diagram error: {error}</div>
  if (!svg) return <div className="flex items-center justify-center h-64 text-muted-foreground">Loading diagram...</div>
  return <div ref={containerRef} className={cn("overflow-auto", className)} dangerouslySetInnerHTML={{ __html: svg }} />
}

// Protocol summary stats card
function ProtocolSummaryCard() {
  const s = PROTOCOL_SUMMARY
  if (s.totalFrames === 0) return null
  return (
    <div className="grid grid-cols-2 md:grid-cols-4 gap-3 mb-4">
      <Card className="p-3">
        <div className="text-xs text-muted-foreground">Protocol Frames</div>
        <div className="text-2xl font-bold">{s.totalFrames}</div>
        <div className="text-xs text-muted-foreground mt-1">
          {Object.entries(s.frameStats).map(([k, v]) => `${k}: ${v}`).join(', ')}
        </div>
      </Card>
      <Card className="p-3">
        <div className="text-xs text-muted-foreground">Transactions</div>
        <div className="text-2xl font-bold">{s.totalTransactions}</div>
        <div className="text-xs text-muted-foreground mt-1">
          {Object.entries(s.txnStats).map(([k, v]) => `${k}: ${v}`).join(', ')}
        </div>
      </Card>
      <Card className="p-3">
        <div className="text-xs text-muted-foreground">Boards</div>
        <div className="text-2xl font-bold">{s.boards.length}</div>
        <div className="text-xs text-muted-foreground mt-1">
          {s.boards.map((b: any) => b.name || b.id).join(', ')}
        </div>
      </Card>
      <Card className="p-3">
        <div className="text-xs text-muted-foreground">Transport Hops</div>
        <div className="text-2xl font-bold">{s.tcpHops.length + s.httpBoards.length}</div>
        <div className="text-xs text-muted-foreground mt-1">
          {s.tcpHops.length} TCP, {s.httpBoards.length} HTTP
        </div>
      </Card>
    </div>
  )
}

// Module-level frame store: persists across component unmount/remount (survives tab navigation)
// Only cleared on full page refresh
let persistedFrames: CapturedFrame[] = []
let persistedSeenIds = new Set<string | number>()

// Transform backend frame format to frontend CapturedFrame interface
function mapFrame(raw: any): CapturedFrame {
  const ps = raw.payload_summary || {}
  const variables = ps.variables || raw.variables || []
  let payloadPreview = raw.payload_preview || ''
  if (!payloadPreview && ps.values) {
    payloadPreview = Object.entries(ps.values)
      .map(([k, v]) => `${k}=${typeof v === 'number' ? (v as number).toFixed(2) : v}`)
      .join(', ')
  } else if (!payloadPreview && Object.keys(ps).length > 0) {
    payloadPreview = JSON.stringify(ps).slice(0, 120)
  }
  return {
    id: raw.id || raw.sequence_id || '',
    timestamp: raw.timestamp || '',
    frame_role: raw.frame_role || '',
    frame_name: raw.frame_name || '',
    source: raw.source || '',
    target: raw.target || '',
    mcu_id: raw.mcu_id || raw.source || '',
    direction: raw.direction || '',
    variables,
    payload_size: raw.payload_size || 0,
    payload_preview: payloadPreview,
    sequence: raw.sequence || raw.id,
  }
}

export default function ProtocolViewer() {
  const [activeTab, setActiveTab] = useState<ViewTab>('live')
  const [frames, setFrames] = useState<CapturedFrame[]>(persistedFrames)
  const [stats, setStats] = useState<ProtocolStats | null>(null)
  const [isLive, setIsLive] = useState(true)
  const [filter, setFilter] = useState('')
  const [roleFilter, setRoleFilter] = useState<string | null>(null)
  const [autoScroll, setAutoScroll] = useState(true)
  const eventSourceRef = useRef<EventSource | null>(null)
  const listRef = useRef<HTMLDivElement>(null)
  const frameCountRef = useRef(0)

  // Sync frames to module-level store for persistence across tab navigations
  useEffect(() => {
    persistedFrames = frames
  }, [frames])

  // Fetch initial frames (merges with any already-persisted frames)
  const fetchFrames = useCallback(async () => {
    try {
      const res = await fetch(`${API_BASE}/protocol/capture?limit=500`)
      if (res.ok) {
        const data = await res.json()
        const rawFrames: CapturedFrame[] = (data.frames || data || []).map(mapFrame)
        setFrames(prev => {
          const merged = [...prev]
          for (const f of rawFrames) {
            if (!persistedSeenIds.has(f.id)) {
              persistedSeenIds.add(f.id)
              merged.push(f)
            }
          }
          if (merged.length > 2000) return merged.slice(-2000)
          return merged
        })
      }
    } catch (e) {
      console.warn('Failed to fetch protocol frames:', e)
    }
  }, [])

  // Fetch stats
  const fetchStats = useCallback(async () => {
    try {
      const res = await fetch(`${API_BASE}/protocol/stats`)
      if (res.ok) {
        const data = await res.json()
        setStats(data)
      }
    } catch (e) {
      console.warn('Failed to fetch protocol stats:', e)
    }
  }, [])

  // SSE connection for live streaming
  useEffect(() => {
    if (!isLive) {
      if (eventSourceRef.current) {
        eventSourceRef.current.close()
        eventSourceRef.current = null
      }
      return
    }

    fetchFrames()
    fetchStats()

    const es = new EventSource(`${API_BASE}/protocol/capture/stream`)
    eventSourceRef.current = es

    es.onmessage = (event) => {
      try {
        const raw = JSON.parse(event.data)
        if (raw.type === 'connected' || raw.type === 'keepalive') return
        const frame = mapFrame(raw)
        if (persistedSeenIds.has(frame.id)) return
        persistedSeenIds.add(frame.id)
        setFrames(prev => {
          const updated = [...prev, frame]
          if (updated.length > 2000) {
            return updated.slice(-2000)
          }
          return updated
        })
        frameCountRef.current++
        if (frameCountRef.current % 20 === 0) {
          fetchStats()
        }
      } catch (e) {
        // ignore parse errors
      }
    }

    es.onerror = () => {
      es.close()
      eventSourceRef.current = null
      // Retry after 3s
      setTimeout(() => {
        if (isLive) {
          fetchFrames()
        }
      }, 3000)
    }

    return () => {
      es.close()
      eventSourceRef.current = null
    }
  }, [isLive, fetchFrames, fetchStats])

  // Auto-scroll
  useEffect(() => {
    if (autoScroll && listRef.current) {
      listRef.current.scrollTop = listRef.current.scrollHeight
    }
  }, [frames, autoScroll])

  // Periodic stats refresh
  useEffect(() => {
    const interval = setInterval(fetchStats, 5000)
    return () => clearInterval(interval)
  }, [fetchStats])

  // Clear buffer (both local persisted and backend)
  const clearFrames = async () => {
    try {
      await fetch(`${API_BASE}/protocol/capture`, { method: 'DELETE' })
      setFrames([])
      persistedFrames = []
      persistedSeenIds = new Set()
      fetchStats()
    } catch (e) {
      console.warn('Failed to clear frames:', e)
    }
  }

  // Export frames as JSON
  const exportFrames = () => {
    const blob = new Blob([JSON.stringify(filteredFrames, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `protocol_capture_${new Date().toISOString().slice(0, 19).replace(/:/g, '-')}.json`
    a.click()
    URL.revokeObjectURL(url)
  }

  // Filter frames
  const filteredFrames = frames.filter(f => {
    if (roleFilter && f.frame_role !== roleFilter) return false
    if (filter) {
      const q = filter.toLowerCase()
      return (
        f.frame_role?.toLowerCase().includes(q) ||
        f.frame_name?.toLowerCase().includes(q) ||
        f.mcu_id?.toLowerCase().includes(q) ||
        f.source?.toLowerCase().includes(q) ||
        f.payload_preview?.toLowerCase().includes(q) ||
        f.variables?.some(v => v.toLowerCase().includes(q))
      )
    }
    return true
  })

  // Available roles for filter buttons
  const availableRoles = [...new Set(frames.map(f => f.frame_role).filter(Boolean))]

  const formatTime = (ts: string) => {
    if (!ts) return ''
    try {
      const d = new Date(ts)
      return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit', fractionalSecondDigits: 3 } as any)
    } catch {
      return ts
    }
  }

  return (
    <div className="min-h-screen bg-background text-foreground p-4">
      {/* Header with Navigation */}
      <header className="sticky top-0 z-50 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60 -mx-4 -mt-4 px-4 mb-4">
        <div className="py-3 flex items-center justify-between">
          <div className="flex items-center gap-6">
            <div>
              <h1 className="text-lg font-bold tracking-tight">Protocol Viewer</h1>
            </div>
            <nav className="flex items-center gap-1">
              <Link to="/" className="flex items-center gap-1.5 px-3 py-1.5 text-sm font-medium rounded-md text-muted-foreground hover:text-foreground hover:bg-muted transition-colors">
                <LayoutDashboard className="h-3.5 w-3.5" />
                Dashboard
              </Link>
              <Link to="/protocol" className="flex items-center gap-1.5 px-3 py-1.5 text-sm font-medium rounded-md bg-primary/10 text-primary">
                <Radio className="h-3.5 w-3.5" />
                Protocol
              </Link>
            </nav>
            {/* View tabs */}
            <div className="flex bg-muted rounded-lg p-0.5">
              <button onClick={() => setActiveTab('live')} className={cn("flex items-center gap-1.5 px-3 py-1 text-xs font-medium rounded-md transition-colors", activeTab === 'live' ? "bg-primary text-primary-foreground" : "text-muted-foreground hover:text-foreground")}>
                <Radio className="h-3 w-3" /> Live Frames
              </button>
              <button onClick={() => setActiveTab('sequence')} className={cn("flex items-center gap-1.5 px-3 py-1 text-xs font-medium rounded-md transition-colors", activeTab === 'sequence' ? "bg-primary text-primary-foreground" : "text-muted-foreground hover:text-foreground")}>
                <MessageSquare className="h-3 w-3" /> Message Flow
              </button>
              <button onClick={() => setActiveTab('architecture')} className={cn("flex items-center gap-1.5 px-3 py-1 text-xs font-medium rounded-md transition-colors", activeTab === 'architecture' ? "bg-primary text-primary-foreground" : "text-muted-foreground hover:text-foreground")}>
                <Network className="h-3 w-3" /> Architecture
              </button>
            </div>
          </div>
          <div className="flex items-center gap-2">
            {activeTab === 'live' && (<>
            <Button
              size="sm"
              variant={isLive ? "default" : "outline"}
              onClick={() => setIsLive(!isLive)}
              className="gap-1"
            >
              {isLive ? <Pause className="h-3.5 w-3.5" /> : <Play className="h-3.5 w-3.5" />}
              {isLive ? 'Pause' : 'Live'}
            </Button>
            <Button size="sm" variant="outline" onClick={exportFrames} className="gap-1">
              <Download className="h-3.5 w-3.5" /> Export
            </Button>
            <Button size="sm" variant="outline" onClick={clearFrames} className="gap-1 text-destructive">
              <Trash2 className="h-3.5 w-3.5" /> Clear
            </Button>
            </>)}
          </div>
        </div>
      </header>

      {/* Diagram views */}
      {activeTab === 'sequence' && (
        <div className="mb-4">
          <ProtocolSummaryCard />
          <Card>
            <CardHeader className="pb-2">
              <CardTitle className="text-base">Message Flow Sequence</CardTitle>
              <CardDescription>Shows how data flows between components at runtime — generated from the protocol model.</CardDescription>
            </CardHeader>
            <CardContent>
              {SEQUENCE_DIAGRAM ? (
                <MermaidDiagram chart={SEQUENCE_DIAGRAM} className="min-h-[400px]" />
              ) : (
                <div className="text-muted-foreground text-sm p-8 text-center">No sequence diagram available. Regenerate the project to generate diagrams.</div>
              )}
            </CardContent>
          </Card>
        </div>
      )}

      {activeTab === 'architecture' && (
        <div className="mb-4">
          <ProtocolSummaryCard />
          <Card>
            <CardHeader className="pb-2">
              <CardTitle className="text-base">Architecture Flow</CardTitle>
              <CardDescription>Shows the component architecture, transport layers, and protocol stack — generated from the circuit design.</CardDescription>
            </CardHeader>
            <CardContent>
              {ARCHITECTURE_DIAGRAM ? (
                <MermaidDiagram chart={ARCHITECTURE_DIAGRAM} className="min-h-[400px]" />
              ) : (
                <div className="text-muted-foreground text-sm p-8 text-center">No architecture diagram available. Regenerate the project to generate diagrams.</div>
              )}
            </CardContent>
          </Card>
        </div>
      )}

      {/* Live frames view */}
      {activeTab === 'live' && stats && (
        <div className="grid grid-cols-2 md:grid-cols-5 gap-2 mb-4">
          <Card className="p-2">
            <div className="text-xs text-muted-foreground">Total Captured</div>
            <div className="text-lg font-bold">{stats.total_captured}</div>
          </Card>
          <Card className="p-2">
            <div className="text-xs text-muted-foreground">Buffer Usage</div>
            <div className="text-lg font-bold">{stats.buffer_usage_pct?.toFixed(0)}%</div>
          </Card>
          {Object.entries(stats.by_role || {}).slice(0, 3).map(([role, count]) => (
            <Card key={role} className="p-2">
              <div className="text-xs text-muted-foreground capitalize">{role.replace('_', ' ')}</div>
              <div className="text-lg font-bold">{count}</div>
            </Card>
          ))}
        </div>
      )}

      {/* Filters — only on live tab */}
      {activeTab === 'live' && <div className="flex items-center gap-2 mb-3 flex-wrap">
        <div className="relative flex-1 max-w-xs">
          <Filter className="absolute left-2 top-1/2 -translate-y-1/2 h-3.5 w-3.5 text-muted-foreground" />
          <Input
            placeholder="Filter frames..."
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
            className="h-8 pl-7 text-sm"
          />
        </div>

        <div className="flex gap-1 flex-wrap">
          <Button
            size="sm"
            variant={roleFilter === null ? "default" : "outline"}
            onClick={() => setRoleFilter(null)}
            className="h-6 px-2 text-xs"
          >
            All
          </Button>
          {availableRoles.map(role => (
            <Button
              key={role}
              size="sm"
              variant={roleFilter === role ? "default" : "outline"}
              onClick={() => setRoleFilter(roleFilter === role ? null : role)}
              className="h-6 px-2 text-xs"
            >
              {role.replace('_', ' ')}
            </Button>
          ))}
        </div>

        <span className="text-xs text-muted-foreground ml-auto">
          {filteredFrames.length} frame{filteredFrames.length !== 1 ? 's' : ''}
        </span>
      </div>}

      {/* Frame list */}
      {activeTab === 'live' && <Card>
        <div
          ref={listRef}
          className="h-[calc(100vh-320px)] overflow-y-auto font-mono text-xs"
          onScroll={(e) => {
            const el = e.currentTarget
            const atBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 50
            setAutoScroll(atBottom)
          }}
        >
          {filteredFrames.length === 0 ? (
            <div className="flex items-center justify-center h-32 text-muted-foreground">
              {isLive ? 'Waiting for protocol frames...' : 'No frames captured. Click Live to start.'}
            </div>
          ) : (
            <table className="w-full">
              <thead className="sticky top-0 bg-card border-b">
                <tr className="text-left text-muted-foreground">
                  <th className="px-2 py-1.5 w-24">Time</th>
                  <th className="px-2 py-1.5 w-6">Dir</th>
                  <th className="px-2 py-1.5 w-28">Role</th>
                  <th className="px-2 py-1.5 w-32">Frame</th>
                  <th className="px-2 py-1.5 w-24">Source</th>
                  <th className="px-2 py-1.5">Variables / Payload</th>
                </tr>
              </thead>
              <tbody>
                {filteredFrames.map((frame, idx) => (
                  <tr
                    key={frame.id || idx}
                    className={cn(
                      "border-b border-border/30 hover:bg-muted/30 transition-colors",
                      frame.frame_role === 'heartbeat' && "opacity-60"
                    )}
                  >
                    <td className="px-2 py-1 text-muted-foreground whitespace-nowrap">
                      {formatTime(frame.timestamp)}
                    </td>
                    <td className="px-2 py-1 text-center">
                      {DIRECTION_ICONS[frame.direction] || '•'}
                    </td>
                    <td className="px-2 py-1">
                      <Badge
                        variant="outline"
                        className={cn(
                          "text-[10px] px-1.5 py-0",
                          ROLE_COLORS[frame.frame_role] || 'bg-gray-500/20 text-gray-400'
                        )}
                      >
                        {frame.frame_role}
                      </Badge>
                    </td>
                    <td className="px-2 py-1 truncate max-w-[200px]" title={frame.frame_name}>
                      {frame.frame_name || '-'}
                    </td>
                    <td className="px-2 py-1 text-muted-foreground truncate">
                      {frame.mcu_id || frame.source || '-'}
                    </td>
                    <td className="px-2 py-1 text-muted-foreground truncate max-w-[400px]" title={frame.payload_preview || frame.variables?.join(', ') || ''}>
                      {frame.payload_preview 
                        ? frame.payload_preview
                        : frame.variables?.length > 0
                          ? frame.variables.join(', ')
                          : '-'
                      }
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          )}
        </div>
      </Card>}

      {/* Auto-scroll indicator */}
      {activeTab === 'live' && !autoScroll && filteredFrames.length > 0 && (
        <div className="fixed bottom-4 right-4">
          <Button
            size="sm"
            onClick={() => {
              setAutoScroll(true)
              if (listRef.current) {
                listRef.current.scrollTop = listRef.current.scrollHeight
              }
            }}
          >
            ↓ Scroll to latest
          </Button>
        </div>
      )}
    </div>
  )
}
