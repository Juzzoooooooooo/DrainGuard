import type {ServoPositions, SystemStatus} from '../types';

const REQUEST_TIMEOUT_MS = 6000;

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null;
}

function readFiniteNumber(
  value: Record<string, unknown>,
  field: keyof SystemStatus,
) {
  const result = value[field];
  if (typeof result !== 'number' || !Number.isFinite(result)) {
    throw new Error('The device returned an invalid status response.');
  }

  return result;
}

export function normalizeDeviceAddress(value: string) {
  return value
    .trim()
    .replace(/^https?:\/\//i, '')
    .replace(/\/+$/, '');
}

export function getBaseUrl(deviceAddress: string) {
  return `http://${normalizeDeviceAddress(deviceAddress)}`;
}

export function isValidDeviceAddress(value: string) {
  const normalized = normalizeDeviceAddress(value);
  if (!normalized || /[\s/?#]/.test(normalized)) {
    return false;
  }

  const portMatch = normalized.match(/:(\d+)$/);
  return !portMatch || Number(portMatch[1]) <= 65535;
}

export function parseSystemStatus(value: unknown): SystemStatus {
  if (!isRecord(value) || typeof value.drain_open !== 'boolean') {
    throw new Error('The device returned an invalid status response.');
  }

  return {
    water_level: readFiniteNumber(value, 'water_level'),
    distance: readFiniteNumber(value, 'distance'),
    drain_open: value.drain_open,
    latitude: readFiniteNumber(value, 'latitude'),
    longitude: readFiniteNumber(value, 'longitude'),
    satellites: readFiniteNumber(value, 'satellites'),
  };
}

export class DrainGuardApi {
  private readonly baseUrl: string;

  constructor(deviceAddress: string) {
    this.baseUrl = getBaseUrl(deviceAddress);
  }

  private async request<T>(
    path: string,
    method: 'GET' | 'POST' = 'GET',
    timeoutMs = REQUEST_TIMEOUT_MS,
  ) {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const response = await fetch(`${this.baseUrl}${path}`, {
        method,
        headers: {'Content-Type': 'application/json'},
        signal: controller.signal,
      });

      if (!response.ok) {
        throw new Error(`Device returned HTTP ${response.status}.`);
      }

      const text = await response.text();
      return (text ? JSON.parse(text) : {}) as T;
    } catch (error) {
      if (error instanceof SyntaxError) {
        throw new Error('The device returned an invalid response.');
      }

      throw error;
    } finally {
      clearTimeout(timeout);
    }
  }

  async getSystemStatus() {
    const result = await this.request<unknown>('/api/status');
    return parseSystemStatus(result);
  }

  controlArm(action: 'open' | 'close') {
    // The firmware sends its response after the smooth multi-servo sequence,
    // which can legitimately take more than the normal six-second timeout.
    return this.request(`/api/arm/${action}`, 'POST', 15000);
  }

  controlServo(servo: keyof ServoPositions, position: number) {
    return this.request(
      `/api/servo/${servo}?position=${Math.round(position)}`,
      'POST',
    );
  }

  async getCameraStreamUrl() {
    // ESP32-CAM streams directly at its static IP — no API call needed
    // First verify it's reachable via the main controller's camera status
    try {
      const result = await this.request<{camera_available?: boolean}>(
        '/api/camera/status',
      );
      if (result.camera_available === false) {
        return '';
      }
    } catch {
      return '';
    }
    // Return the direct MJPEG stream URL from the ESP32-CAM
    return 'http://192.168.4.50/stream';
  }
}
