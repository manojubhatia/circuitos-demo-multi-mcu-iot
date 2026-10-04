/**
 * Multi-Value Card - Compact card showing 2-4 values in a grid
 */

import { Card, CardContent } from '@/components/ui/card'
import { cn } from '@/lib/utils'

interface ValueItem {
  label: string
  value: number | string
  unit?: string
  color?: string
}

interface MultiValueCardProps {
  title?: string
  values: ValueItem[]
  columns?: 2 | 3 | 4
  className?: string
}

export function MultiValueCard({ title, values, columns = 2, className }: MultiValueCardProps) {
  return (
    <Card className={cn('bg-card', className)}>
      <CardContent className="p-3">
        {title && (
          <p className="text-[10px] text-muted-foreground uppercase tracking-wider font-medium mb-2">{title}</p>
        )}
        <div className={cn('grid gap-2', {
          'grid-cols-2': columns === 2,
          'grid-cols-3': columns === 3,
          'grid-cols-4': columns === 4,
        })}>
          {values.map((item, i) => (
            <div key={i} className="min-w-0">
              <p className="text-[9px] text-muted-foreground truncate">{item.label}</p>
              <p className="text-sm font-bold font-mono" style={item.color ? { color: item.color } : undefined}>
                {typeof item.value === 'number' ? item.value.toFixed(1) : item.value}
                {item.unit && <span className="text-[10px] text-muted-foreground ml-0.5">{item.unit}</span>}
              </p>
            </div>
          ))}
        </div>
      </CardContent>
    </Card>
  )
}
