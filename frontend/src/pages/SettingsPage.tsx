/**
 * Settings Page - Configuration for connection, refresh rate, and thresholds
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useCallback } from 'react'
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Label } from '@/components/ui/label'
import { Switch } from '@/components/ui/switch'
import { Slider } from '@/components/ui/slider'
import { Badge } from '@/components/ui/badge'
import { cn } from '@/lib/utils'
import {
  Settings,
  Wifi,
  Clock,
  Bell,
  Palette,
  Info,
  Save,
  RotateCcw,
  CheckCircle2,
  Server,
  Monitor,
  Bluetooth,
  Usb,
  Unplug,
  Plug,
} from 'lucide-react'


interface AppSettings {
  refreshRate: number
  autoReconnect: boolean
  maxHistoryPoints: number
  staleTimeout: number
  soundAlerts: boolean
  browserNotifications: boolean
  compactMode: boolean
  showDebugInfo: boolean
  backendUrl: string
  wsUrl: string
}

const DEFAULT_SETTINGS: AppSettings = {
  refreshRate: 1000,
  autoReconnect: true,
  maxHistoryPoints: 300,
  staleTimeout: 30,
  soundAlerts: true,
  browserNotifications: true,
  compactMode: false,
  showDebugInfo: false,
  backendUrl: '',
  wsUrl: '',
}

const SETTINGS_KEY = 'app-settings'


export default function SettingsPage() {
  const [settings, setSettings] = useState<AppSettings>(() => {
    try {
      const saved = localStorage.getItem(SETTINGS_KEY)
      return saved ? { ...DEFAULT_SETTINGS, ...JSON.parse(saved) } : DEFAULT_SETTINGS
    } catch { return DEFAULT_SETTINGS }
  })
  const [saved, setSaved] = useState(false)

  const updateSetting = <K extends keyof AppSettings>(key: K, value: AppSettings[K]) => {
    setSettings(prev => ({ ...prev, [key]: value }))
    setSaved(false)
  }

  const saveSettings = () => {
    localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings))
    setSaved(true)
    setTimeout(() => setSaved(false), 3000)
  }

  const resetSettings = () => {
    setSettings(DEFAULT_SETTINGS)
    localStorage.removeItem(SETTINGS_KEY)
    setSaved(false)
  }

  return (
    <div className="p-4 lg:p-6 space-y-6 max-w-3xl">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-bold">Settings</h1>
          <p className="text-sm text-muted-foreground">Configure dashboard behavior</p>
        </div>
        <div className="flex items-center gap-2">
          <Button variant="outline" size="sm" onClick={resetSettings}>
            <RotateCcw className="w-4 h-4 mr-1" /> Reset
          </Button>
          <Button size="sm" onClick={saveSettings}>
            <Save className="w-4 h-4 mr-1" /> Save
          </Button>
          {saved && (
            <Badge variant="outline" className="text-green-500 border-green-500">
              <CheckCircle2 className="w-3 h-3 mr-1" /> Saved
            </Badge>
          )}
        </div>
      </div>

      {/* Connection */}
      <Card>
        <CardHeader>
          <CardTitle className="text-sm flex items-center gap-2">
            <Server className="w-4 h-4" /> Connection
          </CardTitle>
          <CardDescription>Backend and WebSocket configuration</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div>
            <Label className="text-xs">Backend URL (leave empty for default)</Label>
            <Input
              value={settings.backendUrl}
              onChange={(e) => updateSetting('backendUrl', e.target.value)}
              placeholder="http://localhost:8000/api"
            />
          </div>
          <div>
            <Label className="text-xs">WebSocket URL (leave empty for default)</Label>
            <Input
              value={settings.wsUrl}
              onChange={(e) => updateSetting('wsUrl', e.target.value)}
              placeholder="ws://localhost:8000/ws"
            />
          </div>
          <div className="flex items-center justify-between">
            <div>
              <Label>Auto-reconnect</Label>
              <p className="text-xs text-muted-foreground">Automatically reconnect on connection loss</p>
            </div>
            <Switch
              checked={settings.autoReconnect}
              onCheckedChange={(v) => updateSetting('autoReconnect', v)}
            />
          </div>
          <div>
            <Label className="text-xs">Stale timeout (seconds)</Label>
            <p className="text-xs text-muted-foreground mb-2">Mark device offline after this many seconds without data</p>
            <Slider
              value={[settings.staleTimeout]}
              min={10}
              max={120}
              step={5}
              onValueChange={([v]) => updateSetting('staleTimeout', v)}
            />
            <span className="text-xs font-mono">{settings.staleTimeout}s</span>
          </div>
        </CardContent>
      </Card>

      {/* Display */}
      <Card>
        <CardHeader>
          <CardTitle className="text-sm flex items-center gap-2">
            <Monitor className="w-4 h-4" /> Display
          </CardTitle>
          <CardDescription>Visual preferences</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div>
            <Label className="text-xs">Chart history buffer (points)</Label>
            <Slider
              value={[settings.maxHistoryPoints]}
              min={50}
              max={1000}
              step={50}
              onValueChange={([v]) => updateSetting('maxHistoryPoints', v)}
            />
            <span className="text-xs font-mono">{settings.maxHistoryPoints} points</span>
          </div>
          <div className="flex items-center justify-between">
            <div>
              <Label>Compact mode</Label>
              <p className="text-xs text-muted-foreground">Reduce spacing for smaller screens</p>
            </div>
            <Switch
              checked={settings.compactMode}
              onCheckedChange={(v) => updateSetting('compactMode', v)}
            />
          </div>
          <div className="flex items-center justify-between">
            <div>
              <Label>Show debug info</Label>
              <p className="text-xs text-muted-foreground">Show raw message count, latency</p>
            </div>
            <Switch
              checked={settings.showDebugInfo}
              onCheckedChange={(v) => updateSetting('showDebugInfo', v)}
            />
          </div>
        </CardContent>
      </Card>

      {/* Notifications */}
      <Card>
        <CardHeader>
          <CardTitle className="text-sm flex items-center gap-2">
            <Bell className="w-4 h-4" /> Notifications
          </CardTitle>
          <CardDescription>Alert notification preferences</CardDescription>
        </CardHeader>
        <CardContent className="space-y-4">
          <div className="flex items-center justify-between">
            <div>
              <Label>Sound alerts</Label>
              <p className="text-xs text-muted-foreground">Play sound on critical alerts</p>
            </div>
            <Switch
              checked={settings.soundAlerts}
              onCheckedChange={(v) => updateSetting('soundAlerts', v)}
            />
          </div>
          <div className="flex items-center justify-between">
            <div>
              <Label>Browser notifications</Label>
              <p className="text-xs text-muted-foreground">Show system notifications (even when tab is in background)</p>
            </div>
            <Switch
              checked={settings.browserNotifications}
              onCheckedChange={(v) => updateSetting('browserNotifications', v)}
            />
          </div>
        </CardContent>
      </Card>



      {/* System Info */}
      <Card>
        <CardHeader>
          <CardTitle className="text-sm flex items-center gap-2">
            <Info className="w-4 h-4" /> System Information
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-2 gap-3 text-sm">
            <div>
              <p className="text-muted-foreground text-xs">Project</p>
              <p className="font-medium">theSpeaker (3 boards)</p>

            </div>
            <div>
              <p className="text-muted-foreground text-xs">Generated</p>
              <p className="font-medium">2026-09-29 13:56</p>

            </div>
            <div>
              <p className="text-muted-foreground text-xs">Frontend Version</p>
              <p className="font-medium">1.0.0</p>
            </div>
            <div>
              <p className="text-muted-foreground text-xs">Protocol Version</p>
              <p className="font-medium">1.0.0</p>

            </div>
          </div>
        </CardContent>
      </Card>
    </div>
  )
}
