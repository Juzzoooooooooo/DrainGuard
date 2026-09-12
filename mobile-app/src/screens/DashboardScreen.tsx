import React, {useMemo} from 'react';
import {
  Linking,
  RefreshControl,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import {ActionButton} from '../components/ActionButton';
import {Card} from '../components/Card';
import {colors, radii, spacing} from '../theme';
import type {AppSettings, SystemStatus, ToastKind} from '../types';
import {
  clamp,
  getWaterLevelState,
  getWaterStateColor,
} from '../utils/waterLevel';

interface DashboardScreenProps {
  settings: AppSettings;
  status: SystemStatus | null;
  refreshing: boolean;
  onRefresh: () => void;
  notify: (message: string, kind?: ToastKind) => void;
}

function formatReading(value: number | undefined, decimals = 1) {
  return typeof value === 'number' && Number.isFinite(value)
    ? value.toFixed(decimals)
    : '—';
}

export function DashboardScreen({
  settings,
  status,
  refreshing,
  onRefresh,
  notify,
}: DashboardScreenProps) {
  const readingAvailable = Boolean(status && status.distance >= 0);
  const waterState = useMemo(
    () =>
      readingAvailable && status
        ? getWaterLevelState(
            status.distance,
            settings.criticalLevel,
            settings.warningLevel,
          )
        : 'normal',
    [readingAvailable, settings, status],
  );
  const stateColor = getWaterStateColor(waterState);
  const percentage = status
    ? clamp((status.water_level / 200) * 100, 0, 100)
    : 0;
  const hasLocation = Boolean(
    status &&
      Number.isFinite(status.latitude) &&
      Number.isFinite(status.longitude) &&
      (status.latitude !== 0 || status.longitude !== 0),
  );

  const openMap = async () => {
    if (!status || !hasLocation) {
      notify('GPS location is not available yet.', 'warning');
      return;
    }

    const mapUrl = `https://www.google.com/maps?q=${status.latitude},${status.longitude}`;
    try {
      await Linking.openURL(mapUrl);
    } catch {
      notify('Unable to open the map on this device.', 'danger');
    }
  };

  return (
    <ScrollView
      contentContainerStyle={styles.content}
      refreshControl={
        <RefreshControl
          colors={[colors.primary]}
          onRefresh={onRefresh}
          refreshing={refreshing}
          tintColor={colors.primary}
        />
      }
      showsVerticalScrollIndicator={false}>
      <View style={styles.screenWidth}>
        <View style={styles.introRow}>
          <View>
            <Text style={styles.eyebrow}>SYSTEM OVERVIEW</Text>
            <Text style={styles.heading}>Dashboard</Text>
          </View>
          <Text style={styles.refreshHint}>Pull to refresh</Text>
        </View>

        <Card icon="≈" title="Water Level">
          <View style={styles.levelDisplay}>
            <Text style={[styles.levelValue, {color: stateColor}]}>
              {formatReading(status?.water_level)}
            </Text>
            <Text style={styles.levelUnit}>cm</Text>
          </View>

          <View
            style={[styles.levelBadge, {backgroundColor: `${stateColor}18`}]}>
            <View style={[styles.badgeDot, {backgroundColor: stateColor}]} />
            <Text style={[styles.levelBadgeText, {color: stateColor}]}>
              {readingAvailable ? waterState.toUpperCase() : 'WAITING FOR DATA'}
            </Text>
          </View>

          <View style={styles.progressTrack}>
            <View
              style={[
                styles.progressFill,
                {
                  backgroundColor: stateColor,
                  width: `${percentage}%`,
                },
              ]}
            />
          </View>

          <View style={styles.metricFooter}>
            <Text style={styles.metricLabel}>Sensor distance</Text>
            <Text style={styles.metricValue}>
              {formatReading(status?.distance)} cm
            </Text>
          </View>
        </Card>

        <Card icon="⌖" title="GPS Location">
          <View style={styles.gpsPanel}>
            <View style={styles.gpsRow}>
              <Text style={styles.gpsLabel}>Latitude</Text>
              <Text style={styles.gpsValue}>
                {hasLocation ? formatReading(status?.latitude, 6) : 'N/A'}
              </Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.gpsRow}>
              <Text style={styles.gpsLabel}>Longitude</Text>
              <Text style={styles.gpsValue}>
                {hasLocation ? formatReading(status?.longitude, 6) : 'N/A'}
              </Text>
            </View>
            <View style={styles.divider} />
            <View style={styles.gpsRow}>
              <Text style={styles.gpsLabel}>Satellites</Text>
              <Text style={styles.gpsValue}>{status?.satellites ?? 0}</Text>
            </View>
          </View>
          <ActionButton
            disabled={!hasLocation}
            icon="⌖"
            label="View on Map"
            onPress={openMap}
          />
        </Card>
      </View>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
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
  introRow: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    justifyContent: 'space-between',
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
  refreshHint: {
    color: colors.textMuted,
    fontSize: 11,
  },
  levelDisplay: {
    flexDirection: 'row',
    alignItems: 'baseline',
    justifyContent: 'center',
    paddingTop: spacing.sm,
  },
  levelValue: {
    fontSize: 60,
    fontWeight: '800',
    letterSpacing: -2,
  },
  levelUnit: {
    color: colors.textMuted,
    fontSize: 20,
    fontWeight: '600',
    marginLeft: spacing.sm,
  },
  levelBadge: {
    alignSelf: 'center',
    flexDirection: 'row',
    alignItems: 'center',
    borderRadius: radii.pill,
    paddingVertical: 7,
    paddingHorizontal: spacing.md,
    marginTop: spacing.sm,
    marginBottom: spacing.xl,
  },
  badgeDot: {
    width: 7,
    height: 7,
    borderRadius: 4,
    marginRight: 7,
  },
  levelBadgeText: {
    fontSize: 11,
    fontWeight: '800',
    letterSpacing: 0.8,
  },
  progressTrack: {
    width: '100%',
    height: 12,
    backgroundColor: '#E7E8EB',
    borderRadius: radii.pill,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    borderRadius: radii.pill,
  },
  metricFooter: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: spacing.md,
  },
  metricLabel: {
    color: colors.textMuted,
    fontSize: 13,
  },
  metricValue: {
    color: colors.text,
    fontSize: 13,
    fontWeight: '700',
  },
  gpsPanel: {
    backgroundColor: colors.inset,
    borderRadius: radii.sm,
    paddingHorizontal: spacing.lg,
    marginBottom: spacing.lg,
  },
  gpsRow: {
    minHeight: 46,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  gpsLabel: {
    color: colors.textMuted,
    fontSize: 13,
  },
  gpsValue: {
    color: colors.text,
    fontSize: 13,
    fontWeight: '700',
  },
  divider: {
    height: StyleSheet.hairlineWidth,
    backgroundColor: colors.border,
  },
});
