import React from 'react';
import {
  Pressable,
  PressableProps,
  StyleProp,
  StyleSheet,
  Text,
  ViewStyle,
} from 'react-native';

import {colors, radii, spacing} from '../theme';

type ButtonVariant = 'primary' | 'success' | 'danger' | 'outline';

interface ActionButtonProps extends PressableProps {
  label: string;
  icon?: string;
  variant?: ButtonVariant;
  style?: StyleProp<ViewStyle>;
}

const variantStyles = StyleSheet.create({
  primary: {backgroundColor: colors.primary},
  success: {backgroundColor: colors.success},
  danger: {backgroundColor: colors.danger},
  outline: {
    backgroundColor: colors.white,
    borderWidth: 2,
    borderColor: colors.primary,
  },
});

export function ActionButton({
  label,
  icon,
  variant = 'primary',
  disabled,
  style,
  ...pressableProps
}: ActionButtonProps) {
  const outline = variant === 'outline';

  return (
    <Pressable
      accessibilityRole="button"
      disabled={disabled}
      style={({pressed}) => [
        styles.button,
        variantStyles[variant],
        pressed && styles.pressed,
        disabled && styles.disabled,
        style,
      ]}
      {...pressableProps}>
      {icon ? (
        <Text style={[styles.icon, outline && styles.outlineText]}>{icon}</Text>
      ) : null}
      <Text style={[styles.label, outline && styles.outlineText]}>{label}</Text>
    </Pressable>
  );
}

const styles = StyleSheet.create({
  button: {
    minHeight: 50,
    borderRadius: radii.sm,
    paddingHorizontal: spacing.lg,
    paddingVertical: spacing.md,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
  },
  pressed: {
    opacity: 0.8,
    transform: [{scale: 0.985}],
  },
  disabled: {
    opacity: 0.5,
  },
  label: {
    color: colors.white,
    fontSize: 15,
    fontWeight: '700',
  },
  icon: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '800',
    marginRight: spacing.sm,
  },
  outlineText: {
    color: colors.primary,
  },
});
