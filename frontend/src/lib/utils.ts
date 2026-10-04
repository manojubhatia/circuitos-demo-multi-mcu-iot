import { clsx, type ClassValue } from "clsx"
import { twMerge } from "tailwind-merge"

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs))
}

export function formatValue(value: unknown, type: string, unit?: string): string {
  if (value === undefined || value === null) return '--'
  if (type === 'bool' || type === 'boolean') return value ? 'ON' : 'OFF'
  if (typeof value === 'number') {
    const formatted = value.toFixed(2)
    return unit ? `${formatted} ${unit}` : formatted
  }
  return String(value)
}

export function formatTimestamp(timestamp: string | Date): string {
  const date = typeof timestamp === 'string' ? new Date(timestamp) : timestamp
  return date.toLocaleTimeString()
}
