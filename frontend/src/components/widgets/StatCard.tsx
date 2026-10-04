/**
 * Stat Card Widget - Big number with trend indicator and sparkline
 */

import { useMemo } from 'react'
import { Card, CardContent } from '@/components/ui/card'
import { cn } from '@/lib/utils'
import { TrendingUp, TrendingDown, Minus } from 'lucide-react'

interface StatCardProps {
  label: string
  value: number | string
  unit?: string
  history?: number[]
  precision?: number
  icon?: React.ReactNode
  className?: string
}

export function StatCard({ label, value, unit, history = [], precision = 1, icon, className }: StatCardProps) {
  const trend = useMemo(() => {
    if (history.length < 2) return 'stable'
    const last = history[history.length - 1]
    const prev = history[history.length - 2]
    if (last > prev * 1.01) return 'up'
    if (last < prev * 0.99) return 'down'
    return 'stable'
  }, [history])

  const sparkline = useMemo(() => {
    if (history.length < 2) return null
    const min = Math.min(...history)
    const max = Math.max(...history)
    const range = max - min || 1
    const w = 60
    const h = 20
    const points = history.slice(-20).map((v, i, arr) => {
      const x = (i / (arr.length - 1)) * w
      const y = h - ((v - min) / range) * h
      return `${x},${y}`
    }).join(' ')
    return { points, w, h }
  }, [history])

  const displayValue = typeof value === 'number' ? value.toFixed(precision) : value

  return (
    <Card className={cn('bg-card', className)}>
      <CardContent className="p-4">
        <div className="flex items-start justify-between mb-2">
          <p className="text-[11px] text-muted-foreground uppercase tracking-wider font-medium truncate">
            {label}
          </p>
          {icon && <div className="text-muted-foreground">{icon}</div>}
        </div>
        <div className="flex items-end gap-2">
          <span className="text-2xl font-bold font-mono leading-none">
            {displayValue}
          </span>
          {unit && <span className="text-xs text-muted-foreground mb-0.5">{unit}</span>}
          <div className="ml-auto">
            {trend === 'up' && <TrendingUp className="w-4 h-4 text-green-500" />}
            {trend === 'down' && <TrendingDown className="w-4 h-4 text-red-500" />}
            {trend === 'stable' && <Minus className="w-4 h-4 text-muted-foreground" />}
          </div>
        </div>
        {sparkline && (
          <svg width={sparkline.w} height={sparkline.h} className="mt-2 opacity-50">
            <polyline
              fill="none"
              stroke={trend === 'up' ? '#22c55e' : trend === 'down' ? '#ef4444' : '#6b7280'}
              strokeWidth="1.5"
              points={sparkline.points}
            />
          </svg>
        )}
      </CardContent>
    </Card>
  )
}
