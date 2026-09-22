@echo off
REM DrainGuard Bluetooth Testing Script for Windows
REM Run this to verify your Bluetooth setup

echo.
echo ==================================================
echo   DrainGuard Bluetooth Diagnostic Tool (Windows)
echo ==================================================
echo.

REM Check if we're in the right directory
if not exist "mobile-app" (
    echo [ERROR] Run this script from the project root directory
    exit /b 1
)

REM Check Node.js
echo [*] Checking Node.js...
where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] Node.js not found
) else (
    echo [OK] Node.js found
)

REM Check npm
where npm >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] npm not found
) else (
    echo [OK] npm found
)

REM Check Android SDK / adb
echo.
echo [*] Checking Android environment...
where adb >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] ADB not found - install Android SDK
) else (
    echo [OK] ADB found
    
    REM Check connected devices
    adb devices | findstr /C:"device" >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Android device(s) connected
    ) else (
        echo [WARNING] No Android devices connected
    )
)

REM Check Node modules
echo.
echo [*] Checking Node.js dependencies...
if not exist "mobile-app\node_modules" (
    echo [WARNING] node_modules not found
    echo           Run: cd mobile-app ^&^& npm install
) else (
    echo [OK] node_modules installed
)

REM Check BLE Module files
echo.
echo [*] Checking BLE Module files...
if exist "mobile-app\android\app\src\main\java\com\drainguardapp\DrainGuardBleModule.java" (
    echo [OK] DrainGuardBleModule.java exists
) else (
    echo [ERROR] DrainGuardBleModule.java NOT FOUND
)

if exist "mobile-app\android\app\src\main\java\com\drainguardapp\DrainGuardBlePackage.java" (
    echo [OK] DrainGuardBlePackage.java exists
) else (
    echo [ERROR] DrainGuardBlePackage.java NOT FOUND
)

REM Check AndroidManifest permissions
echo.
echo [*] Checking AndroidManifest.xml permissions...
findstr /C:"BLUETOOTH_SCAN" "mobile-app\android\app\src\main\AndroidManifest.xml" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] BLUETOOTH_SCAN permission found
) else (
    echo [ERROR] BLUETOOTH_SCAN permission MISSING
)

findstr /C:"BLUETOOTH_CONNECT" "mobile-app\android\app\src\main\AndroidManifest.xml" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] BLUETOOTH_CONNECT permission found
) else (
    echo [ERROR] BLUETOOTH_CONNECT permission MISSING
)

findstr /C:"ACCESS_FINE_LOCATION" "mobile-app\android\app\src\main\AndroidManifest.xml" >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] ACCESS_FINE_LOCATION permission found
) else (
    echo [ERROR] ACCESS_FINE_LOCATION permission MISSING
)

REM Check PlatformIO
echo.
echo [*] Checking PlatformIO...
where pio >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] PlatformIO CLI not found
) else (
    echo [OK] PlatformIO CLI found
)

REM Instructions
echo.
echo ==================================================
echo   NEXT STEPS TO FIX BLUETOOTH
echo ==================================================
echo.
echo 1. REBUILD ANDROID APP (MOST IMPORTANT!):
echo    cd mobile-app\android
echo    gradlew.bat clean
echo    cd ..\..
echo    cd mobile-app
echo    npx react-native run-android
echo.
echo 2. GRANT PERMISSIONS ON PHONE:
echo    - Settings ^> Apps ^> DrainGuard ^> Permissions
echo    - Enable: Nearby devices, Location
echo    - Enable Bluetooth and Location on phone
echo.
echo 3. UPLOAD ESP32 FIRMWARE:
echo    cd firmware
echo    pio run -t upload
echo    pio device monitor -b 115200
echo.
echo 4. CHECK ESP32 SERIAL OUTPUT:
echo    Should show: [BLE] Advertising as: DrainGuard-XXXX
echo.
echo 5. TEST IN MOBILE APP:
echo    - Open app ^> Settings tab
echo    - Tap "Scan for DrainGuard"
echo    - Should see DrainGuard-XXXX in the list
echo.
echo 6. IF STILL NOT WORKING:
echo    - Use nRF Connect app to verify ESP32
echo    - Check logs: npx react-native log-android
echo    - Read BLUETOOTH_FIX_SUMMARY.md
echo.
echo ==================================================
echo   Good luck!
echo ==================================================
echo.
pause
