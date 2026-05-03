@echo off
chcp 65001 >nul
setlocal

echo 正在编译固件 (esp32s3-waveshare-wifi)...
set PYTHONUTF8=1
pio run -e esp32s3-waveshare-wifi
if errorlevel 1 (
    echo 编译失败!
    pause
    exit /b 1
)
echo.
echo 编译成功! 固件文件:
echo   - 合并固件: esp32s3_waveshare_wifi_v1.4.35.bin
echo   - OTA 固件: esp32s3_waveshare_wifi_v1.4.35_ota.bin
pause
