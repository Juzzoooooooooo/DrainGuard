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
import wsAPI from '../services/wsAPI';
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

const BASE_MIN     = 250;   // from reference
const BASE_MAX     = 450;
const SHOULDER_MIN = 150;
const SHOULDER_MAX = 380;
const SERVO_STEP   = 15;

function createStreamHtml(streamUrl: string) {
  const safeUrl = streamUrl
    .replace(/&/g, '&amp;')
    .replace(/"/g, '&quot;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
  return `<!doctype html>
  <html><head><meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1"/>
  <style>html,body{width:100%;height:100%;margin:0;background:#08090b;overflow:hidden}body{display:flex;align-items:center;justify-content:center}img{display:block;width:100%;height:100%;object-fit:cover}.error{display:none;color:#fff;font:600 15px sans-serif;text-align:center}</style></head>
  <body><img src="${safeUrl}" onload="window.ReactNativeWebView.postMessage('loaded')" onerror="this.style.display='none';document.querySelector('.error').style.display='block';window.ReactNativeWebView.postMessage('error')"/><div class="error">Camera stream unavailable</div></body></html>`;
}

export function CameraScreen({
  onArm,
  onExit,
  onServo,
  loadStreamUrl,
  notify,
}: CameraScreenProps) {
  const [streamUrl, setStreamUrl]     = useState('');
  const [streamState, setStreamState] = useState<'loading' | 'live' | 'error'>('loading');
  const [busyAction, setBusyAction]   = useState<'open' | 'close' | null>(null);

  const basePos     = useRef(DEFAULT_SERVO_POSITIONS.base);
  const shoulderPos = useRef(DEFAULT_SERVO_POSITIONS.shoulder);

  // Sync servo positions from ESP32 on mount so claw/shoulder stay in sync
  useEffect(() => {
    // Connect WebSocket on mount, disconnect on unmount
    wsAPI.connect();
    // Fetch initial servo positions via HTTP for sync
    import('../services/httpAPI').then(({default: httpAPI}) => {
      httpAPI.getServoStatus()
        .then(s => {
          basePos.current     = s.base;
          shoulderPos.current = s.shoulder;
        })
        .catch(() => undefined);
    });
    return () => wsAPI.disconnect();
  }, []);

  // ── Stream ──────────────────────────────────────────────────────────────

  const refreshStream = useCallback(async () => {
    setStreamState('loading');
    setStreamUrl('');
    try {
      const nextUrl = await loadStreamUrl();
      if (nextUrl) { setStreamUrl(nextUrl); }
      else         { setStreamState('error'); }
    } catch {
      setStreamUrl('');
      setStreamState('error');
    }
  }, [loadStreamUrl]);

  useEffect(() => { refreshStream(); }, [refreshStream]);

  // ── Motor — hold for continuous, release to stop ───────────────────────

  const startForward  = () => wsAPI.motorForward();
  const startBackward = () => wsAPI.motorBackward();
  const stopDrive     = () => wsAPI.motorStop();

  // Left/right are still one-tap pulse (turning)
  const motorLeft  = () => wsAPI.motorLeft();
  const motorRight = () => wsAPI.motorRight();

  // ── Claw (base servo) — continuous rotation, time-based ────────────────

  const clawLeft  = () => wsAPI.moveBase(-1);   // -1 = rev
  const clawRight = () => wsAPI.moveBase(1);    // 1 = fwd

  // ── Shoulder — continuous rotation, time-based ───────────────────────────

  const shoulderUp   = () => wsAPI.moveShoulder(1);
  const shoulderDown = () => wsAPI.moveShoulder(-1);

  // ── Arm sequence via WebSocket ──────────────────────────────────────────

  const runArmAction = async (action: 'open' | 'close') => {
    setBusyAction(action);
    wsAPI[action === 'open' ? 'armOpen' : 'armClose']();
    // Arm takes ~5s — just clear busy after timeout
    setTimeout(() => setBusyAction(null), 6000);
  };

  return (
    <View style={styles.screen}>
      <View style={styles.cameraStage}>

        {/* Camera feed */}
        {streamUrl ? (
          <WebView
            key={streamUrl}
            allowsInlineMediaPlayback
            javaScriptEnabled
            mixedContentMode="always"
            onMessage={e => setStreamState(e.nativeEvent.data === 'loaded' ? 'live' : 'error')}
            originWhitelist={['*']}
            pointerEvents="none"
            scrollEnabled={false}
            source={{html: createStreamHtml(streamUrl)}}
            style={styles.webView}
          />
        ) : (
          <View style={styles.cameraPlaceholder}>
            {streamState === 'loading'
              ? <ActivityIndicator color={colors.white} size="large" />
              : <Text style={styles.cameraIcon}>▣</Text>}
            <Text style={styles.cameraTitle}>
              {streamState === 'loading' ? 'Connecting to camera…' : 'Camera not available'}
            </Text>
            <Text style={styles.cameraHint}>
              {streamState === 'loading'
                ? 'Waiting for ESP32-CAM stream.'
                : 'Check ESP32-CAM is on DrainGuard-Robot WiFi.'}
            </Text>
          </View>
        )}

        {/* ── Top bar ── */}
        <View pointerEvents="box-none" style={styles.topBar}>
          <View style={styles.topLeft}>
            <Pressable onPress={onExit}
              style={({pressed}) => [styles.topBtn, pressed && styles.pressed]}>
              <Text style={styles.exitIcon}>‹</Text>
              <Text style={styles.topBtnTxt}>EXIT</Text>
            </Pressable>
            <View style={[styles.liveBadge, streamState === 'live' && styles.liveBadgeOn]}>
              <View style={[styles.liveDot, streamState === 'live' && styles.liveDotOn]} />
              <Text style={styles.liveTxt}>
                {streamState === 'loading' ? 'LOADING' : streamState === 'live' ? 'LIVE' : 'OFFLINE'}
              </Text>
            </View>
          </View>
          <Pressable
            disabled={streamState === 'loading'}
            onPress={refreshStream}
            style={({pressed}) => [
              styles.topBtn,
              pressed && styles.pressed,
              streamState === 'loading' && styles.disabled,
            ]}>
            <Text style={styles.refreshIcon}>↻</Text>
            <Text style={styles.topBtnTxt}>REFRESH</Text>
          </Pressable>
        </View>

        {/* ── Bottom Controls ── */}
        <View style={styles.controls}>

          {/* LEFT — Wheels D-pad */}
          <View style={styles.dpad}>
            <Text style={styles.label}>WHEELS</Text>

            <Pressable onPressIn={startForward} onPressOut={stopDrive}
              style={({pressed}) => [styles.btn, styles.btnTop, pressed && styles.btnOn]}>
              <Text style={styles.btnArrow}>▲</Text>
              <Text style={styles.btnTxt}>FWD</Text>
            </Pressable>

            <View style={styles.dRow}>
              <Pressable onPress={motorLeft}
                style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
                <Text style={styles.btnArrow}>◀</Text>
                <Text style={styles.btnTxt}>LEFT</Text>
              </Pressable>
              <View style={styles.dCenter} />
              <Pressable onPress={motorRight}
                style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
                <Text style={styles.btnArrow}>▶</Text>
                <Text style={styles.btnTxt}>RIGHT</Text>
              </Pressable>
            </View>

            <Pressable onPressIn={startBackward} onPressOut={stopDrive}
              style={({pressed}) => [styles.btn, styles.btnBottom, pressed && styles.btnOn]}>
              <Text style={styles.btnArrow}>▼</Text>
              <Text style={styles.btnTxt}>REV</Text>
            </Pressable>
          </View>

          <View style={styles.divider} />

          {/* CENTER — Claw left/right */}
          <View style={styles.clawPanel}>
            <Text style={styles.label}>CLAW</Text>
            <View style={styles.clawRow}>
              <Pressable onPress={clawLeft}
                style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
                <Text style={styles.btnArrow}>◀</Text>
                <Text style={styles.btnTxt}>LEFT</Text>
              </Pressable>
              <Pressable onPress={clawRight}
                style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
                <Text style={styles.btnArrow}>▶</Text>
                <Text style={styles.btnTxt}>RIGHT</Text>
              </Pressable>
            </View>
          </View>

          <View style={styles.divider} />

          {/* RIGHT — Shoulder + Arm */}
          <View style={styles.armPanel}>
            <Text style={styles.label}>ARM</Text>

            <View style={styles.shRow}>
              <Pressable onPress={shoulderUp}
                style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}>
                <Text style={styles.shArrow}>↑</Text>
                <Text style={styles.shTxt}>UP</Text>
              </Pressable>
              <Pressable onPress={shoulderDown}
                style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}>
                <Text style={styles.shArrow}>↓</Text>
                <Text style={styles.shTxt}>DOWN</Text>
              </Pressable>
            </View>

            <ActionButton
              disabled={busyAction !== null}
              icon="↑"
              label={busyAction === 'open' ? 'OPENING…' : 'OPEN ARM'}
              onPress={() => runArmAction('open')}
              style={styles.armBtn}
              variant="success"
            />
            <ActionButton
              disabled={busyAction !== null}
              icon="↓"
              label={busyAction === 'close' ? 'CLOSING…' : 'CLOSE ARM'}
              onPress={() => runArmAction('close')}
              style={[styles.armBtn, {marginBottom: 0}]}
              variant="danger"
            />
          </View>

        </View>
      </View>
    </View>
  );
}

const BTN = 52;

const styles = StyleSheet.create({
  screen:      { flex: 1, backgroundColor: colors.black },
  cameraStage: { flex: 1, overflow: 'hidden', backgroundColor: colors.black },
  webView:     { flex: 1, backgroundColor: colors.black },

  cameraPlaceholder: {
    flex: 1, alignItems: 'center', justifyContent: 'center',
    paddingHorizontal: spacing.xl, paddingBottom: 200,
  },
  cameraIcon:  { color: '#555962', fontSize: 42, fontWeight: '800' },
  cameraTitle: { color: colors.white, fontSize: 15, fontWeight: '700', marginTop: spacing.md },
  cameraHint:  { color: '#9EA2AA', fontSize: 12, lineHeight: 17, textAlign: 'center', marginTop: spacing.xs },

  // ── Top bar ──────────────────────────────────────────────────────────────
  topBar: {
    position: 'absolute', zIndex: 3,
    top: spacing.md, left: spacing.md, right: spacing.md,
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between',
  },
  topLeft: { flexDirection: 'row', alignItems: 'center' },
  topBtn: {
    minHeight: 36, flexDirection: 'row', alignItems: 'center',
    borderWidth: 1, borderColor: 'rgba(255,255,255,0.35)',
    borderRadius: radii.pill, backgroundColor: 'rgba(8,9,11,0.55)',
    paddingLeft: 8, paddingRight: spacing.md, marginRight: spacing.sm,
  },
  topBtnTxt:   { color: colors.white, fontSize: 9, fontWeight: '800', letterSpacing: 0.8 },
  exitIcon:    { color: colors.white, fontSize: 22, fontWeight: '500', marginRight: 3 },
  refreshIcon: { color: colors.white, fontSize: 16, fontWeight: '800', marginRight: 6 },
  pressed:     { opacity: 0.6 },
  disabled:    { opacity: 0.35 },
  liveBadge: {
    flexDirection: 'row', alignItems: 'center',
    paddingHorizontal: spacing.md, paddingVertical: 6, minHeight: 36,
    backgroundColor: 'rgba(8,9,11,0.55)',
    borderRadius: radii.pill, borderWidth: 1, borderColor: 'rgba(255,255,255,0.2)',
  },
  liveBadgeOn: { backgroundColor: 'rgba(137,21,28,0.35)', borderColor: 'rgba(255,112,112,0.5)' },
  liveDot:   { width: 7, height: 7, borderRadius: 4, backgroundColor: '#A4A8B2', marginRight: 6 },
  liveDotOn: { backgroundColor: '#FF5C50' },
  liveTxt:   { color: colors.white, fontSize: 9, fontWeight: '800', letterSpacing: 0.8 },

  // ── Controls ─────────────────────────────────────────────────────────────
  controls: {
    position: 'absolute', zIndex: 2,
    left: 0, right: 0, bottom: 0,
    flexDirection: 'row', alignItems: 'center', justifyContent: 'space-around',
    paddingHorizontal: spacing.md,
    paddingTop: spacing.md,
    paddingBottom: spacing.lg,
  },
  label: {
    color: 'rgba(255,255,255,0.45)', fontSize: 8, fontWeight: '800',
    letterSpacing: 1.2, textAlign: 'center', marginBottom: 8,
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1}, textShadowRadius: 4,
  },
  divider: {
    width: 1, alignSelf: 'stretch',
    backgroundColor: 'rgba(255,255,255,0.08)',
    marginHorizontal: spacing.sm,
  },

  // ── D-pad ────────────────────────────────────────────────────────────────
  dpad:   { alignItems: 'center' },
  dRow:   { flexDirection: 'row', alignItems: 'center', gap: 4 },
  dCenter: { width: 24, height: 24 },

  btn: {
    width: BTN, height: BTN, borderRadius: 12,
    backgroundColor: 'transparent',
    borderWidth: 1.5, borderColor: 'rgba(255,255,255,0.25)',
    alignItems: 'center', justifyContent: 'center',
  },
  btnTop:    { marginBottom: 4 },
  btnBottom: { marginTop: 4 },
  btnOn: {
    backgroundColor: 'rgba(60,140,255,0.25)',
    borderColor: 'rgba(100,180,255,1)',
  },
  btnArrow: {
    color: '#fff', fontSize: 18, fontWeight: '800', lineHeight: 20,
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1}, textShadowRadius: 3,
  },
  btnTxt: {
    color: 'rgba(255,255,255,0.5)', fontSize: 7, fontWeight: '800',
    letterSpacing: 0.5, marginTop: 1,
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1}, textShadowRadius: 3,
  },

  // ── Claw panel ───────────────────────────────────────────────────────────
  clawPanel: { alignItems: 'center', justifyContent: 'center' },
  clawRow:   { flexDirection: 'row', gap: 8, marginTop: 8 },

  // ── Arm panel ────────────────────────────────────────────────────────────
  armPanel: { flex: 1, alignItems: 'stretch', maxWidth: 190 },

  shRow: { flexDirection: 'row', gap: 6, marginBottom: 6 },
  shBtn: {
    flex: 1, height: 44, borderRadius: 10,
    backgroundColor: 'transparent',
    borderWidth: 1.5, borderColor: 'rgba(255,185,80,0.4)',
    alignItems: 'center', justifyContent: 'center',
  },
  shBtnOn: {
    backgroundColor: 'rgba(255,185,80,0.2)',
    borderColor: 'rgba(255,185,80,1)',
  },
  shArrow: {
    color: colors.warning, fontSize: 16, fontWeight: '800', lineHeight: 18,
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1}, textShadowRadius: 3,
  },
  shTxt: {
    color: colors.warning, fontSize: 7, fontWeight: '800', letterSpacing: 0.5,
  },

  armBtn: { marginBottom: 6, minHeight: 44 },
});
