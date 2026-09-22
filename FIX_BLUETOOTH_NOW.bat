@echo off
REM Quick Fix Script for DrainGuard Bluetooth
REM This will rebuild the Android app with the fixed permissions

echo.
echo =====================================================
echo   DrainGuard Bluetooth Quick Fix
echo =====================================================
echo.
echo This will:
echo  1. Clean Android build
echo  2. Rebuild with new Bluetooth permissions
echo  3. Install to your connected phone
echo.
echo Make sure:
echo  [*] Android phone is connected via USB
echo  [*] USB Debugging is enabled
echo  [*] Computer is authorized on phone
echo.
pause
echo.

REM Check if mobile-app exists
if not exist "mobile-app" (
    echo [ERROR] mobile-app folder not found!
    echo         Run this from the project root directory.
    pause
    exit /b 1
)

REM Check if adb is available
where adb >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] ADB not found! Install Android SDK first.
    pause
    exit /b 1
)

REM Check if device is connected
echo [*] Checking for connected devices...
adb devices | findstr /C:"device" >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] No Android device connected!
    echo         Connect your phone via USB and enable USB Debugging.
    pause
    exit /b 1
)

echo [OK] Android device found
echo.

REM Navigate to mobile-app
cd mobile-app

REM Check if node_modules exists
if not exist "node_modules" (
    echo [*] Installing npm dependencies...
    call npm install
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] npm install failed!
        pause
        exit /b 1
    )
)

REM Clean Android build
echo.
echo [*] Cleaning Android build...
cd android
call gradlew.bat clean
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Gradle clean failed!
    cd ..\..
    pause
    exit /b 1
)
cd ..

REM Rebuild and install
echo.
echo [*] Rebuilding and installing app...
echo     This may take 3-5 minutes...
echo.
call npx react-native run-android

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    echo.
    echo Common issues:
    echo  - Check if Java JDK is installed
    echo  - Check if Android SDK is configured
    echo  - Check if phone is still connected
    echo.
    cd ..
    pause
    exit /b 1
)

echo.
echo =====================================================
echo   SUCCESS! App installed with Bluetooth fixes
echo =====================================================
echo.
echo NEXT STEPS:
echo.
echo 1. ON YOUR PHONE:
echo    - Go to Settings ^> Apps ^> DrainGuard
echo    - Tap Permissions
echo    - Allow: Nearby devices (or Bluetooth)
echo    - Allow: Location
echo    - Go back to Settings ^> Turn ON Bluetooth
echo    - Go back to Settings ^> Turn ON Location
echo.
echo 2. ON ESP32:
echo    - Upload firmware: cd firmware ^&^& pio run -t upload
echo    - Monitor: pio device monitor -b 115200
echo    - Check for: [BLE] Advertising as: DrainGuard-XXXX
echo.
echo 3. IN THE APP:
echo    - Open DrainGuard app
echo    - Go to Settings tab
echo    - Tap "Scan for DrainGuard"
echo    - You should see DrainGuard-XXXX appear!
echo.
echo 4. IF STILL NOT WORKING:
echo    - Read BLUETOOTH_FIX_SUMMARY.md
echo    - Run test-bluetooth.bat for diagnostics
echo.
echo =====================================================
echo.
cd ..
pause
