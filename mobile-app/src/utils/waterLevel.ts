import {colors} from '../theme';

export type WaterLevelState = 'normal' | 'warning' | 'critical';

export function clamp(value: number, minimum: number, maximum: number) {
  return Math.min(Math.max(value, minimum), maximum);
}

export function getWaterLevelState(
  distance: number,
  criticalDistance: number,
  warningDistance: number,
): WaterLevelState {
  if (distance <= criticalDistance) {
    return 'critical';
  }

  if (distance <= warningDistance) {
    return 'warning';
  }

  return 'normal';
}

export function getWaterStateColor(state: WaterLevelState) {
  if (state === 'critical') {
    return colors.danger;
  }

  if (state === 'warning') {
    return colors.warning;
  }

  return colors.success;
}
