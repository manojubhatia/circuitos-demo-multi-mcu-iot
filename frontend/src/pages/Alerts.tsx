/**
 * Alerts Page - Threshold configuration & alert history
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useCallback, useMemo } from 'react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Switch } from '@/components/ui/switch'
import { cn } from '@/lib/utils'
import { wsService } from '@/services/websocket'
import { api } from '@/services/api'
import type { SensorReading, SignalMetadata } from '@/services/api'
import {
  Bell,
  BellRing,
  Plus,
  Trash2,
  Check,
  X,
  AlertTriangle,
  AlertCircle,
  Info,
  Filter,
  Download,
  Volume2,
  VolumeX,
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


type Severity = 'info' | 'warning' | 'critical'

interface AlertRule {
  id: string
  signal: string
  condition: 'above' | 'below'
  threshold: number
  severity: Severity
  enabled: boolean
  hysteresis: number
}

interface AlertEvent {
  id: string
  ruleId: string
  signal: string
  value: number
  threshold: number
  severity: Severity
  condition: string
  timestamp: string
  acknowledged: boolean
  resolvedAt?: string
  device?: string
}

const SEVERITY_CONFIG: Record<Severity, { color: string; icon: React.ReactNode; bg: string }> = {
  info: { color: 'text-blue-500', icon: <Info className="w-4 h-4" />, bg: 'bg-blue-500/10' },
  warning: { color: 'text-amber-500', icon: <AlertTriangle className="w-4 h-4" />, bg: 'bg-amber-500/10' },
  critical: { color: 'text-red-500', icon: <AlertCircle className="w-4 h-4" />, bg: 'bg-red-500/10' },
}

const STORAGE_KEY = 'alert-rules'
const ALERT_HISTORY_KEY = 'alert-history'

export default function Alerts() {
  const [rules, setRules] = useState<AlertRule[]>(() => {
    try {
      const saved = localStorage.getItem(STORAGE_KEY)
      return saved ? JSON.parse(saved) : []
    } catch { return [] }
  })
  const [alertHistory, setAlertHistory] = useState<AlertEvent[]>(() => {
    try {
      const saved = localStorage.getItem(ALERT_HISTORY_KEY)
      return saved ? JSON.parse(saved) : []
    } catch { return [] }
  })
  const [showRuleForm, setShowRuleForm] = useState(false)
  const [activeTab, setActiveTab] = useState<'active' | 'history' | 'rules'>('active')
  const [soundEnabled, setSoundEnabled] = useState(true)
  const [filterSeverity, setFilterSeverity] = useState<Severity | 'all'>('all')

  // New rule form state
  const [newRule, setNewRule] = useState<Partial<AlertRule>>({
    signal: SIGNALS[0]?.name || '',
    condition: 'above',
    threshold: 50,
    severity: 'warning',
    hysteresis: 2,
    enabled: true,
  })

  // Persist rules
  useEffect(() => {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(rules))
  }, [rules])

  // Persist alert history
  useEffect(() => {
    localStorage.setItem(ALERT_HISTORY_KEY, JSON.stringify(alertHistory.slice(-500)))
  }, [alertHistory])

  // Evaluate rules against incoming data
  useEffect(() => {
    const alertState = new Map<string, boolean>() // ruleId -> currently in alert

    const unsub = wsService.onMessage((data: SensorReading) => {
      rules.forEach(rule => {
        if (!rule.enabled) return
        const val = (data as any)[rule.signal]
        if (val === undefined || typeof val !== 'number') return

        const inAlert = alertState.get(rule.id) || false
        let shouldAlert = false

        if (rule.condition === 'above') {
          shouldAlert = val > rule.threshold
          // Hysteresis: only clear if value drops below threshold - hysteresis
          if (inAlert && val > (rule.threshold - rule.hysteresis)) {
            shouldAlert = true
          }
        } else {
          shouldAlert = val < rule.threshold
          if (inAlert && val < (rule.threshold + rule.hysteresis)) {
            shouldAlert = true
          }
        }

        if (shouldAlert && !inAlert) {
          // New alert triggered
          const event: AlertEvent = {
            id: `${Date.now()}-${Math.random().toString(36).slice(2, 6)}`,
            ruleId: rule.id,
            signal: rule.signal,
            value: val,
            threshold: rule.threshold,
            severity: rule.severity,
            condition: `${rule.condition} ${rule.threshold}`,
            timestamp: new Date().toISOString(),
            acknowledged: false,
            device: (data as any).mcu_id,
          }
          setAlertHistory(prev => [event, ...prev])
          alertState.set(rule.id, true)

          // Browser notification
          if (Notification.permission === 'granted') {
            new Notification(`Alert: ${rule.signal}`, {
              body: `Value ${val.toFixed(2)} is ${rule.condition} ${rule.threshold}`,
              icon: '/favicon.svg',
            })
          }

          // Sound
          if (soundEnabled && rule.severity === 'critical') {
            try {
              const ctx = new AudioContext()
              const osc = ctx.createOscillator()
              osc.frequency.value = 800
              osc.connect(ctx.destination)
              osc.start()
              setTimeout(() => { osc.stop(); ctx.close() }, 200)
            } catch {}
          }
        } else if (!shouldAlert && inAlert) {
          // Alert resolved
          alertState.set(rule.id, false)
          setAlertHistory(prev =>
            prev.map(a => a.ruleId === rule.id && !a.resolvedAt
              ? { ...a, resolvedAt: new Date().toISOString() }
              : a
            )
          )
        }
      })
    })

    return unsub
  }, [rules, soundEnabled])

  // Request notification permission
  useEffect(() => {
    if (Notification.permission === 'default') {
      Notification.requestPermission()
    }
  }, [])

  const addRule = () => {
    const rule: AlertRule = {
      id: `rule-${Date.now()}`,
      signal: newRule.signal || SIGNALS[0]?.name || '',
      condition: newRule.condition || 'above',
      threshold: newRule.threshold || 50,
      severity: newRule.severity || 'warning',
      enabled: true,
      hysteresis: newRule.hysteresis || 2,
    }
    setRules(prev => [...prev, rule])
    setShowRuleForm(false)
    setNewRule({ signal: SIGNALS[0]?.name || '', condition: 'above', threshold: 50, severity: 'warning', hysteresis: 2, enabled: true })
  }

  const deleteRule = (id: string) => {
    setRules(prev => prev.filter(r => r.id !== id))
  }

  const acknowledgeAlert = (id: string) => {
    setAlertHistory(prev =>
      prev.map(a => a.id === id ? { ...a, acknowledged: true } : a)
    )
  }

  const activeAlerts = alertHistory.filter(a => !a.resolvedAt && !a.acknowledged)
  const filteredHistory = filterSeverity === 'all'
    ? alertHistory
    : alertHistory.filter(a => a.severity === filterSeverity)

  const exportAlerts = () => {
    const csv = [
      'timestamp,signal,value,threshold,severity,condition,acknowledged,resolved',
      ...alertHistory.map(a =>
        `${a.timestamp},${a.signal},${a.value},${a.threshold},${a.severity},${a.condition},${a.acknowledged},${a.resolvedAt || ''}`
      )
    ].join('\n')
    const blob = new Blob([csv], { type: 'text/csv' })
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `alerts_${new Date().toISOString().slice(0, 10)}.csv`
    a.click()
    URL.revokeObjectURL(url)
  }

  return (
    <div className="p-4 lg:p-6 space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold">Alerts</h1>
          <p className="text-sm text-muted-foreground">
            {activeAlerts.length} active alert{activeAlerts.length !== 1 ? 's' : ''}
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Button variant="outline" size="sm" onClick={() => setSoundEnabled(!soundEnabled)}>
            {soundEnabled ? <Volume2 className="w-4 h-4" /> : <VolumeX className="w-4 h-4" />}
          </Button>
          <Button variant="outline" size="sm" onClick={exportAlerts}>
            <Download className="w-4 h-4 mr-1" /> Export
          </Button>
          <Button size="sm" onClick={() => setShowRuleForm(true)}>
            <Plus className="w-4 h-4 mr-1" /> Add Rule
          </Button>
        </div>
      </div>

      {/* Tabs */}
      <div className="flex gap-1 border-b border-border">
        {(['active', 'history', 'rules'] as const).map(tab => (
          <button
            key={tab}
            onClick={() => setActiveTab(tab)}
            className={cn(
              'px-4 py-2 text-sm font-medium border-b-2 transition-colors',
              activeTab === tab
                ? 'border-primary text-foreground'
                : 'border-transparent text-muted-foreground hover:text-foreground'
            )}
          >
            {tab === 'active' && `Active (${activeAlerts.length})`}
            {tab === 'history' && `History (${alertHistory.length})`}
            {tab === 'rules' && `Rules (${rules.length})`}
          </button>
        ))}
      </div>

      {/* Active Alerts */}
      {activeTab === 'active' && (
        <div className="space-y-3">
          {activeAlerts.length === 0 ? (
            <Card>
              <CardContent className="flex flex-col items-center py-12 text-muted-foreground">
                <Bell className="w-12 h-12 mb-3 opacity-30" />
                <p>No active alerts</p>
                <p className="text-xs mt-1">All systems operating within thresholds</p>
              </CardContent>
            </Card>
          ) : (
            activeAlerts.map(alert => {
              const cfg = SEVERITY_CONFIG[alert.severity]
              return (
                <Card key={alert.id} className={cn('border-l-4', {
                  'border-l-blue-500': alert.severity === 'info',
                  'border-l-amber-500': alert.severity === 'warning',
                  'border-l-red-500': alert.severity === 'critical',
                })}>
                  <CardContent className="flex items-center gap-4 p-4">
                    <div className={cn('p-2 rounded-lg', cfg.bg, cfg.color)}>
                      {cfg.icon}
                    </div>
                    <div className="flex-1 min-w-0">
                      <p className="font-medium text-sm">
                        {alert.signal} is {alert.condition}
                      </p>
                      <p className="text-xs text-muted-foreground">
                        Value: {alert.value.toFixed(2)} | Threshold: {alert.threshold} | {new Date(alert.timestamp).toLocaleString()}
                      </p>
                    </div>
                    <Button variant="outline" size="sm" onClick={() => acknowledgeAlert(alert.id)}>
                      <Check className="w-4 h-4 mr-1" /> Acknowledge
                    </Button>
                  </CardContent>
                </Card>
              )
            })
          )}
        </div>
      )}

      {/* Alert History */}
      {activeTab === 'history' && (
        <div className="space-y-3">
          <div className="flex items-center gap-2">
            <Filter className="w-4 h-4 text-muted-foreground" />
            {(['all', 'info', 'warning', 'critical'] as const).map(sev => (
              <Button
                key={sev}
                variant={filterSeverity === sev ? 'default' : 'outline'}
                size="sm"
                className="text-xs"
                onClick={() => setFilterSeverity(sev)}
              >
                {sev === 'all' ? 'All' : sev.charAt(0).toUpperCase() + sev.slice(1)}
              </Button>
            ))}
          </div>
          <div className="overflow-x-auto">
            <table className="w-full text-sm">
              <thead>
                <tr className="border-b border-border">
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Time</th>
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Signal</th>
                  <th className="text-right py-2 px-3 font-medium text-muted-foreground">Value</th>
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Severity</th>
                  <th className="text-left py-2 px-3 font-medium text-muted-foreground">Status</th>
                </tr>
              </thead>
              <tbody>
                {filteredHistory.slice(0, 100).map(alert => (
                  <tr key={alert.id} className="border-b border-border/50 hover:bg-muted/30">
                    <td className="py-2 px-3 text-xs font-mono">
                      {new Date(alert.timestamp).toLocaleString()}
                    </td>
                    <td className="py-2 px-3">{alert.signal}</td>
                    <td className="py-2 px-3 text-right font-mono">{alert.value.toFixed(2)}</td>
                    <td className="py-2 px-3">
                      <Badge variant="outline" className={cn('text-[10px]', SEVERITY_CONFIG[alert.severity].color)}>
                        {alert.severity}
                      </Badge>
                    </td>
                    <td className="py-2 px-3">
                      {alert.resolvedAt
                        ? <Badge variant="outline" className="text-[10px] text-green-500">Resolved</Badge>
                        : alert.acknowledged
                          ? <Badge variant="outline" className="text-[10px]">Acknowledged</Badge>
                          : <Badge variant="destructive" className="text-[10px]">Active</Badge>
                      }
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      )}

      {/* Rules */}
      {activeTab === 'rules' && (
        <div className="space-y-3">
          {rules.map(rule => {
            const sig = SIGNALS.find(s => s.name === rule.signal)
            return (
              <Card key={rule.id}>
                <CardContent className="flex items-center gap-4 p-4">
                  <Switch
                    checked={rule.enabled}
                    onCheckedChange={(v) => setRules(prev => prev.map(r => r.id === rule.id ? { ...r, enabled: v } : r))}
                  />
                  <div className="flex-1">
                    <p className="text-sm font-medium">
                      {sig?.display_name || rule.signal} {rule.condition} {rule.threshold} {sig?.unit || ''}
                    </p>
                    <p className="text-xs text-muted-foreground">
                      Severity: {rule.severity} • Hysteresis: {rule.hysteresis}
                    </p>
                  </div>
                  <Badge variant="outline" className={cn('text-[10px]', SEVERITY_CONFIG[rule.severity].color)}>
                    {rule.severity}
                  </Badge>
                  <Button variant="ghost" size="sm" onClick={() => deleteRule(rule.id)}>
                    <Trash2 className="w-4 h-4 text-destructive" />
                  </Button>
                </CardContent>
              </Card>
            )
          })}

          {/* Add rule form */}
          {showRuleForm && (
            <Card className="border-primary">
              <CardContent className="p-4 space-y-3">
                <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
                  <div>
                    <Label className="text-xs">Signal</Label>
                    <select
                      className="w-full h-9 rounded-md border border-input bg-background px-3 text-sm"
                      value={newRule.signal}
                      onChange={(e) => setNewRule(prev => ({ ...prev, signal: e.target.value }))}
                    >
                      {SIGNALS.map(s => (
                        <option key={s.name} value={s.name}>{s.display_name || s.name}</option>
                      ))}
                    </select>
                  </div>
                  <div>
                    <Label className="text-xs">Condition</Label>
                    <select
                      className="w-full h-9 rounded-md border border-input bg-background px-3 text-sm"
                      value={newRule.condition}
                      onChange={(e) => setNewRule(prev => ({ ...prev, condition: e.target.value as any }))}
                    >
                      <option value="above">Above</option>
                      <option value="below">Below</option>
                    </select>
                  </div>
                  <div>
                    <Label className="text-xs">Threshold</Label>
                    <Input
                      type="number"
                      value={newRule.threshold}
                      onChange={(e) => setNewRule(prev => ({ ...prev, threshold: parseFloat(e.target.value) || 0 }))}
                    />
                  </div>
                  <div>
                    <Label className="text-xs">Severity</Label>
                    <select
                      className="w-full h-9 rounded-md border border-input bg-background px-3 text-sm"
                      value={newRule.severity}
                      onChange={(e) => setNewRule(prev => ({ ...prev, severity: e.target.value as Severity }))}
                    >
                      <option value="info">Info</option>
                      <option value="warning">Warning</option>
                      <option value="critical">Critical</option>
                    </select>
                  </div>
                </div>
                <div className="flex gap-2">
                  <Button size="sm" onClick={addRule}>Save Rule</Button>
                  <Button size="sm" variant="outline" onClick={() => setShowRuleForm(false)}>Cancel</Button>
                </div>
              </CardContent>
            </Card>
          )}
        </div>
      )}
    </div>
  )
}
