@echo off
echo ========================================
echo Building Xiaozhi ESP32 Firmware
echo ========================================
echo.

cd /d "%~dp0"

REM Check if ESP-IDF is already set up
where idf.py >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Setting up ESP-IDF environment...

    REM Try common ESP-IDF installation paths
    if exist "C:\Espressif\frameworks\esp-idf-v5.4\export.bat" (
        echo Found ESP-IDF at C:\Espressif\frameworks\esp-idf-v5.4\
        call "C:\Espressif\frameworks\esp-idf-v5.4\export.bat"
    ) else if exist "C:\Espressif\frameworks\esp-idf-v5.3\export.bat" (
        echo Found ESP-IDF at C:\Espressif\frameworks\esp-idf-v5.3\
        call "C:\Espressif\frameworks\esp-idf-v5.3\export.bat"
    ) else if exist "C:\esp-idf\export.bat" (
        echo Found ESP-IDF at C:\esp-idf\
        call "C:\esp-idf\export.bat"
    ) else (
        echo ERROR: ESP-IDF not found!
        echo Please install ESP-IDF or run this from ESP-IDF command prompt
        pause
        exit /b 1
    )
) else (
    echo ESP-IDF environment is already configured
)

echo.
echo Starting build process...
echo.

idf.py build

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo Build completed successfully!
    echo ========================================
    echo.
    echo To flash: idf.py -p COM3 flash monitor
    echo.
) else (
    echo.
    echo ========================================
    echo Build failed with error code %ERRORLEVEL%
    echo ========================================
    echo.
)

pause
