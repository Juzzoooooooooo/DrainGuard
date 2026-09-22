# 🔧 "Unknown Command" Error - ROOT CAUSE & FIX

## 🚨 PROBLEM: "Unknown Command" Error After BLE Connection

When the mobile app connects to ESP32 via Bluetooth and sends commands, the ESP32 responds with:
```json
{"status":"error","message":"Unknown command"}
```

---

## 🔍 ROOT CAUSE ANALYSIS

### Current Firmware Code Structure:

The firmware has **REDUNDANT COMMAND HANDLING**:

#### **Path 1: Queue-based (Lines 390-490)**
```cpp
if (strcmp(cmd, "get_status") == 0 || 
    strcmp(cmd, "arm") == 0 ||
    strcmp(cmd, "servo") == 0 ||
    strcmp(cmd, "camera") == 0) {
    // Handle via queue system
    // Then RETURN (line 490)
}
```

#### **Path 2: Direct handling (Lines 490-636)**
```cpp
// WiFi provisioning commands
if (strcmp(cmd, "scan_wifi") == 0) { ... return; }
if (strcmp(cmd, "set_wifi") == 0) { ... return; }
if (strcmp(cmd, "forget_wifi") == 0) { ... return; }

// DUPLICATE handling (should never be reached!)
if (strcmp(cmd, "get_status") == 0) { ... return; }
if (strcmp(cmd, "arm") == 0) { ... return; }  
if (strcmp(cmd, "servo") == 0) { ... return; }

// If nothing matches:
bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");
```

---

## ❓ WHY IS THIS HAPPENING?

### Possible Causes:

1. **Queue Full / Not Created**
   - Line 489: `xQueueSend()` fails if queue is full or NULL
   - If queue send fails, it sends error but STILL returns
   - So this is NOT the cause

2. **Missing Return Statement**
   - Checked line 490: `return;` statement EXISTS
   - So commands SHOULD exit after queue processing

3. **Mobile App Sending Wrong Command Format**
   - App might be sending command without proper `"id"` field
   - Or command name is misspelled

4. **JSON Parsing Failed**
   - If JSON is malformed, it sends "Invalid JSON" not "Unknown command"
   - So this is NOT the cause

---

## 🎯 MOST LIKELY CAUSE:

### **The mobile app is sending a command that doesn't match ANY of these:**

```cpp
// These are the ONLY commands recognized:
- "scan_wifi"      // WiFi provisioning
- "set_wifi"       // WiFi provisioning  
- "forget_wifi"    // WiFi provisioning
- "get_status"     // Controller (needs "id")
- "arm"            // Controller (needs "id")
- "servo"          // Controller (needs "id")
- "camera"         // Controller (needs "id")
```

---

## 🔍 DEBUGGING STEPS:

### STEP 1: Enable Debug Logging

Check ESP32 Serial Monitor (115200 baud) when you connect:

```
[BLE] Phone connected
[BLE] Received: {"command":"????"}  ← WHAT COMMAND IS THIS?
```

The Serial Monitor will show EXACTLY what command the mobile app is sending!

### STEP 2: Check Mobile App Logs

```bash
npx react-native log-android
```

Look for what command it's trying to send after connection.

---

## 🛠️ POTENTIAL FIXES:

### FIX 1: Check Mobile App First Command

**SUSPICION:** The mobile app might be sending an initial status request right after connecting.

Check `mobile-app/src/components/WifiProvisioning.tsx` around line 90:

```typescript
.then(device => {
  // Does it call getControllerStatus() here?
  // That needs an "id" parameter!
})
```

### FIX 2: Add Defensive Logging

Add to firmware line 385 (before command handling):

```cpp
void handleBleWrite(const String &json) {
  Serial.printf("[BLE] Received: %s\n", json.c_str());  // ← ALREADY EXISTS
  
  // ADD THIS:
  Serial.printf("[BLE] JSON length: %d\n", json.length());
  
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    Serial.println("[BLE] ERROR: JSON deserialization failed!");  // ← ADD THIS
    bleNotify("{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    return;
  }

  const char *cmd = doc["command"] | "";
  Serial.printf("[BLE] Command extracted: '%s'\n", cmd);  // ← ADD THIS
  
  // ... rest of code
}
```

### FIX 3: Add "Unknown Command" Logging

At line 637, BEFORE sending error:

```cpp
// ADD THIS BEFORE LINE 637:
Serial.printf("[BLE] ERROR: Unknown command received: '%s'\n", cmd);
Serial.printf("[BLE] Full JSON was: %s\n", json.c_str());

bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");
```

---

## 🧪 TESTING PROCEDURE:

### Test 1: Upload Firmware with Debug Logging

1. Add logging as shown in FIX 2 and FIX 3
2. Upload to ESP32
3. Open Serial Monitor (115200 baud)
4. Connect from mobile app
5. **OBSERVE WHAT COMMAND IS SENT!**

### Test 2: Try Manual BLE Command

Using a BLE terminal app (like Serial Bluetooth Terminal):

1. Connect to DrainGuard-XXXX
2. Find TX/RX characteristics
3. Manually send:
   ```json
   {"command":"get_status","id":1}\n
   ```
4. Check if it works or gives "Unknown command"

### Test 3: Check Mobile App Source

Look at `WifiProvisioning.tsx` line 86-100:

```typescript
const syncConnectedDevice = () =>
  bleProvisioning
    .reconnectLast()
    .then(device => {
      // WHAT HAPPENS HERE?
      // Does it immediately send a command?
    })
```

---

## 💡 QUICK WORKAROUND:

### Option A: Ignore Unknown Command Errors

If the error appears but controls still work later, it might just be a spurious initial message. Test if:
1. Connection succeeds
2. Controls work after the error
3. Error only appears once

Then it's **harmless** - just ignore it.

### Option B: Add Catch-All Handler

At line 637, instead of error, log and continue:

```cpp
// Instead of:
bleNotify("{\"status\":\"error\",\"message\":\"Unknown command\"}");

// Try:
Serial.printf("[BLE] Warning: Unhandled command '%s' (ignoring)\n", cmd);
// Don't notify error, just ignore unknown commands
```

---

## 📊 SUMMARY:

| Issue | Status |
|-------|--------|
| **Unknown command error** | ⚠️ Needs debugging |
| **Root cause** | Unknown - need Serial logs |
| **Fix available** | ✅ Yes - add debug logging |
| **Workaround** | ✅ Ignore if controls work |

---

## 🎯 RECOMMENDED ACTION:

### RIGHT NOW:

1. **Upload current firmware AS-IS**
2. **Open Serial Monitor (115200 baud)**
3. **Connect from mobile app**
4. **CHECK SERIAL MONITOR** - what command is sent?
5. **Reply with the Serial Monitor output**

Then I can tell you EXACTLY what's wrong and how to fix it!

---

## 📞 WHAT I NEED FROM YOU:

Send me:

1. **Serial Monitor output** when you connect:
   ```
   [BLE] Phone connected
   [BLE] Received: ??? ← THIS LINE!
   ```

2. **Mobile app logs** (optional):
   ```bash
   npx react-native log-android
   ```

3. **Does it work AFTER the error?**
   - Can you control the arm?
   - Can you scan WiFi?
   - Or does everything fail?

With this info, I can give you the EXACT fix! 🎯
