# DrainGuard Camera Control Guide

## Dual Connectivity: Bluetooth + WiFi Camera Control

The DrainGuard robot now supports **dual connectivity** for camera control, allowing you to interact with the ESP32-CAM module through both **Bluetooth Low Energy (BLE)** and **WiFi** protocols.

## Overview

### Connection Modes

1. **WiFi Mode** (Primary)
   - High-bandwidth streaming
   - HTTP REST API
   - Best for video streaming and fast operations
   - IP: `192.168.4.50:80` (on DrainGuard hotspot)

2. **Bluetooth Mode** (Secondary)
   - Low-power control
   - BLE GATT service
   - Best for remote control without WiFi
   - Works up to 10-30 meters

3. **Dual Mode** (Recommended)
   - Combine both protocols
   - BLE for control commands
   - WiFi for streaming video

## Bluetooth Camera Commands

### BLE Service UUID
- Service: `7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70`
- RX Characteristic (Write): `7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70`
- TX Characteristic (Notify): `7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70`

### Command Format (JSON over BLE)

All camera commands sent via Bluetooth use JSON format:

```json
{
  "command": "camera",
  "action": "<action_name>",
  "id": <request_id>,
  "value": <optional_value>
}
```

### Supported Camera Actions

#### 1. Capture Photo
```json
{
  "command": "camera",
  "action": "capture",
  "id": 1
}
```

#### 2. Start Streaming
```json
{
  "command": "camera",
  "action": "start_stream",
  "id": 2
}
```

#### 3. Stop Streaming
```json
{
  "command": "camera",
  "action": "stop_stream",
  "id": 3
}
```

#### 4. Set Quality
```json
{
  "command": "camera",
  "action": "set_quality",
  "id": 4,
  "value": 2
}
```
Quality levels:
- `0` = Low (320x240)
- `1` = Medium (640x480) - Default
- `2` = High (800x600)
- `3` = Max (1024x768)

#### 5. Set Brightness
```json
{
  "command": "camera",
  "action": "set_brightness",
  "id": 5,
  "value": 1
}
```
Range: `-2` (darkest) to `+2` (brightest)

#### 6. Set Contrast
```json
{
  "command": "camera",
  "action": "set_contrast",
  "id": 6,
  "value": -1
}
```
Range: `-2` (lowest) to `+2` (highest)

#### 7. Enable Flash
```json
{
  "command": "camera",
  "action": "flash_on",
  "id": 7
}
```

#### 8. Disable Flash
```json
{
  "command": "camera",
  "action": "flash_off",
  "id": 8
}
```

#### 9. Get Camera Status
```json
{
  "command": "camera",
  "action": "get_status",
  "id": 9
}
```

### BLE Response Format

Success response:
```json
{
  "status": "command_result",
  "id": 1,
  "ok": true,
  "message": "capture initiated"
}
```

Camera status response:
```json
{
  "status": "camera_status",
  "id": 9,
  "available": true,
  "streaming": false,
  "quality": 1,
  "brightness": 0,
  "contrast": 0,
  "flash": false
}
```

## WiFi Camera Control

### HTTP REST API Endpoints

All endpoints support CORS and are available at `http://192.168.4.1` (ESP32 main controller).

#### 1. Get Camera Stream URL
```http
GET /api/camera/stream
```

Response:
```json
{
  "available": true,
  "stream_url": "http://192.168.4.50:80/stream"
}
```

#### 2. Capture Photo
```http
POST /api/camera/capture
```

Response:
```json
{
  "status": "captured"
}
```

#### 3. Start Streaming
```http
POST /api/camera/start_stream
```

Response:
```json
{
  "status": "streaming_started"
}
```

#### 4. Stop Streaming
```http
POST /api/camera/stop_stream
```

Response:
```json
{
  "status": "streaming_stopped"
}
```

#### 5. Set Quality
```http
POST /api/camera/quality?value=2
```

Response:
```json
{
  "status": "quality_set",
  "value": 2
}
```

#### 6. Set Brightness
```http
POST /api/camera/brightness?value=1
```

Response:
```json
{
  "status": "brightness_set",
  "value": 1
}
```

#### 7. Set Contrast
```http
POST /api/camera/contrast?value=-1
```

Response:
```json
{
  "status": "contrast_set",
  "value": -1
}
```

#### 8. Set Flash
```http
POST /api/camera/flash?enabled=true
```

Response:
```json
{
  "status": "flash_set",
  "enabled": true
}
```

#### 9. Get Camera Status
```http
GET /api/camera/status
```

Response:
```json
{
  "available": true,
  "streaming": false,
  "quality": 1,
  "brightness": 0,
  "contrast": 0,
  "flash": false
}
```

## Mobile App Integration

### React Native Example (Bluetooth)

```typescript
import { BleManager } from 'react-native-ble-plx';

// Connect to DrainGuard via BLE
const sendCameraCommand = async (action: string, value?: number) => {
  const command = {
    command: "camera",
    action: action,
    id: Date.now(),
    ...(value !== undefined && { value })
  };
  
  const commandStr = JSON.stringify(command) + '\n';
  
  await device.writeCharacteristicWithResponseForService(
    '7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70',
    '7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70',
    base64Encode(commandStr)
  );
};

// Capture photo via BLE
await sendCameraCommand('capture');

// Set quality via BLE
await sendCameraCommand('set_quality', 2);

// Enable flash via BLE
await sendCameraCommand('flash_on');
```

### React Native Example (WiFi)

```typescript
// Capture photo via WiFi
const capturePhoto = async () => {
  const response = await fetch('http://192.168.4.1/api/camera/capture', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
  });
  
  const result = await response.json();
  console.log(result.status); // "captured"
};

// Get camera status
const getCameraStatus = async () => {
  const response = await fetch('http://192.168.4.1/api/camera/status');
  const status = await response.json();
  
  console.log('Camera available:', status.available);
  console.log('Streaming:', status.streaming);
  console.log('Quality:', status.quality);
};
```

## Arduino/C++ Client Example

```cpp
#include <BLEDevice.h>
#include <BLEClient.h>

// Connect to DrainGuard and send camera command
void sendCameraCommand() {
  BLEClient* client = BLEDevice::createClient();
  client->connect(deviceAddress);
  
  BLERemoteService* service = client->getService(
    BLEUUID("7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70")
  );
  
  BLERemoteCharacteristic* rxChar = service->getCharacteristic(
    BLEUUID("7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70")
  );
  
  String command = "{\"command\":\"camera\",\"action\":\"capture\",\"id\":1}\n";
  rxChar->writeValue(command.c_str(), command.length());
  
  Serial.println("Camera capture command sent via BLE");
}
```

## Python Client Example

```python
import asyncio
from bleak import BleakClient

SERVICE_UUID = "7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70"
RX_UUID = "7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70"
TX_UUID = "7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70"

async def send_camera_command(address, action, value=None):
    async with BleakClient(address) as client:
        command = {
            "command": "camera",
            "action": action,
            "id": 1
        }
        if value is not None:
            command["value"] = value
        
        command_str = json.dumps(command) + "\n"
        await client.write_gatt_char(RX_UUID, command_str.encode())
        print(f"Sent camera command: {action}")

# Capture photo
await send_camera_command("XX:XX:XX:XX:XX:XX", "capture")

# Set quality
await send_camera_command("XX:XX:XX:XX:XX:XX", "set_quality", 2)
```

## System Status Endpoint

The main status endpoint now includes camera information:

```http
GET /api/status
```

Response includes:
```json
{
  "device_id": "DRAIN_GUARD_001",
  "water_level": 45.2,
  "camera_available": true,
  "camera_streaming": false,
  "camera_quality": 1,
  ...
}
```

## Troubleshooting

### Camera Not Available
1. Check ESP32-CAM is powered on
2. Verify WiFi connection to DrainGuard hotspot (192.168.4.x)
3. Check camera IP address is `192.168.4.50`
4. Monitor serial output for camera probe messages

### BLE Commands Not Working
1. Ensure BLE is paired and connected
2. Check BLE device name: `DrainGuard-XXXX`
3. Verify command JSON format is correct
4. Add `\n` delimiter at end of command

### WiFi Streaming Issues
1. Check signal strength to hotspot
2. Reduce quality if bandwidth is limited
3. Stop other WiFi-intensive operations
4. Restart ESP32-CAM module

## Performance Recommendations

1. **Use WiFi for streaming** - Better bandwidth and stability
2. **Use BLE for control** - Lower power and faster response
3. **Lower quality for battery** - Use quality=0 or 1 for longer runtime
4. **Monitor free heap** - Check system status for memory issues
5. **Periodic reconnection** - Implement retry logic in your app

## Security Notes

1. BLE connection requires pairing (Just Works mode)
2. WiFi hotspot uses WPA2 password: `DrainGuard123`
3. Change default credentials in production
4. Camera stream is unencrypted (HTTP)
5. Consider implementing authentication for production use

## Future Enhancements

Potential improvements for the camera system:
- Direct BLE-to-ESP32-CAM communication
- HTTPS streaming with encryption
- Motion detection triggers
- Scheduled photo capture
- Cloud storage integration
- Face/object recognition
- Battery level monitoring
- OTA firmware updates

## Support

For issues or questions:
1. Check serial monitor for debug output
2. Review connection status via `/api/status`
3. Test with provided examples
4. Update firmware to latest version
