# Drain Guard Mobile App

React Native mobile application for monitoring and controlling the Drain Guard IoT system.

## Features

- Real-time water level monitoring
- Remote drain control (open/close)
- GPS location tracking with map view
- Live camera streaming
- Alert history
- Configurable settings
- Push notifications

## Prerequisites

- Node.js (v16 or higher)
- npm or yarn
- React Native CLI
- For iOS: Xcode and CocoaPods
- For Android: Android Studio and SDK

## Installation

1. **Clone and navigate to mobile-app directory**:
```bash
cd mobile-app
```

2. **Install dependencies**:
```bash
npm install
# or
yarn install
```

3. **iOS specific setup**:
```bash
cd ios
pod install
cd ..
```

4. **Android specific setup**:
- Open Android Studio
- Open `android` folder
- Let Gradle sync complete

## Configuration

1. **Update device IP address** in `src/services/api.js`:
```javascript
let DEVICE_IP = '192.168.1.100'; // Your ESP32 IP
```

Or configure via Settings screen in the app.

2. **Google Maps API Key** (for map functionality):

For iOS, edit `ios/DrainGuardApp/AppDelegate.m`:
```objective-c
#import <GoogleMaps/GoogleMaps.h>
[GMSServices provideAPIKey:@"YOUR_GOOGLE_MAPS_API_KEY"];
```

For Android, edit `android/app/src/main/AndroidManifest.xml`:
```xml
<meta-data
  android:name="com.google.android.geo.API_KEY"
  android:value="YOUR_GOOGLE_MAPS_API_KEY"/>
```

## Running the App

### iOS
```bash
npm run ios
# or
react-native run-ios
```

### Android
```bash
npm run android
# or
react-native run-android
```

### Development Mode
```bash
npm start
# This starts the Metro bundler
```

## Project Structure

```
mobile-app/
├── src/
│   ├── screens/
│   │   ├── HomeScreen.js         # Main dashboard
│   │   ├── LiveStreamScreen.js   # Camera stream viewer
│   │   ├── MapScreen.js          # GPS location map
│   │   ├── SettingsScreen.js     # App settings
│   │   └── HistoryScreen.js      # Alert history
│   └── services/
│       └── api.js                # API communication
├── App.js                        # Main app component
├── package.json
└── README.md
```

## App Screens

### 1. Home Screen
- Current water level display with color-coded status
- Progress bar visualization
- Drain status indicator
- Quick action buttons (Open/Close drain)
- GPS coordinates summary
- Navigation to other screens

### 2. Live Stream Screen
- Real-time video feed from ESP32-CAM
- Live indicator
- Refresh button
- Error handling with retry

### 3. Map Screen
- Google Maps integration
- Device location marker
- User location
- Satellite count display

### 4. Settings Screen
- Device IP configuration
- Alert phone number
- Water level thresholds
- Auto-open drain toggle
- Push notification settings

### 5. History Screen
- Event timeline
- Alert notifications
- User actions log
- Water level at each event

## API Integration

The app communicates with ESP32 via REST API:

### Endpoints Used
```javascript
GET  /api/status          # Get current status
POST /api/drain/open      # Open drain
POST /api/drain/close     # Close drain
GET  /api/gps             # Get GPS data
GET  /api/camera/stream   # Get stream URL
```

### Example Status Response
```json
{
  "water_level": 45.5,
  "distance": 154.5,
  "drain_open": false,
  "latitude": 37.7749,
  "longitude": -122.4194,
  "satellites": 8
}
```

## Customization

### Colors
Edit screen stylesheets to change colors:
```javascript
const styles = StyleSheet.create({
  primaryColor: '#2196F3',  // Blue
  dangerColor: '#f44336',    // Red
  warningColor: '#ff9800',   // Orange
  successColor: '#4caf50',   // Green
});
```

### Thresholds
Update in `HomeScreen.js`:
```javascript
const getWaterLevelColor = (level) => {
  if (level > 150) return '#f44336'; // Critical
  if (level > 100) return '#ff9800'; // Warning
  return '#4caf50'; // Normal
};
```

## Troubleshooting

### Cannot connect to device
- Ensure phone and ESP32 are on same WiFi network
- Check ESP32 IP address in Settings
- Verify ESP32 is powered on and running
- Check firewall settings

### Maps not showing
- Verify Google Maps API key is configured
- Enable Maps SDK for iOS/Android in Google Cloud Console
- Check API key restrictions

### Camera stream not loading
- Verify ESP32-CAM is running and accessible
- Check stream URL in ESP32-CAM firmware
- Ensure stable network connection
- Try accessing stream URL in browser first

### Build errors
```bash
# Clear cache and reinstall
npm start -- --reset-cache
rm -rf node_modules
npm install

# iOS specific
cd ios
pod deintegrate
pod install
cd ..

# Android specific
cd android
./gradlew clean
cd ..
```

### App crashes on startup
- Check React Native version compatibility
- Verify all native dependencies are linked
- Check Android/iOS build logs for errors
- Ensure minimum OS version requirements met

## Testing

### Manual Testing
1. Test all navigation flows
2. Verify data updates in real-time
3. Test drain control actions
4. Check map functionality
5. Test camera stream
6. Verify settings persistence

### Network Testing
- Test with poor WiFi signal
- Test with device offline
- Verify error handling
- Check timeout behavior

## Building for Production

### Android APK
```bash
cd android
./gradlew assembleRelease
# APK location: android/app/build/outputs/apk/release/app-release.apk
```

### iOS Archive
1. Open Xcode
2. Select "Generic iOS Device"
3. Product → Archive
4. Distribute to App Store or Ad Hoc

## Dependencies

Key libraries used:
- **react-navigation**: Navigation framework
- **react-native-maps**: Map integration
- **react-native-webview**: Camera stream display
- **axios**: HTTP requests
- **react-native-vector-icons**: Icon library
- **async-storage**: Local data persistence

## Future Enhancements

- [ ] Push notification integration (Firebase)
- [ ] Historical data charts
- [ ] Multiple device support
- [ ] User authentication
- [ ] Cloud data sync
- [ ] Offline mode with local caching
- [ ] Dark mode support
- [ ] Multi-language support

## Support

For issues and questions:
1. Check ESP32 serial output for errors
2. Verify API endpoints are responding
3. Check app logs for errors
4. Ensure all dependencies are installed

## License

MIT License
