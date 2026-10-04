/**
 * Application Layout with Sidebar Navigation
 * Generated for theSpeaker (3 boards)
 */

import { useState, useEffect, useCallback } from 'react'
import { Link, useLocation } from 'react-router-dom'
import { cn } from '@/lib/utils'
import {
  LayoutDashboard,
  Cpu,
  Bell,
  History,
  Settings,
  Download,
  ChevronLeft,
  ChevronRight,
  Moon,
  Sun,
  Radio,
  Activity,
  Menu,
  X,
  Smartphone,
  WifiOff,
} from 'lucide-react'

interface NavItem {
  path: string
  label: string
  icon: React.ReactNode
  badge?: number
}


export default function Layout({ children, alertCount = 0 }: { children: React.ReactNode; alertCount?: number }) {
  const location = useLocation()
  const [collapsed, setCollapsed] = useState(false)
  const [mobileOpen, setMobileOpen] = useState(false)
  const [isOnline, setIsOnline] = useState(navigator.onLine)
  const [installPrompt, setInstallPrompt] = useState<any>(null)
  const [showInstallBanner, setShowInstallBanner] = useState(false)
  const [darkMode, setDarkMode] = useState(() => {
    if (typeof window !== 'undefined') {
      return localStorage.getItem('theme') !== 'light'
    }
    return true
  })

  // PWA install prompt
  useEffect(() => {
    const handler = (e: Event) => {
      e.preventDefault()
      setInstallPrompt(e)
      const dismissed = localStorage.getItem('pwa-install-dismissed')
      const isStandalone = window.matchMedia('(display-mode: standalone)').matches
      if (!dismissed && !isStandalone) {
        setShowInstallBanner(true)
      }
    }
    window.addEventListener('beforeinstallprompt', handler)
    return () => window.removeEventListener('beforeinstallprompt', handler)
  }, [])

  const handleInstall = useCallback(async () => {
    if (!installPrompt) return
    installPrompt.prompt()
    const result = await installPrompt.userChoice
    if (result.outcome === 'accepted') {
      setShowInstallBanner(false)
    }
    setInstallPrompt(null)
  }, [installPrompt])

  const dismissInstall = useCallback(() => {
    setShowInstallBanner(false)
    localStorage.setItem('pwa-install-dismissed', Date.now().toString())
  }, [])

  // Online/offline detection
  useEffect(() => {
    const onOnline = () => setIsOnline(true)
    const onOffline = () => setIsOnline(false)
    window.addEventListener('online', onOnline)
    window.addEventListener('offline', onOffline)
    return () => {
      window.removeEventListener('online', onOnline)
      window.removeEventListener('offline', onOffline)
    }
  }, [])

  useEffect(() => {
    if (darkMode) {
      document.documentElement.classList.add('dark')
      localStorage.setItem('theme', 'dark')
    } else {
      document.documentElement.classList.remove('dark')
      localStorage.setItem('theme', 'light')
    }
  }, [darkMode])

  // Close mobile menu on route change
  useEffect(() => {
    setMobileOpen(false)
  }, [location.pathname])

  const navItems: NavItem[] = [
    { path: '/', label: 'Dashboard', icon: <LayoutDashboard className="w-5 h-5" /> },
    { path: '/controls', label: 'Device Controls', icon: <LayoutDashboard className="w-5 h-5" /> },

    { path: '/overview', label: 'Overview', icon: <Cpu className="w-5 h-5" /> },
    { path: '/alerts', label: 'Alerts', icon: <Bell className="w-5 h-5" />, badge: alertCount },
    { path: '/history', label: 'History', icon: <History className="w-5 h-5" /> },
    { path: '/protocol', label: 'Protocol', icon: <Activity className="w-5 h-5" /> },
    { path: '/export', label: 'Export', icon: <Download className="w-5 h-5" /> },
    { path: '/settings', label: 'Settings', icon: <Settings className="w-5 h-5" /> },
  ]

  const isActive = (path: string) => {
    if (path === '/') return location.pathname === '/'
    return location.pathname.startsWith(path)
  }

  return (
    <div className="flex h-screen overflow-hidden bg-background">
      {/* Mobile overlay */}
      {mobileOpen && (
        <div
          className="fixed inset-0 z-40 bg-black/50 lg:hidden"
          onClick={() => setMobileOpen(false)}
        />
      )}

      {/* Sidebar */}
      <aside
        className={cn(
          'fixed lg:static inset-y-0 left-0 z-50 flex flex-col border-r border-border bg-card transition-all duration-300',
          collapsed ? 'w-16' : 'w-56',
          mobileOpen ? 'translate-x-0' : '-translate-x-full lg:translate-x-0'
        )}
      >
        {/* Header */}
        <div className="flex items-center h-14 px-3 border-b border-border">
          {!collapsed && (
            <div className="flex items-center gap-2 flex-1 min-w-0">
              <Radio className="w-5 h-5 text-primary shrink-0" />
              <span className="text-sm font-semibold truncate">theSpeaker (3 boards)</span>

            </div>
          )}
          {collapsed && <Radio className="w-5 h-5 text-primary mx-auto" />}
          <button
            onClick={() => setCollapsed(!collapsed)}
            className="hidden lg:flex items-center justify-center w-6 h-6 rounded hover:bg-muted ml-auto"
          >
            {collapsed ? <ChevronRight className="w-4 h-4" /> : <ChevronLeft className="w-4 h-4" />}
          </button>
        </div>

        {/* Navigation */}
        <nav className="flex-1 overflow-y-auto py-2 px-2 space-y-1">
          {navItems.map((item) => (
            <Link
              key={item.path}
              to={item.path}
              className={cn(
                'flex items-center gap-3 px-3 py-2 rounded-md text-sm transition-colors relative',
                isActive(item.path)
                  ? 'bg-primary/10 text-primary font-medium'
                  : 'text-muted-foreground hover:text-foreground hover:bg-muted'
              )}
            >
              {item.icon}
              {!collapsed && <span className="truncate">{item.label}</span>}
              {item.badge && item.badge > 0 && (
                <span className={cn(
                  'flex items-center justify-center min-w-[18px] h-[18px] rounded-full bg-destructive text-destructive-foreground text-[10px] font-bold',
                  collapsed ? 'absolute -top-1 -right-1' : 'ml-auto'
                )}>
                  {item.badge > 99 ? '99+' : item.badge}
                </span>
              )}
            </Link>
          ))}
        </nav>

        {/* Footer */}
        <div className="border-t border-border p-2">
          <button
            onClick={() => setDarkMode(!darkMode)}
            className="flex items-center gap-3 px-3 py-2 rounded-md text-sm text-muted-foreground hover:text-foreground hover:bg-muted w-full"
          >
            {darkMode ? <Sun className="w-5 h-5" /> : <Moon className="w-5 h-5" />}
            {!collapsed && <span>{darkMode ? 'Light Mode' : 'Dark Mode'}</span>}
          </button>
        </div>
      </aside>

      {/* Main content */}
      <main className="flex-1 flex flex-col overflow-hidden">
        {/* Top bar (mobile) */}
        <header className="flex lg:hidden items-center h-14 px-4 border-b border-border bg-card">
          <button
            onClick={() => setMobileOpen(true)}
            className="p-2 -ml-2 rounded-md hover:bg-muted"
          >
            <Menu className="w-5 h-5" />
          </button>
          <span className="ml-3 text-sm font-semibold">theSpeaker (3 boards)</span>

        </header>

        {/* Page content */}
        <div className="flex-1 overflow-y-auto">
          {children}
        </div>

        {/* Offline indicator */}
        {!isOnline && (
          <div className="fixed bottom-0 inset-x-0 bg-yellow-600 text-white text-center py-1.5 px-4 text-sm flex items-center justify-center gap-2 z-50">
            <WifiOff className="w-4 h-4" />
            Offline — showing cached data
          </div>
        )}

        {/* PWA install banner */}
        {showInstallBanner && (
          <div className="fixed bottom-0 inset-x-0 bg-primary text-primary-foreground p-3 flex items-center justify-between gap-3 z-50 safe-bottom shadow-lg">
            <div className="flex items-center gap-3 min-w-0">
              <Smartphone className="w-5 h-5 shrink-0" />
              <span className="text-sm truncate">Install this app for quick access</span>
            </div>
            <div className="flex items-center gap-2 shrink-0">
              <button
                onClick={dismissInstall}
                className="text-sm opacity-70 hover:opacity-100 px-2 py-1"
              >
                Later
              </button>
              <button
                onClick={handleInstall}
                className="text-sm font-semibold bg-white/20 hover:bg-white/30 rounded-md px-3 py-1"
              >
                Install
              </button>
            </div>
          </div>
        )}
      </main>
    </div>
  )
}
