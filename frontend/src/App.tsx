import { Toaster } from '@/components/ui/sonner'
import { BrowserRouter, Routes, Route } from 'react-router-dom'
import { useState, useEffect } from 'react'
import Layout from '@/components/Layout'
import Overview from '@/pages/Overview'
import DeviceDetail from '@/pages/DeviceDetail'
import Dashboard from '@/pages/Dashboard'
import Alerts from '@/pages/Alerts'
import HistoryPage from '@/pages/HistoryPage'
import ProtocolViewer from '@/pages/ProtocolViewer'
import ExportPage from '@/pages/ExportPage'
import SettingsPage from '@/pages/SettingsPage'
import Controls from '@/pages/Controls'

function App() {
  const [alertCount, setAlertCount] = useState(0)

  useEffect(() => {
    document.documentElement.classList.add('dark')
  }, [])

  // Listen for alert count from localStorage (shared with Alerts page)
  useEffect(() => {
    const check = () => {
      try {
        const history = JSON.parse(localStorage.getItem('alert-history') || '[]')
        const active = history.filter((a: any) => !a.resolvedAt && !a.acknowledged)
        setAlertCount(active.length)
      } catch {}
    }
    check()
    const interval = setInterval(check, 5000)
    return () => clearInterval(interval)
  }, [])

  return (
    <BrowserRouter>
      <Layout alertCount={alertCount}>
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route path="/controls" element={<Controls />} />
          <Route path="/overview" element={<Overview />} />
          <Route path="/devices/:deviceId" element={<DeviceDetail />} />
          <Route path="/alerts" element={<Alerts />} />
          <Route path="/history" element={<HistoryPage />} />
          <Route path="/protocol" element={<ProtocolViewer />} />
          <Route path="/export" element={<ExportPage />} />
          <Route path="/settings" element={<SettingsPage />} />
        </Routes>
      </Layout>
      <Toaster />
    </BrowserRouter>
  )
}

export default App