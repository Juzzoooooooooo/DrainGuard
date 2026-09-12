import React from 'react';
import {Image, Platform, StatusBar, StyleSheet, Text, View} from 'react-native';

import {colors, radii, spacing} from '../theme';

interface AppHeaderProps {
  connected: boolean | null;
  compact?: boolean;
}

export function AppHeader({connected, compact = false}: AppHeaderProps) {
  const connectionLabel =
    connected === null
      ? 'Connecting...'
      : connected
      ? 'Connected'
      : 'Disconnected';

  return (
    <View style={[styles.header, compact && styles.headerCompact]}>
      <View style={styles.decorOne} />
      <View style={styles.decorTwo} />
      <View style={styles.brand}>
        <Image
          accessibilityLabel="Drain Guard Robot logo"
          resizeMode="cover"
          source={require('../assets/drain-guard-logo.png')}
          style={[styles.logo, compact && styles.logoCompact]}
        />
        <View>
          <Text style={[styles.title, compact && styles.titleCompact]}>
            Drain Guard
          </Text>
          {compact ? null : (
            <Text style={styles.subtitle}>Smart drainage control</Text>
          )}
        </View>
      </View>

      <View
        style={[
          styles.connectionPill,
          compact && styles.connectionPillCompact,
        ]}>
        <View
          style={[
            styles.connectionDot,
            connected === null
              ? styles.connecting
              : connected
              ? styles.connected
              : styles.disconnected,
          ]}
        />
        <Text style={styles.connectionText}>{connectionLabel}</Text>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  header: {
    minHeight: 104,
    paddingTop:
      Platform.OS === 'android'
        ? (StatusBar.currentHeight ?? 0) + spacing.lg
        : spacing.lg,
    paddingHorizontal: spacing.xl,
    paddingBottom: spacing.xl,
    backgroundColor: colors.primary,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    overflow: 'hidden',
  },
  headerCompact: {
    minHeight: 70,
    paddingTop:
      Platform.OS === 'android'
        ? (StatusBar.currentHeight ?? 0) + spacing.xs
        : spacing.sm,
    paddingHorizontal: spacing.lg,
    paddingBottom: spacing.sm,
  },
  decorOne: {
    position: 'absolute',
    width: 160,
    height: 160,
    borderRadius: 80,
    backgroundColor: colors.primaryDark,
    opacity: 0.38,
    top: -72,
    right: -20,
  },
  decorTwo: {
    position: 'absolute',
    width: 86,
    height: 86,
    borderRadius: 43,
    borderWidth: 18,
    borderColor: colors.white,
    opacity: 0.08,
    bottom: -50,
    left: 116,
  },
  brand: {
    flexDirection: 'row',
    alignItems: 'center',
    flexShrink: 1,
  },
  logo: {
    width: 58,
    height: 58,
    borderRadius: 15,
    marginRight: spacing.md,
    borderWidth: 1.5,
    borderColor: 'rgba(255,255,255,0.5)',
  },
  logoCompact: {
    width: 42,
    height: 42,
    borderRadius: 11,
    marginRight: spacing.sm,
  },
  title: {
    color: colors.white,
    fontSize: 22,
    fontWeight: '800',
    letterSpacing: -0.4,
  },
  titleCompact: {
    fontSize: 18,
  },
  subtitle: {
    color: 'rgba(255,255,255,0.78)',
    fontSize: 11,
    marginTop: 1,
  },
  connectionPill: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: 'rgba(255,255,255,0.18)',
    borderRadius: radii.pill,
    paddingHorizontal: spacing.md,
    paddingVertical: spacing.sm,
    marginLeft: spacing.sm,
  },
  connectionPillCompact: {
    paddingHorizontal: spacing.sm,
    paddingVertical: 6,
  },
  connectionDot: {
    width: 9,
    height: 9,
    borderRadius: 5,
    marginRight: 7,
    borderWidth: 1.5,
    borderColor: 'rgba(255,255,255,0.7)',
  },
  connecting: {
    backgroundColor: colors.warning,
  },
  connected: {
    backgroundColor: colors.success,
  },
  disconnected: {
    backgroundColor: colors.danger,
  },
  connectionText: {
    color: colors.white,
    fontSize: 11,
    fontWeight: '700',
  },
});
