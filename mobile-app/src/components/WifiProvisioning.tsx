import React, {useEffect, useRef, useState} from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';

import {
  bleProvisioning,
  DrainGuardBleDevice,
  DrainGuardProvisioningStatus,
  DrainGuardWifiNetwork,
} from '../services/bleProvisioning';
import {colors, radii, spacing} from '../theme';
import type {ToastKind} from '../types';
import {ActionButton} from './ActionButton';
import {Card} from './Card';

interface WifiProvisioningProps {
  notify: (message: string, kind?: ToastKind) => void;
  onProvisioned: (deviceIp: string) => Promise<void>;
}

const DEVICE_SCAN_TIMEOUT_MS = 12500;
const WIFI_SETUP_TIMEOUT_MS = 35000;

function errorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Bluetooth operation failed.';
}

function signalLabel(rssi: number) {
  if (rssi >= -60) {
    return 'Strong';
  }
  if (rssi >= -75) {
    return 'Good';
  }
  return 'Weak';
}

export function WifiProvisioning({
  notify,
  onProvisioned,
}: WifiProvisioningProps) {
  const [devices, setDevices] = useState<DrainGuardBleDevice[]>([]);
  const [selectedDevice, setSelectedDevice] =
    useState<DrainGuardBleDevice | null>(null);
  const [connectingId, setConnectingId] = useState<string | null>(null);
  const [scanning, setScanning] = useState(false);
  const [wifiScanning, setWifiScanning] = useState(false);
  const [networks, setNetworks] = useState<DrainGuardWifiNetwork[]>([]);
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [provisioning, setProvisioning] = useState(false);
  const [statusMessage, setStatusMessage] = useState(
    'Scan for your DrainGuard controller to begin.',
  );
  const [connectedWifi, setConnectedWifi] = useState<{
    ssid: string;
    ip?: string;
  } | null>(null);
  const deviceScanTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const wifiSetupTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const provisioningActive = useRef(false);

  useEffect(() => {
    const deviceSubscription = bleProvisioning.onDevice(device => {
      setDevices(current => {
        const withoutDevice = current.filter(item => item.id !== device.id);
        return [...withoutDevice, device].sort(
          (left, right) => right.rssi - left.rssi,
        );
      });
    });
    const stateSubscription = bleProvisioning.onState(state => {
      setStatusMessage(state.message);
      if (state.state === 'scanning') {
        setScanning(true);
      }
      if (state.state === 'connected') {
        setConnectingId(null);
      }
      if (state.state === 'disconnected') {
        setConnectingId(null);
        setSelectedDevice(null);
        setWifiScanning(false);
      }
      if (state.state === 'error') {
        setScanning(false);
        notify(state.message, 'danger');
      }
    });
    const statusSubscription = bleProvisioning.onStatus(
      handleProvisioningStatus,
    );

    return () => {
      deviceSubscription.remove();
      stateSubscription.remove();
      statusSubscription.remove();
      if (deviceScanTimer.current) {
        clearTimeout(deviceScanTimer.current);
      }
      if (wifiSetupTimer.current) {
        clearTimeout(wifiSetupTimer.current);
      }
      bleProvisioning.stopScan().catch(() => undefined);
      bleProvisioning.disconnect().catch(() => undefined);
    };
    // Subscriptions remain stable for this screen's lifetime.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const handleProvisioningStatus = (status: DrainGuardProvisioningStatus) => {
    if (status.status === 'network' && status.ssid) {
      const network: DrainGuardWifiNetwork = {
        ssid: status.ssid,
        rssi: status.rssi ?? -100,
        secure: status.secure ?? true,
      };
      setNetworks(current => {
        const withoutNetwork = current.filter(
          item => item.ssid !== network.ssid,
        );
        return [...withoutNetwork, network].sort(
          (left, right) => right.rssi - left.rssi,
        );
      });
      return;
    }

    if (status.status === 'scanning_wifi') {
      setWifiScanning(true);
      setStatusMessage('DrainGuard is scanning nearby 2.4 GHz Wi-Fi networks.');
      return;
    }
    if (status.status === 'scan_complete') {
      setWifiScanning(false);
      setStatusMessage('Choose a Wi-Fi network or enter its name manually.');
      return;
    }
    if (status.status === 'connecting') {
      setProvisioning(true);
      setStatusMessage(`Connecting DrainGuard to ${status.ssid ?? 'Wi-Fi'}…`);
      return;
    }
    if (status.status === 'connected') {
      setProvisioning(false);
      provisioningActive.current = false;
      if (wifiSetupTimer.current) {
        clearTimeout(wifiSetupTimer.current);
      }
      setConnectedWifi({ssid: status.ssid ?? ssid, ip: status.ip});
      setStatusMessage(
        `Internet connected${status.ip ? ` — ${status.ip}` : ''}`,
      );
      if (status.ssid) {
        notify(`DrainGuard connected to ${status.ssid}.`, 'success');
      }
      if (status.ip) {
        onProvisioned(status.ip).catch(() => undefined);
      }
      return;
    }
    if (status.status === 'failed') {
      setProvisioning(false);
      provisioningActive.current = false;
      if (wifiSetupTimer.current) {
        clearTimeout(wifiSetupTimer.current);
      }
      const reason =
        status.reason === 'authentication_failed'
          ? 'Check the Wi-Fi password.'
          : status.message ?? 'DrainGuard could not join that Wi-Fi network.';
      setStatusMessage(reason);
      notify(reason, 'danger');
      return;
    }
    if (
      status.status === 'invalid' ||
      status.status === 'busy' ||
      status.status === 'scan_failed'
    ) {
      setWifiScanning(false);
      setProvisioning(false);
      const message =
        status.message ?? 'DrainGuard rejected the setup request.';
      setStatusMessage(message);
      notify(message, 'danger');
    }
  };

  const scanForDevices = async () => {
    setDevices([]);
    setSelectedDevice(null);
    setConnectedWifi(null);
    setScanning(true);
    setStatusMessage('Looking for DrainGuard controllers…');
    try {
      await bleProvisioning.startScan();
      if (deviceScanTimer.current) {
        clearTimeout(deviceScanTimer.current);
      }
      deviceScanTimer.current = setTimeout(() => {
        setScanning(false);
        setStatusMessage(current =>
          current.startsWith('Looking')
            ? 'Scan complete. Select a DrainGuard controller.'
            : current,
        );
      }, DEVICE_SCAN_TIMEOUT_MS);
    } catch (error) {
      setScanning(false);
      const message = errorMessage(error);
      setStatusMessage(message);
      notify(message, 'warning');
    }
  };

  const scanWifiNetworks = async () => {
    setNetworks([]);
    setWifiScanning(true);
    setStatusMessage('Asking DrainGuard to scan nearby Wi-Fi…');
    try {
      await bleProvisioning.scanWifi();
    } catch (error) {
      setWifiScanning(false);
      const message = errorMessage(error);
      setStatusMessage(message);
      notify(message, 'danger');
    }
  };

  const connectDevice = async (device: DrainGuardBleDevice) => {
    setConnectingId(device.id);
    setStatusMessage(`Connecting to ${device.name}…`);
    try {
      await bleProvisioning.connect(device.id);
      setSelectedDevice(device);
      setScanning(false);
      notify(`${device.name} connected by Bluetooth.`, 'success');
      await scanWifiNetworks();
    } catch (error) {
      setConnectingId(null);
      const message = errorMessage(error);
      setStatusMessage(message);
      notify(message, 'danger');
    }
  };

  const connectWifi = async () => {
    const cleanSsid = ssid.trim();
    if (!cleanSsid || cleanSsid.length > 32) {
      notify('Enter a Wi-Fi name between 1 and 32 characters.', 'warning');
      return;
    }
    if (password.length > 63) {
      notify('Wi-Fi password cannot exceed 63 characters.', 'warning');
      return;
    }

    provisioningActive.current = true;
    setConnectedWifi(null);
    setProvisioning(true);
    setStatusMessage(`Sending ${cleanSsid} credentials…`);
    try {
      await bleProvisioning.provisionWifi(cleanSsid, password);
      wifiSetupTimer.current = setTimeout(() => {
        if (!provisioningActive.current) {
          return;
        }
        provisioningActive.current = false;
        setProvisioning(false);
        setStatusMessage(
          'Wi-Fi setup timed out. Check the credentials and retry.',
        );
        notify('Wi-Fi setup timed out.', 'danger');
      }, WIFI_SETUP_TIMEOUT_MS);
    } catch (error) {
      provisioningActive.current = false;
      setProvisioning(false);
      const message = errorMessage(error);
      setStatusMessage(message);
      notify(message, 'danger');
    }
  };

  return (
    <Card icon="⌁" title="Bluetooth Wi-Fi Setup">
      <View
        style={[
          styles.statusPanel,
          connectedWifi ? styles.statusPanelSuccess : null,
        ]}>
        <View
          style={[
            styles.statusDot,
            connectedWifi ? styles.statusDotSuccess : null,
          ]}
        />
        <Text style={styles.statusText}>{statusMessage}</Text>
      </View>

      <Text style={styles.stepLabel}>1. CONNECT TO DRAINGUARD</Text>
      <Text style={styles.description}>
        Keep the robot powered on and nearby while the Bluetooth connection is
        established.
      </Text>
      <ActionButton
        disabled={scanning || connectingId !== null}
        icon="⌁"
        label={scanning ? 'Scanning…' : 'Scan for DrainGuard'}
        onPress={scanForDevices}
      />

      {devices.map(device => {
        const connecting = connectingId === device.id;
        const selected = selectedDevice?.id === device.id;
        return (
          <Pressable
            accessibilityRole="button"
            disabled={connectingId !== null || selected}
            key={device.id}
            onPress={() => connectDevice(device)}
            style={({pressed}) => [
              styles.deviceRow,
              selected && styles.deviceRowSelected,
              pressed && styles.rowPressed,
            ]}>
            <View style={styles.deviceCopy}>
              <Text style={styles.deviceName}>{device.name}</Text>
              <Text style={styles.deviceMeta}>
                {signalLabel(device.rssi)} signal · {device.id}
              </Text>
            </View>
            {connecting ? (
              <ActivityIndicator color={colors.primary} size="small" />
            ) : (
              <Text style={styles.connectLabel}>
                {selected ? 'CONNECTED' : 'CONNECT'}
              </Text>
            )}
          </Pressable>
        );
      })}

      {selectedDevice ? (
        <View style={styles.wifiSection}>
          <View style={styles.stepHeadingRow}>
            <Text style={styles.stepLabel}>2. ENTER WI-FI CREDENTIALS</Text>
            <Pressable
              accessibilityRole="button"
              disabled={wifiScanning}
              onPress={scanWifiNetworks}>
              <Text style={styles.rescanText}>
                {wifiScanning ? 'SCANNING…' : 'RESCAN WI-FI'}
              </Text>
            </Pressable>
          </View>
          <Text style={styles.description}>
            DrainGuard supports 2.4 GHz Wi-Fi networks.
          </Text>

          {networks.length > 0 ? (
            <View style={styles.networkList}>
              {networks.slice(0, 8).map(network => (
                <Pressable
                  accessibilityRole="button"
                  key={network.ssid}
                  onPress={() => setSsid(network.ssid)}
                  style={({pressed}) => [
                    styles.networkRow,
                    ssid === network.ssid && styles.networkRowSelected,
                    pressed && styles.rowPressed,
                  ]}>
                  <View style={styles.networkCopy}>
                    <Text style={styles.networkName}>{network.ssid}</Text>
                    <Text style={styles.networkMeta}>
                      {network.secure ? 'Secured' : 'Open'} ·{' '}
                      {signalLabel(network.rssi)} signal
                    </Text>
                  </View>
                  <Text style={styles.networkSelect}>
                    {ssid === network.ssid ? '✓' : '›'}
                  </Text>
                </Pressable>
              ))}
            </View>
          ) : null}

          <Text style={styles.inputLabel}>Wi-Fi name (SSID)</Text>
          <TextInput
            autoCapitalize="none"
            autoCorrect={false}
            maxLength={32}
            onChangeText={setSsid}
            placeholder="Home Wi-Fi"
            placeholderTextColor="#A0A3AA"
            style={styles.input}
            value={ssid}
          />

          <Text style={styles.inputLabel}>Wi-Fi password</Text>
          <View style={styles.passwordRow}>
            <TextInput
              autoCapitalize="none"
              autoCorrect={false}
              maxLength={63}
              onChangeText={setPassword}
              placeholder="Enter password"
              placeholderTextColor="#A0A3AA"
              secureTextEntry={!showPassword}
              style={styles.passwordInput}
              value={password}
            />
            <Pressable
              accessibilityRole="button"
              onPress={() => setShowPassword(current => !current)}
              style={styles.showButton}>
              <Text style={styles.showText}>
                {showPassword ? 'HIDE' : 'SHOW'}
              </Text>
            </Pressable>
          </View>

          <ActionButton
            disabled={provisioning || !ssid.trim()}
            icon="⌁"
            label={
              provisioning ? 'Connecting to Internet…' : 'Connect DrainGuard'
            }
            onPress={connectWifi}
            variant="success"
          />
        </View>
      ) : null}

      <Pressable
        accessibilityRole="button"
        onPress={() =>
          bleProvisioning.openBluetoothSettings().catch(() => undefined)
        }
        style={styles.settingsLink}>
        <Text style={styles.settingsLinkText}>
          OPEN PHONE BLUETOOTH SETTINGS
        </Text>
      </Pressable>
    </Card>
  );
}

const styles = StyleSheet.create({
  statusPanel: {
    minHeight: 48,
    flexDirection: 'row',
    alignItems: 'center',
    borderRadius: radii.sm,
    backgroundColor: colors.primarySoft,
    paddingHorizontal: spacing.md,
    marginBottom: spacing.xl,
  },
  statusPanelSuccess: {backgroundColor: colors.successSoft},
  statusDot: {
    width: 9,
    height: 9,
    borderRadius: 5,
    backgroundColor: colors.primary,
    marginRight: spacing.sm,
  },
  statusDotSuccess: {backgroundColor: colors.success},
  statusText: {
    flex: 1,
    color: colors.text,
    fontSize: 12,
    lineHeight: 17,
    fontWeight: '600',
  },
  stepLabel: {
    color: colors.primary,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1,
  },
  description: {
    color: colors.textMuted,
    fontSize: 12,
    lineHeight: 18,
    marginTop: spacing.xs,
    marginBottom: spacing.md,
  },
  deviceRow: {
    minHeight: 58,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: radii.sm,
    backgroundColor: colors.inset,
    paddingHorizontal: spacing.md,
    marginTop: spacing.sm,
  },
  deviceRowSelected: {
    borderColor: colors.success,
    backgroundColor: colors.successSoft,
  },
  rowPressed: {opacity: 0.7},
  deviceCopy: {flex: 1},
  deviceName: {color: colors.text, fontSize: 14, fontWeight: '700'},
  deviceMeta: {color: colors.textMuted, fontSize: 10, marginTop: 3},
  connectLabel: {
    color: colors.primary,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  wifiSection: {
    borderTopWidth: 1,
    borderTopColor: colors.border,
    marginTop: spacing.xl,
    paddingTop: spacing.xl,
  },
  stepHeadingRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  rescanText: {
    color: colors.primary,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 0.5,
  },
  networkList: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: radii.sm,
    overflow: 'hidden',
    marginBottom: spacing.lg,
  },
  networkRow: {
    minHeight: 48,
    flexDirection: 'row',
    alignItems: 'center',
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: colors.border,
    backgroundColor: colors.card,
    paddingHorizontal: spacing.md,
  },
  networkRowSelected: {backgroundColor: colors.primarySoft},
  networkCopy: {flex: 1},
  networkName: {color: colors.text, fontSize: 13, fontWeight: '700'},
  networkMeta: {color: colors.textMuted, fontSize: 10, marginTop: 2},
  networkSelect: {color: colors.primary, fontSize: 20, fontWeight: '700'},
  inputLabel: {
    color: colors.text,
    fontSize: 12,
    fontWeight: '700',
    marginBottom: 6,
  },
  input: {
    height: 48,
    borderWidth: 1.5,
    borderColor: colors.border,
    borderRadius: radii.sm,
    backgroundColor: colors.inset,
    color: colors.text,
    fontSize: 14,
    paddingHorizontal: spacing.md,
    marginBottom: spacing.lg,
  },
  passwordRow: {
    height: 48,
    flexDirection: 'row',
    borderWidth: 1.5,
    borderColor: colors.border,
    borderRadius: radii.sm,
    backgroundColor: colors.inset,
    overflow: 'hidden',
    marginBottom: spacing.lg,
  },
  passwordInput: {
    flex: 1,
    color: colors.text,
    fontSize: 14,
    paddingHorizontal: spacing.md,
  },
  showButton: {
    justifyContent: 'center',
    paddingHorizontal: spacing.md,
  },
  showText: {color: colors.primary, fontSize: 10, fontWeight: '800'},
  settingsLink: {
    alignSelf: 'center',
    marginTop: spacing.lg,
    padding: spacing.sm,
  },
  settingsLinkText: {
    color: colors.textMuted,
    fontSize: 9,
    fontWeight: '700',
    letterSpacing: 0.6,
  },
});
