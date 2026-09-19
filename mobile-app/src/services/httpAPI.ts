/**
 * HTTP API Service for DrainGuard (Hotspot-Only Version)
 * 
 * Phone connects to ESP32 hotspot → uses HTTP API for all controls
 * No Bluetooth needed!
 */

const API_BASE_URL = 'http://192.168.4.1/api';
const REQUEST_TIMEOUT = 5000; // 5 seconds

export interface SystemStatus {
  device_id: string;
  water_level: number;
  distance: number;
  drain_open: boolean;
  camera_available: boolean;
  camera_streaming: boolean;
  camera_quality: number;
  latitude: number;
  longitude: number;
  satellites: number;
  gps_valid: boolean;
  uptime_s: number;
  free_heap: number;
  clients_connected: number;
}

export interface ServoStatus {
  base: number;
  shoulder: number;
  elbow: number;
  gripper: number;
}

export interface AutoModeStatus {
  enabled: boolean;
  operating: boolean;
  detection_range: number;
}

export interface GPSStatus {
  latitude: number;
  longitude: number;
  satellites: number;
  valid: boolean;
}

export interface CameraStatus {
  available: boolean;
  streaming: boolean;
  quality: number;
  brightness: number;
  contrast: number;
  flash: boolean;
}

class HTTPAPIService {
  private baseURL: string;

  constructor(baseURL: string = API_BASE_URL) {
    this.baseURL = baseURL;
  }

  /**
   * Fetch with timeout
   */
  private async fetchWithTimeout(
    url: string,
    options: RequestInit = {},
    timeout: number = REQUEST_TIMEOUT
  ): Promise<Response> {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), timeout);

    try {
      const response = await fetch(url, {
        ...options,
        signal: controller.signal,
      });
      clearTimeout(timeoutId);
      return response;
    } catch (error) {
      clearTimeout(timeoutId);
      throw error;
    }
  }

  /**
   * GET request helper
   */
  private async get<T>(endpoint: string): Promise<T> {
    const response = await this.fetchWithTimeout(`${this.baseURL}${endpoint}`, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
      },
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    return response.json();
  }

  /**
   * POST request helper — supports query string params for servo endpoints
   */
  private async post<T>(endpoint: string, data?: any): Promise<T> {
    const response = await this.fetchWithTimeout(`${this.baseURL}${endpoint}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: data ? JSON.stringify(data) : undefined,
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    return response.json();
  }

  // ============================================================================
  // SYSTEM STATUS
  // ============================================================================

  /**
   * Get complete system status
   */
  async getStatus(): Promise<SystemStatus> {
    return this.get<SystemStatus>('/status');
  }

  // ============================================================================
  // MOTOR / WHEEL CONTROL
  // ============================================================================

  /** Drive forward */
  async motorForward(): Promise<{status: string}> {
    return this.post('/motor/forward');
  }

  /** Drive backward */
  async motorBackward(): Promise<{status: string}> {
    return this.post('/motor/backward');
  }

  /** Turn left */
  async motorLeft(): Promise<{status: string}> {
    return this.post('/motor/left');
  }

  /** Turn right */
  async motorRight(): Promise<{status: string}> {
    return this.post('/motor/right');
  }

  /** Stop all motors */
  async motorStop(): Promise<{status: string}> {
    return this.post('/motor/stop');
  }

  // ============================================================================
  // DRAIN CONTROL
  // ============================================================================

  /**
   * Open drain (motor control)
   */
  async openDrain(): Promise<{status: string}> {
    return this.post('/drain/open');
  }

  /**
   * Close drain (motor control)
   */
  async closeDrain(): Promise<{status: string}> {
    return this.post('/drain/close');
  }

  // ============================================================================
  // ARM CONTROL
  // ============================================================================

  /**
   * Open drain with robotic arm
   */
  async armOpen(): Promise<{status: string}> {
    return this.post('/arm/open');
  }

  /**
   * Close drain with robotic arm
   */
  async armClose(): Promise<{status: string}> {
    return this.post('/arm/close');
  }

  /**
   * Move arm to home position
   */
  async armHome(): Promise<{status: string}> {
    return this.post('/arm/home');
  }

  // ============================================================================
  // SERVO CONTROL
  // ============================================================================

  /**
   * Get current servo positions
   */
  async getServoStatus(): Promise<ServoStatus> {
    return this.get<ServoStatus>('/servo/status');
  }

  /**
   * Move base servo
   */
  async moveBase(position: number): Promise<{status: string; position: number}> {
    return this.post(`/servo/base?position=${position}`);
  }

  /**
   * Move shoulder servo
   */
  async moveShoulder(position: number): Promise<{status: string; position: number}> {
    return this.post(`/servo/shoulder?position=${position}`);
  }

  /**
   * Move elbow servo
   */
  async moveElbow(position: number): Promise<{status: string; position: number}> {
    return this.post(`/servo/elbow?position=${position}`);
  }

  /**
   * Move gripper servo
   */
  async moveGripper(position: number): Promise<{status: string; position: number}> {
    return this.post(`/servo/gripper?position=${position}`);
  }

  // ============================================================================
  // AUTO MODE
  // ============================================================================

  /**
   * Start automatic drain opening mode
   */
  async autoStart(): Promise<{status: string}> {
    return this.post('/auto/start');
  }

  /**
   * Stop automatic mode
   */
  async autoStop(): Promise<{status: string}> {
    return this.post('/auto/stop');
  }

  /**
   * Get auto mode status
   */
  async getAutoStatus(): Promise<AutoModeStatus> {
    return this.get<AutoModeStatus>('/auto/status');
  }

  /**
   * Set detection range for auto mode
   */
  async setAutoRange(range: number): Promise<{status: string; range: number}> {
    return this.post(`/auto/range?value=${range}`);
  }

  // ============================================================================
  // GPS
  // ============================================================================

  /**
   * Get GPS coordinates
   */
  async getGPS(): Promise<GPSStatus> {
    return this.get<GPSStatus>('/gps');
  }

  // ============================================================================
  // CAMERA
  // ============================================================================

  /**
   * Get camera status
   */
  async getCameraStatus(): Promise<CameraStatus> {
    return this.get<CameraStatus>('/camera/status');
  }

  /**
   * Capture photo
   */
  async cameraCapture(): Promise<{status: string}> {
    return this.post('/camera/capture');
  }

  /**
   * Start camera stream
   */
  async cameraStreamStart(): Promise<{status: string}> {
    return this.post('/camera/stream/start');
  }

  /**
   * Stop camera stream
   */
  async cameraStreamStop(): Promise<{status: string}> {
    return this.post('/camera/stream/stop');
  }

  // ============================================================================
  // CONNECTION TEST
  // ============================================================================

  /**
   * Test if ESP32 is reachable
   */
  async testConnection(): Promise<boolean> {
    try {
      await this.get('/status');
      return true;
    } catch (error) {
      return false;
    }
  }
}

// Export singleton instance
export const httpAPI = new HTTPAPIService();
export default httpAPI;
