@echo off
echo ========================================
echo Building DrainGuard APK (Debug)
echo ========================================
echo.

cd /d "%~dp0android"

echo Cleaning previous builds...
call gradlew clean

echo Building debug APK...
call gradlew assembleDebug

echo.
echo ========================================
echo Build complete!
echo APK location: android\app\build\outputs\apk\debug\app-debug.apk
echo ========================================
pause
