@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ========================================
echo 编译定时提醒功能固件
echo ========================================
echo.

REM 设置 ESP-IDF 环境
call export.bat

echo.
echo 开始编译...
idf.py build

if %ERRORLEVEL% == 0 (
    echo.
    echo ========================================
    echo 编译成功！
    echo ========================================
    echo.
    echo 固件位置: build\xiaozhi.bin
    echo.
    echo 烧录命令:
    echo idf.py -p COM3 flash monitor
    echo.
) else (
    echo.
    echo ========================================
    echo 编译失败！
    echo ========================================
    echo.
)

pause
