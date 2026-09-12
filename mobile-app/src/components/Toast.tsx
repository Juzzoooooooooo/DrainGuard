import React, {useEffect, useRef} from 'react';
import {Animated, StyleSheet, Text} from 'react-native';

import {colors, radii, spacing} from '../theme';
import type {ToastKind} from '../types';

interface ToastProps {
  message: string;
  kind: ToastKind;
  onHide: () => void;
}

const backgrounds: Record<ToastKind, string> = {
  success: colors.success,
  danger: colors.danger,
  warning: colors.warning,
  info: colors.primary,
};

export function Toast({message, kind, onHide}: ToastProps) {
  const translateY = useRef(new Animated.Value(-28)).current;
  const opacity = useRef(new Animated.Value(0)).current;

  useEffect(() => {
    Animated.parallel([
      Animated.spring(translateY, {
        toValue: 0,
        damping: 15,
        stiffness: 190,
        mass: 0.7,
        useNativeDriver: true,
      }),
      Animated.timing(opacity, {
        toValue: 1,
        duration: 180,
        useNativeDriver: true,
      }),
    ]).start();

    const timeout = setTimeout(() => {
      Animated.parallel([
        Animated.timing(translateY, {
          toValue: -20,
          duration: 180,
          useNativeDriver: true,
        }),
        Animated.timing(opacity, {
          toValue: 0,
          duration: 180,
          useNativeDriver: true,
        }),
      ]).start(onHide);
    }, 2600);

    return () => clearTimeout(timeout);
  }, [onHide, opacity, translateY]);

  return (
    <Animated.View
      accessibilityLiveRegion="polite"
      style={[
        styles.toast,
        {
          backgroundColor: backgrounds[kind],
          opacity,
          transform: [{translateY}],
        },
      ]}>
      <Text style={styles.text}>{message}</Text>
    </Animated.View>
  );
}

const styles = StyleSheet.create({
  toast: {
    position: 'absolute',
    zIndex: 20,
    top: spacing.lg,
    left: spacing.lg,
    right: spacing.lg,
    borderRadius: radii.sm,
    paddingHorizontal: spacing.lg,
    paddingVertical: 14,
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 4},
    shadowOpacity: 0.2,
    shadowRadius: 8,
    elevation: 7,
  },
  text: {
    color: colors.white,
    textAlign: 'center',
    fontSize: 14,
    fontWeight: '700',
  },
});
