# Drain Guard Mobile App

React Native control panel for the Drain Guard ESP32 system. The interface mirrors the web app's Dashboard, Camera, and Settings experience while using native touch controls and local settings storage.

## Features

- Live water level, sensor distance, drain state, and connection status
- Motor-based open and close controls
- GPS coordinates with one-tap Google Maps launch
- ESP32-CAM stream with refresh and offline states
- Arm quick actions, a touch joystick, hold-to-move elbow controls, and claw toggle
- Persistent controller address, refresh rate, and warning thresholds
- Support for the private `DrainGuard-Robot` hotspot at `192.168.4.1`

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

1. Connect the phone to the `DrainGuard-Robot` Wi-Fi network.
2. Open Settings in the app.
3. Keep the controller IP at `192.168.4.1` so both robot commands and camera access use the private hotspot.
4. Save settings and return to the Dashboard.

The app uses unencrypted HTTP because it talks directly to the ESP32 on a private local network. Android cleartext traffic and iOS local-network access are enabled in the native project for this purpose.

Bluetooth provisioning gives the controller an optional internet uplink, but local controls and the ESP32-CAM continue to use the private hotspot. After provisioning, keep the phone on `DrainGuard-Robot` and keep the controller address at `192.168.4.1`. To remove the saved uplink, reconnect to the controller by Bluetooth and tap **Forget Saved Wi-Fi**.

## Checks

```bash
npm run typecheck
npm run lint
npm test
# or run all three
npm run verify
```
