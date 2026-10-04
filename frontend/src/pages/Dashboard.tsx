/**
 * Dashboard Page for theSpeaker (3 boards)
 * Generated from dataflow configuration and Frontend UI design
 */

import React, { useState, useEffect, useCallback, useMemo, useRef, useSyncExternalStore } from 'react'
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Progress } from '@/components/ui/progress'
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'
import { Button } from '@/components/ui/button'
import { Switch } from '@/components/ui/switch'
import { Slider } from '@/components/ui/slider'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { cn, formatValue, formatTimestamp } from '@/lib/utils'
import { wsService } from '@/services/websocket'
import { api } from '@/services/api'
import type { SensorReading, SignalMetadata } from '@/services/api'
import Plot from 'react-plotly.js'
import GridLayout from 'react-grid-layout'
import 'react-grid-layout/css/styles.css'
import 'react-resizable/css/styles.css'
import { Play, Pause, Settings2, Move, Plus, Trash2, Radio, LayoutDashboard, ChevronDown, Check } from 'lucide-react'
import { Link } from 'react-router-dom'
import { LineChart, Line, AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, ReferenceLine } from 'recharts'

// Mobile breakpoint detection
const mobileQuery = typeof window !== 'undefined' ? window.matchMedia('(max-width: 768px)') : null
function useMobile() {
  return useSyncExternalStore(
    (cb) => { mobileQuery?.addEventListener('change', cb); return () => mobileQuery?.removeEventListener('change', cb) },
    () => mobileQuery?.matches ?? false,
    () => false,
  )
}

// Signal metadata from architecture
const SIGNALS: SignalMetadata[] = [
  {
    name: 'environment_aht10_temperature',
    type: 'float',
    unit: '',
    source: 'board_1',
    display_name: 'Environment Aht10 Temperature',
    direction: 'output',
  },
  {
    name: 'environment_aht10_humidity',
    type: 'float',
    unit: '',
    source: 'board_1',
    display_name: 'Environment Aht10 Humidity',
    direction: 'output',
  },
  {
    name: 'motor_n_mike_inmp441_audio_samples',
    type: 'buffer',
    unit: '',
    source: 'board_5',
    display_name: 'Inmp441 Audio Samples',
    direction: 'output',
  },
  {
    name: 'motor_n_mike_inmp441_audio_level',
    type: 'int',
    unit: '',
    source: 'board_5',
    display_name: 'Inmp441 Audio Level',
    direction: 'output',
  },
  {
    name: 'motor_n_mike_inmp441_peak_amplitude',
    type: 'int',
    unit: '',
    source: 'board_5',
    display_name: 'Inmp441 Peak Amplitude',
    direction: 'output',
  },
  {
    name: 'motor_n_mike_inmp441_rms_level',
    type: 'float',
    unit: '',
    source: 'board_5',
    display_name: 'Inmp441 Rms Level',
    direction: 'output',
  },
  {
    name: 'motor_n_mike_inmp441_is_clipping',
    type: 'bool',
    unit: '',
    source: 'board_5',
    display_name: 'Inmp441 Clipping',
    direction: 'output',
  },
  {
    name: 'thespeaker_bluetoothspeaker_connected',
    type: 'bool',
    unit: '',
    source: 'board_0',
    display_name: 'Thespeaker Bluetoothspeaker Connected',
    direction: 'output',
  },
  {
    name: 'thespeaker_bluetoothspeaker_device_name',
    type: 'string',
    unit: '',
    source: 'board_0',
    display_name: 'Bluetoothspeaker Name',
    direction: 'output',
  },
  {
    name: 'thespeaker_bluetoothspeaker_rssi',
    type: 'int',
    unit: '',
    source: 'board_0',
    display_name: 'Thespeaker Bluetoothspeaker Rssi',
    direction: 'output',
  },
]

// Bidirectional IO signals (can be simulated from frontend)
const BIDIRECTIONAL_SIGNALS = SIGNALS.filter(s => s.direction === 'bidirectional')

// Chart colors
const CHART_COLORS = ['#ef4444', '#22c55e', '#3b82f6', '#f59e0b', '#8b5cf6', '#ec4899', '#14b8a6', '#f97316']



interface HistoryEntry extends SensorReading {
  timestamp: string
}

// Trigger types for oscilloscope-like functionality
type TriggerEdge = 'rising' | 'falling' | 'both' | 'none'
interface TriggerConfig {
  enabled: boolean
  signal: string
  edge: TriggerEdge
  level: number
  armed: boolean
}

const DASHBOARD_PLOT_CONFIG = { displayModeBar: false, responsive: true }

function toISOTimestamp(ts: any): string {
  if (!ts) return new Date().toISOString()
  if (typeof ts === 'number') return new Date(ts > 1e12 ? ts : ts * 1000).toISOString()
  const d = new Date(ts)
  return isNaN(d.getTime()) ? new Date().toISOString() : d.toISOString()
}

const MemoizedPlot = React.memo(
  ({ data, layout, config, revision, style, useResizeHandler }: {
    data: any[]; layout: any; config: any; revision: number;
    style?: React.CSSProperties; useResizeHandler?: boolean;
  }) => (
    <Plot
      data={data}
      layout={layout}
      config={config}
      style={style}
      useResizeHandler={useResizeHandler}
      revision={revision}
    />
  ),
  (prev, next) => prev.revision === next.revision
)

// PID Controller Visualization Widget
interface PIDVizProps {
  label: string
  processField: string
  outputField: string
  setpointField?: string
  setpointValue?: number
  kp?: number; ki?: number; kd?: number
  interactive?: boolean
  processLabel?: string
  outputLabel?: string
  history: any[]
  onSetpointChange?: (v: number) => void
  onGainsChange?: (g: { kp: number; ki: number; kd: number }) => void
}

function PIDVisualizationWidget({
  label, processField, outputField, setpointField, setpointValue = 0,
  kp: kpInit = 1, ki: kiInit = 0, kd: kdInit = 0,
  interactive = true, processLabel, outputLabel, history: histData,
  onSetpointChange, onGainsChange,
}: PIDVizProps) {
  // Derive human-readable names from field keys or explicit labels
  const pvName = processLabel || (processField ? processField.replace(/_/g, ' ').replace(/\b\w/g, (c: string) => c.toUpperCase()) : 'Process (PV)')
  const cvName = outputLabel || (outputField ? outputField.replace(/_/g, ' ').replace(/\b\w/g, (c: string) => c.toUpperCase()) : 'Output (CV)')
  const containerRef = useRef<HTMLDivElement>(null)
  const [dims, setDims] = useState({ w: 400, h: 300 })
  const [sp, setSp] = useState(setpointValue)
  const [kp, setKp] = useState(kpInit)
  const [ki, setKi] = useState(kiInit)
  const [kd, setKd] = useState(kdInit)
  const [spInitialized, setSpInitialized] = useState(false)

  // Sync setpoint from live telemetry on first data arrival (so refresh doesn't reset to 0)
  useEffect(() => {
    if (spInitialized || !histData.length) return
    if (setpointField && histData.length > 0) {
      const latestSp = histData[histData.length - 1]?.[setpointField]
      if (latestSp !== undefined && latestSp !== null && latestSp !== 0) {
        setSp(latestSp)
        setSpInitialized(true)
      }
    }
  }, [histData, setpointField, spInitialized])

  useEffect(() => {
    if (!containerRef.current) return
    const ro = new ResizeObserver(entries => {
      for (const e of entries) setDims({ w: e.contentRect.width, h: e.contentRect.height })
    })
    ro.observe(containerRef.current)
    return () => ro.disconnect()
  }, [])

  const handleSPBlur = () => onSetpointChange?.(sp)
  const handleGainsBlur = () => onGainsChange?.({ kp, ki, kd })
  const inputCls = "w-16 bg-background border border-border rounded px-1 py-0.5 text-[10px] text-foreground text-center focus:outline-none focus:ring-1 focus:ring-primary"

  const times = histData.map((_: any, i: number) => i)
  const pvData = processField ? histData.map((h: any) => h[processField] ?? null) : []
  const cvData = outputField ? histData.map((h: any) => h[outputField] ?? null) : []

  const traces: any[] = useMemo(() => {
    const t: any[] = []
    // Setpoint trace (dashed red)
    if (setpointField) {
      t.push({
        x: times, y: histData.map((h: any) => h[setpointField] ?? sp),
        name: 'Setpoint', type: 'scatter', mode: 'lines',
        line: { color: '#ef4444', width: 2, dash: 'dash' },
      })
    } else {
      t.push({
        x: [0, Math.max(times.length - 1, 1)], y: [sp, sp],
        name: 'Setpoint', type: 'scatter', mode: 'lines',
        line: { color: '#ef4444', width: 2, dash: 'dash' },
      })
    }
    // Process variable (green)
    if (processField) {
      t.push({
        x: times, y: pvData, name: pvName, type: 'scatter', mode: 'lines',
        line: { color: '#22c55e', width: 2 }, fill: 'tozeroy', fillcolor: 'rgba(34,197,94,0.08)',
      })
    }
    // Output / CV (blue, secondary y-axis)
    if (outputField) {
      t.push({
        x: times, y: cvData, name: cvName, type: 'scatter', mode: 'lines',
        line: { color: '#3b82f6', width: 1.5 }, yaxis: 'y2',
      })
    }
    return t
  }, [histData, sp, processField, outputField, setpointField, pvData, cvData, times])

  return (
    <div className="flex h-full flex-col p-1 bg-card rounded-lg border overflow-hidden">
      <div className="flex items-center justify-between px-1 shrink-0">
        <div className="text-xs font-medium uppercase tracking-[0.12em] text-muted-foreground">{label}</div>
        {!interactive && (
          <div className="flex items-center gap-2 text-[9px] text-muted-foreground/70">
            <span>Kp={kp}</span> <span>Ki={ki}</span> <span>Kd={kd}</span>
          </div>
        )}
      </div>
      {interactive && (
        <div className="flex items-center gap-2 flex-wrap px-1 shrink-0">
          <label className="flex items-center gap-1 text-[10px] text-muted-foreground">
            <span className="inline-block w-2 h-2 rounded-full bg-red-500" />SP
            <input type="number" step="any" value={sp}
              onChange={e => setSp(parseFloat(e.target.value) || 0)}
              onBlur={handleSPBlur} onKeyDown={e => e.key === 'Enter' && handleSPBlur()}
              className={inputCls} />
          </label>
          <label className="flex items-center gap-1 text-[10px] text-muted-foreground">
            Kp <input type="number" step="0.01" value={kp}
              onChange={e => setKp(parseFloat(e.target.value) || 0)}
              onBlur={handleGainsBlur} onKeyDown={e => e.key === 'Enter' && handleGainsBlur()}
              className={inputCls} />
          </label>
          <label className="flex items-center gap-1 text-[10px] text-muted-foreground">
            Ki <input type="number" step="0.01" value={ki}
              onChange={e => setKi(parseFloat(e.target.value) || 0)}
              onBlur={handleGainsBlur} onKeyDown={e => e.key === 'Enter' && handleGainsBlur()}
              className={inputCls} />
          </label>
          <label className="flex items-center gap-1 text-[10px] text-muted-foreground">
            Kd <input type="number" step="0.01" value={kd}
              onChange={e => setKd(parseFloat(e.target.value) || 0)}
              onBlur={handleGainsBlur} onKeyDown={e => e.key === 'Enter' && handleGainsBlur()}
              className={inputCls} />
          </label>
        </div>
      )}
      <div ref={containerRef} className="flex-1 min-h-0 overflow-hidden">
        <Plot
          data={traces}
          layout={{
            autosize: true,
            paper_bgcolor: 'transparent', plot_bgcolor: 'transparent',
            font: { color: '#94a3b8', size: 10 },
            margin: { t: 5, r: 45, b: 25, l: 45 },
            showlegend: true,
            legend: {
              orientation: 'h' as const, x: 0.5, xanchor: 'center' as const,
              y: 1.02, yanchor: 'bottom' as const,
              font: { size: 9, color: '#94a3b8' }, bgcolor: 'transparent',
            },
            xaxis: { showticklabels: false, showgrid: true, gridcolor: 'rgba(148,163,184,0.1)' },
            yaxis: { title: { text: pvName, font: { size: 9 } }, showgrid: true, gridcolor: 'rgba(148,163,184,0.1)', tickfont: { size: 9 } },
            yaxis2: {
              overlaying: 'y', side: 'right',
              title: { text: cvName, font: { size: 9, color: '#3b82f6' } },
              gridcolor: 'rgba(59,130,246,0.1)', tickfont: { color: '#3b82f6', size: 9 }, showgrid: false,
            },
            hovermode: 'x unified' as any,
            datarevision: histData.length,
          }}
          config={{ displayModeBar: true, displaylogo: false, modeBarButtonsToRemove: ['toImage', 'sendDataToCloud'] as any }}
          style={{ width: '100%', height: '100%' }}
          useResizeHandler
        />
      </div>
    </div>
  )
}

export default function Dashboard() {

  const isMobile = useMobile()
  const [connected, setConnected] = useState(false)
  const [latestData, setLatestData] = useState<SensorReading | null>(null)
  const [history, setHistory] = useState<HistoryEntry[]>([])
  const [selectedSignals, setSelectedSignals] = useState<string[]>(SIGNALS.map(s => s.name))
  const [isDropdownOpen, setIsDropdownOpen] = useState(false)
  
  // Oscilloscope trigger state
  const [trigger, setTrigger] = useState<TriggerConfig>({
    enabled: false,
    signal: SIGNALS[0]?.name || '',
    edge: 'rising',
    level: 50,
    armed: false,
  })
  const [isCapturing, setIsCapturing] = useState(true)
  const [capturedHistory, setCapturedHistory] = useState<HistoryEntry[]>([])
  const [triggerIndex, setTriggerIndex] = useState<number | null>(null)
  
  // Cumulative frame: merges partial MCU readings into one flat frame
  const cumulativeFrameRef = useRef<Record<string, any>>({})
  
  // Charts tab - Design/Run mode and play/pause
  const [chartsMode, setChartsMode] = useState<'run' | 'design'>('run')
  const [chartsPaused, setChartsPaused] = useState(false)
  const [pausedHistory, setPausedHistory] = useState<HistoryEntry[]>([])
  const pausedRevisionRef = useRef<number>(0)
  const chartsContainerRef = useRef<HTMLDivElement>(null)
  const [chartsContainerWidth, setChartsContainerWidth] = useState(800)
  
  // Grid layout for charts in design mode (persisted to localStorage)
  interface ChartLayout { i: string; x: number; y: number; w: number; h: number; signalName?: string; signalNames?: string[] }
  const CHART_STORAGE_KEY = 'chart-layouts-20260929_135652'

  const [chartLayouts, setChartLayouts] = useState<ChartLayout[]>(() => {
    try {
      const saved = localStorage.getItem(CHART_STORAGE_KEY)
      if (saved) {
        const parsed = JSON.parse(saved)
        if (Array.isArray(parsed) && parsed.length > 0) return parsed
      }
    } catch { /* ignore parse errors */ }
    // Default: charts placed side-by-side (6 cols each = 2 per row)
    return SIGNALS.map((s, idx) => ({
      i: `chart-${s.name}`,
      x: (idx % 2) * 6,
      y: Math.floor(idx / 2) * 4,
      w: 6,
      h: 4,
      signalName: s.name,
      signalNames: [s.name],
    }))
  })

  // Persist chart layouts to localStorage whenever they change
  useEffect(() => {
    try {
      localStorage.setItem(CHART_STORAGE_KEY, JSON.stringify(chartLayouts))
    } catch { /* quota exceeded or private browsing */ }
  }, [chartLayouts])

  // Bidirectional IO simulation state (with pending flag for ACK-based flow)
  const [simulatedInputs, setSimulatedInputs] = useState<Record<string, { active: boolean; value: boolean | number; pending?: boolean }>>({})
  
  // Listen for set_ack / parameter updates from WebSocket to confirm state changes
  useEffect(() => {
    const unsub = wsService.onParameterUpdate((variableId: string, value: any, success?: boolean) => {
      if (success) {
        setSimulatedInputs(prev => {
          const current = prev[variableId]
          if (current?.pending || current?.active) {
            return {
              ...prev,
              [variableId]: { active: true, value: value !== 0 && value !== false, pending: false }
            }
          }
          return prev
        })
      }
    })
    return unsub
  }, [])
  
  // Handler to send a set_output command to the device (ACK-based)
  const handleSimulate = async (signalName: string, value: boolean | number) => {
    try {
      // Set pending state (optimistic UI with loading indicator)
      setSimulatedInputs(prev => ({
        ...prev,
        [signalName]: { active: true, value, pending: true }
      }))
      
      await api.simulateSignal(signalName, value, true)
      
      // Set a timeout: if no ACK received within 5s, confirm optimistically
      setTimeout(() => {
        setSimulatedInputs(prev => {
          const current = prev[signalName]
          if (current?.pending) {
            return { ...prev, [signalName]: { ...current, pending: false } }
          }
          return prev
        })
      }, 5000)
    } catch (error) {
      console.error('Failed to send command:', error)
      // Revert on error
      setSimulatedInputs(prev => ({
        ...prev,
        [signalName]: { active: false, value: false, pending: false }
      }))
    }
  }
  
  // Handler to clear simulation (return to real hardware input)
  const handleClearSimulation = async (signalName: string) => {
    try {
      await api.clearSimulation(signalName)
      setSimulatedInputs(prev => ({
        ...prev,
        [signalName]: { active: false, value: prev[signalName]?.value || false }
      }))
    } catch (error) {
      console.error('Failed to clear simulation:', error)
    }
  }

  // Connect to WebSocket on mount - only run once
  useEffect(() => {
    wsService.connect()
    const unsubConnection = wsService.onConnectionChange((isConnected) => {
      setConnected(isConnected)
      if (isConnected) {
        setTimeout(() => {
          const hist = wsService.getHistory()
          if (hist.length > 0 && history.length === 0) {
            setHistory(hist.map(r => ({ ...r, timestamp: r.timestamp || new Date().toISOString() } as HistoryEntry)))
            setLatestData(hist[hist.length - 1])
          }
        }, 500)
      }
    })

    // Fetch history from REST API as fallback for chart display
    api.getHistoricalData(100).then(resp => {
      const data = resp?.data || resp
      if (Array.isArray(data) && data.length > 0 && history.length === 0) {
        setHistory(data.map((r: any) => ({ ...r, timestamp: r.timestamp || new Date().toISOString() } as HistoryEntry)))
        setLatestData(data[data.length - 1])
      }
    }).catch(() => {})

    // Start streaming variables needed by this dashboard (demand-driven)
    const dashboardVariables = [
      'board_1::environment_aht10_temperature',
      'board_1::environment_aht10_humidity',
      'board_5::motor_n_mike_inmp441_audio_samples',
      'board_5::motor_n_mike_inmp441_audio_level',
      'board_5::motor_n_mike_inmp441_peak_amplitude',
      'board_5::motor_n_mike_inmp441_rms_level',
      'board_5::motor_n_mike_inmp441_is_clipping',
      'board_0::thespeaker_bluetoothspeaker_connected',
      'board_0::thespeaker_bluetoothspeaker_device_name',
      'board_0::thespeaker_bluetoothspeaker_rssi',
    ]
    wsService.requestStream(dashboardVariables)


    return () => {
      unsubConnection()
      wsService.stopStream(dashboardVariables)

    }
  }, []) // eslint-disable-line react-hooks/exhaustive-deps


  // Handle incoming WebSocket messages separately
  useEffect(() => {
    const signalNames = new Set(SIGNALS.map(s => s.name))
    const metadataKeys = new Set(['timestamp', 'mcu_id', 'frame_id', 'frame_name'])
    const acceptAll = signalNames.size === 0

    const unsubMessage = wsService.onMessage((data) => {
      // When SIGNALS is empty, accept all data keys (open-ended discovery)
      // When SIGNALS is populated, filter to only known signal keys
      const hasSignalData = acceptAll
        ? Object.entries(data).some(([key, val]) => !metadataKeys.has(key) && val !== null && val !== undefined)
        : Object.entries(data).some(([key, val]) => !metadataKeys.has(key) && signalNames.has(key) && val !== null && val !== undefined)
      if (!hasSignalData) return

      // Merge incoming partial data into cumulative frame
      // Each MCU sends only its signals; merge preserves other MCU values
      const frame = cumulativeFrameRef.current
      for (const [key, val] of Object.entries(data)) {
        if ((acceptAll || signalNames.has(key)) && !metadataKeys.has(key) && val !== null && val !== undefined) {
          frame[key] = val
        }
      }
      frame.timestamp = data.timestamp || new Date().toISOString()
      if (data.mcu_id) frame.mcu_id = data.mcu_id

      const mergedData = { ...frame } as SensorReading
      setLatestData(mergedData)
      
      if (isCapturing) {
        setHistory(prev => {
          const updated = [...prev, mergedData as HistoryEntry]
          
          // Check for trigger condition using prev state
          if (trigger.enabled && trigger.armed && trigger.signal) {
            const currentValue = (data as Record<string, number>)[trigger.signal]
            const prevEntry = prev[prev.length - 1]
            const prevValue = prevEntry ? (prevEntry as Record<string, number>)[trigger.signal] : currentValue
            
            let triggered = false
            if (trigger.edge === 'rising' && prevValue < trigger.level && currentValue >= trigger.level) {
              triggered = true
            } else if (trigger.edge === 'falling' && prevValue > trigger.level && currentValue <= trigger.level) {
              triggered = true
            } else if (trigger.edge === 'both' && 
              ((prevValue < trigger.level && currentValue >= trigger.level) ||
               (prevValue > trigger.level && currentValue <= trigger.level))) {
              triggered = true
            }
            
            if (triggered) {
              setTriggerIndex(prev.length)
              setCapturedHistory([...prev, data as HistoryEntry])
              setIsCapturing(false)
              setTrigger(t => ({ ...t, armed: false }))
            }
          }
          
          return updated.slice(-1200) // Keep last 1200 entries (supports 1000 chart display)
        })
      }
    })

    return () => {
      unsubMessage()
    }
  }, [isCapturing, trigger])

  const toggleSignal = useCallback((signalName: string) => {
    setSelectedSignals(prev =>
      prev.includes(signalName)
        ? prev.filter(s => s !== signalName)
        : [...prev, signalName]
    )
  }, [])

  const getSignalValue = (signal: SignalMetadata) => {
    if (!latestData) return '--'
    const value = latestData[signal.name as keyof SensorReading]
    return formatValue(value, signal.type, signal.unit)
  }
  
  // Arm the trigger
  const armTrigger = useCallback(() => {
    setTrigger(t => ({ ...t, armed: true }))
    setIsCapturing(true)
    setTriggerIndex(null)
  }, [])
  
  // Single capture (one-shot)
  const singleCapture = useCallback(() => {
    setTrigger(t => ({ ...t, enabled: true, armed: true }))
    setIsCapturing(true)
    setTriggerIndex(null)
  }, [])
  
  // Resume continuous capture
  const runContinuous = useCallback(() => {
    setTrigger(t => ({ ...t, enabled: false, armed: false }))
    setIsCapturing(true)
    setTriggerIndex(null)
  }, [])
  
  // Display data based on trigger state
  const displayHistory = useMemo(() => {
    if (trigger.enabled && !isCapturing && capturedHistory.length > 0) {
      return capturedHistory
    }
    return history
  }, [trigger.enabled, isCapturing, capturedHistory, history])
  
  // Charts pause handling - freeze data when paused
  // Only react to chartsPaused toggle, not to every history update
  useEffect(() => {
    if (chartsPaused) {
      if (pausedHistory.length === 0 && history.length > 0) {
        setPausedHistory([...history])
        pausedRevisionRef.current = history.length
      }
    } else {
      setPausedHistory([])
      pausedRevisionRef.current = 0
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [chartsPaused])
  
  const chartsDisplayHistory = chartsPaused ? pausedHistory : displayHistory
  const chartsRevision = chartsPaused ? pausedRevisionRef.current : displayHistory.length
  
  // Measure charts container width for react-grid-layout
  useEffect(() => {
    if (!chartsContainerRef.current) return
    const resizeObserver = new ResizeObserver((entries) => {
      for (const entry of entries) {
        setChartsContainerWidth(entry.contentRect.width)
      }
    })
    resizeObserver.observe(chartsContainerRef.current)
    return () => resizeObserver.disconnect()
  }, [])
  
  // Grid layout change handler
  const handleChartLayoutChange = useCallback((newLayout: GridLayout.Layout[]) => {
    setChartLayouts(prev => prev.map(item => {
      const updated = newLayout.find(l => l.i === item.i)
      return updated ? { ...item, x: updated.x, y: updated.y, w: updated.w, h: updated.h } : item
    }))
  }, [])
  
  // Add new chart to grid
  const addChart = useCallback(() => {
    const maxY = Math.max(...chartLayouts.map(l => l.y + l.h), 0)
    const newChart: ChartLayout = {
      i: `chart-${Date.now()}`,
      x: 0,
      y: maxY,
      w: 6,
      h: 4,
      signalName: SIGNALS[0]?.name,
      signalNames: [SIGNALS[0]?.name || ''],
    }
    setChartLayouts(prev => [...prev, newChart])
  }, [chartLayouts])
  
  // Remove chart from grid
  const removeChart = useCallback((chartId: string) => {
    setChartLayouts(prev => prev.filter(l => l.i !== chartId))
  }, [])
  
  // Update chart signal assignment (single select - keeps backward compat)
  const updateChartSignal = useCallback((chartId: string, signalName: string) => {
    setChartLayouts(prev => prev.map(l => 
      l.i === chartId ? { ...l, signalName, signalNames: [signalName] } : l
    ))
  }, [])
  
  // Toggle a signal on/off for a chart (multi-select)
  const toggleChartSignal = useCallback((chartId: string, signalName: string) => {
    setChartLayouts(prev => prev.map(l => {
      if (l.i !== chartId) return l
      const current = l.signalNames || [l.signalName || SIGNALS[0]?.name || '']
      const isActive = current.includes(signalName)
      const updated = isActive 
        ? current.filter(n => n !== signalName) 
        : [...current, signalName]
      if (updated.length === 0) return l
      return { ...l, signalNames: updated, signalName: updated[0] }
    }))
  }, [])

  // Multi-select dropdown state for chart signal selection
  const [openSignalDropdown, setOpenSignalDropdown] = useState<string | null>(null)
  
  // Close dropdown on outside click
  useEffect(() => {
    if (!openSignalDropdown) return
    const handleClick = () => setOpenSignalDropdown(null)
    const timeout = setTimeout(() => document.addEventListener('click', handleClick), 0)
    return () => { clearTimeout(timeout); document.removeEventListener('click', handleClick) }
  }, [openSignalDropdown])

  return (
    <div className="min-h-screen bg-background">
      {/* Header with Navigation */}
      <header className="sticky top-0 z-50 border-b bg-background/95 backdrop-blur supports-[backdrop-filter]:bg-background/60">
        <div className="container mx-auto px-3 sm:px-6 py-2 sm:py-3 flex items-center justify-between gap-2">
          <div className="flex items-center gap-2 sm:gap-6 min-w-0">
            <div className="hidden sm:block">
              <h1 className="text-lg font-bold tracking-tight">theSpeaker (3 boards)</h1>

            </div>
            <nav className="flex items-center gap-1">
              <Link to="/" className="flex items-center gap-1.5 px-2 sm:px-3 py-1.5 text-sm font-medium rounded-md bg-primary/10 text-primary">
                <LayoutDashboard className="h-3.5 w-3.5" />
                <span className="hidden sm:inline">Dashboard</span>
              </Link>
              <Link to="/protocol" className="flex items-center gap-1.5 px-2 sm:px-3 py-1.5 text-sm font-medium rounded-md text-muted-foreground hover:text-foreground hover:bg-muted transition-colors">
                <Radio className="h-3.5 w-3.5" />
                <span className="hidden sm:inline">Protocol</span>
              </Link>
            </nav>
          </div>
          <div className="flex items-center gap-2 sm:gap-4 shrink-0">
            <Badge variant={connected ? 'default' : 'destructive'} className={cn("text-xs", connected ? 'bg-green-500' : '')}>
              <span className={cn("w-2 h-2 rounded-full mr-1.5", connected ? "bg-green-300 animate-pulse" : "bg-red-300")} />
              {connected ? 'Live' : 'Offline'}
            </Badge>
            {latestData?.timestamp && !isMobile && (
              <span className="text-sm text-muted-foreground">
                Last update: {formatTimestamp(latestData.timestamp)}
              </span>
            )}
          </div>
        </div>
      </header>

      <main className="p-4 flex-1 flex flex-col" style={{ minHeight: 'calc(100vh - 100px)' }}>

        <Tabs defaultValue="design" className="flex-1 flex flex-col gap-0" onValueChange={(value) => {
          // Re-measure charts container when switching to Charts tab (fixes initial width)
          if (value === 'charts') {
            setTimeout(() => {
              requestAnimationFrame(() => {
                if (chartsContainerRef.current) {
                  setChartsContainerWidth(chartsContainerRef.current.getBoundingClientRect().width)
                }
              })
            }, 50)
          }
        }}>
          <TabsList className="mb-1">
            <TabsTrigger value="design">Sensor Dashboard</TabsTrigger>

            <TabsTrigger value="charts">Charts</TabsTrigger>
            {BIDIRECTIONAL_SIGNALS.length > 0 && (
              <TabsTrigger value="simulation">🎮 Simulation</TabsTrigger>
            )}

          </TabsList>
          
          {/* User's Custom Design - Responsive: stacked on mobile, pixel-perfect on desktop */}
          <TabsContent value="design" className="flex-1 flex flex-col mt-0 pt-0 data-[state=inactive]:hidden">
            <div 
              className={cn(
                "rounded-lg border flex-1",
                isMobile ? "flex flex-col gap-3 p-3 overflow-y-auto" : "relative overflow-auto scrollbar-hidden"
              )}
              style={isMobile ? {} : { 
                background: 'linear-gradient(hsl(217.2 32.6% 10%) 0%, hsl(217.2 32.6% 8%) 100%)',
                backgroundImage: `
                  linear-gradient(hsl(var(--border) / 0.12) 1px, transparent 1px),
                  linear-gradient(90deg, hsl(var(--border) / 0.12) 1px, transparent 1px)
                `,
                backgroundSize: '25px 15px',
                backgroundPosition: '8px 8px',
                minHeight: 'calc(100vh - 200px)',
                padding: '8px'
              }}
            >
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '0px', top: '0px', width: '600px', height: '420px', padding: '4px' }}
                  >
                    <div className="bg-card rounded-lg border p-2 h-full flex flex-col overflow-hidden">
                      <Label className="text-xs text-muted-foreground mb-1 shrink-0">AHT10 Temperature/Humidity Sensor</Label>

                      <div className="flex-1 min-h-0">
                        <MemoizedPlot
                          data={[

                            {
                              x: history.map((d: Record<string, any>) => toISOTimestamp(d.timestamp)),
                              y: history.map((d: Record<string, any>) => d['environment_aht10_temperature']),
                              type: 'scatter' as const,
                              mode: 'lines' as const,
                              name: 'Environment Aht10 Temperature',
                              line: { color: '#14b8a6', width: 2 },

                            },

                            {
                              x: history.map((d: Record<string, any>) => toISOTimestamp(d.timestamp)),
                              y: history.map((d: Record<string, any>) => d['environment_aht10_humidity']),
                              type: 'scatter' as const,
                              mode: 'lines' as const,
                              name: 'Environment Aht10 Humidity',
                              line: { color: '#f59e0b', width: 2 },

                            },
                          ]}

                          layout={{
                            autosize: true,
                            margin: { t: 10, r: 10, b: 30, l: 40 },
                            paper_bgcolor: 'transparent',
                            plot_bgcolor: 'transparent',
                            font: { color: '#94a3b8', size: 10 },
                            xaxis: {
                              type: 'date' as const,
                              showgrid: true,
                              gridcolor: 'hsl(217.2 32.6% 25%)',
                              tickformat: '%H:%M:%S',
                              tickfont: { size: 9 },
                            },
                            yaxis: {
                              showgrid: true,
                              gridcolor: 'hsl(217.2 32.6% 25%)',
                              tickfont: { size: 9 },
                            },
                            legend: {
                              orientation: 'h' as const,
                              yanchor: 'bottom' as const,
                              y: 1.02,
                              xanchor: 'right' as const,
                              x: 1,
                              font: { size: 10 },
                            },
                            showlegend: true,
                            datarevision: history.length,
                          }}
                          config={DASHBOARD_PLOT_CONFIG}
                          useResizeHandler
                          style={{ width: '100%', height: '100%' }}
                          revision={history.length}
                        />
                      </div>
                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '600px', top: '0px', width: '600px', height: '420px', padding: '4px' }}
                  >
                    <div className="bg-card rounded-lg border p-2 h-full flex flex-col overflow-hidden">
                      <Label className="text-xs text-muted-foreground mb-1 shrink-0">INMP441 I2S MEMS Microphone</Label>

                      <div className="flex-1 min-h-0">
                        <MemoizedPlot
                          data={[

                            {
                              x: history.map((d: Record<string, any>) => toISOTimestamp(d.timestamp)),
                              y: history.map((d: Record<string, any>) => d['motor_n_mike_inmp441_audio_level']),
                              type: 'scatter' as const,
                              mode: 'lines' as const,
                              name: 'Motor N Mike Inmp441 Audio Level',
                              line: { color: '#14b8a6', width: 2 },

                            },

                            {
                              x: history.map((d: Record<string, any>) => toISOTimestamp(d.timestamp)),
                              y: history.map((d: Record<string, any>) => d['motor_n_mike_inmp441_peak_amplitude']),
                              type: 'scatter' as const,
                              mode: 'lines' as const,
                              name: 'Motor N Mike Inmp441 Peak Amplitude',
                              line: { color: '#f59e0b', width: 2 },

                            },

                            {
                              x: history.map((d: Record<string, any>) => toISOTimestamp(d.timestamp)),
                              y: history.map((d: Record<string, any>) => d['motor_n_mike_inmp441_rms_level']),
                              type: 'scatter' as const,
                              mode: 'lines' as const,
                              name: 'Motor N Mike Inmp441 Rms Level',
                              line: { color: '#8b5cf6', width: 2 },

                            },
                          ]}

                          layout={{
                            autosize: true,
                            margin: { t: 10, r: 10, b: 30, l: 40 },
                            paper_bgcolor: 'transparent',
                            plot_bgcolor: 'transparent',
                            font: { color: '#94a3b8', size: 10 },
                            xaxis: {
                              type: 'date' as const,
                              showgrid: true,
                              gridcolor: 'hsl(217.2 32.6% 25%)',
                              tickformat: '%H:%M:%S',
                              tickfont: { size: 9 },
                            },
                            yaxis: {
                              showgrid: true,
                              gridcolor: 'hsl(217.2 32.6% 25%)',
                              tickfont: { size: 9 },
                            },
                            legend: {
                              orientation: 'h' as const,
                              yanchor: 'bottom' as const,
                              y: 1.02,
                              xanchor: 'right' as const,
                              x: 1,
                              font: { size: 10 },
                            },
                            showlegend: true,
                            datarevision: history.length,
                          }}
                          config={DASHBOARD_PLOT_CONFIG}
                          useResizeHandler
                          style={{ width: '100%', height: '100%' }}
                          revision={history.length}
                        />
                      </div>
                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '0px', top: '420px', width: '600px', height: '75px', padding: '4px' }}
                  >
                    <div className="bg-card rounded-lg border p-3 h-full flex flex-col justify-center">
                      <Label className="text-xs text-muted-foreground mb-1">[motor_n_mike] INMP441 I2S MEMS Microphone - Audio Samples</Label>

                      <div className="flex items-baseline gap-1">
                        <span className="text-2xl font-bold tabular-nums">
                          {formatValue(latestData?.['motor_n_mike_inmp441_audio_samples'], 'float', '')}
                        </span>

                      </div>
                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '600px', top: '420px', width: '600px', height: '75px', padding: '4px' }}
                  >
                    <div className="flex items-center gap-3 bg-card rounded-lg px-4 py-3 border h-full">

                      <div
                        className={cn(
                          "w-4 h-4 rounded-full transition-colors shrink-0",
                          latestData?.['motor_n_mike_inmp441_is_clipping'] ? "bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.6)]" : "bg-muted"
                        )}
                      />
                      <Label className="text-sm font-medium truncate">[motor_n_mike] INMP441 I2S MEMS Microphone - Is Clipping</Label>

                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '1200px', top: '420px', width: '600px', height: '75px', padding: '4px' }}
                  >
                    <div className="bg-card rounded-lg border p-3 h-full flex flex-col justify-center">
                      <Label className="text-xs text-muted-foreground mb-1 block truncate">[theSpeaker] Bluetooth Speaker - Rssi</Label>

                      <div className="text-xl font-bold tabular-nums truncate">
                        {latestData?.['thespeaker_bluetoothspeaker_rssi'] ?? '--'}
                      </div>
                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '1800px', top: '420px', width: '600px', height: '75px', padding: '4px' }}
                  >
                    <div className="flex items-center gap-3 bg-card rounded-lg px-4 py-3 border h-full">

                      <div
                        className={cn(
                          "w-4 h-4 rounded-full transition-colors shrink-0",
                          latestData?.['thespeaker_bluetoothspeaker_connected'] ? "bg-green-500 shadow-[0_0_8px_rgba(34,197,94,0.6)]" : "bg-muted"
                        )}
                      />
                      <Label className="text-sm font-medium truncate">[theSpeaker] Bluetooth Speaker - Connected</Label>

                    </div>
                  </div>
                  <div 
                    className={isMobile ? "w-full" : "absolute"}
                    style={isMobile ? {} : { left: '0px', top: '495px', width: '600px', height: '75px', padding: '4px' }}
                  >
                    <div className="bg-card rounded-lg border p-3 h-full flex flex-col justify-center">
                      <Label className="text-xs text-muted-foreground mb-1">[theSpeaker] Bluetooth Speaker - Device Name</Label>

                      <div className="flex items-baseline gap-1">
                        <span className="text-2xl font-bold tabular-nums">
                          {formatValue(latestData?.['thespeaker_bluetoothspeaker_device_name'], 'float', '')}
                        </span>

                      </div>
                    </div>
                  </div>

            </div>
          </TabsContent>
          
          {/* Charts View with Design/Run modes and Plotly */}
          <TabsContent value="charts" className="mt-0 pt-0 space-y-0 data-[state=inactive]:hidden flex-1 w-full">
            <Card className="border-0 shadow-none bg-transparent w-full">
              <CardHeader className="px-0 py-0 pb-1">
                <div className="flex items-center justify-between">
                  <div>
                    <CardTitle className="text-sm">Signal Charts</CardTitle>
                    <CardDescription className="text-xs leading-tight">Real-time data visualization with draggable, resizable charts</CardDescription>
                  </div>
                  {/* Mode & Control Buttons */}
                  <div className="flex items-center gap-2">
                    {/* Play/Pause */}
                    <Button
                      size="sm"
                      variant={chartsPaused ? "default" : "outline"}
                      onClick={() => setChartsPaused(!chartsPaused)}
                      className="h-7 px-2 gap-1"
                      title={chartsPaused ? "Resume streaming" : "Pause for analysis"}
                    >
                      {chartsPaused ? <Play className="h-3.5 w-3.5" /> : <Pause className="h-3.5 w-3.5" />}
                      <span className="text-xs">{chartsPaused ? 'Resume' : 'Pause'}</span>
                    </Button>
                    
                    {chartsPaused && (
                      <Badge variant="outline" className="text-yellow-500 border-yellow-500">PAUSED</Badge>
                    )}
                    
                    {/* Design/Run Toggle */}
                    <div className="flex items-center gap-1 border-l pl-2 ml-2">
                      <Button
                        size="sm"
                        variant={chartsMode === 'run' ? "default" : "outline"}
                        onClick={() => setChartsMode('run')}
                        className="h-7 px-2 gap-1"
                      >
                        <Play className="h-3 w-3" />
                        <span className="text-xs">Run</span>
                      </Button>
                      <Button
                        size="sm"
                        variant={chartsMode === 'design' ? "default" : "outline"}
                        onClick={() => setChartsMode('design')}
                        className="h-7 px-2 gap-1"
                      >
                        <Settings2 className="h-3 w-3" />
                        <span className="text-xs">Design</span>
                      </Button>
                    </div>
                    
                    {chartsMode === 'design' && (
                      <Button
                        size="sm"
                        variant="outline"
                        onClick={addChart}
                        className="h-7 px-2 gap-1"
                      >
                        <Plus className="h-3 w-3" />
                        <span className="text-xs">Add Chart</span>
                      </Button>
                    )}
                  </div>
                </div>
              </CardHeader>
              <CardContent className="px-0 pt-2">
                {/* Charts Grid Container */}
                <div 
                  ref={chartsContainerRef} 
                  className="relative border rounded-lg"
                  style={{ 
                    minHeight: '500px',
                    background: chartsMode === 'design' 
                      ? 'linear-gradient(hsl(217.2 32.6% 10%) 0%, hsl(217.2 32.6% 8%) 100%)' 
                      : 'transparent'
                  }}
                >
                  {chartsDisplayHistory.length === 0 && (
                    <div className="absolute inset-0 flex items-center justify-center text-muted-foreground bg-muted/30 rounded-lg border border-dashed z-10">
                      <div className="text-center">
                        <svg className="w-12 h-12 mx-auto mb-2 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z" />
                        </svg>
                        <p className="text-sm font-medium">Waiting for data...</p>
                        <p className="text-xs mt-1">Connect a device or start streaming</p>
                      </div>
                    </div>
                  )}
                  
                  <GridLayout
                    className="layout"
                    layout={chartLayouts.map(l => isMobile 
                      ? { i: l.i, x: 0, y: l.y, w: 12, h: l.h, minW: 12, minH: 3 }
                      : { i: l.i, x: l.x, y: l.y, w: l.w, h: l.h, minW: 3, minH: 3 }
                    )}
                    cols={12}
                    rowHeight={60}
                    width={chartsContainerWidth}
                    onLayoutChange={isMobile ? undefined : handleChartLayoutChange}
                    draggableHandle=".chart-drag-handle"
                    isDraggable={!isMobile && chartsMode === 'design'}
                    isResizable={!isMobile && chartsMode === 'design'}
                    resizeHandles={['se', 'e', 's']}
                    margin={[8, 8]}
                    containerPadding={[0, 0]}
                  >
                    {chartLayouts.map((chart) => {
                      const activeSignalNames = chart.signalNames || [chart.signalName || SIGNALS[0]?.name || '']
                      const activeSignals = SIGNALS.filter(s => activeSignalNames.includes(s.name))
                      const primarySignal = activeSignals[0] || SIGNALS[0]
                      const primaryColor = CHART_COLORS[SIGNALS.findIndex(s => s.name === primarySignal?.name) % CHART_COLORS.length]
                      
                      // Prepare Plotly data - filter out entries with null/undefined Y values
                      // to prevent X-axis jumps from incomplete MCU readings
                      const plotData = activeSignals.map((sig, idx) => {
                        const sigIdx = SIGNALS.findIndex(s => s.name === sig.name)
                        const color = CHART_COLORS[sigIdx % CHART_COLORS.length]
                        const filteredX: string[] = []
                        const filteredY: number[] = []
                        chartsDisplayHistory.forEach(d => {
                          const yVal = (d as Record<string, any>)[sig.name]
                          if (yVal !== null && yVal !== undefined && typeof yVal === 'number') {
                            filteredX.push(d.timestamp as string)
                            filteredY.push(yVal)
                          }
                        })
                        return {
                          x: filteredX,
                          y: filteredY,
                          type: 'scatter' as const,
                          mode: 'lines' as const,
                          name: `${sig.display_name}${sig.unit ? ` (${sig.unit})` : ''}`,
                          line: { color, width: 2 },
                          hovertemplate: `%{y:.2f}${sig.unit ? ' ' + sig.unit : ''}<extra>${sig.display_name}</extra>`,
                        }
                      })
                      
                      // Compute Y-range with padding to prevent axis jumping on small fluctuations
                      const allYValues = plotData.flatMap(t => t.y.filter((v: any): v is number => typeof v === 'number' && v !== 0))
                      let yRange: [number, number] | undefined
                      if (allYValues.length > 0) {
                        const yMin = Math.min(...allYValues)
                        const yMax = Math.max(...allYValues)
                        const spread = yMax - yMin
                        const padding = Math.max(spread * 0.15, 0.5)
                        yRange = [yMin - padding, yMax + padding]
                      }

                      // Compute X-range from filtered data to prevent axis flicker
                      let xRange: [string, string] | undefined
                      if (plotData.length > 0 && plotData[0].x.length > 0) {
                        const allX = plotData.flatMap(t => t.x)
                        xRange = [allX[0], allX[allX.length - 1]]
                      }

                      const plotLayout: any = {
                        autosize: true,
                        margin: { l: 50, r: 20, t: 10, b: 40 },
                        paper_bgcolor: 'transparent',
                        plot_bgcolor: 'transparent',
                        font: { color: '#94a3b8' },
                        xaxis: {
                          gridcolor: 'rgba(148, 163, 184, 0.1)',
                          tickformat: '%H:%M:%S',
                          tickfont: { size: 10 },
                          ...(chartsPaused ? {} : { range: xRange, autorange: xRange ? false : true }),
                        },
                        yaxis: {
                          gridcolor: 'rgba(148, 163, 184, 0.1)',
                          tickfont: { size: 10 },
                          ...(chartsPaused ? {} : { range: yRange, autorange: yRange ? false : true }),
                        },
                        showlegend: activeSignals.length > 1,
                        legend: {
                          orientation: 'h' as const,
                          y: -0.15,
                          x: 0.5,
                          xanchor: 'center' as const,
                          font: { size: 10, color: '#94a3b8' },
                        },
                        hovermode: 'x unified' as const,
                        datarevision: chartsRevision,
                      }
                      
                      const plotConfig = {
                        displayModeBar: chartsMode === 'run',
                        modeBarButtonsToRemove: ['lasso2d', 'select2d'] as any[],
                        displaylogo: false,
                        responsive: true,
                        staticPlot: false,
                        scrollZoom: true,
                      }
                      
                      return (
                        <div key={chart.i} className="bg-card rounded-lg border overflow-hidden flex flex-col">
                          {/* Chart Header - draggable in design mode */}
                          <div className={cn(
                            "flex items-center justify-between px-3 py-1.5 border-b bg-muted/30",
                            chartsMode === 'design' && "chart-drag-handle cursor-grab"
                          )}>
                            <div className="flex items-center gap-2 min-w-0 flex-1">
                              {chartsMode === 'design' && (
                                <Move className="h-3.5 w-3.5 text-muted-foreground flex-shrink-0" />
                              )}
                              {activeSignals.map((sig) => {
                                const sigIdx = SIGNALS.findIndex(s => s.name === sig.name)
                                return <span key={sig.name} className="w-2.5 h-2.5 rounded-full flex-shrink-0" style={{ backgroundColor: CHART_COLORS[sigIdx % CHART_COLORS.length] }} />
                              })}
                              {chartsMode === 'design' ? (
                                <div className="relative" onMouseDown={(e) => e.stopPropagation()} onClick={(e) => e.stopPropagation()}>
                                  <button
                                    onClick={() => setOpenSignalDropdown(openSignalDropdown === chart.i ? null : chart.i)}
                                    className="h-6 px-2 text-[11px] rounded border bg-background flex items-center gap-1 max-w-[200px]"
                                  >
                                    <span className="truncate">
                                      {activeSignals.length === 0 ? 'Select signals' : 
                                       activeSignals.length === 1 ? activeSignals[0].display_name :
                                       `${activeSignals.length} signals`}
                                    </span>
                                    <ChevronDown className="h-3 w-3 flex-shrink-0 opacity-50" />
                                  </button>
                                  {openSignalDropdown === chart.i && (
                                    <div className="absolute top-7 left-0 z-[100] min-w-[180px] bg-popover border rounded-md shadow-lg py-1" onMouseDown={(e) => e.stopPropagation()} onClick={(e) => e.stopPropagation()}>
                                      {SIGNALS.map(s => {
                                        const isSelected = activeSignalNames.includes(s.name)
                                        const sigIdx = SIGNALS.findIndex(sig => sig.name === s.name)
                                        return (
                                          <button
                                            key={s.name}
                                            onClick={() => toggleChartSignal(chart.i, s.name)}
                                            className="w-full flex items-center gap-2 px-2 py-1 text-[11px] hover:bg-accent text-left"
                                          >
                                            <span className={cn("w-3.5 h-3.5 rounded-sm border flex items-center justify-center", isSelected ? "bg-primary border-primary" : "border-muted-foreground/40")}>
                                              {isSelected && <Check className="h-2.5 w-2.5 text-primary-foreground" />}
                                            </span>
                                            <span className="w-2 h-2 rounded-full flex-shrink-0" style={{ backgroundColor: CHART_COLORS[sigIdx % CHART_COLORS.length] }} />
                                            <span className="truncate">{s.display_name}</span>
                                          </button>
                                        )
                                      })}
                                    </div>
                                  )}
                                </div>
                              ) : (
                                <span className="text-xs font-medium truncate">
                                  {activeSignals.map(s => s.display_name).join(', ')}
                                </span>
                              )}
                            </div>
                            {chartsMode === 'design' && (
                              <Button
                                size="sm"
                                variant="ghost"
                                className="h-5 w-5 p-0 flex-shrink-0 text-muted-foreground hover:text-destructive"
                                onMouseDown={(e) => e.stopPropagation()}
                                onClick={(e) => { e.stopPropagation(); e.preventDefault(); removeChart(chart.i); }}
                              >
                                <Trash2 className="h-3 w-3" />
                              </Button>
                            )}
                          </div>
                          
                          {/* Chart Content */}
                          <div className="flex-1 min-h-0 p-1">
                            {chartsDisplayHistory.length > 0 ? (
                              <MemoizedPlot
                                data={plotData}
                                layout={plotLayout}
                                config={plotConfig}
                                style={{ width: '100%', height: '100%' }}
                                useResizeHandler
                                revision={chartsRevision}
                              />
                            ) : (
                              <div className="h-full flex items-center justify-center text-muted-foreground text-xs">
                                Waiting for data...
                              </div>
                            )}
                          </div>
                        </div>
                      )
                    })}
                  </GridLayout>
                </div>
              </CardContent>
            </Card>
          </TabsContent>
          
          {/* Bidirectional IO Simulation Tab */}
          {BIDIRECTIONAL_SIGNALS.length > 0 && (
            <TabsContent value="simulation" className="mt-0 pt-0 space-y-0">
              <Card className="border-0 shadow-none bg-transparent">
                <CardHeader className="px-0 py-0 pb-1">
                  <div className="flex items-center justify-between">
                    <div>
                      <CardTitle className="text-sm">Input Simulation</CardTitle>
                      <CardDescription className="text-xs leading-tight">
                        Override hardware inputs with simulated values. Last value wins - real hardware updates will override simulation.
                      </CardDescription>
                    </div>
                    <Badge variant="outline" className="text-yellow-500 border-yellow-500">
                      {Object.keys(simulatedInputs).filter(k => simulatedInputs[k]?.active).length} active
                    </Badge>
                  </div>
                </CardHeader>
                <CardContent className="px-0 pt-2">
                  <div className="grid gap-4 md:grid-cols-2 lg:grid-cols-3">
                    {BIDIRECTIONAL_SIGNALS.map((signal) => {
                      const simState = simulatedInputs[signal.name] || { active: false, value: false };
                      const realValue = latestData?.[signal.name];
                      
                      return (
                        <Card key={signal.name} className={cn(
                          "transition-all",
                          simState.active ? "border-yellow-500 bg-yellow-500/5" : ""
                        )}>
                          <CardHeader className="pb-2">
                            <div className="flex items-center justify-between">
                              <CardTitle className="text-sm">{signal.display_name}</CardTitle>
                              <Badge variant={simState.active ? "default" : "outline"} className="text-xs">
                                {simState.active ? "Simulated" : "Real"}
                              </Badge>
                            </div>
                            <CardDescription className="text-xs">
                              Real: {realValue !== undefined ? String(realValue) : 'N/A'} | 
                              Sim: {simState.active ? String(simState.value) : 'Off'}
                            </CardDescription>
                          </CardHeader>
                          <CardContent className="space-y-3">
                            {signal.type === 'bool' ? (
                              /* Boolean toggle for button/switch inputs */
                              <div className="flex items-center gap-4">
                                <Button
                                  variant={simState.active && simState.value ? "default" : "outline"}
                                  size="sm"
                                  className="flex-1"
                                  onClick={() => handleSimulate(signal.name, true)}
                                >
                                  Press
                                </Button>
                                <Button
                                  variant={simState.active && !simState.value ? "default" : "outline"}
                                  size="sm"
                                  className="flex-1"
                                  onClick={() => handleSimulate(signal.name, false)}
                                >
                                  Release
                                </Button>
                              </div>
                            ) : (
                              /* Numeric input for other types */
                              <div className="flex items-center gap-2">
                                <Input
                                  type="number"
                                  placeholder="Value"
                                  value={simState.value || ''}
                                  onChange={(e) => setSimulatedInputs(prev => ({
                                    ...prev,
                                    [signal.name]: { ...prev[signal.name], value: parseFloat(e.target.value) || 0 }
                                  }))}
                                  className="flex-1"
                                />
                                <Button
                                  size="sm"
                                  onClick={() => handleSimulate(signal.name, simState.value)}
                                >
                                  Set
                                </Button>
                              </div>
                            )}
                            
                            {simState.active && (
                              <Button
                                variant="outline"
                                size="sm"
                                className="w-full text-yellow-500 border-yellow-500 hover:bg-yellow-500/10"
                                onClick={() => handleClearSimulation(signal.name)}
                              >
                                Clear Simulation
                              </Button>
                            )}
                          </CardContent>
                        </Card>
                      );
                    })}
                  </div>
                </CardContent>
              </Card>
            </TabsContent>
          )}

        </Tabs>

      </main>
    </div>
  )
}
