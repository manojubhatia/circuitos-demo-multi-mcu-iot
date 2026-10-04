/**
 * Event Log Widget - Scrollable event feed with auto-scroll
 */

import { useRef, useEffect, useState } from 'react'
import { cn } from '@/lib/utils'
import { Badge } from '@/components/ui/badge'

export type LogSeverity = 'info' | 'warning' | 'error' | 'success'

export interface LogEntry {
  id: string
  timestamp: string
  message: string
  severity: LogSeverity
  source?: string
}

interface EventLogProps {
  entries: LogEntry[]
  maxEntries?: number
  autoScroll?: boolean
  className?: string
}

const SEVERITY_STYLES: Record<LogSeverity, string> = {
  info: 'text-blue-400',
  warning: 'text-amber-400',
  error: 'text-red-400',
  success: 'text-green-400',
}

export function EventLog({ entries, maxEntries = 100, autoScroll = true, className }: EventLogProps) {
  const scrollRef = useRef<HTMLDivElement>(null)
  const [pinned, setPinned] = useState(true)

  useEffect(() => {
    if (autoScroll && pinned && scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight
    }
  }, [entries, pinned, autoScroll])

  const handleScroll = () => {
    if (!scrollRef.current) return
    const { scrollTop, scrollHeight, clientHeight } = scrollRef.current
    setPinned(scrollHeight - scrollTop - clientHeight < 30)
  }

  const visibleEntries = entries.slice(-maxEntries)

  return (
    <div className={cn('flex flex-col h-full', className)}>
      <div
        ref={scrollRef}
        onScroll={handleScroll}
        className="flex-1 overflow-y-auto font-mono text-xs space-y-0.5 p-2"
      >
        {visibleEntries.map(entry => (
          <div key={entry.id} className="flex items-start gap-2 py-0.5 hover:bg-muted/30 px-1 rounded">
            <span className="text-muted-foreground shrink-0 w-16">
              {new Date(entry.timestamp).toLocaleTimeString()}
            </span>
            <span className={cn('shrink-0 w-1 h-3 rounded-full mt-0.5', {
              'bg-blue-400': entry.severity === 'info',
              'bg-amber-400': entry.severity === 'warning',
              'bg-red-400': entry.severity === 'error',
              'bg-green-400': entry.severity === 'success',
            })} />
            {entry.source && (
              <span className="text-muted-foreground shrink-0">[{entry.source}]</span>
            )}
            <span className={cn(SEVERITY_STYLES[entry.severity])}>
              {entry.message}
            </span>
          </div>
        ))}
        {visibleEntries.length === 0 && (
          <p className="text-muted-foreground text-center py-4">No events</p>
        )}
      </div>
      {!pinned && (
        <button
          onClick={() => { setPinned(true); scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight, behavior: 'smooth' }) }}
          className="text-[10px] text-center py-1 bg-primary/10 text-primary hover:bg-primary/20 transition-colors"
        >
          ↓ New events below — click to scroll
        </button>
      )}
    </div>
  )
}
