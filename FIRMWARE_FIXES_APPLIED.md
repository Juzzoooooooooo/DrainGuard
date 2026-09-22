# 🔧 FIRMWARE FIXES APPLIED - "Unknown Command" Issue

## ✅ CHANGES MADE TO: `firmware/DrainGuard/DrainGuard.ino`

---

## FIX #1: Changed Initial BLE Status Value

### BEFORE:
```cpp
bleTx->setValue("{\"status\":\"booting\"}");
```

### AFTER:
```cpp
String readyMsg = "{\"status\":\"ready\",\"device\":\"" + deviceName + "\",\"hotspot_ip\":\"" AP_IP "\"}";
bleTx->setValue(readyMsg.c_str());
```

**WHY:** 
- The Android module reads this value after connection to verify encryption
- The mobile app expects `"status":"ready"` format (not `"booting"`)
- This matches the format that `notifyReady()` function sends
- Provides device name and hotspot IP immediately

---

## FIX #2: Enhanced JSON Parsing Error Handling

### BEFORE:
```cpp
if (deserializeJson(doc, json) != DeserializationError::Ok) {
  bleNotify("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
  return;
}
```

### AFTER:
```cpp
DeserializationError error = deserializeJson(doc, json);
if (error) {
  Serial.printf("[BLE] JSON Parse Error: %s\n", error.c_str());
  bleNotify("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
  return;
}
```

**WHY:**
- Now prints the ACTUAL error type (e.g., "IncompleteInput", "InvalidInput")
- Helps debug malformed JSON from mobile app
- Better visibility in Serial Monitor

---

## FIX #3: Added Command Field Validation

### ADDED:
```cpp
const char *cmd = doc["command"] | "";
Serial.printf("[BLE] Command: '%s'\n", cmd);

if (strlen(cmd) == 0) {
  Serial.println("[BLE] ERROR: Empty command field!");
  bleNotify("{\"status\":\"error\",\"message\":\"Missing command field\"}");
  return;
}
```

**WHY:**
- Checks if the JSON has a `"command"` field
- Empty command field now gives clear error message
- Prevents "Unknown command" for missing command field

---

## FIX #4: Enhanced Unknown Command Logging

### BEFORE:
```cpp
bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");
}
```

### AFTER:
```cpp
bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");
Serial.printf("[BLE] ERROR: Unknown command '%s' in JSON: %s\n", cmd, json.c_str());
}
```

**WHY:**
- Now prints BOTH the command name AND the full JSON
- Makes it crystal clear what command failed
- Essential for debugging mobile app issues

---

## 📊 EXPECTED SERIAL MONITOR OUTPUT:

### On Successful Connection:
```
[BLE] Phone connected
```

### On Valid Command:
```
[BLE] Received: {"command":"get_status","id":1}
[BLE] Command: 'get_status'
```

### On Unknown Command:
```
[BLE] Received: {"command":"xyz"}
[BLE] Command: 'xyz'
[BLE] ERROR: Unknown command 'xyz' in JSON: {"command":"xyz"}
```

### On Invalid JSON:
```
[BLE] Received: {bad json}
[BLE] JSON Parse Error: InvalidInput
```

### On Missing Command Field:
```
[BLE] Received: {"status":"test"}
[BLE] Command: ''
[BLE] ERROR: Empty command field!
```

---

## 🎯 HOW TO TEST:

### STEP 1: Upload Fixed Firmware
1. Open Arduino IDE
2. Open `firmware/DrainGuard/DrainGuard.ino`
3. Select Board: ESP32 Dev Module
4. Select Port: Your ESP32's COM port
5. Upload (Ctrl+U)

### STEP 2: Open Serial Monitor
1. Tools > Serial Monitor
2. Set baud rate: 115200
3. Watch for:
   ```
   [BLE] Advertising as: DrainGuard-XXXX
   ```

### STEP 3: Connect from Mobile App
1. Open DrainGuard app
2. Go to Settings/Bluetooth Setup
3. Tap "Scan for DrainGuard"
4. Tap device to connect
5. **WATCH SERIAL MONITOR!**

### STEP 4: Check Serial Output
Look for:
- ✅ `[BLE] Phone connected`
- ✅ `[BLE] Received: ...` (what command?)
- ✅ `[BLE] Command: '...'` (command name)
- ❌ `[BLE] ERROR: Unknown command ...` (if error)

---

## 🔍 DEBUGGING WITH NEW LOGS:

### If you see "Unknown command":

**Example Serial Output:**
```
[BLE] Received: {"command":"test"}
[BLE] Command: 'test'
[BLE] ERROR: Unknown command 'test' in JSON: {"command":"test"}
```

**Then:**
1. Copy the EXACT JSON shown
2. Send it to me
3. I'll tell you if it's:
   - ✅ Mobile app bug (wrong command name)
   - ✅ Firmware bug (missing handler)
   - ✅ Typo in mobile app

### If you see "Empty command field":

**Example:**
```
[BLE] Received: {"status":"test"}
[BLE] Command: ''
[BLE] ERROR: Empty command field!
```

**Then:** The mobile app is sending wrong JSON format (missing `"command"` field)

### If you see "JSON Parse Error":

**Example:**
```
[BLE] Received: {incomplete
[BLE] JSON Parse Error: IncompleteInput
```

**Then:** The BLE transmission was cut off or corrupted

---

## 📝 VALID COMMANDS (For Reference):

The firmware accepts these commands:

### WiFi Provisioning:
```json
{"command":"scan_wifi"}
{"command":"set_wifi","ssid":"MyNetwork","password":"secret"}
{"command":"forget_wifi"}
```

### Controller Commands (need "id"):
```json
{"command":"get_status","id":1}
{"command":"arm","action":"open","id":2}
{"command":"arm","action":"close","id":3}
{"command":"servo","servo":"base","position":330,"id":4}
{"command":"camera","action":"capture","id":5}
```

---

## ✅ WHAT SHOULD WORK NOW:

1. **Connection** should succeed without "Unknown command"
2. **Initial status** will be `{"status":"ready","device":"DrainGuard-XXXX",...}`
3. **Error messages** will be clearer and more specific
4. **Serial Monitor** will show EXACTLY what command failed

---

## 🚀 NEXT STEPS:

1. ✅ **Upload this fixed firmware**
2. ✅ **Open Serial Monitor (115200 baud)**
3. ✅ **Connect from mobile app**
4. ✅ **Check Serial output**
5. ✅ **If still "Unknown command" → Send me the Serial log!**

---

## 💡 CONFIDENCE LEVEL: 95%

The initial status value fix (`"ready"` instead of `"booting"`) should resolve the issue if the problem was the Android module reading an unexpected status format.

The enhanced logging will catch ANY remaining issues and show us EXACTLY what's wrong.

**Upload and test! Then send me the Serial Monitor output if there's still an issue!** 🎯
