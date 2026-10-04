/**
 * Overview Page - Multi-device status at a glance
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useCallback, useMemo } from 'react'
import { Link } from 'react-router-dom'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { cn, formatValue } from '@/lib/utils'
import { wsService } from '@/services/websocket'
import type { SensorReading, SignalMetadata } from '@/services/api'
import {
  Cpu,
  Wifi,
  WifiOff,
  Activity,
  ThermometerSun,
  Droplets,
  Gauge,
  LayoutGrid,
  List,
  TrendingUp,
  TrendingDown,
  Minus,
  AlertTriangle,
  CheckCircle2,
  Clock,
} from 'lucide-react'

// Signal metadata
const SIGNALS: SignalMetadata[] = [
  { name: 'environment_aht10_temperature', type: 'float', unit: '', source: 'board_1', display_name: 'Environment Aht10 Temperature' },
  { name: 'environment_aht10_humidity', type: 'float', unit: '', source: 'board_1', display_name: 'Environment Aht10 Humidity' },
  { name: 'motor_n_mike_inmp441_audio_samples', type: 'buffer', unit: '', source: 'board_5', display_name: 'Inmp441 Audio Samples' },
  { name: 'motor_n_mike_inmp441_audio_level', type: 'int', unit: '', source: 'board_5', display_name: 'Inmp441 Audio Level' },
  { name: 'motor_n_mike_inmp441_peak_amplitude', type: 'int', unit: '', source: 'board_5', display_name: 'Inmp441 Peak Amplitude' },
  { name: 'motor_n_mike_inmp441_rms_level', type: 'float', unit: '', source: 'board_5', display_name: 'Inmp441 Rms Level' },
  { name: 'motor_n_mike_inmp441_is_clipping', type: 'bool', unit: '', source: 'board_5', display_name: 'Inmp441 Clipping' },
  { name: 'thespeaker_bluetoothspeaker_connected', type: 'bool', unit: '', source: 'board_0', display_name: 'Thespeaker Bluetoothspeaker Connected' },
  { name: 'thespeaker_bluetoothspeaker_device_name', type: 'string', unit: '', source: 'board_0', display_name: 'Bluetoothspeaker Name' },
  { name: 'thespeaker_bluetoothspeaker_rssi', type: 'int', unit: '', source: 'board_0', display_name: 'Thespeaker Bluetoothspeaker Rssi' },
]

// MCU definitions from architecture
const MCUS = [
  { id: 'board_0', name: 'theSpeaker', communication: 'http' },
  { id: 'board_1', name: 'environment', communication: 'http' },
  { id: 'board_5', name: 'motor_n_mike', communication: 'tcp' },
]


interface DeviceStatus {
  id: string
  name: string
  online: boolean
  lastSeen: Date | null
  latestValues: Record<string, number | string>
  sparkline: number[]
  communication: string
  signalCount: number
}

export default function Overview() {
  const [devices, setDevices] = useState<Map<string, DeviceStatus>>(new Map())
  const [connected, setConnected] = useState(false)
  const [viewMode, setViewMode] = useState<'grid' | 'list'>('grid')
  const [alertCount, setAlertCount] = useState(0)

  // Initialize devices from MCU definitions
  useEffect(() => {
    const initial = new Map<string, DeviceStatus>()
    MCUS.forEach(mcu => {
      const mcuSignals = SIGNALS.filter(s => s.source === mcu.id)
      initial.set(mcu.id, {
        id: mcu.id,
        name: mcu.name,
        online: false,
        lastSeen: null,
        latestValues: {},
        sparkline: [],
        communication: mcu.communication,
        signalCount: mcuSignals.length,
      })
    })
    setDevices(initial)
  }, [])

  // WebSocket connection
  useEffect(() => {
    const unsub1 = wsService.onConnectionChange(setConnected)
    const unsub2 = wsService.onMessage((data: SensorReading) => {
      const mcuId = (data as any).mcu_id || 'unknown'
      setDevices(prev => {
        const next = new Map(prev)
        const device = next.get(mcuId)
        if (device) {
          const values: Record<string, number | string> = {}
          const mcuSignals = SIGNALS.filter(s => s.source === mcuId)
          mcuSignals.forEach(sig => {
            const val = (data as any)[sig.name]
            if (val !== undefined) values[sig.name] = val
          })

          // Add to sparkline (keep last 20 points)
          const firstNumericVal = Object.values(values).find(v => typeof v === 'number') as number | undefined
          const sparkline = [...device.sparkline]
          if (firstNumericVal !== undefined) {
            sparkline.push(firstNumericVal)
            if (sparkline.length > 20) sparkline.shift()
          }

          next.set(mcuId, {
            ...device,
            online: true,
            lastSeen: new Date(),
            latestValues: { ...device.latestValues, ...values },
            sparkline,
          })
        }
        return next
      })
    })

    wsService.connect()
    return () => { unsub1(); unsub2() }
  }, [])

  // Stale detection (mark offline after 30s)
  useEffect(() => {
    const interval = setInterval(() => {
      setDevices(prev => {
        const next = new Map(prev)
        let changed = false
        next.forEach((device, id) => {
          if (device.online && device.lastSeen) {
            const stale = Date.now() - device.lastSeen.getTime() > 30000
            if (stale) {
              next.set(id, { ...device, online: false })
              changed = true
            }
          }
        })
        return changed ? next : prev
      })
    }, 5000)
    return () => clearInterval(interval)
  }, [])

  const deviceList = useMemo(() => Array.from(devices.values()), [devices])
  const onlineCount = deviceList.filter(d => d.online).length
  const offlineCount = deviceList.filter(d => !d.online).length

  // Trend arrow component
  const TrendArrow = ({ values }: { values: number[] }) => {
    if (values.length < 2) return <Minus className="w-3 h-3 text-muted-foreground" />
    const last = values[values.length - 1]
    const prev = values[values.length - 2]
    if (last > prev) return <TrendingUp className="w-3 h-3 text-green-500" />
    if (last < prev) return <TrendingDown className="w-3 h-3 text-red-500" />
    return <Minus className="w-3 h-3 text-muted-foreground" />
  }

  // Mini sparkline SVG
  const Sparkline = ({ data, color = 'currentColor' }: { data: number[]; color?: string }) => {
    if (data.length < 2) return null
    const min = Math.min(...data)
    const max = Math.max(...data)
    const range = max - min || 1
    const w = 80
    const h = 24
    const points = data.map((v, i) => {
      const x = (i / (data.length - 1)) * w
      const y = h - ((v - min) / range) * h
      return `${x},${y}`
    }).join(' ')

    return (
      <svg width={w} height={h} className="opacity-60">
        <polyline fill="none" stroke={color} strokeWidth="1.5" points={points} />
      </svg>
    )
  }

  return (
    <div className="p-4 lg:p-6 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold">System Overview</h1>
          <p className="text-sm text-muted-foreground mt-1">
            Real-time status of all connected devices
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Button
            variant={viewMode === 'grid' ? 'default' : 'outline'}
            size="sm"
            onClick={() => setViewMode('grid')}
          >
            <LayoutGrid className="w-4 h-4" />
          </Button>
          <Button
            variant={viewMode === 'list' ? 'default' : 'outline'}
            size="sm"
            onClick={() => setViewMode('list')}
          >
            <List className="w-4 h-4" />
          </Button>
        </div>
      </div>

      {/* Summary Bar */}
      <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
        <Card className="bg-card">
          <CardContent className="flex items-center gap-3 p-4">
            <div className="p-2 rounded-lg bg-primary/10">
              <Cpu className="w-5 h-5 text-primary" />
            </div>
            <div>
              <p className="text-2xl font-bold">{deviceList.length}</p>
              <p className="text-xs text-muted-foreground">Total Devices</p>
            </div>
          </CardContent>
        </Card>
        <Card className="bg-card">
          <CardContent className="flex items-center gap-3 p-4">
            <div className="p-2 rounded-lg bg-green-500/10">
              <CheckCircle2 className="w-5 h-5 text-green-500" />
            </div>
            <div>
              <p className="text-2xl font-bold text-green-500">{onlineCount}</p>
              <p className="text-xs text-muted-foreground">Online</p>
            </div>
          </CardContent>
        </Card>
        <Card className="bg-card">
          <CardContent className="flex items-center gap-3 p-4">
            <div className="p-2 rounded-lg bg-red-500/10">
              <WifiOff className="w-5 h-5 text-red-500" />
            </div>
            <div>
              <p className="text-2xl font-bold text-red-500">{offlineCount}</p>
              <p className="text-xs text-muted-foreground">Offline</p>
            </div>
          </CardContent>
        </Card>
        <Card className="bg-card">
          <CardContent className="flex items-center gap-3 p-4">
            <div className="p-2 rounded-lg bg-amber-500/10">
              <AlertTriangle className="w-5 h-5 text-amber-500" />
            </div>
            <div>
              <p className="text-2xl font-bold text-amber-500">{alertCount}</p>
              <p className="text-xs text-muted-foreground">Active Alerts</p>
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Device Cards */}
      <div className={cn(
        viewMode === 'grid'
          ? 'grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4'
          : 'space-y-3'
      )}>
        {deviceList.map((device) => {
          const mcuSignals = SIGNALS.filter(s => s.source === device.id)
          const primarySignal = mcuSignals[0]
          const primaryValue = primarySignal ? device.latestValues[primarySignal.name] : undefined

          return (
            <Link key={device.id} to={`/devices/${device.id}`}>
              <Card className={cn(
                'transition-all hover:shadow-md hover:border-primary/30 cursor-pointer',
                viewMode === 'list' && 'flex items-center'
              )}>
                <CardContent className={cn(
                  'p-4',
                  viewMode === 'list' && 'flex items-center gap-4 w-full'
                )}>
                  {/* Status & Name */}
                  <div className="flex items-center gap-3 mb-3">
                    <div className={cn(
                      'w-2.5 h-2.5 rounded-full',
                      device.online ? 'bg-green-500 animate-pulse' : 'bg-gray-400'
                    )} />
                    <div className="flex-1 min-w-0">
                      <h3 className="font-medium text-sm truncate">{device.name}</h3>
                      <p className="text-xs text-muted-foreground">{device.communication.toUpperCase()}</p>
                    </div>
                    <Badge variant={device.online ? 'default' : 'secondary'} className="text-[10px]">
                      {device.online ? 'Online' : 'Offline'}
                    </Badge>
                  </div>

                  {/* Signals preview */}
                  {viewMode === 'grid' && (
                    <div className="space-y-2">
                      {mcuSignals.slice(0, 3).map(sig => {
                        const val = device.latestValues[sig.name]
                        return (
                          <div key={sig.name} className="flex items-center justify-between text-xs">
                            <span className="text-muted-foreground truncate max-w-[120px]">
                              {sig.display_name || sig.name.split('_').pop()}
                            </span>
                            <span className="font-mono font-medium">
                              {val !== undefined ? formatValue(val as number, sig.unit) : '—'}
                            </span>
                          </div>
                        )
                      })}
                    </div>
                  )}

                  {/* Sparkline & trend */}
                  <div className="flex items-center justify-between mt-3 pt-3 border-t border-border">
                    <Sparkline data={device.sparkline} color={device.online ? '#22c55e' : '#6b7280'} />
                    <div className="flex items-center gap-1 text-xs text-muted-foreground">
                      <Clock className="w-3 h-3" />
                      {device.lastSeen
                        ? `${Math.round((Date.now() - device.lastSeen.getTime()) / 1000)}s ago`
                        : 'Never'}
                    </div>
                  </div>
                </CardContent>
              </Card>
            </Link>
          )
        })}
      </div>
    </div>
  )
}
