# DrainGuard — Build APK Guide

## Prerequisites

Before building, make sure you have the following installed:

- [Node.js 18+](https://nodejs.org/)
- [Android Studio](https://developer.android.com/studio) (includes JDK 17/21 and Android SDK)
- USB cable (if installing directly to phone)

---

## Step 1 — Install Node Dependencies

Open PowerShell or CMD, then run:

```powershell
cd C:\Users\Juzzoooo\Downloads\Drainguard\DrainGuard\mobile-app
npm install
```

This installs:
- `react-native-push-notification` — local push notifications
- `react-native-webview` — camera stream
- All other React Native dependencies

---

## Step 2 — Build APK (Android Studio)

This is the **most reliable** method.

1. Open **Android Studio**
2. Click **File → Open**
3. Navigate to:
   ```
   C:\Users\Juzzoooo\Downloads\Drainguard\DrainGuard\mobile-app\android
   ```
4. Click **OK** and wait for Gradle sync to complete
5. Once synced, go to **Build → Build Bundle(s) / APK(s) → Build APK(s)**
6. Wait for the build to finish (usually 2–5 minutes)
7. Click **locate** in the popup, or find the APK at:
   ```
   mobile-app\android\app\build\outputs\apk\debug\app-debug.apk
   ```

---

## Step 3 — Install APK on Phone

### Option A — USB (ADB)

Make sure **USB Debugging** is enabled on your phone:
- Settings → Developer Options → USB Debugging → ON

Then run:

```powershell
adb install C:\Users\Juzzoooo\Downloads\Drainguard\DrainGuard\mobile-app\android\app\build\outputs\apk\debug\app-debug.apk
```

### Option B — Manual Install

1. Copy `app-debug.apk` to your phone (via USB or file sharing)
2. Open the file on your phone
3. If prompted, enable **Install from unknown sources**:
   - Settings → Security → Install unknown apps → Allow
4. Tap **Install**

---

## Step 4 — First Launch

1. Connect phone to **DrainGuard-Robot** WiFi
   - Password: `DrainGuard123`
2. Open the **DrainGuard** app
3. When prompted, tap **Allow** for notification permission
4. App will auto-connect to ESP32 at `192.168.4.1`

---

## What's in This Build

### Firmware Changes (flash `DrainGuard.ino` separately)

| Feature | Value |
|---|---|
| Base LEFT/RIGHT | 5 ticks per press (small step) |
| Shoulder speed | 8× slower (80ms/tick) |
| Elbow speed | 3× slower (30ms/tick) |
| Gripper speed | 2× slower (20ms/tick) |
| Shoulder range | 120–450 ticks |
| Elbow range | 250–450 ticks |
| Gripper range | 350–510 ticks |
| Push alerts | WebSocket broadcast on high water |

### Mobile App Changes

| File | Change |
|---|---|
| `App.tsx` | WebSocket listener + notification trigger |
| `notificationService.ts` | NEW — local push notification handler |
| `CameraScreen.tsx` | Gripper open/close direction fixed |
| `AndroidManifest.xml` | Notification + vibration permissions added |
| `package.json` | Added `react-native-push-notification` |

---

## Troubleshooting

### ❌ `JAVA_HOME is not set`

Set it manually in PowerShell before building:

```powershell
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
$env:PATH = "$env:JAVA_HOME\bin;$env:PATH"
```

Then retry:

```powershell
cd mobile-app\android
.\gradlew.bat assembleDebug
```

### ❌ `Execution failed for task ':gradle-plugin:compileKotlin'`

Use Android Studio to build instead of PowerShell. This error is caused by a Kotlin/Gradle version mismatch that Android Studio resolves automatically.

### ❌ `App installed but not connecting`

- Make sure phone WiFi is set to **DrainGuard-Robot**
- ESP32 must be powered and running the latest firmware
- Check `192.168.4.1` is reachable (open in browser)

### ❌ `No push notifications`

- Settings → Apps → DrainGuard → Notifications → Enable all
- Make sure app has notification permission (tap Allow on first launch)
- Make sure phone is not in Do Not Disturb mode

### ❌ `Gripper not opening/closing correctly`

- OPEN → decreases ticks (410 → 350)
- CLOSE → increases ticks (410 → 510)
- If reversed, check `CameraScreen.tsx` gripperOpen/gripperClose directions

---

## Flash Firmware (Separate Step)

File to flash:
```
C:\Users\Juzzoooo\Downloads\Drainguard\DrainGuard\firmware\DrainGuard\DrainGuard.ino
```

Steps:
1. Open **Arduino IDE**
2. **File → Open** → select `DrainGuard.ino`
3. **Tools → Board** → ESP32 Dev Module
4. **Tools → Port** → select your COM port
5. Click **Upload (→)**
6. Wait for `Done uploading`

---

## Quick Build Summary

```
1. cd mobile-app && npm install
2. Open Android Studio → File → Open → mobile-app/android
3. Build → Build APK(s)
4. Install APK on phone
5. Flash DrainGuard.ino to ESP32
6. Connect phone to DrainGuard-Robot WiFi
7. Open app → Allow notification permission
8. Done!
```
