/**
 * Connection Bar - Persistent status strip showing connection health
 */

import { useState, useEffect, useRef } from 'react'
import { cn } from '@/lib/utils'
import { wsService } from '@/services/websocket'
import { Wifi, WifiOff, Loader2, Activity } from 'lucide-react'

type ConnectionState = 'connected' | 'connecting' | 'reconnecting' | 'offline'

export function ConnectionBar() {
  const [state, setState] = useState<ConnectionState>('connecting')
  const [messageRate, setMessageRate] = useState(0)
  const [latency, setLatency] = useState<number | null>(null)
  const msgCountRef = useRef(0)

  useEffect(() => {
    const unsub1 = wsService.onConnectionChange((connected) => {
      setState(connected ? 'connected' : 'reconnecting')
    })

    const unsub2 = wsService.onMessage(() => {
      msgCountRef.current++
    })

    // Calculate message rate every second
    const interval = setInterval(() => {
      setMessageRate(msgCountRef.current)
      msgCountRef.current = 0
    }, 1000)

    // Latency ping
    const pingInterval = setInterval(() => {
      const start = Date.now()
      try {
        wsService.send({ type: 'ping', ts: start })
      } catch {}
    }, 10000)

    return () => {
      unsub1()
      unsub2()
      clearInterval(interval)
      clearInterval(pingInterval)
    }
  }, [])

  const stateConfig: Record<ConnectionState, { color: string; icon: React.ReactNode; label: string }> = {
    connected: { color: 'text-green-500', icon: <Wifi className="w-3 h-3" />, label: 'Connected' },
    connecting: { color: 'text-amber-500', icon: <Loader2 className="w-3 h-3 animate-spin" />, label: 'Connecting...' },
    reconnecting: { color: 'text-amber-500', icon: <Loader2 className="w-3 h-3 animate-spin" />, label: 'Reconnecting...' },
    offline: { color: 'text-red-500', icon: <WifiOff className="w-3 h-3" />, label: 'Offline' },
  }

  const config = stateConfig[state]

  return (
    <div className="fixed bottom-0 left-0 right-0 h-6 bg-card border-t border-border flex items-center px-4 gap-4 text-[10px] z-40">
      <div className={cn('flex items-center gap-1', config.color)}>
        {config.icon}
        <span>{config.label}</span>
      </div>
      {state === 'connected' && (
        <>
          <div className="flex items-center gap-1 text-muted-foreground">
            <Activity className="w-3 h-3" />
            <span>{messageRate} msg/s</span>
          </div>
          {latency !== null && (
            <div className="text-muted-foreground">
              {latency}ms
            </div>
          )}
        </>
      )}
      <div className="ml-auto text-muted-foreground">
        v{wsService.protocolVersion}
      </div>
    </div>
  )
}
