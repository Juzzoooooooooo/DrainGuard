import React, {useCallback, useEffect, useMemo, useState} from 'react';
import {
  BackHandler,
  NativeModules,
  SafeAreaView,
  StatusBar,
  StyleSheet,
  useWindowDimensions,
  View,
} from 'react-native';

import {AppHeader} from './src/components/AppHeader';
import {TabBar} from './src/components/TabBar';
import {Toast} from './src/components/Toast';
import {CameraScreen} from './src/screens/CameraScreen';
import {DashboardScreen} from './src/screens/DashboardScreen';
import {SettingsScreen} from './src/screens/SettingsScreen';
import {DrainGuardApi} from './src/services/api';
import {
  loadSettings as loadSavedSettings,
  saveSettings as persistSettings,
} from './src/services/settingsStorage';
import {colors} from './src/theme';
import {
  AppSettings,
  AppTab,
  DEFAULT_SETTINGS,
  ServoPositions,
  SystemStatus,
  ToastKind,
} from './src/types';

interface ToastState {
  id: number;
  message: string;
  kind: ToastKind;
}

const screenMode = NativeModules.ScreenMode as
  | {setCameraMode?: (enabled: boolean) => void}
  | undefined;

function App() {
  const {height, width} = useWindowDimensions();
  const landscape = width > height;
  const [activeTab, setActiveTab] = useState<AppTab>('dashboard');
  const cameraMode = activeTab === 'camera';
  const [settings, setSettings] = useState<AppSettings>(DEFAULT_SETTINGS);
  const [status, setStatus] = useState<SystemStatus | null>(null);
  const [connected, setConnected] = useState<boolean | null>(null);
  // Track consecutive failures — only show disconnected after 2 in a row
  const failCount = React.useRef(0);
  const [ready, setReady] = useState(false);
  const [refreshing, setRefreshing] = useState(false);
  const [toast, setToast] = useState<ToastState | null>(null);

  const api = useMemo(
    () => new DrainGuardApi(settings.deviceIp),
    [settings.deviceIp],
  );

  const notify = useCallback((message: string, kind: ToastKind = 'info') => {
    setToast({id: Date.now(), message, kind});
  }, []);

  useEffect(() => {
    let mounted = true;

    loadSavedSettings()
      .then(savedSettings => {
        if (mounted) {
          setSettings(savedSettings);
        }
      })
      .catch(() => {
        if (mounted) {
          notify('Saved settings could not be loaded.', 'warning');
        }
      })
      .finally(() => {
        if (mounted) {
          setReady(true);
        }
      });

    return () => {
      mounted = false;
    };
  }, [notify]);

  useEffect(() => {
    screenMode?.setCameraMode?.(cameraMode);

    return () => {
      if (cameraMode) {
        screenMode?.setCameraMode?.(false);
      }
    };
  }, [cameraMode]);

  useEffect(() => {
    if (!cameraMode) {
      return;
    }

    const subscription = BackHandler.addEventListener(
      'hardwareBackPress',
      () => {
        setActiveTab('dashboard');
        return true;
      },
    );

    return () => subscription.remove();
  }, [cameraMode]);

  const refreshStatus = useCallback(async () => {
    try {
      const nextStatus = await api.getSystemStatus();
      setStatus(nextStatus);
      setConnected(true);
      failCount.current = 0;
      return true;
    } catch {
      failCount.current += 1;
      // Only mark as disconnected after 2 consecutive failures.
      // This prevents a single slow response from flipping the indicator.
      if (failCount.current >= 2) {
        setConnected(false);
      }
      return false;
    }
  }, [api]);

  const manualRefresh = useCallback(async () => {
    setRefreshing(true);
    const succeeded = await refreshStatus();
    setRefreshing(false);

    if (!succeeded) {
      notify('Could not reach the Drain Guard controller.', 'danger');
    }
  }, [notify, refreshStatus]);

  useEffect(() => {
    if (!ready) {
      return;
    }

    // Start polling immediately and keep it running across tab switches.
    // Switching tabs no longer resets the interval.
    refreshStatus();
    const interval = setInterval(
      refreshStatus,
      Math.max(settings.refreshRate, 1) * 1000,
    );

    return () => clearInterval(interval);
  }, [ready, refreshStatus, settings.refreshRate]);

  const controlArm = useCallback(
    async (action: 'open' | 'close') => {
      try {
        await api.controlArm(action);
        setConnected(true);
        failCount.current = 0;
        notify(`Arm ${action} command sent.`, 'success');
      } catch (error) {
        // Don't flip connection state on arm command failure — arm sequences
        // take up to 15s and a timeout doesn't mean the device is offline.
        notify(`Failed to run the arm ${action} action.`, 'danger');
        throw error;
      }
    },
    [api, notify],
  );

  const controlServo = useCallback(
    async (servo: keyof ServoPositions, position: number) => {
      try {
        await api.controlServo(servo, position);
        setConnected(true);
        failCount.current = 0;
      } catch (error) {
        // Silently ignore individual servo failures — joystick sends many
        // commands rapidly and occasional drops are expected on WiFi.
        throw error;
      }
    },
    [api],
  );

  const loadStreamUrl = useCallback(async () => {
    try {
      const url = await api.getCameraStreamUrl();
      setConnected(true);
      failCount.current = 0;
      return url;
    } catch (error) {
      failCount.current += 1;
      if (failCount.current >= 2) {
        setConnected(false);
      }
      throw error;
    }
  }, [api]);

  const updateSettings = useCallback(
    async (nextSettings: AppSettings) => {
      try {
        await persistSettings(nextSettings);
        setSettings(nextSettings);
        setConnected(null);
        notify('Settings saved successfully.', 'success');
      } catch {
        notify('Settings could not be saved.', 'danger');
      }
    },
    [notify],
  );

  const useProvisionedDeviceIp = useCallback(
    async (deviceIp: string) => {
      const nextSettings = {...settings, deviceIp};
      await persistSettings(nextSettings);
      setSettings(nextSettings);
      setConnected(null);
    },
    [settings],
  );

  const useHotspotDeviceIp = useCallback(async () => {
    const nextSettings = {
      ...settings,
      deviceIp: DEFAULT_SETTINGS.deviceIp,
    };
    await persistSettings(nextSettings);
    setSettings(nextSettings);
    setConnected(null);
  }, [settings]);

  const toastView = toast ? (
    <Toast
      key={toast.id}
      kind={toast.kind}
      message={toast.message}
      onHide={() =>
        setToast(current => (current?.id === toast.id ? null : current))
      }
    />
  ) : null;

  if (cameraMode) {
    return (
      <View style={styles.cameraFrame}>
        <StatusBar hidden />
        <CameraScreen
          loadStreamUrl={loadStreamUrl}
          notify={notify}
          onArm={controlArm}
          onExit={() => setActiveTab('dashboard')}
          onServo={controlServo}
        />
        {toastView}
      </View>
    );
  }

  return (
    <SafeAreaView style={styles.safeArea}>
      <StatusBar backgroundColor={colors.primary} barStyle="light-content" />
      <View style={styles.shellAccent} />
      <View style={styles.appFrame}>
        <AppHeader compact={landscape} connected={connected} />
        <TabBar
          activeTab={activeTab}
          compact={landscape}
          onChange={setActiveTab}
        />
        <View style={styles.page}>
          {activeTab === 'dashboard' ? (
            <DashboardScreen
              notify={notify}
              onRefresh={manualRefresh}
              refreshing={refreshing}
              settings={settings}
              status={status}
            />
          ) : (
            <SettingsScreen
              notify={notify}
              onForgetWifi={useHotspotDeviceIp}
              onProvisioned={useProvisionedDeviceIp}
              onSave={updateSettings}
              settings={settings}
            />
          )}
        </View>
      </View>

      {toastView}
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  cameraFrame: {
    flex: 1,
    backgroundColor: colors.black,
  },
  safeArea: {
    flex: 1,
    backgroundColor: colors.shell,
  },
  shellAccent: {
    position: 'absolute',
    left: 0,
    right: 0,
    bottom: 0,
    height: '48%',
    backgroundColor: colors.shellDark,
  },
  appFrame: {
    flex: 1,
    width: '100%',
    maxWidth: 960,
    alignSelf: 'center',
    backgroundColor: colors.page,
  },
  page: {
    flex: 1,
    backgroundColor: colors.page,
  },
});

export default App;
