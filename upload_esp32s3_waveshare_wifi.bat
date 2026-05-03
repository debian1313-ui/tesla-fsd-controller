@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

echo ====================================
echo ESP32-S3 Waveshare WiFi 固件编译并刷写工具
echo ====================================
echo.

REM 设置 Python UTF-8 模式避免编码问题
set PYTHONUTF8=1

REM 检查 PlatformIO 是否安装
pio --version >nul 2>&1
if errorlevel 1 (
    echo [错误] PlatformIO 未安装或不在 PATH 中
    echo 请先安装 PlatformIO: https://platformio.org/install
    pause
    exit /b 1
)

echo [1/2] 正在编译固件 (esp32s3-waveshare-wifi)...
echo.
pio run -e esp32s3-waveshare-wifi
if errorlevel 1 (
    echo.
    echo [失败] 编译出错，请检查错误信息
    pause
    exit /b 1
)

echo.
echo [2/2] 正在刷写固件到 ESP32-S3...
echo 提示: 如果刷写失败，请手动进入烧录模式:
echo   1. 按住 BOOT 键
echo   2. 按一下 RST 键
echo   3. 松开 RST 键
echo   4. 松开 BOOT 键
echo.
pio run -e esp32s3-waveshare-wifi -t upload
if errorlevel 1 (
    echo.
    echo [失败] 刷写失败，请检查设备连接和烧录模式
    pause
    exit /b 1
)

echo.
echo ====================================
echo 成功! 固件已刷写到设备
echo ====================================
echo.
echo 下一步:
echo 1. 设备会自动重启
echo 2. 用手机连接 WiFi 热点 "FSD-Controller"
echo 3. 密码: 12345678
echo 4. 浏览器访问: 9.9.9.9
echo.
pause
