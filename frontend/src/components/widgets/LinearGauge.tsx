/**
 * Linear Gauge Widget - Horizontal/vertical bar with range markers and zones
 */

import { cn } from '@/lib/utils'

interface LinearGaugeProps {
  value: number
  min?: number
  max?: number
  unit?: string
  label?: string
  orientation?: 'horizontal' | 'vertical'
  zones?: { from: number; to: number; color: string }[]
  showValue?: boolean
  height?: number
  className?: string
}

export function LinearGauge({
  value,
  min = 0,
  max = 100,
  unit = '',
  label = '',
  orientation = 'horizontal',
  zones,
  showValue = true,
  height = 24,
  className,
}: LinearGaugeProps) {
  const clamped = Math.max(min, Math.min(max, value))
  const ratio = ((clamped - min) / (max - min)) * 100

  const defaultZones = zones || [
    { from: 0, to: 60, color: '#22c55e' },
    { from: 60, to: 80, color: '#f59e0b' },
    { from: 80, to: 100, color: '#ef4444' },
  ]

  const getZoneColor = (pct: number) => {
    for (const zone of defaultZones) {
      if (pct >= zone.from && pct <= zone.to) return zone.color
    }
    return '#3b82f6'
  }

  const fillColor = getZoneColor(ratio)

  if (orientation === 'vertical') {
    return (
      <div className={cn('flex flex-col items-center gap-1', className)}>
        {label && <span className="text-[10px] text-muted-foreground">{label}</span>}
        <div className="relative w-6 bg-muted rounded-full overflow-hidden" style={{ height: `${height * 4}px` }}>
          <div
            className="absolute bottom-0 left-0 right-0 rounded-full transition-all duration-300"
            style={{ height: `${ratio}%`, backgroundColor: fillColor }}
          />
        </div>
        {showValue && (
          <span className="text-xs font-mono">{value.toFixed(1)}{unit && ` ${unit}`}</span>
        )}
      </div>
    )
  }

  return (
    <div className={cn('w-full', className)}>
      {(label || showValue) && (
        <div className="flex items-center justify-between mb-1">
          {label && <span className="text-xs text-muted-foreground">{label}</span>}
          {showValue && <span className="text-xs font-mono">{value.toFixed(1)}{unit && ` ${unit}`}</span>}
        </div>
      )}
      <div className="relative w-full bg-muted rounded-full overflow-hidden" style={{ height: `${height}px` }}>
        {/* Zone backgrounds */}
        {defaultZones.map((zone, i) => (
          <div
            key={i}
            className="absolute top-0 bottom-0 opacity-10"
            style={{
              left: `${zone.from}%`,
              width: `${zone.to - zone.from}%`,
              backgroundColor: zone.color,
            }}
          />
        ))}
        {/* Fill bar */}
        <div
          className="absolute top-0 bottom-0 left-0 rounded-full transition-all duration-300"
          style={{ width: `${ratio}%`, backgroundColor: fillColor }}
        />
        {/* Needle indicator */}
        <div
          className="absolute top-0 bottom-0 w-0.5 bg-foreground transition-all duration-300"
          style={{ left: `${ratio}%` }}
        />
      </div>
      {/* Min/Max labels */}
      <div className="flex justify-between mt-0.5">
        <span className="text-[9px] text-muted-foreground">{min}</span>
        <span className="text-[9px] text-muted-foreground">{max}</span>
      </div>
    </div>
  )
}
