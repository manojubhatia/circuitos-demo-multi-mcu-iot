/**
 * Auto-generated API Service Tests - theSpeaker (3 boards)
 * Run with: npx vitest run
 */
import { describe, it, expect, vi, beforeEach } from 'vitest';
import { api } from '../services/api';

const mockFetch = vi.fn();
global.fetch = mockFetch;

describe('API Service', () => {
  beforeEach(() => {
    mockFetch.mockClear();
  });

  it('should check API health', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ status: 'ok' }),
    });
    const result = await api.getHealth();
    expect(result).toBeDefined();
  });

  it('should fetch environment_aht10_temperature data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'environment_aht10_temperature', value: 0 }),
    });
    const result = await api.getSignal('environment_aht10_temperature');
    expect(result).toBeDefined();
  });

  it('should fetch environment_aht10_humidity data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'environment_aht10_humidity', value: 0 }),
    });
    const result = await api.getSignal('environment_aht10_humidity');
    expect(result).toBeDefined();
  });

  it('should fetch motor_n_mike_inmp441_audio_samples data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'motor_n_mike_inmp441_audio_samples', value: 0 }),
    });
    const result = await api.getSignal('motor_n_mike_inmp441_audio_samples');
    expect(result).toBeDefined();
  });

  it('should fetch motor_n_mike_inmp441_audio_level data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'motor_n_mike_inmp441_audio_level', value: 0 }),
    });
    const result = await api.getSignal('motor_n_mike_inmp441_audio_level');
    expect(result).toBeDefined();
  });

  it('should fetch motor_n_mike_inmp441_peak_amplitude data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'motor_n_mike_inmp441_peak_amplitude', value: 0 }),
    });
    const result = await api.getSignal('motor_n_mike_inmp441_peak_amplitude');
    expect(result).toBeDefined();
  });

  it('should fetch motor_n_mike_inmp441_rms_level data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'motor_n_mike_inmp441_rms_level', value: 0 }),
    });
    const result = await api.getSignal('motor_n_mike_inmp441_rms_level');
    expect(result).toBeDefined();
  });

  it('should fetch motor_n_mike_inmp441_is_clipping data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'motor_n_mike_inmp441_is_clipping', value: 0 }),
    });
    const result = await api.getSignal('motor_n_mike_inmp441_is_clipping');
    expect(result).toBeDefined();
  });

  it('should fetch thespeaker_bluetoothspeaker_connected data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'thespeaker_bluetoothspeaker_connected', value: 0 }),
    });
    const result = await api.getSignal('thespeaker_bluetoothspeaker_connected');
    expect(result).toBeDefined();
  });

  it('should fetch thespeaker_bluetoothspeaker_device_name data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'thespeaker_bluetoothspeaker_device_name', value: 0 }),
    });
    const result = await api.getSignal('thespeaker_bluetoothspeaker_device_name');
    expect(result).toBeDefined();
  });

  it('should fetch thespeaker_bluetoothspeaker_rssi data', async () => {
    mockFetch.mockResolvedValueOnce({
      ok: true,
      json: async () => ({ name: 'thespeaker_bluetoothspeaker_rssi', value: 0 }),
    });
    const result = await api.getSignal('thespeaker_bluetoothspeaker_rssi');
    expect(result).toBeDefined();
  });

});
