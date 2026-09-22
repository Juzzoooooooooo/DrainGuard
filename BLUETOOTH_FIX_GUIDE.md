# DrainGuard Bluetooth Fix Guide - KOMPLETONG SOLUSYON

## Problema: LAHAT NG BLUETOOTH CONNECTIONS HINDI GUMAGANA

Nag-troubleshoot ako at nahanap ko ang **LAHAT NG POSSIBLE ISSUES**. Sundin mo ito step-by-step.

---

## ✅ STEP 1: CHECK ANDROID PERMISSIONS (AndroidManifest.xml)

**File:** `mobile-app/android/app/src/main/AndroidManifest.xml`

Siguraduhing may ganito:

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.CHANGE_WIFI_STATE" />

<!-- Bluetooth Permissions for Android 12+ (API 31+) -->
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"
                 android:usesPermissionFlags="neverForLocation" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.BLUETOOTH_ADVERTISE" />

<!-- Bluetooth Permissions for Android 11 and below -->
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />

<!-- Feature declarations -->
<uses-feature android:name="android.hardware.bluetooth_le" android:required="true" />
```

---

## ✅ STEP 2: REBUILD ANDROID APP (VERY IMPORTANT!)

```bash
cd mobile-app

# Clean everything
cd android
./gradlew clean
cd ..

# Rebuild from scratch
npx react-native run-android --no-jetifier
```

**HINDI SAPAT ang hot reload!** Kailangan ng **FULL REBUILD** para ma-register ang native BLE module.

---

## ✅ STEP 3: CHECK ESP32 FIRMWARE - BLE ADVERTISING

Ang firmware mo ay may BLE implementation na, pero may potential issues. Here's what to verify:

### 3a. ESP32 Serial Monitor - Check BLE Status

Upload ang firmware tapos tignan sa Serial Monitor (115200 baud):

```
=== DrainGuard Starting ===
[AP] Hotspot started — 192.168.4.1
[BLE] Advertising as: DrainGuard-XXXX    <--- DAPAT MAY GANITO
[HTTP] API ready at http://192.168.4.1
```

**KUNG WALANG "[BLE] Advertising"** = may problema sa ESP32 BLE init.

### 3b. Possible ESP32 Issues:

1. **Kulang ang memory** - BLE + WiFi AP + WebServer = mataas ang memory usage
2. **Bluetooth disabled sa menuconfig** - rare pero possible
3. **Crash during BLE init** - check kung may brownout or watchdog reset

---

## ✅ STEP 4: TEST ANDROID BLE MODULE

Gawa ng simple test sa mobile app. Add this sa `App.tsx`:

```typescript
import {NativeModules} from 'react-native';

// Test if BLE module is loaded
useEffect(() => {
  console.log('DrainGuardBle module:', NativeModules.DrainGuardBle);
  if (NativeModules.DrainGuardBle) {
    NativeModules.DrainGuardBle.isSupported()
      .then((supported: boolean) => {
        console.log('BLE supported:', supported);
      })
      .catch((error: Error) => {
        console.error('BLE check failed:', error);
      });
  } else {
    console.error('❌ WALANG DrainGuardBle MODULE!');
  }
}, []);
```

Run tapos check ang React Native logs:

```bash
npx react-native log-android
```

**Expected output:**
```
DrainGuardBle module: [Object object]
BLE supported: true
```

**KUNG WALANG MODULE** = hindi na-rebuild properly ang Android app.

---

## ✅ STEP 5: TEST BLE SCANNING

Sa mobile app, try this manual test:

```typescript
import {PermissionsAndroid, Platform} from 'react-native';

async function testBleScan() {
  // Request permissions first
  if (Platform.OS === 'android') {
    const granted = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
      PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
    ]);
    console.log('Permissions:', granted);
  }

  // Start scan
  try {
    await NativeModules.DrainGuardBle.startScan();
    console.log('✅ BLE scan started successfully!');
  } catch (error) {
    console.error('❌ BLE scan failed:', error);
  }
}
```

---

## ✅ STEP 6: COMMON ERRORS AND SOLUTIONS

### Error: "BLUETOOTH_DISABLED"
**Solution:** Turn on Bluetooth sa Android settings

### Error: "Permission denied"
**Solution:** Manually enable permissions sa Android Settings > Apps > DrainGuard > Permissions

### Error: "DrainGuard Bluetooth setup is only available on Android"
**Solution:** Testing mo ba sa iOS? Currently Android-only implementation

### Error: "Unable to start DrainGuard service discovery"
**Solution:** 
- ESP32 may not be advertising BLE properly
- Check Serial Monitor ng ESP32
- Try power cycle ng ESP32

### Error: Walang devices na lumalabas sa scan
**Possible causes:**
1. ESP32 BLE hindi nag-start (check serial monitor)
2. Bluetooth permission denied
3. Location permission denied (required for BLE scanning on Android <12)
4. Bluetooth adapter disabled sa phone

---

## ✅ STEP 7: ESP32 BLE DEBUGGING

Kung walang lumalabas na devices, i-verify ang ESP32:

### Using nRF Connect app (Android):

1. Install "nRF Connect for Mobile" from Play Store
2. Open app, scan for devices
3. Look for "DrainGuard-XXXX"
4. Check if service UUID `7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70` is advertised

**KUNG MAKITA MO SA nRF Connect pero hindi sa app:**
- App permissions issue
- Module not properly registered

**KUNG HINDI MAKITA KAHIT SA nRF Connect:**
- ESP32 BLE not initialized
- ESP32 crashed or low memory
- Wrong firmware uploaded

---

## ✅ STEP 8: INCREASE ESP32 LOGGING

Add sa firmware setup():

```cpp
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== DrainGuard Starting ===");
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("ESP32 Chip ID: %04X\n", (uint16_t)(ESP.getEfuseMac() & 0xFFFF));
  
  // ... existing setup code ...
  
  Serial.println("[BLE] Initializing...");
  initBLE();
  Serial.printf("[BLE] Done! Advertising as: %s\n", deviceName.c_str());
  Serial.printf("Free heap after BLE: %d bytes\n", ESP.getFreeHeap());
}
```

This will show kung nag-crash during BLE init.

---

## ✅ STEP 9: ULTIMATE RESET

Kung wala pa ring gumagana after all steps:

```bash
# 1. RESET ANDROID BUILD
cd mobile-app/android
./gradlew clean
rm -rf build
rm -rf app/build
cd ../..

# 2. RESET NODE MODULES
cd mobile-app
rm -rf node_modules
rm -f package-lock.json
npm install

# 3. RESET METRO
npx react-native start --reset-cache

# 4. REBUILD (in another terminal)
cd mobile-app
npx react-native run-android
```

# 5. FLASH ESP32 FRESH
- Erase flash: `pio run -t erase`
- Upload: `pio run -t upload`
- Monitor: `pio device monitor -b 115200`

---

## ✅ EXPECTED WORKING FLOW

### ESP32 Serial Output:
```
=== DrainGuard Starting ===
Free heap: 298420 bytes
ESP32 Chip ID: A3F2
[AP] Hotspot started — 192.168.4.1
[BLE] Initializing...
[BLE] Done! Advertising as: DrainGuard-A3F2
Free heap after BLE: 245680 bytes
[HTTP] API ready at http://192.168.4.1
```

### Android Logs:
```
DrainGuardBle module: [Object object]
BLE supported: true
Permissions: {BLUETOOTH_SCAN: granted, BLUETOOTH_CONNECT: granted}
BLE scan started successfully!
```

### Mobile App UI:
```
Status: Looking for DrainGuard controllers
[Device List]
  DrainGuard-A3F2 (Strong signal · XX:XX:XX:XX:XX:XX)
    [CONNECT]
```

---

## 🚨 KUNG WALA PA RIN AFTER LAHAT NG ITO

Reply mo sa akin with these logs:

1. **ESP32 Serial Monitor** - full output from boot
2. **Android Logcat** - `adb logcat | grep -i bluetooth`
3. **React Native logs** - `npx react-native log-android`
4. **nRF Connect screenshot** - kung nakikita mo ba ang DrainGuard device

Tutulungan pa kita!

---

## 📱 QUICK TEST CHECKLIST

- [ ] AndroidManifest.xml has all BLE permissions
- [ ] Android app rebuilt from scratch (`./gradlew clean` then rebuild)
- [ ] Bluetooth enabled on phone
- [ ] Location enabled on phone (required for BLE on Android <12)
- [ ] Permissions granted in Android Settings > Apps > DrainGuard
- [ ] ESP32 firmware uploaded successfully
- [ ] ESP32 Serial Monitor shows "[BLE] Advertising"
- [ ] nRF Connect app can see DrainGuard-XXXX
- [ ] DrainGuardBle module exists in NativeModules
- [ ] BLE scan starts without permission errors

---

**Good luck, pre! Sundin mo lang yan one by one, gagana yan. Reply ka kung may specific error message para matulungan pa kita!** 💪
