# DrainGuard Camera Control - Quick Reference

## Bluetooth Commands

All commands use JSON format with `\n` delimiter.

### Basic Commands

| Action | Command JSON |
|--------|-------------|
| **Capture Photo** | `{"command":"camera","action":"capture","id":1}` |
| **Start Stream** | `{"command":"camera","action":"start_stream","id":2}` |
| **Stop Stream** | `{"command":"camera","action":"stop_stream","id":3}` |
| **Get Status** | `{"command":"camera","action":"get_status","id":4}` |

### Settings Commands

| Action | Command JSON |
|--------|-------------|
| **Quality Low** | `{"command":"camera","action":"set_quality","id":5,"value":0}` |
| **Quality Medium** | `{"command":"camera","action":"set_quality","id":5,"value":1}` |
| **Quality High** | `{"command":"camera","action":"set_quality","id":5,"value":2}` |
| **Quality Max** | `{"command":"camera","action":"set_quality","id":5,"value":3}` |
| **Brightness** | `{"command":"camera","action":"set_brightness","id":6,"value":0}` |
| **Contrast** | `{"command":"camera","action":"set_contrast","id":7,"value":0}` |
| **Flash On** | `{"command":"camera","action":"flash_on","id":8}` |
| **Flash Off** | `{"command":"camera","action":"flash_off","id":9}` |

## WiFi Endpoints

Base URL: `http://192.168.4.1`

### GET Endpoints

| Endpoint | Description |
|----------|-------------|
| `/api/camera/stream` | Get camera stream URL |
| `/api/camera/status` | Get camera status and settings |
| `/api/status` | Get complete system status (includes camera) |

### POST Endpoints

| Endpoint | Parameters | Example |
|----------|-----------|---------|
| `/api/camera/capture` | None | `POST /api/camera/capture` |
| `/api/camera/start_stream` | None | `POST /api/camera/start_stream` |
| `/api/camera/stop_stream` | None | `POST /api/camera/stop_stream` |
| `/api/camera/quality` | `value` (0-3) | `POST /api/camera/quality?value=2` |
| `/api/camera/brightness` | `value` (-2 to 2) | `POST /api/camera/brightness?value=1` |
| `/api/camera/contrast` | `value` (-2 to 2) | `POST /api/camera/contrast?value=-1` |
| `/api/camera/flash` | `enabled` (true/false) | `POST /api/camera/flash?enabled=true` |

## Quality Levels

| Value | Resolution | Description |
|-------|-----------|-------------|
| 0 | 320x240 | Low - Fast, battery-efficient |
| 1 | 640x480 | Medium - Balanced (default) |
| 2 | 800x600 | High - Better quality |
| 3 | 1024x768 | Max - Best quality |

## BLE UUIDs

| Service/Characteristic | UUID |
|----------------------|------|
| **Provisioning Service** | `7b0d1001-5f6b-4c4f-9a7e-2f3b4d5e6f70` |
| **RX (Write)** | `7b0d1002-5f6b-4c4f-9a7e-2f3b4d5e6f70` |
| **TX (Notify)** | `7b0d1003-5f6b-4c4f-9a7e-2f3b4d5e6f70` |

## Connection Details

| Parameter | Value |
|-----------|-------|
| **WiFi SSID** | `DrainGuard-Robot` |
| **WiFi Password** | `DrainGuard123` |
| **Main Controller IP** | `192.168.4.1` |
| **Camera IP** | `192.168.4.50` |
| **Camera Stream** | `http://192.168.4.50/stream` |
| **BLE Device Name** | `DrainGuard-XXXX` |

## cURL Examples

```bash
# Get camera status
curl http://192.168.4.1/api/camera/status

# Capture photo
curl -X POST http://192.168.4.1/api/camera/capture

# Set quality to high
curl -X POST "http://192.168.4.1/api/camera/quality?value=2"

# Enable flash
curl -X POST "http://192.168.4.1/api/camera/flash?enabled=true"

# Start streaming
curl -X POST http://192.168.4.1/api/camera/start_stream

# Get complete system status
curl http://192.168.4.1/api/status
```

## Response Formats

### BLE Success Response
```json
{
  "status": "command_result",
  "id": 1,
  "ok": true,
  "message": "capture initiated"
}
```

### BLE Camera Status
```json
{
  "status": "camera_status",
  "id": 4,
  "available": true,
  "streaming": false,
  "quality": 1,
  "brightness": 0,
  "contrast": 0,
  "flash": false
}
```

### WiFi Camera Status
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

## Mobile App Snippets

### JavaScript/TypeScript (React Native)
```javascript
// BLE capture
const cmd = '{"command":"camera","action":"capture","id":1}\n';
await device.writeCharacteristic(SERVICE, RX_CHAR, base64(cmd));

// WiFi capture
await fetch('http://192.168.4.1/api/camera/capture', {method: 'POST'});
```

### Python
```python
# WiFi control
import requests
requests.post('http://192.168.4.1/api/camera/quality?value=2')

# BLE control
import asyncio
from bleak import BleakClient
await client.write_gatt_char(RX_UUID, b'{"command":"camera","action":"capture","id":1}\n')
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Camera not available | Check WiFi connection, verify IP 192.168.4.50 |
| BLE commands ignored | Add `\n` delimiter, check JSON format |
| Streaming laggy | Lower quality to 0 or 1 |
| Flash not working | Check LED_GPIO_NUM pin (GPIO 4) |
| Connection drops | Reduce distance, check power supply |

## Status LED Indicators

| Pattern | Meaning |
|---------|---------|
| Solid | BLE connected |
| Fast blink (100ms) | WiFi connecting |
| Slow blink (800ms) | Idle, waiting for connection |
| Double blink | WiFi connected to internet |

## Performance Tips

1. ✅ Use WiFi for streaming (better bandwidth)
2. ✅ Use BLE for quick commands (lower latency)
3. ✅ Lower quality for battery life
4. ✅ Monitor free heap via `/api/status`
5. ✅ Check RSSI signal strength regularly

## Support

Serial Monitor Baud Rate: **115200**

Debug output format:
```
[BLE-Camera] Capture photo command
[WiFi-Camera] Quality set to 2
[CAM] Probe OK (WiFi:active BLE:ready)
```
