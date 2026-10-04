/**
 * Status LED Widget - Colored indicator for boolean/state values
 */

import { cn } from '@/lib/utils'

interface StatusLEDProps {
  state: boolean | 'on' | 'off' | 'warning' | 'error' | 'idle'
  label?: string
  size?: 'sm' | 'md' | 'lg'
  className?: string
}

const STATE_COLORS: Record<string, string> = {
  on: 'bg-green-500 shadow-green-500/50',
  off: 'bg-gray-400',
  true: 'bg-green-500 shadow-green-500/50',
  false: 'bg-gray-400',
  warning: 'bg-amber-500 shadow-amber-500/50',
  error: 'bg-red-500 shadow-red-500/50',
  idle: 'bg-blue-400 shadow-blue-400/50',
}

const SIZE_MAP = {
  sm: 'w-2 h-2',
  md: 'w-3 h-3',
  lg: 'w-4 h-4',
}

export function StatusLED({ state, label, size = 'md', className }: StatusLEDProps) {
  const stateKey = String(state)
  const colorClass = STATE_COLORS[stateKey] || STATE_COLORS['off']
  const isActive = state === true || state === 'on' || state === 'warning' || state === 'error'

  return (
    <div className={cn('flex items-center gap-2', className)}>
      <div className={cn(
        'rounded-full transition-all',
        SIZE_MAP[size],
        colorClass,
        isActive && 'shadow-md animate-pulse'
      )} />
      {label && <span className="text-xs text-muted-foreground">{label}</span>}
    </div>
  )
}
