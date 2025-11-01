@echo off
chcp 65001 >nul
cd /d "%~dp0"
set MCP_ENDPOINT=wss://api.xiaozhi.me/mcp/?token=eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjM2MTEyNywiYWdlbnRJZCI6Mzk5NDc2LCJlbmRwb2ludElkIjoiYWdlbnRfMzk5NDc2IiwicHVycG9zZSI6Im1jcC1lbmRwb2ludCIsImlhdCI6MTc2MTg4MjQzMywiZXhwIjoxNzkzNDQwMDMzfQ.vUvf0BrUck__K5pzaeT1CSM4_KBVnornPTISVXhMebQFtP3o2Z3pxeTHmOZHkGFCEELA25q2XKEigTVe0YJiXA

echo ========================================
echo 启动小智 MCP 服务
echo ========================================
echo.
echo 已配置服务:
echo - Calculator (计算器)
echo - Reminder (定时提醒)
echo.
echo 正在连接到: wss://api.xiaozhi.me/mcp/
echo.

python mcp_pipe.py
