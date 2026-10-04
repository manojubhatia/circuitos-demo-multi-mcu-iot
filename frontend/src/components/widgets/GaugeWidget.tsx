/**
 * Radial Gauge Widget - SVG-based gauge with animated needle
 */

import { useMemo } from 'react'
import { cn } from '@/lib/utils'

interface GaugeWidgetProps {
  value: number
  min?: number
  max?: number
  unit?: string
  label?: string
  size?: number
  zones?: { from: number; to: number; color: string }[]
  className?: string
}

export function GaugeWidget({
  value,
  min = 0,
  max = 100,
  unit = '',
  label = '',
  size = 160,
  zones,
  className,
}: GaugeWidgetProps) {
  const startAngle = -135
  const endAngle = 135
  const totalAngle = endAngle - startAngle

  const clampedValue = Math.max(min, Math.min(max, value))
  const ratio = (clampedValue - min) / (max - min)
  const needleAngle = startAngle + ratio * totalAngle

  const cx = size / 2
  const cy = size / 2
  const radius = size * 0.38

  const defaultZones = zones || [
    { from: 0, to: 0.6, color: '#22c55e' },
    { from: 0.6, to: 0.8, color: '#f59e0b' },
    { from: 0.8, to: 1.0, color: '#ef4444' },
  ]

  const arcPath = (startRatio: number, endRatio: number) => {
    const start = startAngle + startRatio * totalAngle
    const end = startAngle + endRatio * totalAngle
    const startRad = (start * Math.PI) / 180
    const endRad = (end * Math.PI) / 180
    const x1 = cx + radius * Math.cos(startRad)
    const y1 = cy + radius * Math.sin(startRad)
    const x2 = cx + radius * Math.cos(endRad)
    const y2 = cy + radius * Math.sin(endRad)
    const largeArc = end - start > 180 ? 1 : 0
    return `M ${x1} ${y1} A ${radius} ${radius} 0 ${largeArc} 1 ${x2} ${y2}`
  }

  const needleRad = (needleAngle * Math.PI) / 180
  const needleLength = radius * 0.85
  const nx = cx + needleLength * Math.cos(needleRad)
  const ny = cy + needleLength * Math.sin(needleRad)

  return (
    <div className={cn('flex flex-col items-center', className)}>
      <svg width={size} height={size * 0.7} viewBox={`0 0 ${size} ${size * 0.7}`}>
        {/* Zone arcs */}
        {defaultZones.map((zone, i) => (
          <path
            key={i}
            d={arcPath(zone.from, zone.to)}
            fill="none"
            stroke={zone.color}
            strokeWidth={size * 0.06}
            strokeLinecap="round"
            opacity={0.3}
          />
        ))}

        {/* Value arc */}
        <path
          d={arcPath(0, ratio)}
          fill="none"
          stroke="hsl(var(--primary))"
          strokeWidth={size * 0.04}
          strokeLinecap="round"
        />

        {/* Needle */}
        <line
          x1={cx}
          y1={cy}
          x2={nx}
          y2={ny}
          stroke="hsl(var(--foreground))"
          strokeWidth={2}
          strokeLinecap="round"
          style={{ transition: 'all 0.3s ease-out' }}
        />
        <circle cx={cx} cy={cy} r={4} fill="hsl(var(--foreground))" />

        {/* Value text */}
        <text
          x={cx}
          y={cy + size * 0.18}
          textAnchor="middle"
          className="fill-foreground"
          fontSize={size * 0.14}
          fontWeight="bold"
          fontFamily="monospace"
        >
          {typeof value === 'number' ? value.toFixed(1) : value}
        </text>
        <text
          x={cx}
          y={cy + size * 0.28}
          textAnchor="middle"
          className="fill-muted-foreground"
          fontSize={size * 0.07}
        >
          {unit}
        </text>
      </svg>
      {label && <p className="text-xs text-muted-foreground mt-1">{label}</p>}
    </div>
  )
}
