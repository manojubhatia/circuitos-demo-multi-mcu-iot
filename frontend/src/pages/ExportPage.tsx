/**
 * Export Page - Data export in CSV/JSON formats
 * Generated for theSpeaker (3 boards)
 */

import { useState, useCallback } from 'react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Switch } from '@/components/ui/switch'
import { Badge } from '@/components/ui/badge'
import { api } from '@/services/api'
import type { SignalMetadata } from '@/services/api'
import { Download, FileJson, FileText, Clock, CheckCircle2 } from 'lucide-react'

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


export default function ExportPage() {
  const [fromDate, setFromDate] = useState(new Date(Date.now() - 86400000).toISOString().slice(0, 16))
  const [toDate, setToDate] = useState(new Date().toISOString().slice(0, 16))
  const [selectedSignals, setSelectedSignals] = useState<string[]>(SIGNALS.map(s => s.name))
  const [includeTimestamp, setIncludeTimestamp] = useState(true)
  const [exporting, setExporting] = useState(false)
  const [lastExport, setLastExport] = useState<string | null>(null)

  const toggleSignal = (name: string) => {
    setSelectedSignals(prev =>
      prev.includes(name)
        ? prev.filter(s => s !== name)
        : [...prev, name]
    )
  }

  const exportData = useCallback(async (format: 'csv' | 'json') => {
    setExporting(true)
    try {
      const response = await api.getHistoricalData(10000)
      const allData = response.data || []

      const fromTime = new Date(fromDate).getTime()
      const toTime = new Date(toDate).getTime()
      const filtered = allData.filter((d: any) => {
        const t = new Date(d.timestamp).getTime()
        return t >= fromTime && t <= toTime
      })

      // Filter columns
      const exportData = filtered.map((d: any) => {
        const row: any = {}
        if (includeTimestamp) row.timestamp = d.timestamp
        selectedSignals.forEach(sig => {
          if (d[sig] !== undefined) row[sig] = d[sig]
        })
        return row
      })

      let blob: Blob
      let filename: string

      if (format === 'csv') {
        const headers = Object.keys(exportData[0] || { timestamp: '' })
        const rows = exportData.map((row: any) => headers.map(h => row[h] ?? '').join(','))
        const csv = [headers.join(','), ...rows].join('\n')
        blob = new Blob([csv], { type: 'text/csv' })
        filename = `export_${new Date().toISOString().slice(0, 10)}.csv`
      } else {
        blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' })
        filename = `export_${new Date().toISOString().slice(0, 10)}.json`
      }

      const url = URL.createObjectURL(blob)
      const a = document.createElement('a')
      a.href = url
      a.download = filename
      a.click()
      URL.revokeObjectURL(url)

      setLastExport(`${exportData.length} records exported as ${format.toUpperCase()}`)
    } catch (err) {
      console.error('Export failed:', err)
    } finally {
      setExporting(false)
    }
  }, [fromDate, toDate, selectedSignals, includeTimestamp])

  return (
    <div className="p-4 lg:p-6 space-y-6">
      <div>
        <h1 className="text-2xl font-bold">Export Data</h1>
        <p className="text-sm text-muted-foreground">Download telemetry data in CSV or JSON format</p>
      </div>

      {/* Date Range */}
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-sm flex items-center gap-2">
            <Clock className="w-4 h-4" /> Date Range
          </CardTitle>
        </CardHeader>
        <CardContent className="space-y-3">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <Label className="text-xs">From</Label>
              <Input
                type="datetime-local"
                value={fromDate}
                onChange={(e) => setFromDate(e.target.value)}
              />
            </div>
            <div>
              <Label className="text-xs">To</Label>
              <Input
                type="datetime-local"
                value={toDate}
                onChange={(e) => setToDate(e.target.value)}
              />
            </div>
          </div>
          <div className="flex items-center gap-2">
            <Switch checked={includeTimestamp} onCheckedChange={setIncludeTimestamp} />
            <Label className="text-sm">Include timestamp column</Label>
          </div>
        </CardContent>
      </Card>

      {/* Signal Selection */}
      <Card>
        <CardHeader className="pb-2">
          <CardTitle className="text-sm">Signals to Export</CardTitle>
        </CardHeader>
        <CardContent>
          <div className="flex flex-wrap gap-2">
            <Button
              variant="outline"
              size="sm"
              className="text-xs"
              onClick={() => setSelectedSignals(SIGNALS.map(s => s.name))}
            >
              Select All
            </Button>
            <Button
              variant="outline"
              size="sm"
              className="text-xs"
              onClick={() => setSelectedSignals([])}
            >
              Clear All
            </Button>
          </div>
          <div className="flex flex-wrap gap-2 mt-3">
            {SIGNALS.map(sig => (
              <Button
                key={sig.name}
                variant={selectedSignals.includes(sig.name) ? 'default' : 'outline'}
                size="sm"
                className="text-xs"
                onClick={() => toggleSignal(sig.name)}
              >
                {sig.display_name || sig.name}
              </Button>
            ))}
          </div>
        </CardContent>
      </Card>

      {/* Export Actions */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        <Card className="cursor-pointer hover:border-primary/50 transition-colors" onClick={() => exportData('csv')}>
          <CardContent className="flex items-center gap-4 p-6">
            <div className="p-3 rounded-lg bg-green-500/10">
              <FileText className="w-8 h-8 text-green-500" />
            </div>
            <div>
              <h3 className="font-medium">Export as CSV</h3>
              <p className="text-xs text-muted-foreground">Comma-separated values, compatible with Excel</p>
            </div>
          </CardContent>
        </Card>
        <Card className="cursor-pointer hover:border-primary/50 transition-colors" onClick={() => exportData('json')}>
          <CardContent className="flex items-center gap-4 p-6">
            <div className="p-3 rounded-lg bg-blue-500/10">
              <FileJson className="w-8 h-8 text-blue-500" />
            </div>
            <div>
              <h3 className="font-medium">Export as JSON</h3>
              <p className="text-xs text-muted-foreground">Structured format, compatible with APIs & scripts</p>
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Status */}
      {lastExport && (
        <div className="flex items-center gap-2 text-sm text-green-500">
          <CheckCircle2 className="w-4 h-4" />
          {lastExport}
        </div>
      )}
    </div>
  )
}
