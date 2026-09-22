#!/bin/bash

# DrainGuard Bluetooth Testing Script
# Run this to verify your Bluetooth setup

echo "🔍 DrainGuard Bluetooth Diagnostic Tool"
echo "========================================"
echo ""

# Check if we're in the right directory
if [ ! -d "mobile-app" ]; then
    echo "❌ Error: Run this script from the project root directory"
    exit 1
fi

# Check Android SDK
echo "📱 Checking Android environment..."
if ! command -v adb &> /dev/null; then
    echo "⚠️  ADB not found - install Android SDK"
else
    echo "✅ ADB found"
    
    # Check connected devices
    DEVICES=$(adb devices | grep -v "List" | grep "device$" | wc -l)
    if [ "$DEVICES" -eq 0 ]; then
        echo "⚠️  No Android devices connected"
    else
        echo "✅ $DEVICES Android device(s) connected"
    fi
fi

# Check Node modules
echo ""
echo "📦 Checking Node.js dependencies..."
if [ ! -d "mobile-app/node_modules" ]; then
    echo "⚠️  node_modules not found - run: cd mobile-app && npm install"
else
    echo "✅ node_modules installed"
fi

# Check if BLE module exists
echo ""
echo "🔌 Checking BLE Module files..."
BLE_MODULE="mobile-app/android/app/src/main/java/com/drainguardapp/DrainGuardBleModule.java"
BLE_PACKAGE="mobile-app/android/app/src/main/java/com/drainguardapp/DrainGuardBlePackage.java"

if [ -f "$BLE_MODULE" ]; then
    echo "✅ DrainGuardBleModule.java exists"
else
    echo "❌ DrainGuardBleModule.java NOT FOUND"
fi

if [ -f "$BLE_PACKAGE" ]; then
    echo "✅ DrainGuardBlePackage.java exists"
else
    echo "❌ DrainGuardBlePackage.java NOT FOUND"
fi

# Check AndroidManifest permissions
echo ""
echo "📄 Checking AndroidManifest.xml permissions..."
MANIFEST="mobile-app/android/app/src/main/AndroidManifest.xml"

if grep -q "BLUETOOTH_SCAN" "$MANIFEST"; then
    echo "✅ BLUETOOTH_SCAN permission found"
else
    echo "❌ BLUETOOTH_SCAN permission MISSING"
fi

if grep -q "BLUETOOTH_CONNECT" "$MANIFEST"; then
    echo "✅ BLUETOOTH_CONNECT permission found"
else
    echo "❌ BLUETOOTH_CONNECT permission MISSING"
fi

if grep -q "ACCESS_FINE_LOCATION" "$MANIFEST"; then
    echo "✅ ACCESS_FINE_LOCATION permission found"
else
    echo "❌ ACCESS_FINE_LOCATION permission MISSING"
fi

# Check PlatformIO
echo ""
echo "⚙️  Checking PlatformIO..."
if ! command -v pio &> /dev/null; then
    echo "⚠️  PlatformIO CLI not found"
else
    echo "✅ PlatformIO CLI found"
fi

# Instructions
echo ""
echo "📋 NEXT STEPS:"
echo "=============="
echo ""
echo "1. REBUILD ANDROID APP (IMPORTANT!):"
echo "   cd mobile-app/android"
echo "   ./gradlew clean"
echo "   cd ../.."
echo "   cd mobile-app && npx react-native run-android"
echo ""
echo "2. UPLOAD ESP32 FIRMWARE:"
echo "   cd firmware"
echo "   pio run -t upload"
echo "   pio device monitor -b 115200"
echo ""
echo "3. CHECK ESP32 SERIAL OUTPUT:"
echo "   Should show: [BLE] Advertising as: DrainGuard-XXXX"
echo ""
echo "4. TEST IN MOBILE APP:"
echo "   - Enable Bluetooth on phone"
echo "   - Enable Location on phone"
echo "   - Grant all permissions to app"
echo "   - Tap 'Scan for DrainGuard'"
echo ""
echo "5. IF STILL NOT WORKING:"
echo "   - Use nRF Connect app to verify ESP32 is advertising"
echo "   - Check Android logs: npx react-native log-android"
echo "   - Check ESP32 logs in serial monitor"
echo ""
echo "✨ Good luck!"
