# Drain Guard Web App

Web-based control panel for the Drain Guard IoT system. Access from any browser on your phone, tablet, or computer!

## Features

✅ **Real-time Monitoring**
- Water level display with color-coded status
- Live progress bar visualization
- Distance sensor readings
- GPS location tracking

✅ **Drain Control**
- Open/Close drain remotely
- Status indicators
- Motor and servo arm control

✅ **Servo Arm Control**
- Manual servo sliders (Base, Shoulder, Elbow, Gripper)
- Quick action buttons
- Pre-programmed sequences
- Demo mode

✅ **Live Camera Feed**
- ESP32-CAM video streaming
- Refresh on demand
- Full-screen view

✅ **Settings**
- Configure device IP
- Set alert thresholds
- Adjust refresh rate
- Save preferences locally

## Setup

### 1. Deploy Web App

**Option A: Run Locally**
```bash
cd web-app
# Open with any local server

# Using Python 3
python -m http.server 8080

# Using Python 2
python -m SimpleHTTPServer 8080

# Using Node.js
npx http-server -p 8080

# Using PHP
php -S localhost:8080
```

**Option B: Deploy to Web Server**
```bash
# Upload files to your web hosting
web-app/
├── index.html
├── styles.css
├── app.js
└── README.md
```

**Option C: Open Directly**
```bash
# Just open index.html in your browser
# Works for testing but no CORS for camera
```

### 2. Access Web App

Open your browser and navigate to:
```
http://localhost:8080
```

Or if deployed:
```
http://your-domain.com/drain-guard
```

### 3. Configure Device IP

1. Click **Settings** tab
2. Enter your ESP32 IP address (e.g., `192.168.1.100`)
3. Click **Save Settings**
4. Refresh the page

## Usage

### Dashboard Tab

**Water Level Monitoring:**
- View current water level in cm
- Color-coded status (Normal/Warning/Critical)
- Progress bar visualization
- Distance from sensor

**Drain Control:**
- **Open Drain** - Opens the drain using motors
- **Close Drain** - Closes the drain

**GPS Location:**
- View current GPS coordinates
- Satellite count
- **View on Map** - Opens Google Maps

### Servo Arm Tab

**Quick Actions:**
- **Open with Arm** - Use robotic arm to open drain
- **Close with Arm** - Use robotic arm to close drain
- **Home Position** - Return arm to home position
- **Demo Sequence** - Run demonstration

**Manual Control:**
- **Base Slider** - Rotate base (150-450)
- **Shoulder Slider** - Up/down movement (150-380)
- **Elbow Slider** - Extend/retract (300-380)
- **Gripper Slider** - Open/close gripper (410-510)

### Camera Tab

**Live Streaming:**
- View real-time video from ESP32-CAM
- **Refresh Stream** - Reload camera feed
- Live indicator shows streaming status

### Settings Tab

**Device Configuration:**
- **Device IP Address** - ESP32 IP on your network
- **Auto Refresh Rate** - Update interval (seconds)

**Alert Thresholds:**
- **Critical Level** - Trigger emergency alerts (cm)
- **Warning Level** - Pre-warning threshold (cm)

## Network Requirements

### Same WiFi Network
Both your device (phone/computer) and ESP32 must be on the **same WiFi network**.

```
Router
├─> ESP32 (192.168.1.100)
└─> Your Phone/Computer (192.168.1.x)
```

### Port Forwarding (Optional)
For remote access outside your network:

1. Forward port 80 on your router to ESP32 IP
2. Use your public IP or domain
3. **Not recommended without SSL/security**

## Browser Compatibility

### Recommended Browsers:
- ✅ Chrome/Edge (Best)
- ✅ Firefox
- ✅ Safari
- ✅ Opera

### Mobile Browsers:
- ✅ Chrome Mobile
- ✅ Safari iOS
- ✅ Firefox Mobile
- ✅ Samsung Internet

## Features by Tab

| Feature | Dashboard | Servo | Camera | Settings |
|---------|-----------|-------|--------|----------|
| Water Level | ✅ | | | |
| Drain Control | ✅ | | | |
| GPS Location | ✅ | | | |
| Servo Control | | ✅ | | |
| Arm Quick Actions | | ✅ | | |
| Live Video | | | ✅ | |
| Device Config | | | | ✅ |

## Responsive Design

The web app is fully responsive:

### Desktop (1200px+)
- Full layout with sidebar
- Large controls and displays
- Multi-column grid

### Tablet (768px - 1199px)
- Optimized layout
- Touch-friendly buttons
- Readable text sizes

### Mobile (< 768px)
- Single column layout
- Large touch targets
- Simplified navigation
- Icon-only tabs

## Local Storage

Settings are saved in browser's local storage:
- Device IP address
- Refresh rate
- Alert thresholds
- Persists across sessions

## Troubleshooting

### Can't Connect to Device

**Problem:** "Connection Error" message

**Solutions:**
1. Check ESP32 is powered on
2. Verify ESP32 IP address in Settings
3. Ensure on same WiFi network
4. Ping ESP32 from terminal:
   ```bash
   ping 192.168.1.100
   ```
5. Check ESP32 serial output for IP

### Camera Not Loading

**Problem:** Camera shows "not available"

**Solutions:**
1. Verify ESP32-CAM is running
2. Check camera stream URL
3. Test direct access:
   ```
   http://192.168.1.100/stream
   ```
4. Click **Refresh Stream** button
5. Check browser console for errors

### Servos Not Moving

**Problem:** Sliders move but servos don't respond

**Solutions:**
1. Check PCA9685 connections
2. Verify servo power (5V)
3. Test individual servo via API
4. Check ESP32 serial output

### Auto Refresh Not Working

**Problem:** Data doesn't update

**Solutions:**
1. Check refresh rate in Settings
2. Verify connection status (green dot)
3. Manually refresh browser
4. Check browser console for errors

## API Endpoints

The web app communicates with ESP32 via REST API:

```javascript
// Status
GET  /api/status

// Drain Control
POST /api/drain/open
POST /api/drain/close

// Arm Control
POST /api/arm/open
POST /api/arm/close
POST /api/arm/home
POST /api/arm/demo

// Servo Control
POST /api/servo/base?position=330
POST /api/servo/shoulder?position=200
POST /api/servo/elbow?position=350
POST /api/servo/gripper?position=450
GET  /api/servo/status

// Camera
GET  /api/camera/stream

// GPS
GET  /api/gps
```

## Security Considerations

⚠️ **Important Security Notes:**

### Current Setup (Development)
- No authentication required
- No encryption (HTTP only)
- Local network only

### For Production Use:
1. **Add Authentication**
   - Basic auth
   - Token-based auth
   - Password protection

2. **Use HTTPS**
   - SSL certificate
   - Encrypted traffic
   - Secure connections

3. **Network Security**
   - Firewall rules
   - VPN access only
   - MAC address filtering

4. **Rate Limiting**
   - Prevent abuse
   - Limit API calls
   - Throttle requests

## Customization

### Change Colors

Edit `styles.css`:
```css
:root {
    --primary-color: #2196F3;  /* Blue */
    --success-color: #4CAF50;  /* Green */
    --danger-color: #F44336;   /* Red */
    --warning-color: #FF9800;  /* Orange */
}
```

### Change Refresh Rate

Edit `app.js`:
```javascript
let config = {
    refreshRate: 5  // seconds (default)
};
```

### Modify Thresholds

In Settings tab or edit `app.js`:
```javascript
let config = {
    criticalLevel: 20,  // cm
    warningLevel: 50    // cm
};
```

## Development

### File Structure
```
web-app/
├── index.html     # Main HTML structure
├── styles.css     # All styling
├── app.js         # JavaScript logic
└── README.md      # This file
```

### Add New Features

1. **Add HTML** in `index.html`
2. **Style** in `styles.css`
3. **Logic** in `app.js`
4. Test on multiple devices

### Debug Mode

Open browser console (F12) to see:
- API requests/responses
- Error messages
- Network activity
- Console logs

## Progressive Web App (PWA)

Want to install as an app?

### Create manifest.json:
```json
{
  "name": "Drain Guard",
  "short_name": "DrainGuard",
  "start_url": "/",
  "display": "standalone",
  "background_color": "#2196F3",
  "theme_color": "#2196F3",
  "icons": [
    {
      "src": "icon.png",
      "sizes": "192x192",
      "type": "image/png"
    }
  ]
}
```

Add to `index.html`:
```html
<link rel="manifest" href="manifest.json">
```

## Performance Tips

1. **Optimize Refresh Rate**
   - Lower rate = less battery
   - Higher rate = more real-time

2. **Cache Settings**
   - Stored in localStorage
   - No server needed

3. **Lazy Load Camera**
   - Only loads when viewing
   - Saves bandwidth

## Future Enhancements

- [ ] Push notifications
- [ ] Historical data charts
- [ ] Event logs
- [ ] Multiple device support
- [ ] Dark mode
- [ ] Multi-language support
- [ ] Offline mode
- [ ] PWA installation

## Support

For issues or questions:
1. Check ESP32 serial output
2. Verify network connectivity
3. Test API endpoints directly
4. Check browser console

## License

MIT License
