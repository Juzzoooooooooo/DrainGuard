import React, {useCallback, useEffect, useState} from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {WebView} from 'react-native-webview';

import wsAPI from '../services/wsAPI';
import {colors, radii, spacing} from '../theme';
import {ServoPositions, ToastKind} from '../types';

interface CameraScreenProps {
  onJoint: (joint: keyof ServoPositions, direction: -1 | 0 | 1) => Promise<void>;
  onExit: () => void;
  loadStreamUrl: () => Promise<string>;
  notify: (message: string, kind?: ToastKind) => void;
}

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
  onJoint,
  onExit,
  loadStreamUrl,
  notify,
}: CameraScreenProps) {
  const [streamUrl, setStreamUrl]     = useState('');
  const [streamState, setStreamState] = useState<'loading' | 'live' | 'error'>('loading');

  useEffect(() => {
    wsAPI.connect();
    return () => wsAPI.disconnect();
  }, []);

  useEffect(() => () => {
    for (const joint of ['base', 'shoulder', 'elbow', 'gripper'] as const)
      void onJoint(joint, 0).catch(() => undefined);
  }, [onJoint]);

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

  // ── Individual positional servo controls ───────────────────────────────

  const startJoint = (joint: keyof ServoPositions, direction: -1 | 1) => {
    void onJoint(joint, direction).catch(() => notify(`Failed to move ${joint}.`, 'danger'));
  };
  // ── Shoulder ────────────────────────────────────────────────────────────
  // SHOULDER ticks: 150=raised (home/up), 380=lowered (down)
  // direction -1 → ticks decrease → arm raises (UP)
  // direction +1 → ticks increase → arm lowers (DOWN)
  const shoulderUp   = () => startJoint('shoulder', -1);
  const shoulderDown = () => startJoint('shoulder',  1);
  const elbowUp      = () => startJoint('elbow', -1);
  const elbowDown    = () => startJoint('elbow', 1);
  const gripperOpen  = () => startJoint('gripper', -1);  // ticks decrease → 410→350 = open
  const gripperClose = () => startJoint('gripper',  1);  // ticks increase → 410→510 = close

  // ── Arm sequence via WebSocket ──────────────────────────────────────────

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

      </View>
      <View style={styles.controls}>
        <Text style={styles.label}>WHEELS</Text>
        <View style={styles.controlRow}>
          <Pressable onPressIn={startForward} onPressOut={stopDrive} style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
            <Text style={styles.btnArrow}>▲</Text><Text style={styles.btnTxt}>FWD</Text>
          </Pressable>
          <Pressable onPress={motorLeft} style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
            <Text style={styles.btnArrow}>◀</Text><Text style={styles.btnTxt}>LEFT</Text>
          </Pressable>
          <Pressable onPress={motorRight} style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
            <Text style={styles.btnArrow}>▶</Text><Text style={styles.btnTxt}>RIGHT</Text>
          </Pressable>
          <Pressable onPressIn={startBackward} onPressOut={stopDrive} style={({pressed}) => [styles.btn, pressed && styles.btnOn]}>
            <Text style={styles.btnArrow}>▼</Text><Text style={styles.btnTxt}>REV</Text>
          </Pressable>
        </View>
        <Text style={styles.label}>ARM</Text>
        <View style={styles.controlRow}>
          <View style={styles.jointGroup}>
            <Text style={styles.jointLabel}>BASE</Text>
            <View style={styles.jointRow}>
              <Pressable onPress={() => startJoint('base', 1)} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>LEFT</Text></Pressable>
              <Pressable onPress={() => startJoint('base', -1)} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>RIGHT</Text></Pressable>
            </View>
          </View>
          <View style={styles.jointGroup}>
            <Text style={styles.jointLabel}>SHOULDER</Text>
            <View style={styles.jointRow}>
              <Pressable onPress={shoulderUp} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>UP</Text></Pressable>
              <Pressable onPress={shoulderDown} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>DOWN</Text></Pressable>
            </View>
          </View>
          <View style={styles.jointGroup}>
            <Text style={styles.jointLabel}>ELBOW</Text>
            <View style={styles.jointRow}>
              <Pressable onPress={elbowUp} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>UP</Text></Pressable>
              <Pressable onPress={elbowDown} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>DOWN</Text></Pressable>
            </View>
          </View>
          <View style={styles.jointGroup}>
            <Text style={styles.jointLabel}>CLAW</Text>
            <View style={styles.jointRow}>
              <Pressable onPress={gripperOpen} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>OPEN</Text></Pressable>
              <Pressable onPress={gripperClose} style={({pressed}) => [styles.shBtn, pressed && styles.shBtnOn]}><Text style={styles.shTxt}>CLOSE</Text></Pressable>
            </View>
          </View>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  screen:      { flex: 1, backgroundColor: colors.black },
  cameraStage: { flex: 1, overflow: 'hidden', backgroundColor: colors.black },
  webView:     { flex: 1, backgroundColor: colors.black },

  cameraPlaceholder: {
    flex: 1, alignItems: 'center', justifyContent: 'center',
    paddingHorizontal: spacing.xl,
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
    backgroundColor: '#101319',
    borderTopWidth: 1, borderTopColor: 'rgba(255,255,255,0.15)',
    paddingHorizontal: spacing.md, paddingTop: spacing.sm, paddingBottom: spacing.md,
  },
  label: {
    color: 'rgba(255,255,255,0.7)', fontSize: 9, fontWeight: '800',
    letterSpacing: 1.2, marginBottom: 5, marginTop: 4,
  },
  controlRow: { flexDirection: 'row', gap: 6, marginBottom: 6 },

  btn: {
    flex: 1, minHeight: 48, borderRadius: 10,
    backgroundColor: '#1B222C',
    borderWidth: 1.5, borderColor: 'rgba(255,255,255,0.25)',
    alignItems: 'center', justifyContent: 'center',
  },
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
    color: 'rgba(255,255,255,0.8)', fontSize: 9, fontWeight: '800',
    letterSpacing: 0.5, marginTop: 1,
    textShadowColor: 'rgba(0,0,0,0.9)',
    textShadowOffset: {width: 0, height: 1}, textShadowRadius: 3,
  },

  jointGroup: { flex: 1, minWidth: 0 },
  jointLabel: { color: colors.warning, fontSize: 8, fontWeight: '800', marginBottom: 4, textAlign: 'center' },
  jointRow: { flexDirection: 'row', gap: 3 },
  shBtn: {
    flex: 1, minHeight: 44, borderRadius: 8,
    backgroundColor: '#2A2319',
    borderWidth: 1.5, borderColor: 'rgba(255,185,80,0.4)',
    alignItems: 'center', justifyContent: 'center',
  },
  shBtnOn: {
    backgroundColor: 'rgba(255,185,80,0.2)',
    borderColor: 'rgba(255,185,80,1)',
  },
  shTxt: {
    color: colors.warning, fontSize: 8, fontWeight: '800',
  },
});
