@echo off
cd /d "C:\Users\Administrator\Desktop\xiaozhi-esp32-2.0.3"
call "C:\Espressif\frameworks\esp-idf-v5.4\export.bat"
idf.py build
