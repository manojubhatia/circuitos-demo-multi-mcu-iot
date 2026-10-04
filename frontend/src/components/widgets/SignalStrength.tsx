/**
 * Signal Strength Widget - WiFi/RSSI bars indicator
 */

import { cn } from '@/lib/utils'

interface SignalStrengthProps {
  rssi?: number  // dBm value (-30 to -100)
  level?: number  // 0-4 bars directly
  label?: string
  className?: string
}

export function SignalStrength({ rssi, level, label, className }: SignalStrengthProps) {
  // Convert RSSI to bars (0-4)
  const bars = level !== undefined ? level : (() => {
    if (!rssi || rssi === 0) return 0
    if (rssi >= -50) return 4
    if (rssi >= -60) return 3
    if (rssi >= -70) return 2
    if (rssi >= -80) return 1
    return 0
  })()

  const getColor = () => {
    if (bars >= 3) return '#22c55e'
    if (bars >= 2) return '#f59e0b'
    if (bars >= 1) return '#ef4444'
    return '#6b7280'
  }

  return (
    <div className={cn('flex items-end gap-0.5', className)}>
      {[1, 2, 3, 4].map(i => (
        <div
          key={i}
          className={cn(
            'w-1.5 rounded-sm transition-colors',
            i <= bars ? '' : 'opacity-20'
          )}
          style={{
            height: `${4 + i * 3}px`,
            backgroundColor: i <= bars ? getColor() : '#6b7280',
          }}
        />
      ))}
      {(rssi !== undefined || label) && (
        <span className="text-[10px] text-muted-foreground ml-1">
          {label || (rssi ? `${rssi}dBm` : '')}
        </span>
      )}
    </div>
  )
}
