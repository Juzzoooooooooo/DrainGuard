import React from 'react';
import {Pressable, StyleSheet, Text, View} from 'react-native';

import {colors, spacing} from '../theme';
import type {AppTab} from '../types';

interface TabBarProps {
  activeTab: AppTab;
  compact?: boolean;
  onChange: (tab: AppTab) => void;
}

const tabs: Array<{id: AppTab; icon: string; label: string}> = [
  {id: 'dashboard', icon: '⌂', label: 'Dashboard'},
  {id: 'camera', icon: '▣', label: 'Camera'},
  {id: 'settings', icon: '⚙', label: 'Settings'},
];

export function TabBar({activeTab, compact = false, onChange}: TabBarProps) {
  return (
    <View style={styles.container}>
      {tabs.map(tab => {
        const active = tab.id === activeTab;
        return (
          <Pressable
            accessibilityRole="tab"
            accessibilityState={{selected: active}}
            key={tab.id}
            onPress={() => onChange(tab.id)}
            style={({pressed}) => [
              styles.tab,
              compact && styles.tabCompact,
              pressed && styles.pressed,
            ]}>
            <Text style={[styles.icon, active && styles.activeText]}>
              {tab.icon}
            </Text>
            <Text style={[styles.label, active && styles.activeText]}>
              {tab.label}
            </Text>
            {active ? <View style={styles.activeBar} /> : null}
          </Pressable>
        );
      })}
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    backgroundColor: colors.card,
    borderBottomWidth: 1,
    borderBottomColor: colors.border,
    elevation: 2,
  },
  tab: {
    minHeight: 62,
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingTop: spacing.sm,
    paddingBottom: spacing.sm,
  },
  tabCompact: {
    minHeight: 44,
    paddingTop: spacing.xs,
    paddingBottom: spacing.xs,
  },
  pressed: {
    backgroundColor: colors.primarySoft,
  },
  icon: {
    color: colors.textMuted,
    fontSize: 19,
    fontWeight: '800',
    lineHeight: 20,
  },
  label: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '600',
    marginTop: 3,
  },
  activeText: {
    color: colors.primary,
  },
  activeBar: {
    position: 'absolute',
    bottom: 0,
    left: '24%',
    right: '24%',
    height: 3,
    borderTopLeftRadius: 3,
    borderTopRightRadius: 3,
    backgroundColor: colors.primary,
  },
});
