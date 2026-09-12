import React, {PropsWithChildren, ReactNode} from 'react';
import {StyleSheet, Text, View, ViewStyle} from 'react-native';

import {colors, radii, shadow, spacing} from '../theme';

interface CardProps extends PropsWithChildren {
  title: string;
  icon: string;
  accessory?: ReactNode;
  style?: ViewStyle;
}

export function Card({title, icon, accessory, style, children}: CardProps) {
  return (
    <View style={[styles.card, style]}>
      <View style={styles.header}>
        <View style={styles.iconBubble}>
          <Text style={styles.icon}>{icon}</Text>
        </View>
        <Text style={styles.title}>{title}</Text>
        {accessory}
      </View>
      {children}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.card,
    borderRadius: radii.md,
    padding: spacing.xl,
    marginBottom: spacing.lg,
    ...shadow,
  },
  header: {
    minHeight: 42,
    flexDirection: 'row',
    alignItems: 'center',
    borderBottomWidth: 1.5,
    borderBottomColor: colors.border,
    paddingBottom: spacing.md,
    marginBottom: spacing.lg,
  },
  iconBubble: {
    width: 34,
    height: 34,
    borderRadius: 17,
    backgroundColor: colors.primarySoft,
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: spacing.md,
  },
  icon: {
    color: colors.primary,
    fontSize: 18,
    fontWeight: '800',
  },
  title: {
    flex: 1,
    color: colors.text,
    fontSize: 18,
    fontWeight: '700',
  },
});
