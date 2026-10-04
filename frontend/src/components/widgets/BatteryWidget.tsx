/**
 * Battery Widget - Visual battery level indicator
 */

import { cn } from '@/lib/utils'

interface BatteryWidgetProps {
  level: number  // 0-100
  charging?: boolean
  voltage?: number
  label?: string
  className?: string
}

export function BatteryWidget({ level, charging = false, voltage, label, className }: BatteryWidgetProps) {
  const clampedLevel = Math.max(0, Math.min(100, level))
  
  const getColor = () => {
    if (clampedLevel <= 10) return '#ef4444'
    if (clampedLevel <= 25) return '#f59e0b'
    if (clampedLevel <= 50) return '#eab308'
    return '#22c55e'
  }

  return (
    <div className={cn('flex items-center gap-2', className)}>
      <div className="relative flex items-center">
        {/* Battery body */}
        <div className="relative w-10 h-5 border-2 border-muted-foreground rounded-sm">
          {/* Fill */}
          <div
            className="absolute top-0.5 bottom-0.5 left-0.5 rounded-[1px] transition-all duration-500"
            style={{ width: `${clampedLevel}%`, backgroundColor: getColor(), maxWidth: 'calc(100% - 4px)' }}
          />
          {/* Charging bolt */}
          {charging && (
            <svg className="absolute inset-0 w-full h-full" viewBox="0 0 40 20">
              <path d="M22 3 L16 10 L20 10 L18 17 L24 10 L20 10 Z" fill="white" opacity={0.9} />
            </svg>
          )}
        </div>
        {/* Battery tip */}
        <div className="w-1 h-2.5 bg-muted-foreground rounded-r-sm" />
      </div>
      <div className="flex flex-col">
        <span className="text-xs font-mono font-medium">{clampedLevel}%</span>
        {voltage !== undefined && (
          <span className="text-[10px] text-muted-foreground">{voltage.toFixed(2)}V</span>
        )}
        {label && <span className="text-[10px] text-muted-foreground">{label}</span>}
      </div>
    </div>
  )
}
