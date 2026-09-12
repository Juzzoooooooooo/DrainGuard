import React, {useMemo, useRef} from 'react';
import {
  Animated,
  PanResponder,
  PanResponderGestureState,
  StyleSheet,
  Text,
  View,
} from 'react-native';

import {colors} from '../theme';

const UPDATE_INTERVAL_MS = 110;

interface JoystickProps {
  onMove: (x: number, y: number) => void;
  size?: number;
}

function getOffset(gesture: PanResponderGestureState, maximumOffset: number) {
  const distance = Math.sqrt(gesture.dx ** 2 + gesture.dy ** 2);
  const scale = distance > maximumOffset ? maximumOffset / distance : 1;
  return {x: gesture.dx * scale, y: gesture.dy * scale};
}

export function Joystick({onMove, size = 190}: JoystickProps) {
  const position = useRef(new Animated.ValueXY()).current;
  const lastUpdate = useRef(0);
  const borderWidth = Math.max(4, Math.round(size * 0.037));
  const stickSize = Math.round(size * 0.39);
  const maximumOffset = (size - stickSize) / 2 - borderWidth;
  const crossSize = Math.round(size * 0.58);
  const dotSize = Math.round(size * 0.095);
  const directionInset = Math.max(6, Math.round(size * 0.055));

  const responder = useMemo(
    () =>
      PanResponder.create({
        onStartShouldSetPanResponder: () => true,
        onMoveShouldSetPanResponder: () => true,
        onPanResponderMove: (_, gesture) => {
          const offset = getOffset(gesture, maximumOffset);
          position.setValue(offset);

          const now = Date.now();
          if (now - lastUpdate.current >= UPDATE_INTERVAL_MS) {
            lastUpdate.current = now;
            onMove(offset.x / maximumOffset, offset.y / maximumOffset);
          }
        },
        onPanResponderRelease: () => {
          Animated.spring(position, {
            toValue: {x: 0, y: 0},
            damping: 12,
            stiffness: 180,
            useNativeDriver: true,
          }).start();
        },
        onPanResponderTerminate: () => {
          Animated.spring(position, {
            toValue: {x: 0, y: 0},
            useNativeDriver: true,
          }).start();
        },
      }),
    [maximumOffset, onMove, position],
  );

  return (
    <View style={styles.wrapper}>
      <View
        style={[
          styles.base,
          {
            width: size,
            height: size,
            borderRadius: size / 2,
            borderWidth,
          },
        ]}>
        <Text style={[styles.direction, {top: directionInset}]}>UP</Text>
        <Text style={[styles.direction, {bottom: directionInset}]}>DOWN</Text>
        <Text style={[styles.direction, {left: directionInset}]}>LEFT</Text>
        <Text style={[styles.direction, {right: directionInset}]}>RIGHT</Text>
        <View style={[styles.crossHorizontal, {width: crossSize}]} />
        <View style={[styles.crossVertical, {height: crossSize}]} />
        <Animated.View
          accessibilityLabel="Arm movement joystick"
          accessibilityRole="adjustable"
          {...responder.panHandlers}
          style={[
            styles.stick,
            {
              width: stickSize,
              height: stickSize,
              borderRadius: stickSize / 2,
              transform: [{translateX: position.x}, {translateY: position.y}],
            },
          ]}>
          <View
            style={[
              styles.stickDot,
              {width: dotSize, height: dotSize, borderRadius: dotSize / 2},
            ]}
          />
        </Animated.View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  wrapper: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  base: {
    backgroundColor: 'rgba(12,18,28,0.12)',
    borderColor: 'rgba(255,255,255,0.44)',
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 2},
    shadowOpacity: 0.16,
    shadowRadius: 8,
    elevation: 3,
  },
  crossHorizontal: {
    position: 'absolute',
    height: 1,
    backgroundColor: 'rgba(255,255,255,0.18)',
  },
  crossVertical: {
    position: 'absolute',
    width: 1,
    backgroundColor: 'rgba(255,255,255,0.18)',
  },
  stick: {
    backgroundColor: 'rgba(33,150,243,0.32)',
    borderWidth: 3,
    borderColor: 'rgba(157,218,255,0.68)',
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: colors.primary,
    shadowOffset: {width: 0, height: 5},
    shadowOpacity: 0.36,
    shadowRadius: 8,
    elevation: 8,
  },
  stickDot: {
    backgroundColor: colors.white,
  },
  direction: {
    position: 'absolute',
    color: 'rgba(255,255,255,0.66)',
    fontSize: 10,
    fontWeight: '800',
  },
});
