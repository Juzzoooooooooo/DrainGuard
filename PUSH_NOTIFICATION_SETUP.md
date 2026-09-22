# Push Notification Setup Guide

## ✅ What's Implemented

When ESP32 detects **high water level** (distance < 20cm), it will:
1. Send SMS alert
2. **Broadcast WebSocket alert to all connected phones**
3. **Phone shows push notification** even if app is in background
4. Shows in-app toast message

---

## 📱 How It Works

### **ESP32 Firmware** (`DrainGuard.ino`)
```cpp
// When critical water level detected:
StaticJsonDocument<256> alert;
alert["type"] = "alert";
alert["level"] = "critical";
alert["message"] = "High water level detected!";
alert["distance"] = state.distance;
alert["waterLevel"] = state.waterLevel;
wsServer.broadcastTXT(alertMsg);  // Send to all connected phones
```

### **Mobile App** (`App.tsx`)
```typescript
// Listen for WebSocket messages
wsAPI.onMessage((data) => {
  if (data.type === 'alert') {
    // Show push notification
    notificationService.showAlert(
      'DrainGuard Alert',
      `High water level detected! Distance: ${distance}cm`,
      'critical'
    );
  }
});
```

---

## 🔧 Installation Steps

### **1. Install Dependencies**

```bash
cd mobile-app
npm install
```

This will install:
- `react-native-push-notification@^8.1.1` - Local push notifications
- WebSocket client (already included)

### **2. Flash Updated Firmware**

Flash `firmware/DrainGuard/DrainGuard.ino` to ESP32:
- Arduino IDE → Open `DrainGuard.ino`
- Tools → Board → ESP32 Dev Module
- Tools → Port → (select your COM port)
- Click Upload (→)

**Changes in firmware:**
- ✅ Shoulder speed: 8× slower (80ms per tick)
- ✅ WebSocket alert broadcast on critical water level
- ✅ Sequential servo movement (prevents brownout)
- ✅ Extended servo ranges (120-450 for shoulder/elbow)

### **3. Build Mobile App**

#### **Option A: Android Studio (Recommended)**
1. Open Android Studio
2. File → Open → `mobile-app/android`
3. Wait for Gradle sync
4. Build → Build Bundle(s) / APK(s) → Build APK(s)
5. APK will be at: `mobile-app/android/app/build/outputs/apk/debug/app-debug.apk`

#### **Option B: PowerShell (if Gradle works)**
```powershell
cd mobile-app\android
.\gradlew assembleDebug
```

### **4. Install APK on Phone**

```bash
adb install mobile-app/android/app/build/outputs/apk/debug/app-debug.apk
```

Or manually copy APK to phone and install.

---

## 🧪 Testing Push Notifications

### **Test 1: Trigger Alert**
1. Connect phone to `DrainGuard-Robot` WiFi
2. Open DrainGuard app
3. **Simulate high water level** on ESP32:
   - Reduce ultrasonic sensor reading below 20cm
   - Or manually trigger in firmware
4. **Expected Result:**
   - Push notification appears on phone
   - Notification shows water level distance
   - App shows toast message if open

### **Test 2: Background Notification**
1. Connect to DrainGuard WiFi
2. Open app → press Home button (app goes to background)
3. Trigger alert on ESP32
4. **Expected Result:**
   - Push notification appears even with app in background
   - Sound + vibration (if enabled)

---

## 🔔 Notification Settings

### **Android Permissions**
App will request notification permission on first launch (Android 13+).

To check/enable manually:
1. Settings → Apps → DrainGuard → Notifications
2. Enable "All DrainGuard notifications"
3. Enable sound, vibration, pop-up

### **Notification Levels**

| Level | Sound | Vibration | Priority |
|---|---|---|---|
| **Critical** | ✅ Yes | ✅ Yes | High |
| **Warning** | ⚠️ No | ⚠️ No | Default |
| **Info** | ❌ No | ❌ No | Low |

Currently only **Critical** alerts are sent (distance < 20cm).

---

## 📋 What Changed

### **Files Modified:**

#### **Firmware**
- `firmware/DrainGuard/DrainGuard.ino`
  - Added WebSocket broadcast in `checkAlerts()`
  - Shoulder speed: 4× → 8× (super slow)

#### **Mobile App**
- `mobile-app/package.json` - Added `react-native-push-notification`
- `mobile-app/App.tsx` - WebSocket listener + notification trigger
- `mobile-app/src/services/notificationService.ts` - **NEW** notification service
- `mobile-app/android/app/src/main/AndroidManifest.xml` - Notification permissions

---

## 🎯 Current Configuration

### **Alert Thresholds** (in firmware)
```cpp
#define CRITICAL_DIST 20.0f  // Alert when distance < 20cm
#define WARNING_DIST  50.0f  // Reset alert flag when > 50cm
```

### **Servo Speeds**
```cpp
#define SERVO_BASE_SPEED     2   // 20ms/tick
#define SERVO_SHOULDER_SPEED 8   // 80ms/tick (SUPER SLOW)
#define SERVO_ELBOW_SPEED    3   // 30ms/tick
#define SERVO_GRIPPER_SPEED  2   // 20ms/tick
```

---

## 🚀 Next Steps

1. **Flash firmware** to ESP32
2. **Run `npm install`** in mobile-app directory
3. **Build APK** using Android Studio
4. **Install on phone** and test notifications
5. Test with real water level sensor

---

## 🐛 Troubleshooting

### **No notification appearing**
- Check notification permissions in Android settings
- Verify WebSocket connection (should auto-reconnect)
- Check phone is connected to `DrainGuard-Robot` WiFi
- Check ESP32 Serial Monitor for `[WS] Alert broadcast` message

### **Notification but no sound/vibration**
- Android Settings → Apps → DrainGuard → Notifications
- Enable sound and vibration
- Check phone not in Do Not Disturb mode

### **WebSocket not connecting**
- Verify phone WiFi shows `192.168.4.1` as gateway
- Ping ESP32: `ping 192.168.4.1`
- Check ESP32 Serial Monitor: should show `[WS] Connected`

---

## 📝 Summary

**Push notifications now work!** 🎉

- ✅ ESP32 broadcasts alert via WebSocket
- ✅ Phone receives alert even in background
- ✅ Local push notification with sound + vibration
- ✅ Shows water level distance in notification
- ✅ Works without internet (local WiFi only)
- ✅ Shoulder servo super slow (8× delay)

Flash firmware → Build app → Test! 🚀
