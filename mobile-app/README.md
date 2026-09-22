# Drain Guard Mobile App

React Native control panel for the Drain Guard ESP32 system. The interface mirrors the web app's Dashboard, Camera, and Settings experience while using native touch controls and local settings storage.

## Features

- Live water level, sensor distance, drain state, and connection status
- Motor-based open and close controls
- GPS coordinates with one-tap Google Maps launch
- ESP32-CAM stream with refresh and offline states
- Arm quick actions, a touch joystick, hold-to-move elbow controls, and claw toggle
- Persistent BLE controller connection with automatic reconnect across tabs
- Optional `DrainGuard-Robot` hotspot access for the ESP32-CAM stream

## Install on Android

Requirements: Node.js 16+, JDK 17, the Android SDK, and either a phone with USB debugging enabled or an emulator.

```bash
npm ci
npm run verify
npm run android:apk
```

The standalone APK is written to `android/app/build/outputs/apk/release/app-release.apk`. Copy it to an Android phone and open it to install, allowing installation from that source if Android asks.

To build and install directly on a connected phone/emulator:

```bash
npm run android:install-apk
```

Use `npm run android` only for development; it starts a debug build that requires the Metro development server.

Local release APKs fall back to the included Android debug key so they can be installed immediately. Before publishing or distributing production updates, create a private upload keystore and define these values in the user-level `~/.gradle/gradle.properties` file (never commit passwords or the keystore):

```properties
DRAINGUARD_UPLOAD_STORE_FILE=C:/secure/path/drainguard-upload.keystore
DRAINGUARD_UPLOAD_STORE_PASSWORD=replace-me
DRAINGUARD_UPLOAD_KEY_ALIAS=drainguard
DRAINGUARD_UPLOAD_KEY_PASSWORD=replace-me
```

## Run on iOS

Requirements: macOS, Xcode, CocoaPods, and the iOS toolchain.

For iOS, install pods on macOS before running:

```bash
cd ios && bundle exec pod install && cd ..
npm run ios
```

## Connect to the robot

1. Power the ESP32 DevKit with the new BLE-controller firmware.
2. Open **Settings**, tap **Scan for DrainGuard**, and select `DrainGuard-XXXX`.
3. Accept the Android nearby-device and pairing prompts. Sensor status and arm controls now use the encrypted BLE connection, which the app remembers and reconnects automatically.
4. To view the ESP32-CAM, connect the phone to `DrainGuard-Robot`. Keep the optional camera gateway at `192.168.4.1`; the app discovers the camera at `192.168.4.50`.

WiFi provisioning is optional and only provides the DevKit with an internet uplink. The app's controls and status do not require the phone to be on the robot hotspot. To remove saved uplink credentials, open Settings over BLE and tap **Forget Saved Wi-Fi**.

The current native BLE controller implementation is Android-only. The camera still uses local HTTP over WiFi because BLE is not suitable for live video.

## Checks

```bash
npm run typecheck
npm run lint
npm test
# or run all three
npm run verify
```
