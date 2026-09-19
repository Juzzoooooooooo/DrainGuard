/**
 * WiFi Hotspot Connection Instructions (Hotspot-Only Version)
 * Replaces the old BLE provisioning component.
 * Shows hotspot SSID/password and tests HTTP connection.
 */

import React, {useCallback, useEffect, useState} from 'react';
import {
  ActivityIndicator,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from 'react-native';

import httpAPI from '../services/httpAPI';
import {colors, radii, spacing} from '../theme';
import type {ToastKind} from '../types';

interface WifiProvisioningProps {
  notify: (message: string, kind?: ToastKind) => void;
  onProvisioned: (deviceIp: string) => Promise<void>;
  onForgetWifi: () => Promise<void>;
}

export function WifiProvisioning({notify, onProvisioned}: WifiProvisioningProps) {
  const [testing, setTesting] = useState(false);
  const [connected, setConnected] = useState(false);

  // Auto-test on mount
  useEffect(() => {
    testConnection(false);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const testConnection = useCallback(
    async (showToast = true) => {
      setTesting(true);
      try {
        const ok = await httpAPI.testConnection();
        setConnected(ok);
        if (ok) {
          await onProvisioned('192.168.4.1');
          if (showToast) notify('Connected to DrainGuard!', 'success');
        } else {
          if (showToast)
            notify(
              'Not connected. Join DrainGuard-Robot WiFi first.',
              'danger',
            );
        }
      } catch {
        setConnected(false);
        if (showToast)
          notify('Connection error. Check your WiFi.', 'danger');
      } finally {
        setTesting(false);
      }
    },
    [notify, onProvisioned],
  );

  return (
    <View style={styles.card}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.icon}>📡</Text>
        <View style={styles.headerText}>
          <Text style={styles.title}>WiFi Connection</Text>
          <Text style={styles.subtitle}>
            Connect your phone to the DrainGuard hotspot
          </Text>
        </View>
      </View>

      {/* Instructions */}
      <View style={styles.steps}>
        <View style={styles.stepRow}>
          <View style={styles.stepNum}>
            <Text style={styles.stepNumText}>1</Text>
          </View>
          <Text style={styles.stepText}>
            Open your phone's <Text style={styles.bold}>WiFi Settings</Text>
          </Text>
        </View>

        <View style={styles.stepRow}>
          <View style={styles.stepNum}>
            <Text style={styles.stepNumText}>2</Text>
          </View>
          <View>
            <Text style={styles.stepText}>
              Connect to:{' '}
              <Text style={styles.ssid}>DrainGuard-Robot</Text>
            </Text>
            <Text style={styles.stepText}>
              Password:{' '}
              <Text style={styles.bold}>DrainGuard123</Text>
            </Text>
          </View>
        </View>

        <View style={styles.stepRow}>
          <View style={styles.stepNum}>
            <Text style={styles.stepNumText}>3</Text>
          </View>
          <Text style={styles.stepText}>
            Return here and tap <Text style={styles.bold}>Test Connection</Text>
          </Text>
        </View>
      </View>

      {/* Status */}
      {connected && (
        <View style={styles.connectedBadge}>
          <Text style={styles.connectedText}>✅  Connected to DrainGuard</Text>
        </View>
      )}

      {/* Button */}
      <TouchableOpacity
        disabled={testing}
        onPress={() => testConnection(true)}
        style={[styles.button, testing && styles.buttonDisabled]}>
        {testing ? (
          <ActivityIndicator color="#fff" size="small" />
        ) : (
          <Text style={styles.buttonText}>
            {connected ? 'Re-test Connection' : 'Test Connection'}
          </Text>
        )}
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.page,
    borderRadius: radii.md,
    borderWidth: 1.5,
    borderColor: colors.border,
    padding: spacing.lg,
    marginBottom: spacing.lg,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: spacing.lg,
  },
  icon: {
    fontSize: 28,
    marginRight: spacing.md,
  },
  headerText: {flex: 1},
  title: {
    color: colors.text,
    fontSize: 16,
    fontWeight: '800',
  },
  subtitle: {
    color: colors.textMuted,
    fontSize: 12,
    marginTop: 2,
  },
  steps: {
    marginBottom: spacing.lg,
    gap: spacing.md,
  },
  stepRow: {
    flexDirection: 'row',
    alignItems: 'flex-start',
    gap: spacing.sm,
  },
  stepNum: {
    width: 22,
    height: 22,
    borderRadius: 11,
    backgroundColor: colors.primary,
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 1,
  },
  stepNumText: {
    color: '#fff',
    fontSize: 11,
    fontWeight: '800',
  },
  stepText: {
    color: colors.textMuted,
    fontSize: 13,
    lineHeight: 20,
    flex: 1,
  },
  bold: {
    color: colors.text,
    fontWeight: '700',
  },
  ssid: {
    color: colors.primary,
    fontWeight: '800',
  },
  connectedBadge: {
    backgroundColor: '#E8F5E9',
    borderRadius: radii.sm,
    padding: spacing.md,
    alignItems: 'center',
    marginBottom: spacing.md,
  },
  connectedText: {
    color: '#2E7D32',
    fontSize: 14,
    fontWeight: '700',
  },
  button: {
    backgroundColor: colors.primary,
    borderRadius: radii.sm,
    height: 48,
    alignItems: 'center',
    justifyContent: 'center',
  },
  buttonDisabled: {
    opacity: 0.6,
  },
  buttonText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: '700',
  },
});
