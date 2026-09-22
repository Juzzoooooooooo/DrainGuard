# DrainGuard Camera Control - Installation Checklist

Use this checklist to install and test the new camera control features.

## ✅ Pre-Installation

### Hardware Check
- [ ] ESP32 DevKit V1 (main controller) is accessible
- [ ] ESP32-CAM module is available
- [ ] FTDI USB-to-Serial adapter for ESP32-CAM
- [ ] Both modules have power supply
- [ ] USB cables ready for programming

### Software Check
- [ ] Arduino IDE 1.8+ or PlatformIO installed
- [ ] ESP32 board support installed
- [ ] ArduinoJson library installed (already in project)
- [ ] Serial monitor tool ready

## 📥 Step 1: Backup Current System

- [ ] Save current firmware (if you want to roll back)
- [ ] Take photo of current wiring
- [ ] Document current IP addresses
- [ ] Note any custom settings

## 🔧 Step 2: Upload Main Controller Firmware

### Upload to ESP32 DevKit V1
1. [ ] Open `firmware/DrainGuard/DrainGuard.ino` in Arduino IDE
2. [ ] Select Board: "ESP32 Dev Module"
3. [ ] Select Port: Your ESP32's COM port
4. [ ] Upload speed: 921600
5. [ ] Click Upload
6. [ ] Wait for "Hard resetting via RTS pin..."
7. [ ] Open Serial Monitor (115200 baud)
8. [ ] Verify startup messages appear

### Expected Serial Output
```
=== DrainGuard Starting ===
[AP] Hotspot started — 192.168.4.1
[BLE] Advertising as: DrainGuard-XXXX
[HTTP] API ready at http://192.168.4.1
```

- [ ] Hotspot started successfully
- [ ] BLE advertising active
- [ ] HTTP server running
- [ ] No error messages

## 📷 Step 3: Upload ESP32-CAM Firmware

### Prepare ESP32-CAM
1. [ ] Connect FTDI adapter:
   - [ ] FTDI RX → ESP32-CAM TX
   - [ ] FTDI TX → ESP32-CAM RX
   - [ ] FTDI GND → ESP32-CAM GND
   - [ ] FTDI 5V → ESP32-CAM 5V
2. [ ] Connect GPIO0 to GND (enter programming mode)
3. [ ] Press Reset button

### Upload Firmware
1. [ ] Open `firmware/ESP32-CAM/DrainGuard_Camera.ino`
2. [ ] Select Board: "AI Thinker ESP32-CAM"
3. [ ] Select Port: FTDI COM port
4. [ ] Click Upload
5. [ ] Wait for upload complete
6. [ ] Disconnect GPIO0 from GND
7. [ ] Press Reset button

### Expected Serial Output
```
=== DrainGuard ESP32-CAM ===
PSRAM found - using high quality
Camera initialized successfully
Connecting to DrainGuard hotspot... Connected!
IP: 192.168.4.50
Stream URL: http://192.168.4.50/stream
DrainGuard Camera Ready!
```

- [ ] Camera initialized
- [ ] Connected to hotspot
- [ ] IP is 192.168.4.50
- [ ] Ready message displayed

## 🔌 Step 4: Power Up System

1. [ ] Power ESP32-CAM via 5V supply
2. [ ] Power ESP32 DevKit V1
3. [ ] Wait 30 seconds for connection
4. [ ] Check both serial monitors for errors

## 🧪 Step 5: Basic WiFi Testing

### Test from Computer/Phone

1. [ ] Connect to WiFi: `DrainGuard-Robot`
2. [ ] Password: `DrainGuard123`
3. [ ] Open browser or terminal

### Test Main Controller
```bash
curl http://192.168.4.1/api/status
```
Expected: JSON with `"camera_available": true`

- [ ] Request succeeds
- [ ] Camera available is true
- [ ] All fields present

### Test Camera Direct
```bash
curl http://192.168.4.50/status
```
Expected: JSON with camera settings

- [ ] Request succeeds
- [ ] Camera responds
- [ ] Settings visible

### Test Camera Stream
Open in browser: `http://192.168.4.50/stream`

- [ ] Stream loads
- [ ] Video appears
- [ ] Frame rate acceptable
- [ ] No errors in console

## 📡 Step 6: Test WiFi Camera Control

### Test Quality Change
```bash
curl -X POST "http://192.168.4.1/api/camera/quality?value=0"
```
Expected: `{"status":"quality_set","value":0}`

- [ ] Command succeeds
- [ ] Response correct
- [ ] Stream quality changed

### Test Brightness
```bash
curl -X POST "http://192.168.4.1/api/camera/brightness?value=1"
```
Expected: `{"status":"brightness_set","value":1}`

- [ ] Command succeeds
- [ ] Image brightened

### Test Contrast
```bash
curl -X POST "http://192.168.4.1/api/camera/contrast?value=-1"
```
Expected: `{"status":"contrast_set","value":-1}`

- [ ] Command succeeds
- [ ] Contrast changed

### Test Flash
```bash
curl -X POST "http://192.168.4.1/api/camera/flash?enabled=true"
```
Expected: `{"status":"flash_set","enabled":true}`

- [ ] Command succeeds
- [ ] Flash setting updated

### Test Capture
```bash
curl -X POST http://192.168.4.1/api/camera/capture
```
Expected: `{"status":"captured"}`

- [ ] Command succeeds
- [ ] Photo captured message

### Test Stream Control
```bash
curl -X POST http://192.168.4.1/api/camera/start_stream
curl -X POST http://192.168.4.1/api/camera/stop_stream
```

- [ ] Start command works
- [ ] Stop command works
- [ ] Status updates correctly

## 📱 Step 7: Test Bluetooth Control

### Connect via BLE

#### Using nRF Connect (Mobile App)
1. [ ] Install nRF Connect app
2. [ ] Scan for devices
3. [ ] Find `DrainGuard-XXXX`
4. [ ] Connect to device
5. [ ] Pair when prompted

#### Discover Services
1. [ ] Find service `7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70`
2. [ ] Find RX char `7b0d1002-...` (Write)
3. [ ] Find TX char `7b0d1003-...` (Notify)
4. [ ] Enable notifications on TX

### Test BLE Commands

#### Capture Photo
Write to RX (as UTF-8 Text):
```json
{"command":"camera","action":"capture","id":1}

```
**Note:** Include newline at end!

Expected notification on TX:
```json
{"status":"command_result","id":1,"ok":true,"message":"capture initiated"}
```

- [ ] Command sent
- [ ] Response received
- [ ] Photo captured

#### Get Camera Status
Write to RX:
```json
{"command":"camera","action":"get_status","id":2}

```

Expected:
```json
{
  "status":"camera_status",
  "id":2,
  "available":true,
  "streaming":false,
  "quality":1,
  "brightness":0,
  "contrast":0,
  "flash":false
}
```

- [ ] Status received
- [ ] All fields present
- [ ] Values correct

#### Set Quality
Write to RX:
```json
{"command":"camera","action":"set_quality","id":3,"value":2}

```

- [ ] Command accepted
- [ ] Quality changed

#### Set Brightness
Write to RX:
```json
{"command":"camera","action":"set_brightness","id":4,"value":1}

```

- [ ] Brightness adjusted
- [ ] Response received

#### Enable Flash
Write to RX:
```json
{"command":"camera","action":"flash_on","id":5}

```

- [ ] Flash enabled
- [ ] Confirmation received

#### Disable Flash
Write to RX:
```json
{"command":"camera","action":"flash_off","id":6}

```

- [ ] Flash disabled

## 🔍 Step 8: Integration Testing

### Test Combined Operations
1. [ ] Capture photo via BLE
2. [ ] Change quality via WiFi
3. [ ] View stream in browser
4. [ ] Adjust brightness via BLE
5. [ ] Check status via WiFi API
6. [ ] Enable flash via BLE
7. [ ] Capture with flash
8. [ ] Disable flash via WiFi

### Test Concurrent Operations
1. [ ] Stream in browser
2. [ ] Send BLE commands
3. [ ] Both work simultaneously
4. [ ] No crashes or hangs

### Test Error Conditions
1. [ ] Send invalid BLE command
   - [ ] Error message received
2. [ ] Invalid WiFi endpoint
   - [ ] 404 or error response
3. [ ] Disconnect ESP32-CAM
   - [ ] camera_available becomes false
   - [ ] System continues working

## 📊 Step 9: Performance Testing

### Measure Response Times
- [ ] BLE command latency: <500ms
- [ ] WiFi API latency: <200ms
- [ ] Stream startup time: <3s
- [ ] Photo capture time: <1s

### Test Different Qualities
- [ ] Quality 0 (320x240): Smooth streaming
- [ ] Quality 1 (640x480): Good balance
- [ ] Quality 2 (800x600): Higher quality
- [ ] Quality 3 (1024x768): Max quality

### Monitor System Health
Check `/api/status`:
- [ ] `free_heap` > 50000 bytes
- [ ] `camera_available` = true
- [ ] No brownout resets
- [ ] Uptime increasing

## 🎯 Step 10: Mobile App Integration

### Update Mobile App
1. [ ] Add camera control UI
2. [ ] Implement BLE commands
3. [ ] Add WiFi fallback
4. [ ] Test both protocols
5. [ ] Verify stream display

### Test Complete User Flow
1. [ ] Open mobile app
2. [ ] Connect via BLE
3. [ ] View live stream
4. [ ] Adjust camera settings
5. [ ] Capture photos
6. [ ] Save images
7. [ ] Disconnect cleanly

## ✅ Final Verification

### System Status
- [ ] Main controller running stable
- [ ] ESP32-CAM streaming smoothly
- [ ] BLE commands responsive
- [ ] WiFi API accessible
- [ ] No memory leaks
- [ ] No crashes in 1 hour test

### Documentation Review
- [ ] Read CAMERA_CONTROL_GUIDE.md
- [ ] Review CAMERA_QUICK_REFERENCE.md
- [ ] Understand all commands
- [ ] Know troubleshooting steps

### Cleanup
- [ ] Remove test code if any
- [ ] Organize wiring
- [ ] Label components
- [ ] Document custom settings

## 🐛 Troubleshooting Guide

### Issue: Camera Not Available
**Check:**
1. [ ] ESP32-CAM powered on
2. [ ] WiFi credentials correct
3. [ ] IP address 192.168.4.50 responds
4. [ ] Check ESP32-CAM serial output

### Issue: BLE Commands Fail
**Check:**
1. [ ] Device paired
2. [ ] JSON format correct
3. [ ] Newline included at end
4. [ ] Request ID provided
5. [ ] Check ESP32 serial output

### Issue: Stream Laggy
**Solutions:**
1. [ ] Lower quality to 0 or 1
2. [ ] Move closer to hotspot
3. [ ] Restart ESP32-CAM
4. [ ] Check signal strength

### Issue: Commands Timeout
**Check:**
1. [ ] WiFi signal strong
2. [ ] IP addresses correct
3. [ ] No network congestion
4. [ ] Free heap sufficient

## 📝 Notes & Observations

### Installation Date: _____________

### Serial Numbers:
- ESP32 DevKit: _____________
- ESP32-CAM: _____________

### Custom Configuration:
```
WiFi SSID: _____________
WiFi Password: _____________
Camera IP: _____________
```

### Issues Encountered:
1. _________________________________
2. _________________________________
3. _________________________________

### Solutions Applied:
1. _________________________________
2. _________________________________
3. _________________________________

### Performance Notes:
- Stream quality: _____________
- BLE latency: _____________
- Battery life: _____________
- Signal range: _____________

## 🎉 Success Criteria

All checkboxes completed = ✅ Installation successful!

- [ ] Main firmware uploaded
- [ ] Camera firmware uploaded
- [ ] WiFi API tested (8 endpoints)
- [ ] BLE commands tested (9 commands)
- [ ] Stream working
- [ ] Quality adjustment works
- [ ] Brightness/contrast work
- [ ] Flash control works
- [ ] No errors in serial monitor
- [ ] System stable for 1 hour
- [ ] Mobile app integration tested

## 📞 Support

If you encounter issues:
1. Check serial monitor at 115200 baud
2. Review CAMERA_CONTROL_GUIDE.md
3. Test individual components
4. Check network connectivity
5. Verify power supply

---

**Congratulations!** You now have dual connectivity camera control on your DrainGuard robot! 🎉

Next steps:
- Integrate with mobile app UI
- Add advanced features
- Test in real-world scenarios
- Enjoy the enhanced capabilities!
