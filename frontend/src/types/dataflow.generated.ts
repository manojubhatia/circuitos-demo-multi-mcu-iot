/**
 * Auto-generated TypeScript types for 
 * Protocol Version: 1.0.0
 * Schema Hash: 00000000
 * Generated: 
 *
 * DO NOT EDIT - This file is regenerated from data_flow_frames.json
 */

// Protocol version for compatibility checking
export const PROTOCOL_VERSION = '1.0.0';
export const SCHEMA_HASH = '00000000';

// Base types
export type VariableId = string;
export type RequestId = string;
export type Timestamp = number;
export type Sequence = number;

// Frame roles
export enum FrameRole {
}

// ============================================================================
// Frame Payload Types - Generated from data_flow_frames.json
// ============================================================================


// ============================================================================
// Signal Types - Variables streamed from devices
// ============================================================================

export interface EnvironmentAht10TemperatureSignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}
export interface EnvironmentAht10HumiditySignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}
export interface MotorNMikeInmp441AudioSamplesSignal {
  variable_id: string;
  value: buffer;
  timestamp: number;
  sequence?: number;
}
export interface MotorNMikeInmp441AudioLevelSignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}
export interface MotorNMikeInmp441PeakAmplitudeSignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}
export interface MotorNMikeInmp441RmsLevelSignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}
export interface MotorNMikeInmp441IsClippingSignal {
  variable_id: string;
  value: boolean;
  timestamp: number;
  sequence?: number;
}
export interface ThespeakerBluetoothspeakerConnectedSignal {
  variable_id: string;
  value: boolean;
  timestamp: number;
  sequence?: number;
}
export interface ThespeakerBluetoothspeakerDeviceNameSignal {
  variable_id: string;
  value: string;
  timestamp: number;
  sequence?: number;
}
export interface ThespeakerBluetoothspeakerRssiSignal {
  variable_id: string;
  value: number;
  timestamp: number;
  sequence?: number;
}

// ============================================================================
// Parameter Types - Configurable values sent to devices
// ============================================================================


// ============================================================================
// Aggregated Types
// ============================================================================

export interface SensorReadingTyped {
  timestamp: string;
  environment_aht10_temperature?: number;
  environment_aht10_humidity?: number;
  motor_n_mike_inmp441_audio_samples?: buffer;
  motor_n_mike_inmp441_audio_level?: number;
  motor_n_mike_inmp441_peak_amplitude?: number;
  motor_n_mike_inmp441_rms_level?: number;
  motor_n_mike_inmp441_is_clipping?: boolean;
  thespeaker_bluetoothspeaker_connected?: boolean;
  thespeaker_bluetoothspeaker_device_name?: string;
  thespeaker_bluetoothspeaker_rssi?: number;
}

export interface ParameterValuesTyped {
}

// ============================================================================
// Protocol Frame Names - Maps frame IDs to human-readable names
// Generated from data_flow_frames.json protocol model
// ============================================================================

export const PROTOCOL_FRAME_NAMES: Record<number, { name: string; role: string; source: string; target: string; variables: string[] }> = {
  256: { name: 'heartbeat_board_0_to_backend', role: 'heartbeat', source: 'board_0', target: 'system_backend', variables: [] },
  257: { name: 'heartbeat_board_1_to_backend', role: 'heartbeat', source: 'board_1', target: 'system_backend', variables: [] },
  258: { name: 'heartbeat_board_5_to_backend', role: 'heartbeat', source: 'board_5', target: 'system_backend', variables: [] },
};

export const PROTOCOL_FRAME_TYPES: Record<number, string> = {
  0x01: 'DATA',
  0x02: 'REQUEST',
  0x03: 'BEGIN',
  0x04: 'CONTINUE',
  0x05: 'END',
  0x06: 'ACK',
  0x07: 'NACK',
  0x08: 'START_STREAM',
  0x09: 'STOP_STREAM',
  0x0A: 'HEARTBEAT',
};

/**
 * Get human-readable name for a protocol frame
 */
export function getFrameName(frameId: number): string {
  const info = PROTOCOL_FRAME_NAMES[frameId];
  return info ? info.name : `frame_0x${frameId.toString(16).padStart(4, '0')}`;
}

/**
 * Get all frame IDs associated with a variable
 */
export function getFrameIdsForVariable(variableId: string): number[] {
  return Object.entries(PROTOCOL_FRAME_NAMES)
    .filter(([_, info]) => info.variables.includes(variableId))
    .map(([id, _]) => parseInt(id));
}

// Type guard for checking protocol compatibility
export function isCompatibleProtocol(remoteVersion: string, remoteHash: string): boolean {
  const [remoteMajor] = remoteVersion.split('.').map(Number);
  const [localMajor] = PROTOCOL_VERSION.split('.').map(Number);
  
  // Major version must match
  if (remoteMajor !== localMajor) {
    console.warn(`Protocol version mismatch: local ${PROTOCOL_VERSION} vs remote ${remoteVersion}`);
    return false;
  }
  
  // Schema hash mismatch is a warning, not an error
  if (remoteHash !== SCHEMA_HASH) {
    console.warn(`Schema hash mismatch: local ${SCHEMA_HASH} vs remote ${remoteHash}`);
  }
  
  return true;
}