/**
 * Device Detail Page - Signals, charts, and controls for a single device
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useMemo, useCallback, useRef } from 'react'
import { useParams, Link } from 'react-router-dom'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Slider } from '@/components/ui/slider'
import { Switch } from '@/components/ui/switch'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { cn, formatValue, formatTimestamp } from '@/lib/utils'
import { wsService } from '@/services/websocket'
import type { SensorReading, SignalMetadata } from '@/services/api'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts'
import {
  ArrowLeft,
  Wifi,
  WifiOff,
  Activity,
  TrendingUp,
  TrendingDown,
  Minus,
  Maximize2,
  Minimize2,
  Pause,
  Play,
  RefreshCcw,
} from 'lucide-react'

const SIGNALS: SignalMetadata[] = [
  { name: 'environment_aht10_temperature', type: 'float', unit: '', source: 'board_1', display_name: 'Environment Aht10 Temperature', direction: 'output' },
  { name: 'environment_aht10_humidity', type: 'float', unit: '', source: 'board_1', display_name: 'Environment Aht10 Humidity', direction: 'output' },
  { name: 'motor_n_mike_inmp441_audio_samples', type: 'buffer', unit: '', source: 'board_5', display_name: 'Inmp441 Audio Samples', direction: 'output' },
  { name: 'motor_n_mike_inmp441_audio_level', type: 'int', unit: '', source: 'board_5', display_name: 'Inmp441 Audio Level', direction: 'output' },
  { name: 'motor_n_mike_inmp441_peak_amplitude', type: 'int', unit: '', source: 'board_5', display_name: 'Inmp441 Peak Amplitude', direction: 'output' },
  { name: 'motor_n_mike_inmp441_rms_level', type: 'float', unit: '', source: 'board_5', display_name: 'Inmp441 Rms Level', direction: 'output' },
  { name: 'motor_n_mike_inmp441_is_clipping', type: 'bool', unit: '', source: 'board_5', display_name: 'Inmp441 Clipping', direction: 'output' },
  { name: 'thespeaker_bluetoothspeaker_connected', type: 'bool', unit: '', source: 'board_0', display_name: 'Thespeaker Bluetoothspeaker Connected', direction: 'output' },
  { name: 'thespeaker_bluetoothspeaker_device_name', type: 'string', unit: '', source: 'board_0', display_name: 'Bluetoothspeaker Name', direction: 'output' },
  { name: 'thespeaker_bluetoothspeaker_rssi', type: 'int', unit: '', source: 'board_0', display_name: 'Thespeaker Bluetoothspeaker Rssi', direction: 'output' },
]

const MCUS = [
  { id: 'board_0', name: 'theSpeaker', communication: 'http' },
  { id: 'board_1', name: 'environment', communication: 'http' },
  { id: 'board_5', name: 'motor_n_mike', communication: 'tcp' },
]


const CHART_COLORS = ['#ef4444', '#22c55e', '#3b82f6', '#f59e0b', '#8b5cf6', '#ec4899', '#14b8a6', '#f97316']


interface HistoryEntry {
  timestamp: string
  [key: string]: any
}

export default function DeviceDetail() {
  const { deviceId } = useParams<{ deviceId: string }>()
  const [connected, setConnected] = useState(false)
  const [online, setOnline] = useState(false)
  const [lastSeen, setLastSeen] = useState<Date | null>(null)
  const [latestValues, setLatestValues] = useState<Record<string, number | string>>({})
  const [history, setHistory] = useState<HistoryEntry[]>([])
  const [paused, setPaused] = useState(false)
  const [expandedChart, setExpandedChart] = useState(false)
  const pausedHistoryRef = useRef<HistoryEntry[]>([])

  const mcu = MCUS.find(m => m.id === deviceId)
  const deviceSignals = useMemo(() => SIGNALS.filter(s => s.source === deviceId), [deviceId])
  const deviceParams = useMemo(() => {
    const params = typeof PARAMETERS !== 'undefined' ? PARAMETERS : []
    return params.filter((p: any) => p.target_mcu === deviceId || p.target_mcu === '')
  }, [deviceId])

  useEffect(() => {
    const unsub1 = wsService.onConnectionChange(setConnected)
    const unsub2 = wsService.onMessage((data: SensorReading) => {
      const mcuId = (data as any).mcu_id
      if (mcuId !== deviceId) return

      setOnline(true)
      setLastSeen(new Date())

      const values: Record<string, number | string> = {}
      deviceSignals.forEach(sig => {
        const val = (data as any)[sig.name]
        if (val !== undefined) values[sig.name] = val
      })
      setLatestValues(prev => ({ ...prev, ...values }))

      if (!paused) {
        const entry: HistoryEntry = { timestamp: new Date().toISOString(), ...values }
        setHistory(prev => {
          const next = [...prev, entry]
          return next.length > 300 ? next.slice(-300) : next
        })
      }
    })

    wsService.connect()
    return () => { unsub1(); unsub2() }
  }, [deviceId, deviceSignals, paused])

  // Stale detection
  useEffect(() => {
    const interval = setInterval(() => {
      if (lastSeen && Date.now() - lastSeen.getTime() > 30000) {
        setOnline(false)
      }
    }, 5000)
    return () => clearInterval(interval)
  }, [lastSeen])

  const handlePause = () => {
    if (!paused) {
      pausedHistoryRef.current = [...history]
    }
    setPaused(!paused)
  }

  const handleParamChange = (paramName: string, value: any) => {
    wsService.send({ type: 'set', variable_id: paramName, value, mcu_id: deviceId })
  }

  const chartData = paused ? pausedHistoryRef.current : history

  if (!mcu) {
    return (
      <div className="p-6">
        <Link to="/" className="flex items-center gap-2 text-sm text-muted-foreground hover:text-foreground mb-4">
          <ArrowLeft className="w-4 h-4" /> Back to Overview
        </Link>
        <p className="text-muted-foreground">Device not found: {deviceId}</p>
      </div>
    )
  }

  return (
    <div className="p-4 lg:p-6 space-y-6">
      {/* Breadcrumb & Header */}
      <div className="flex items-center justify-between">
        <div>
          <Link to="/" className="flex items-center gap-1 text-xs text-muted-foreground hover:text-foreground mb-1">
            <ArrowLeft className="w-3 h-3" /> Overview
          </Link>
          <div className="flex items-center gap-3">
            <h1 className="text-xl font-bold">{mcu.name}</h1>
            <Badge variant={online ? 'default' : 'secondary'}>
              {online ? 'Online' : 'Offline'}
            </Badge>
            <Badge variant="outline" className="text-xs">
              {mcu.communication.toUpperCase()}
            </Badge>
          </div>
          <p className="text-xs text-muted-foreground mt-1">
            {deviceSignals.length} signals • Last seen: {lastSeen ? formatTimestamp(lastSeen.toISOString()) : 'Never'}
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Button variant="outline" size="sm" onClick={handlePause}>
            {paused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
          </Button>
          <Button variant="outline" size="sm" onClick={() => setExpandedChart(!expandedChart)}>
            {expandedChart ? <Minimize2 className="w-4 h-4" /> : <Maximize2 className="w-4 h-4" />}
          </Button>
        </div>
      </div>

      {/* Signal Cards */}
      <div className="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 xl:grid-cols-5 gap-3">
        {deviceSignals.map((sig, idx) => {
          const val = latestValues[sig.name]
          const numVal = typeof val === 'number' ? val : undefined
          return (
            <Card key={sig.name} className="bg-card">
              <CardContent className="p-3">
                <p className="text-[10px] text-muted-foreground uppercase tracking-wide truncate mb-1">
                  {sig.display_name || sig.name.split('_').pop()}
                </p>
                <div className="flex items-end gap-1">
                  <span className="text-xl font-bold font-mono">
                    {numVal !== undefined ? numVal.toFixed(1) : '—'}
                  </span>
                  <span className="text-xs text-muted-foreground mb-0.5">{sig.unit}</span>
                </div>
              </CardContent>
            </Card>
          )
        })}
      </div>

      {/* Chart */}
      <Card className={cn(expandedChart && 'fixed inset-4 z-50')}>
        <CardHeader className="pb-2">
          <CardTitle className="text-sm flex items-center gap-2">
            <Activity className="w-4 h-4" />
            Signal Traces
            {paused && <Badge variant="secondary" className="text-[10px]">Paused</Badge>}
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className={cn('w-full', expandedChart ? 'h-[calc(100vh-12rem)]' : 'h-64 lg:h-80')}>
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" />
                <XAxis
                  dataKey="timestamp"
                  tickFormatter={(t) => new Date(t).toLocaleTimeString()}
                  stroke="hsl(var(--muted-foreground))"
                  fontSize={10}
                />
                <YAxis stroke="hsl(var(--muted-foreground))" fontSize={10} />
                <Tooltip
                  contentStyle={{ background: 'hsl(var(--card))', border: '1px solid hsl(var(--border))' }}
                  labelFormatter={(t) => new Date(t).toLocaleTimeString()}
                />
                <Legend />
                {deviceSignals.map((sig, idx) => (
                  <Line
                    key={sig.name}
                    type="monotone"
                    dataKey={sig.name}
                    stroke={CHART_COLORS[idx % CHART_COLORS.length]}
                    dot={false}
                    strokeWidth={1.5}
                    name={sig.display_name || sig.name.split('_').pop() || sig.name}
                  />
                ))}
              </LineChart>
            </ResponsiveContainer>
          </div>
        </CardContent>
      </Card>

      {/* Signal Table */}
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-sm">Signal Details</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Signal</th>
                  <th className="text-right py-2 px-3 font-medium text-muted-foreground">Value</th>
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Unit</th>
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Type</th>
                </tr>
              </thead>
              <tbody>
                {deviceSignals.map(sig => {
                  const val = latestValues[sig.name]
                  return (
                    <tr key={sig.name} className="border-b border-border/50 hover:bg-muted/30">
                      <td className="py-2 px-3 font-medium">{sig.display_name || sig.name}</td>
                      <td className="py-2 px-3 text-right font-mono">
                        {val !== undefined ? String(val) : '—'}
                      </td>
                      <td className="py-2 px-3 text-muted-foreground">{sig.unit || '—'}</td>
                      <td className="py-2 px-3">
                        <Badge variant="outline" className="text-[10px]">{sig.type}</Badge>
                      </td>
                    </tr>
                  )
                })}
              </tbody>
            </table>
          </div>
        </CardContent>
      </Card>

      {/* Parameters (if any) */}
      {deviceParams.length > 0 && (
        <Card>
          <CardHeader className="pb-2">
            <CardTitle className="text-sm">Controls & Parameters</CardTitle>
          </CardHeader>
          <CardContent className="space-y-4">
            {deviceParams.map((param: any) => (
              <div key={param.name} className="flex items-center gap-4">
                <Label className="min-w-[120px] text-sm">{param.display_name}</Label>
                {param.type === 'bool' ? (
                  <Switch
                    defaultChecked={param.default_value === 'true'}
                    onCheckedChange={(v) => handleParamChange(param.name, v)}
                  />
                ) : (
                  <div className="flex-1 flex items-center gap-3">
                    <Slider
                      defaultValue={[parseFloat(param.default_value) || 0]}
                      min={0}
                      max={100}
                      step={1}
                      onValueCommit={(v) => handleParamChange(param.name, v[0])}
                      className="flex-1"
                    />
                    <span className="text-xs font-mono w-12 text-right">
                      {wsService.getParameterValue(param.name) ?? param.default_value}
                    </span>
                  </div>
                )}
              </div>
            ))}
          </CardContent>
        </Card>
      )}
    </div>
  )
}
