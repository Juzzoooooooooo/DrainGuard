import AsyncStorage from '@react-native-async-storage/async-storage';

import {AppSettings, DEFAULT_SETTINGS} from '../types';

const SETTINGS_KEY = '@drain_guard/settings';

function toFiniteNumber(value: unknown, fallback: number) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

export async function loadSettings(): Promise<AppSettings> {
  const saved = await AsyncStorage.getItem(SETTINGS_KEY);

  if (!saved) {
    return DEFAULT_SETTINGS;
  }

  try {
    const parsed = JSON.parse(saved) as Partial<AppSettings>;
    return {
      deviceIp:
        typeof parsed.deviceIp === 'string' && parsed.deviceIp.trim()
          ? parsed.deviceIp
          : DEFAULT_SETTINGS.deviceIp,
      refreshRate: toFiniteNumber(
        parsed.refreshRate,
        DEFAULT_SETTINGS.refreshRate,
      ),
      criticalLevel: toFiniteNumber(
        parsed.criticalLevel,
        DEFAULT_SETTINGS.criticalLevel,
      ),
      warningLevel: toFiniteNumber(
        parsed.warningLevel,
        DEFAULT_SETTINGS.warningLevel,
      ),
    };
  } catch {
    return DEFAULT_SETTINGS;
  }
}

export function saveSettings(settings: AppSettings) {
  return AsyncStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
}
