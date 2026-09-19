import React, {useEffect, useState} from 'react';
import {
  KeyboardAvoidingView,
  Platform,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';

import {ActionButton} from '../components/ActionButton';
import {Card} from '../components/Card';
import {WifiProvisioning} from '../components/WifiProvisioning';
import {isValidDeviceAddress, normalizeDeviceAddress} from '../services/api';
import {colors, radii, spacing} from '../theme';
import type {AppSettings, ToastKind} from '../types';

interface SettingsScreenProps {
  settings: AppSettings;
  onSave: (settings: AppSettings) => Promise<void>;
  onProvisioned: (deviceIp: string) => Promise<void>;
  onForgetWifi: () => Promise<void>;
  notify: (message: string, kind?: ToastKind) => void;
}

interface FieldProps {
  label: string;
  value: string;
  onChangeText: (value: string) => void;
  hint?: string;
  keyboardType?: 'default' | 'number-pad' | 'decimal-pad';
}

function Field({
  label,
  value,
  onChangeText,
  hint,
  keyboardType = 'default',
}: FieldProps) {
  return (
    <View style={styles.field}>
      <Text style={styles.label}>{label}</Text>
      <TextInput
        autoCapitalize="none"
        autoCorrect={false}
        keyboardType={keyboardType}
        onChangeText={onChangeText}
        placeholderTextColor="#A0A3AA"
        selectTextOnFocus
        style={styles.input}
        value={value}
      />
      {hint ? <Text style={styles.hint}>{hint}</Text> : null}
    </View>
  );
}

export function SettingsScreen({
  settings,
  onSave,
  onProvisioned,
  onForgetWifi,
  notify,
}: SettingsScreenProps) {
  const [deviceIp, setDeviceIp] = useState(settings.deviceIp);
  const [refreshRate, setRefreshRate] = useState(String(settings.refreshRate));
  const [criticalLevel, setCriticalLevel] = useState(
    String(settings.criticalLevel),
  );
  const [warningLevel, setWarningLevel] = useState(
    String(settings.warningLevel),
  );
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    setDeviceIp(settings.deviceIp);
    setRefreshRate(String(settings.refreshRate));
    setCriticalLevel(String(settings.criticalLevel));
    setWarningLevel(String(settings.warningLevel));
  }, [settings]);

  const save = async () => {
    const normalizedAddress = normalizeDeviceAddress(deviceIp);
    const parsedRefreshRate = Number(refreshRate);
    const parsedCriticalLevel = Number(criticalLevel);
    const parsedWarningLevel = Number(warningLevel);

    if (!isValidDeviceAddress(normalizedAddress)) {
      notify('Enter a valid device IP address or hostname.', 'warning');
      return;
    }
    if (
      !Number.isFinite(parsedRefreshRate) ||
      parsedRefreshRate < 1 ||
      parsedRefreshRate > 60
    ) {
      notify('Refresh rate must be between 1 and 60 seconds.', 'warning');
      return;
    }
    if (
      !Number.isFinite(parsedCriticalLevel) ||
      !Number.isFinite(parsedWarningLevel) ||
      parsedCriticalLevel < 0 ||
      parsedWarningLevel > 200 ||
      parsedCriticalLevel >= parsedWarningLevel
    ) {
      notify(
        'Critical distance must be lower than warning distance (0–200 cm).',
        'warning',
      );
      return;
    }

    setSaving(true);
    try {
      await onSave({
        deviceIp: normalizedAddress,
        refreshRate: parsedRefreshRate,
        criticalLevel: parsedCriticalLevel,
        warningLevel: parsedWarningLevel,
      });
    } finally {
      setSaving(false);
    }
  };

  return (
    <KeyboardAvoidingView
      behavior={Platform.OS === 'ios' ? 'padding' : undefined}
      style={styles.flex}>
      <ScrollView
        contentContainerStyle={styles.content}
        keyboardShouldPersistTaps="handled"
        showsVerticalScrollIndicator={false}>
        <View style={styles.screenWidth}>
          <View style={styles.intro}>
            <Text style={styles.eyebrow}>PREFERENCES</Text>
            <Text style={styles.heading}>Settings</Text>
          </View>

          <WifiProvisioning
            notify={notify}
            onForgetWifi={onForgetWifi}
            onProvisioned={async nextDeviceIp => {
              setDeviceIp(nextDeviceIp);
              await onProvisioned(nextDeviceIp);
            }}
          />

          <Card icon="⌁" title="Camera Network">
            <View style={styles.networkNote}>
              <Text style={styles.networkIcon}>⌁</Text>
              <View style={styles.networkCopy}>
                <Text style={styles.networkTitle}>Optional camera gateway</Text>
                <Text style={styles.networkText}>
                  Controls use the WiFi hotspot (192.168.4.1). This address is
                  only needed to discover the ESP32-CAM stream if it runs on a
                  different IP.
                </Text>
              </View>
            </View>

            <Field
              hint="Default hotspot controller: 192.168.4.1"
              label="Camera gateway IP address"
              onChangeText={setDeviceIp}
              value={deviceIp}
            />
            <Field
              hint="How often the dashboard polls the device status over WiFi."
              keyboardType="number-pad"
              label="Auto refresh rate (seconds)"
              onChangeText={setRefreshRate}
              value={refreshRate}
            />
            <ActionButton
              disabled={saving}
              icon="✓"
              label={saving ? 'Saving…' : 'Save Settings'}
              onPress={save}
            />
          </Card>

          <Card icon="!" title="Alert Thresholds">
            <Text style={styles.sectionDescription}>
              Alerts use the ultrasonic sensor distance. A smaller distance
              means the water is closer to the sensor.
            </Text>
            <Field
              hint="Critical when sensor distance is at or below this value."
              keyboardType="decimal-pad"
              label="Critical distance (cm)"
              onChangeText={setCriticalLevel}
              value={criticalLevel}
            />
            <Field
              hint="Warning when sensor distance is at or below this value."
              keyboardType="decimal-pad"
              label="Warning distance (cm)"
              onChangeText={setWarningLevel}
              value={warningLevel}
            />
          </Card>

          <Card icon="i" title="About">
            <View style={styles.aboutRow}>
              <Text style={styles.aboutLabel}>Version</Text>
              <Text style={styles.aboutValue}>1.0.0</Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.aboutRow}>
              <Text style={styles.aboutLabel}>Controller</Text>
              <Text style={styles.aboutValue}>ESP32 DevKit V1</Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.aboutRow}>
              <Text style={styles.aboutLabel}>Camera</Text>
              <Text style={styles.aboutValue}>ESP32-CAM</Text>
            </View>
          </Card>
        </View>
      </ScrollView>
    </KeyboardAvoidingView>
  );
}

const styles = StyleSheet.create({
  flex: {flex: 1},
  content: {
    flexGrow: 1,
    padding: spacing.lg,
    paddingBottom: 40,
  },
  screenWidth: {
    width: '100%',
    maxWidth: 760,
    alignSelf: 'center',
  },
  intro: {
    marginBottom: spacing.lg,
    paddingHorizontal: spacing.xs,
  },
  eyebrow: {
    color: colors.primary,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 1.4,
  },
  heading: {
    color: colors.text,
    fontSize: 25,
    fontWeight: '800',
    marginTop: 2,
  },
  networkNote: {
    flexDirection: 'row',
    alignItems: 'center',
    borderLeftWidth: 4,
    borderLeftColor: colors.primary,
    borderRadius: radii.sm,
    backgroundColor: colors.primarySoft,
    padding: spacing.md,
    marginBottom: spacing.xl,
  },
  networkIcon: {
    color: colors.primary,
    fontSize: 24,
    fontWeight: '800',
    marginRight: spacing.md,
  },
  networkCopy: {flex: 1},
  networkTitle: {
    color: colors.primaryDark,
    fontSize: 13,
    fontWeight: '800',
  },
  networkText: {
    color: colors.textMuted,
    fontSize: 11,
    lineHeight: 16,
    marginTop: 2,
  },
  field: {marginBottom: spacing.xl},
  label: {
    color: colors.text,
    fontSize: 13,
    fontWeight: '700',
    marginBottom: spacing.sm,
  },
  input: {
    height: 50,
    borderWidth: 1.5,
    borderColor: colors.border,
    borderRadius: radii.sm,
    backgroundColor: colors.inset,
    color: colors.text,
    fontSize: 15,
    paddingHorizontal: spacing.lg,
  },
  hint: {
    color: colors.textMuted,
    fontSize: 10,
    lineHeight: 14,
    marginTop: 6,
  },
  sectionDescription: {
    color: colors.textMuted,
    fontSize: 12,
    lineHeight: 18,
    marginBottom: spacing.xl,
  },
  aboutRow: {
    minHeight: 43,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  aboutLabel: {
    color: colors.textMuted,
    fontSize: 13,
  },
  aboutValue: {
    color: colors.text,
    fontSize: 13,
    fontWeight: '700',
  },
  divider: {
    height: StyleSheet.hairlineWidth,
    backgroundColor: colors.border,
  },
});
