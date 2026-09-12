import {
  NativeEventEmitter,
  NativeModules,
  PermissionsAndroid,
  Platform,
} from 'react-native';

export interface DrainGuardBleDevice {
  id: string;
  name: string;
  rssi: number;
}

export interface DrainGuardBleState {
  state: string;
  message: string;
}

export interface DrainGuardWifiNetwork {
  ssid: string;
  rssi: number;
  secure: boolean;
}

export interface DrainGuardProvisioningStatus {
  status: string;
  message?: string;
  reason?: string;
  device?: string;
  configured?: boolean;
  hotspot_ip?: string;
  ssid?: string;
  ip?: string;
  saved?: boolean;
  count?: number;
  rssi?: number;
  secure?: boolean;
}

interface DrainGuardBleNativeModule {
  isSupported(): Promise<boolean>;
  startScan(): Promise<void>;
  stopScan(): Promise<void>;
  connect(deviceId: string): Promise<{id: string; name: string}>;
  disconnect(): Promise<void>;
  openBluetoothSettings(): Promise<void>;
  scanWifi(): Promise<void>;
  forgetWifi(): Promise<void>;
  provisionWifi(
    ssid: string,
    password: string,
    apiEndpoint: string,
  ): Promise<void>;
}

type Subscription = {remove: () => void};

const nativeModule = NativeModules.DrainGuardBle as
  | DrainGuardBleNativeModule
  | undefined;
const eventEmitter = nativeModule
  ? new NativeEventEmitter(NativeModules.DrainGuardBle)
  : null;

function requireAndroidModule() {
  if (Platform.OS !== 'android' || !nativeModule) {
    throw new Error('DrainGuard Bluetooth setup is only available on Android.');
  }
  return nativeModule;
}

async function requestBluetoothPermissions() {
  if (Platform.OS !== 'android') {
    return false;
  }

  const androidVersion = Number(Platform.Version);
  const permissions =
    androidVersion >= 31
      ? [
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        ]
      : [PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION];
  const results = await PermissionsAndroid.requestMultiple(permissions);
  return permissions.every(
    permission => results[permission] === PermissionsAndroid.RESULTS.GRANTED,
  );
}

function subscribe<T>(eventName: string, listener: (event: T) => void) {
  if (!eventEmitter) {
    return {remove: () => undefined} as Subscription;
  }
  return eventEmitter.addListener(eventName, listener);
}

export const bleProvisioning = {
  async startScan() {
    const allowed = await requestBluetoothPermissions();
    if (!allowed) {
      throw new Error('Bluetooth permission is required to find DrainGuard.');
    }
    return requireAndroidModule().startScan();
  },

  stopScan() {
    return requireAndroidModule().stopScan();
  },

  async connect(deviceId: string) {
    const allowed = await requestBluetoothPermissions();
    if (!allowed) {
      throw new Error(
        'Bluetooth permission is required to connect to DrainGuard.',
      );
    }
    return requireAndroidModule().connect(deviceId);
  },

  disconnect() {
    return requireAndroidModule().disconnect();
  },

  openBluetoothSettings() {
    return requireAndroidModule().openBluetoothSettings();
  },

  scanWifi() {
    return requireAndroidModule().scanWifi();
  },

  forgetWifi() {
    return requireAndroidModule().forgetWifi();
  },

  provisionWifi(ssid: string, password: string) {
    return requireAndroidModule().provisionWifi(ssid, password, '');
  },

  onDevice(listener: (device: DrainGuardBleDevice) => void) {
    return subscribe('DrainGuardBleDevice', listener);
  },

  onState(listener: (state: DrainGuardBleState) => void) {
    return subscribe('DrainGuardBleState', listener);
  },

  onStatus(listener: (status: DrainGuardProvisioningStatus) => void) {
    return subscribe<{payload: string}>('DrainGuardBleStatus', event => {
      try {
        const parsed = JSON.parse(event.payload) as unknown;
        if (
          typeof parsed === 'object' &&
          parsed !== null &&
          typeof (parsed as DrainGuardProvisioningStatus).status === 'string'
        ) {
          listener(parsed as DrainGuardProvisioningStatus);
        }
      } catch {
        // Ignore incomplete or malformed controller notifications.
      }
    });
  },
};
