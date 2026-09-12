export type AppTab = 'dashboard' | 'camera' | 'settings';

export type ToastKind = 'success' | 'danger' | 'warning' | 'info';

export interface AppSettings {
  deviceIp: string;
  refreshRate: number;
  criticalLevel: number;
  warningLevel: number;
}

export interface SystemStatus {
  water_level: number;
  distance: number;
  drain_open: boolean;
  latitude: number;
  longitude: number;
  satellites: number;
}

export interface ServoPositions {
  base: number;
  shoulder: number;
  elbow: number;
  gripper: number;
}

export const DEFAULT_SETTINGS: AppSettings = {
  deviceIp: '192.168.4.1',
  refreshRate: 5,
  criticalLevel: 20,
  warningLevel: 50,
};

export const DEFAULT_SERVO_POSITIONS: ServoPositions = {
  base: 330,
  shoulder: 150,
  elbow: 300,
  gripper: 410,
};
