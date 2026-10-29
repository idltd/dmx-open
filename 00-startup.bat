@echo off
echo === DMX-Open Build + Flash ===
echo.

set PIO=C:\Users\paul\.platformio\penv\Scripts\pio.exe
cd /d "%~dp0firmware"

echo [1/2] Uploading filesystem (PWA + shows)...
"%PIO%" run -t uploadfs
if errorlevel 1 (
    echo FAILED: filesystem upload
    pause
    exit /b 1
)

echo [2/2] Building and uploading firmware...
"%PIO%" run -t upload
if errorlevel 1 (
    echo FAILED: firmware upload
    pause
    exit /b 1
)

echo.
echo Done. Connect to WiFi "DMX-Open" (password: dmxopen1)
echo Then open http://192.168.4.1 in your browser.
echo.
pause
