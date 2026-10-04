/**
 * History Page - Historical data browsing with date range
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useCallback, useMemo } from 'react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { cn, formatValue } from '@/lib/utils'
import { api } from '@/services/api'
import type { SensorReading, SignalMetadata } from '@/services/api'
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer, Brush } from 'recharts'
import {
  Calendar,
  Download,
  Clock,
  BarChart3,
  RefreshCcw,
  ZoomIn,
  ZoomOut,
} from 'lucide-react'

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

const CHART_COLORS = ['#ef4444', '#22c55e', '#3b82f6', '#f59e0b', '#8b5cf6', '#ec4899', '#14b8a6', '#f97316']


type TimeRange = '1h' | '6h' | '24h' | '7d' | '30d' | 'custom'

interface HistoryStats {
  signal: string
  min: number
  max: number
  avg: number
  stddev: number
  count: number
}

export default function HistoryPage() {
  const [data, setData] = useState<SensorReading[]>([])
  const [loading, setLoading] = useState(false)
  const [timeRange, setTimeRange] = useState<TimeRange>('24h')
  const [customFrom, setCustomFrom] = useState('')
  const [customTo, setCustomTo] = useState('')
  const [selectedSignals, setSelectedSignals] = useState<string[]>(SIGNALS.slice(0, 4).map(s => s.name))
  const [stats, setStats] = useState<HistoryStats[]>([])

  const fetchHistory = useCallback(async () => {
    setLoading(true)
    try {
      let from: string
      let to: string = new Date().toISOString()

      if (timeRange === 'custom') {
        from = customFrom || new Date(Date.now() - 86400000).toISOString()
        to = customTo || to
      } else {
        const ms = { '1h': 3600000, '6h': 21600000, '24h': 86400000, '7d': 604800000, '30d': 2592000000 }
        from = new Date(Date.now() - ms[timeRange]).toISOString()
      }

      const response = await api.getHistoricalData(1000)
      const allData = response.data || []

      // Filter by time range (client-side if backend doesn't support range queries)
      const fromDate = new Date(from).getTime()
      const toDate = new Date(to).getTime()
      const filtered = allData.filter((d: any) => {
        const t = new Date(d.timestamp).getTime()
        return t >= fromDate && t <= toDate
      })

      setData(filtered)

      // Compute stats
      const statsResult: HistoryStats[] = selectedSignals.map(sigName => {
        const values = filtered
          .map((d: any) => d[sigName])
          .filter((v: any) => typeof v === 'number') as number[]

        if (values.length === 0) return { signal: sigName, min: 0, max: 0, avg: 0, stddev: 0, count: 0 }

        const min = Math.min(...values)
        const max = Math.max(...values)
        const avg = values.reduce((a, b) => a + b, 0) / values.length
        const variance = values.reduce((sum, v) => sum + Math.pow(v - avg, 2), 0) / values.length
        const stddev = Math.sqrt(variance)

        return { signal: sigName, min, max, avg, stddev, count: values.length }
      })
      setStats(statsResult)
    } catch (err) {
      console.error('Failed to fetch history:', err)
    } finally {
      setLoading(false)
    }
  }, [timeRange, customFrom, customTo, selectedSignals])

  useEffect(() => {
    fetchHistory()
  }, [timeRange])

  const toggleSignal = (name: string) => {
    setSelectedSignals(prev =>
      prev.includes(name)
        ? prev.filter(s => s !== name)
        : [...prev, name]
    )
  }

  const exportCSV = () => {
    const headers = ['timestamp', ...selectedSignals]
    const rows = data.map((d: any) =>
      [d.timestamp, ...selectedSignals.map(s => d[s] ?? '')].join(',')
    )
    const csv = [headers.join(','), ...rows].join('\n')
    const blob = new Blob([csv], { type: 'text/csv' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `history_${timeRange}_${new Date().toISOString().slice(0, 10)}.csv`
    a.click()
    URL.revokeObjectURL(url)
  }

  const exportJSON = () => {
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `history_${timeRange}_${new Date().toISOString().slice(0, 10)}.json`
    a.click()
    URL.revokeObjectURL(url)
  }

  return (
    <div className="p-4 lg:p-6 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold">History</h1>
          <p className="text-sm text-muted-foreground">
            {data.length} data points in selected range
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Button variant="outline" size="sm" onClick={fetchHistory} disabled={loading}>
            <RefreshCcw className={cn('w-4 h-4 mr-1', loading && 'animate-spin')} /> Refresh
          </Button>
          <Button variant="outline" size="sm" onClick={exportCSV}>
            <Download className="w-4 h-4 mr-1" /> CSV
          </Button>
          <Button variant="outline" size="sm" onClick={exportJSON}>
            <Download className="w-4 h-4 mr-1" /> JSON
          </Button>
        </div>
      </div>

      {/* Time Range Selector */}
      <Card>
        <CardContent className="flex flex-wrap items-center gap-2 p-4">
          <Clock className="w-4 h-4 text-muted-foreground" />
          {(['1h', '6h', '24h', '7d', '30d', 'custom'] as TimeRange[]).map(range => (
            <Button
              key={range}
              variant={timeRange === range ? 'default' : 'outline'}
              size="sm"
              onClick={() => setTimeRange(range)}
            >
              {range === 'custom' ? 'Custom' : range.toUpperCase()}
            </Button>
          ))}
          {timeRange === 'custom' && (
            <div className="flex items-center gap-2 ml-4">
              <Input
                type="datetime-local"
                value={customFrom}
                onChange={(e) => setCustomFrom(e.target.value)}
                className="h-8 text-xs w-44"
              />
              <span className="text-muted-foreground">to</span>
              <Input
                type="datetime-local"
                value={customTo}
                onChange={(e) => setCustomTo(e.target.value)}
                className="h-8 text-xs w-44"
              />
              <Button size="sm" onClick={fetchHistory}>Apply</Button>
            </div>
          )}
        </CardContent>
      </Card>

      {/* Signal Selector */}
      <div className="flex flex-wrap gap-2">
        {SIGNALS.map((sig, idx) => (
          <Button
            key={sig.name}
            variant={selectedSignals.includes(sig.name) ? 'default' : 'outline'}
            size="sm"
            className="text-xs"
            style={selectedSignals.includes(sig.name) ? { backgroundColor: CHART_COLORS[idx % CHART_COLORS.length] } : {}}
            onClick={() => toggleSignal(sig.name)}
          >
            {sig.display_name || sig.name.split('_').pop()}
          </Button>
        ))}
      </div>

      {/* Chart */}
      <Card>
        <CardContent className="p-4">
          <div className="w-full h-72 lg:h-96">
            {data.length > 0 ? (
              <ResponsiveContainer width="100%" height="100%">
                <LineChart data={data}>
                  <CartesianGrid strokeDasharray="3 3" stroke="hsl(var(--border))" />
                  <XAxis
                    dataKey="timestamp"
                    tickFormatter={(t) => {
                      const d = new Date(t)
                      return timeRange === '1h' || timeRange === '6h'
                        ? d.toLocaleTimeString()
                        : d.toLocaleDateString() + ' ' + d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
                    }}
                    stroke="hsl(var(--muted-foreground))"
                    fontSize={10}
                  />
                  <YAxis stroke="hsl(var(--muted-foreground))" fontSize={10} />
                  <Tooltip
                    contentStyle={{ background: 'hsl(var(--card))', border: '1px solid hsl(var(--border))' }}
                    labelFormatter={(t) => new Date(t).toLocaleString()}
                  />
                  <Legend />
                  <Brush dataKey="timestamp" height={30} stroke="hsl(var(--primary))" />
                  {selectedSignals.map((sigName, idx) => {
                    const sig = SIGNALS.find(s => s.name === sigName)
                    return (
                      <Line
                        key={sigName}
                        type="monotone"
                        dataKey={sigName}
                        stroke={CHART_COLORS[idx % CHART_COLORS.length]}
                        dot={false}
                        strokeWidth={1.5}
                        name={sig?.display_name || sigName.split('_').pop() || sigName}
                      />
                    )
                  })}
                </LineChart>
              </ResponsiveContainer>
            ) : (
              <div className="flex items-center justify-center h-full text-muted-foreground">
                <div className="text-center">
                  <BarChart3 className="w-12 h-12 mx-auto mb-3 opacity-30" />
                  <p>{loading ? 'Loading...' : 'No data in selected range'}</p>
                </div>
              </div>
            )}
          </div>
        </CardContent>
      </Card>

      {/* Statistics */}
      {stats.length > 0 && stats[0].count > 0 && (
        <Card>
          <CardHeader className="pb-2">
            <CardTitle className="text-sm">Statistics</CardTitle>
          </CardHeader>
          <CardContent>
            <div className="overflow-x-auto">
              <table className="w-full text-sm">
                <thead>
                  <tr className="border-b border-border">
                    <th className="text-left py-2 px-3 font-medium text-muted-foreground">Signal</th>
                    <th className="text-right py-2 px-3 font-medium text-muted-foreground">Min</th>
                    <th className="text-right py-2 px-3 font-medium text-muted-foreground">Max</th>
                    <th className="text-right py-2 px-3 font-medium text-muted-foreground">Average</th>
                    <th className="text-right py-2 px-3 font-medium text-muted-foreground">Std Dev</th>
                    <th className="text-right py-2 px-3 font-medium text-muted-foreground">Samples</th>
                  </tr>
                </thead>
                <tbody>
                  {stats.filter(s => s.count > 0).map(stat => {
                    const sig = SIGNALS.find(s => s.name === stat.signal)
                    return (
                      <tr key={stat.signal} className="border-b border-border/50">
                        <td className="py-2 px-3 font-medium">{sig?.display_name || stat.signal}</td>
                        <td className="py-2 px-3 text-right font-mono">{stat.min.toFixed(2)}</td>
                        <td className="py-2 px-3 text-right font-mono">{stat.max.toFixed(2)}</td>
                        <td className="py-2 px-3 text-right font-mono">{stat.avg.toFixed(2)}</td>
                        <td className="py-2 px-3 text-right font-mono">{stat.stddev.toFixed(3)}</td>
                        <td className="py-2 px-3 text-right font-mono">{stat.count}</td>
                      </tr>
                    )
                  })}
                </tbody>
              </table>
            </div>
          </CardContent>
        </Card>
      )}
    </div>
  )
}
