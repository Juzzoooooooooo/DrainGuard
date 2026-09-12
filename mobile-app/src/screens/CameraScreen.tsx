import React, {useCallback, useEffect, useRef, useState} from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {WebView} from 'react-native-webview';

import {ActionButton} from '../components/ActionButton';
import {Joystick} from '../components/Joystick';
import {colors, radii, spacing} from '../theme';
import {DEFAULT_SERVO_POSITIONS, ServoPositions, ToastKind} from '../types';
import {clamp} from '../utils/waterLevel';

interface CameraScreenProps {
  onArm: (action: 'open' | 'close') => Promise<void>;
  onExit: () => void;
  onServo: (servo: keyof ServoPositions, position: number) => Promise<void>;
  loadStreamUrl: () => Promise<string>;
  notify: (message: string, kind?: ToastKind) => void;
}

const servoLimits: Record<
  keyof ServoPositions,
  {minimum: number; maximum: number}
> = {
  base: {minimum: 150, maximum: 450},
  shoulder: {minimum: 150, maximum: 380},
  elbow: {minimum: 300, maximum: 380},
  gripper: {minimum: 410, maximum: 510},
};

function createStreamHtml(streamUrl: string) {
  const safeUrl = streamUrl
    .replace(/&/g, '&amp;')
    .replace(/"/g, '&quot;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');

  return `<!doctype html>
  <html><head><meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1" />
  <style>html,body{width:100%;height:100%;margin:0;background:#08090b;overflow:hidden}body{display:flex;align-items:center;justify-content:center}img{display:block;width:100%;height:100%;object-fit:cover}.error{display:none;color:#fff;font:600 15px -apple-system,sans-serif;text-align:center}</style></head>
  <body><img src="${safeUrl}" onload="window.ReactNativeWebView.postMessage('loaded')" onerror="this.style.display='none';document.querySelector('.error').style.display='block';window.ReactNativeWebView.postMessage('error')"/><div class="error">Camera stream unavailable</div></body></html>`;
}

export function CameraScreen({
  onArm,
  onExit,
  onServo,
  loadStreamUrl,
  notify,
}: CameraScreenProps) {
  const [streamUrl, setStreamUrl] = useState('');
  const [streamState, setStreamState] = useState<'loading' | 'live' | 'error'>(
    'loading',
  );
  const [busyAction, setBusyAction] = useState<'open' | 'close' | null>(null);
  const [clawOpen, setClawOpen] = useState(false);
  const positions = useRef({...DEFAULT_SERVO_POSITIONS});
  const elbowTimer = useRef<ReturnType<typeof setInterval> | null>(null);

  const refreshStream = useCallback(async () => {
    setStreamState('loading');
    setStreamUrl('');
    try {
      const nextUrl = await loadStreamUrl();
      if (nextUrl) {
        setStreamUrl(nextUrl);
      } else {
        // No camera connected — show placeholder but keep controls working
        setStreamState('error');
      }
    } catch {
      setStreamUrl('');
      setStreamState('error');
    }
  }, [loadStreamUrl]);

  useEffect(() => {
    refreshStream();
  }, [refreshStream]);

  useEffect(
    () => () => {
      if (elbowTimer.current) {
        clearInterval(elbowTimer.current);
      }
    },
    [],
  );

  const moveServo = useCallback(
    (servo: keyof ServoPositions, delta: number) => {
      const limits = servoLimits[servo];
      const next = Math.round(
        clamp(positions.current[servo] + delta, limits.minimum, limits.maximum),
      );

      if (next === positions.current[servo]) {
        return;
      }

      positions.current[servo] = next;
      onServo(servo, next).catch(() => undefined);
    },
    [onServo],
  );

  const handleJoystick = useCallback(
    (x: number, y: number) => {
      if (Math.abs(x) > 0.2) {
        moveServo('base', x * 10);
      }
      if (Math.abs(y) > 0.2) {
        moveServo('shoulder', y * 10);
      }
    },
    [moveServo],
  );

  const stopElbow = () => {
    if (elbowTimer.current) {
      clearInterval(elbowTimer.current);
      elbowTimer.current = null;
    }
  };

  const startElbow = (delta: number) => {
    stopElbow();
    moveServo('elbow', delta);
    elbowTimer.current = setInterval(() => moveServo('elbow', delta), 130);
  };

  const toggleClaw = async () => {
    const nextOpen = !clawOpen;
    const nextPosition = nextOpen
      ? servoLimits.gripper.maximum
      : servoLimits.gripper.minimum;
    positions.current.gripper = nextPosition;
    try {
      await onServo('gripper', nextPosition);
      setClawOpen(nextOpen);
      notify(nextOpen ? 'Claw opened.' : 'Claw closed.', 'success');
    } catch {
      positions.current.gripper = clawOpen
        ? servoLimits.gripper.maximum
        : servoLimits.gripper.minimum;
    }
  };

  const runArmAction = async (action: 'open' | 'close') => {
    setBusyAction(action);
    try {
      await onArm(action);
    } catch {
      // The parent reports command failures through the app toast.
    } finally {
      setBusyAction(null);
    }
  };

  return (
    <View style={styles.screen}>
      <View style={styles.cameraStage}>
        {streamUrl ? (
          <WebView
            key={streamUrl}
            allowsInlineMediaPlayback
            javaScriptEnabled
            mixedContentMode="always"
            onMessage={event =>
              setStreamState(
                event.nativeEvent.data === 'loaded' ? 'live' : 'error',
              )
            }
            originWhitelist={['*']}
            pointerEvents="none"
            scrollEnabled={false}
            source={{html: createStreamHtml(streamUrl)}}
            style={styles.webView}
          />
        ) : (
          <View style={styles.cameraPlaceholder}>
            {streamState === 'loading' ? (
              <ActivityIndicator color={colors.white} size="large" />
            ) : (
              <Text style={styles.cameraIcon}>▣</Text>
            )}
            <Text style={styles.cameraTitle}>
              {streamState === 'loading'
                ? 'Connecting to camera…'
                : 'Camera not available'}
            </Text>
            <Text style={styles.cameraHint}>
              {streamState === 'loading'
                ? 'Waiting for the ESP32-CAM stream.'
                : 'Check the ESP32-CAM and refresh the stream.'}
            </Text>
          </View>
        )}

        <View pointerEvents="box-none" style={styles.topOverlay}>
          <View style={styles.topLeftGroup}>
            <Pressable
              accessibilityLabel="Exit camera"
              accessibilityRole="button"
              onPress={onExit}
              style={({pressed}) => [
                styles.exitButton,
                pressed && styles.controlPressed,
              ]}>
              <Text style={styles.exitIcon}>‹</Text>
              <Text style={styles.exitText}>EXIT</Text>
            </Pressable>

            <View
              style={[
                styles.liveBadge,
                streamState === 'live' && styles.liveBadgeActive,
              ]}>
              <View
                style={[
                  styles.liveDot,
                  streamState === 'live' && styles.liveDotActive,
                ]}
              />
              <Text style={styles.liveText}>
                {streamState === 'loading'
                  ? 'LOADING'
                  : streamState === 'live'
                  ? 'LIVE'
                  : 'OFFLINE'}
              </Text>
            </View>
          </View>

          <Pressable
            accessibilityLabel="Refresh camera stream"
            accessibilityRole="button"
            disabled={streamState === 'loading'}
            onPress={refreshStream}
            style={({pressed}) => [
              styles.refreshButton,
              pressed && styles.controlPressed,
              streamState === 'loading' && styles.controlDisabled,
            ]}>
            <Text style={styles.refreshIcon}>↻</Text>
            <Text style={styles.refreshText}>REFRESH</Text>
          </Pressable>
        </View>

        <View style={styles.controlsOverlay}>
          <View style={styles.joystickPanel}>
            <Text style={styles.overlayTitle}>ARM MOVEMENT</Text>
            <Joystick onMove={handleJoystick} size={128} />
            <Text style={styles.joystickHint}>
              Base: left/right · Shoulder: up/down
            </Text>
          </View>

          <View style={styles.overlayDivider} />

          <View style={styles.actionPanel}>
            <Text style={styles.overlayTitle}>ARM ACTIONS</Text>
            <View style={styles.quickActions}>
              <ActionButton
                disabled={busyAction !== null}
                icon="↑"
                label={busyAction === 'open' ? 'OPENING…' : 'OPEN ARM'}
                onPress={() => runArmAction('open')}
                style={[styles.quickButton, styles.openGlassButton]}
                variant="success"
              />
              <ActionButton
                disabled={busyAction !== null}
                icon="↓"
                label={busyAction === 'close' ? 'CLOSING…' : 'CLOSE ARM'}
                onPress={() => runArmAction('close')}
                style={[styles.quickButton, styles.closeGlassButton]}
                variant="danger"
              />
            </View>

            <View style={styles.toolRow}>
              <Pressable
                accessibilityLabel="Move elbow forward"
                accessibilityRole="button"
                onPressIn={() => startElbow(8)}
                onPressOut={stopElbow}
                style={({pressed}) => [
                  styles.toolButton,
                  styles.elbowButton,
                  pressed && styles.controlPressed,
                ]}>
                <Text style={styles.elbowArrow}>↑</Text>
                <Text style={styles.elbowText}>ELBOW FWD</Text>
              </Pressable>
              <Pressable
                accessibilityLabel="Move elbow in reverse"
                accessibilityRole="button"
                onPressIn={() => startElbow(-8)}
                onPressOut={stopElbow}
                style={({pressed}) => [
                  styles.toolButton,
                  styles.elbowButton,
                  pressed && styles.controlPressed,
                ]}>
                <Text style={styles.elbowArrow}>↓</Text>
                <Text style={styles.elbowText}>ELBOW REV</Text>
              </Pressable>
              <Pressable
                accessibilityLabel={clawOpen ? 'Close claw' : 'Open claw'}
                accessibilityRole="button"
                onPress={toggleClaw}
                style={({pressed}) => [
                  styles.toolButton,
                  styles.clawButton,
                  pressed && styles.controlPressed,
                ]}>
                <Text style={styles.clawIcon}>{clawOpen ? '◇' : '◆'}</Text>
                <Text style={styles.clawText}>
                  {clawOpen ? 'CLAW OPEN' : 'CLAW GRIP'}
                </Text>
              </Pressable>
            </View>
          </View>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen: {
    flex: 1,
    backgroundColor: colors.black,
  },
  cameraStage: {
    flex: 1,
    overflow: 'hidden',
    backgroundColor: colors.black,
  },
  webView: {
    flex: 1,
    backgroundColor: colors.black,
  },
  topOverlay: {
    position: 'absolute',
    zIndex: 3,
    top: spacing.md,
    left: spacing.md,
    right: spacing.md,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  topLeftGroup: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  exitButton: {
    minHeight: 38,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.42)',
    borderRadius: radii.pill,
    backgroundColor: 'rgba(12,18,28,0.18)',
    paddingLeft: 9,
    paddingRight: spacing.md,
    marginRight: spacing.sm,
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 5},
    shadowOpacity: 0.24,
    shadowRadius: 12,
    elevation: 6,
  },
  exitIcon: {
    color: colors.white,
    fontSize: 25,
    fontWeight: '500',
    lineHeight: 25,
    marginRight: 3,
  },
  exitText: {
    color: colors.white,
    fontSize: 10,
    fontWeight: '800',
    letterSpacing: 0.8,
  },
  liveBadge: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: spacing.md,
    paddingVertical: 6,
    minHeight: 38,
    backgroundColor: 'rgba(12,18,28,0.18)',
    borderRadius: radii.pill,
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.24)',
  },
  liveBadgeActive: {
    backgroundColor: 'rgba(137,21,28,0.24)',
    borderColor: 'rgba(255,112,112,0.48)',
  },
  liveDot: {
    width: 7,
    height: 7,
    borderRadius: 4,
    backgroundColor: '#A4A8B2',
    marginRight: 6,
  },
  liveDotActive: {backgroundColor: '#FF5C50'},
  liveText: {
    color: colors.white,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 0.8,
  },
  refreshButton: {
    minHeight: 38,
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.42)',
    borderRadius: radii.pill,
    backgroundColor: 'rgba(12,18,28,0.18)',
    paddingHorizontal: spacing.md,
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 5},
    shadowOpacity: 0.24,
    shadowRadius: 12,
    elevation: 6,
  },
  refreshIcon: {
    color: colors.white,
    fontSize: 17,
    fontWeight: '800',
    marginRight: 6,
  },
  refreshText: {
    color: colors.white,
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 0.7,
  },
  cameraPlaceholder: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingHorizontal: spacing.xl,
    paddingBottom: 126,
  },
  cameraIcon: {
    color: '#555962',
    fontSize: 42,
    fontWeight: '800',
  },
  cameraTitle: {
    color: colors.white,
    fontSize: 15,
    fontWeight: '700',
    marginTop: spacing.md,
  },
  cameraHint: {
    color: '#9EA2AA',
    fontSize: 12,
    lineHeight: 17,
    textAlign: 'center',
    marginTop: spacing.xs,
  },
  controlsOverlay: {
    position: 'absolute',
    zIndex: 2,
    left: spacing.md,
    right: spacing.md,
    bottom: spacing.md,
    minHeight: 146,
    flexDirection: 'row',
    alignItems: 'stretch',
  },
  joystickPanel: {
    width: 164,
    alignItems: 'center',
    justifyContent: 'center',
    borderWidth: 0,
    backgroundColor: 'transparent',
    paddingHorizontal: spacing.sm,
    paddingVertical: 7,
  },
  overlayDivider: {
    width: spacing.sm,
  },
  actionPanel: {
    flex: 1,
    minWidth: 0,
    borderWidth: 0,
    backgroundColor: 'transparent',
    padding: spacing.sm,
  },
  overlayTitle: {
    color: 'rgba(255,255,255,0.78)',
    fontSize: 9,
    fontWeight: '800',
    letterSpacing: 1.1,
    textAlign: 'center',
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1},
    textShadowRadius: 3,
    marginBottom: 5,
  },
  quickActions: {
    flexDirection: 'row',
    marginHorizontal: -3,
    marginBottom: 6,
  },
  quickButton: {
    flex: 1,
    minHeight: 43,
    marginHorizontal: 3,
    paddingHorizontal: spacing.sm,
    paddingVertical: spacing.xs,
    borderWidth: 1,
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 4},
    shadowOpacity: 0.22,
    shadowRadius: 8,
    elevation: 4,
  },
  openGlassButton: {
    backgroundColor: 'rgba(40,190,108,0.16)',
    borderColor: 'rgba(111,255,177,0.62)',
  },
  closeGlassButton: {
    backgroundColor: 'rgba(232,67,74,0.16)',
    borderColor: 'rgba(255,132,137,0.62)',
  },
  joystickHint: {
    color: 'rgba(255,255,255,0.68)',
    fontSize: 8,
    lineHeight: 11,
    textAlign: 'center',
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1},
    textShadowRadius: 3,
    marginTop: 3,
  },
  toolRow: {
    flex: 1,
    flexDirection: 'row',
    marginHorizontal: -3,
  },
  toolButton: {
    minHeight: 54,
    flex: 1,
    borderWidth: 1,
    borderRadius: radii.sm,
    backgroundColor: 'rgba(12,18,28,0.16)',
    alignItems: 'center',
    justifyContent: 'center',
    marginHorizontal: 3,
    paddingHorizontal: 3,
    shadowColor: '#000000',
    shadowOffset: {width: 0, height: 4},
    shadowOpacity: 0.22,
    shadowRadius: 8,
    elevation: 4,
  },
  controlPressed: {
    opacity: 0.72,
    transform: [{scale: 0.97}],
  },
  controlDisabled: {
    opacity: 0.55,
  },
  elbowButton: {
    borderColor: 'rgba(255,185,80,0.58)',
  },
  elbowArrow: {
    color: colors.warning,
    fontSize: 18,
    fontWeight: '800',
    lineHeight: 19,
  },
  elbowText: {
    color: colors.warning,
    fontSize: 8,
    fontWeight: '800',
  },
  clawButton: {
    borderColor: 'rgba(218,124,236,0.58)',
  },
  clawIcon: {
    color: colors.purple,
    fontSize: 17,
    fontWeight: '800',
    lineHeight: 19,
  },
  clawText: {
    color: colors.purple,
    fontSize: 8,
    fontWeight: '800',
  },
});
