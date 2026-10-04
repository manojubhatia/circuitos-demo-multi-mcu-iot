/**
 * API Service for theSpeaker (3 boards)
 * Generated from dataflow configuration
 */

const API_BASE_URL = import.meta.env.VITE_API_URL || '/api'

export interface SignalMetadata {
  name: string
  type: string
  unit: string
  source: string
  display_name?: string
  direction?: 'output' | 'input' | 'bidirectional'  // For bidirectional IO simulation
}

export interface SensorReading {
  timestamp: string
  'environment_aht10_temperature'?: number
  'environment_aht10_humidity'?: number
  'motor_n_mike_inmp441_audio_samples'?: number
  'motor_n_mike_inmp441_audio_level'?: number
  'motor_n_mike_inmp441_peak_amplitude'?: number
  'motor_n_mike_inmp441_rms_level'?: number
  'motor_n_mike_inmp441_is_clipping'?: boolean
  'thespeaker_bluetoothspeaker_connected'?: boolean
  'thespeaker_bluetoothspeaker_device_name'?: string
  'thespeaker_bluetoothspeaker_rssi'?: number
}

export interface ApiResponse<T> {
  data: T
  status: string
  message?: string
}

class ApiService {
  private baseUrl: string

  constructor(baseUrl: string = API_BASE_URL) {
    this.baseUrl = baseUrl
  }

  private async request<T>(endpoint: string, options: RequestInit = {}): Promise<T> {
    const url = `${this.baseUrl}${endpoint}`
    const response = await fetch(url, {
      ...options,
      headers: {
        'Content-Type': 'application/json',
        ...options.headers,
      },
    })

    if (!response.ok) {
      throw new Error(`API Error: ${response.status} ${response.statusText}`)
    }

    return response.json()
  }

  async getSignals(): Promise<SignalMetadata[]> {
    return this.request<SignalMetadata[]>('/signals')
  }

  async getLatestData(): Promise<SensorReading | null> {
    return this.request<SensorReading | null>('/data/latest')
  }

  async getHistoricalData(limit: number = 100): Promise<{ data: SensorReading[] }> {
    return this.request<{ data: SensorReading[] }>(`/data/history?limit=${limit}`)
  }

  async postData(reading: Partial<SensorReading>): Promise<{ status: string }> {
    return this.request<{ status: string }>('/data', {
      method: 'POST',
      body: JSON.stringify(reading),
    })
  }

  async getHealth(): Promise<{ status: string }> {
    return this.request<{ status: string }>('/')
  }

  // Bidirectional IO simulation methods
  async simulateSignal(name: string, value: boolean | number, active: boolean = true): Promise<{ status: string }> {
    return this.request<{ status: string }>(`/simulate/${name}`, {
      method: 'POST',
      body: JSON.stringify({ value, simulation_active: active }),
    })
  }

  async clearSimulation(name: string): Promise<{ status: string }> {
    return this.request<{ status: string }>(`/simulate/${name}/clear`, {
      method: 'POST',
    })
  }
}

export const api = new ApiService()
export const apiService = api
export default api