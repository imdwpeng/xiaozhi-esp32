@echo off
echo Building xiaozhi-esp32 for ESP32-S3...

REM 设置 ESP-IDF 环境 (请根据你的安装路径修改)
REM 如果 ESP-IDF 环境变量已设置，可以注释掉下面这行
REM call %IDF_PATH%\export.bat

REM 设置目标芯片
idf.py set-target esp32s3

REM 清理并构建项目
echo Cleaning project...
idf.py clean

echo Building project...
idf.py build

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo.
    echo Flash command: idf.py -p COM3 flash monitor
    echo Replace COM3 with your actual serial port
) else (
    echo Build failed!
)

pause