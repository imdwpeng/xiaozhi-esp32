@echo off
echo Building and flashing...
call C:/Users/Administrator/esp/v5.5.1/esp-idf/export.bat
cd /d c:/esp/xiaozhi-esp32-2.0.5
idf.py build
if %ERRORLEVEL% EQU 0 (
    echo Build successful, flashing...
    idf.py flash
) else (
    echo Build failed!
)
pause