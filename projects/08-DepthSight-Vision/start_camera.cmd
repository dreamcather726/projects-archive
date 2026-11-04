@echo off
setlocal
cd /d "%~dp0"
if not exist ".venv\Scripts\python.exe" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_env.ps1"
  if errorlevel 1 goto failed
)
".venv\Scripts\python.exe" -c "import ultralytics, cv2" >nul 2>&1
if errorlevel 1 (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_env.ps1"
  if errorlevel 1 goto failed
)
".venv\Scripts\python.exe" camera_depth.py %*
if errorlevel 1 goto failed
exit /b 0
:failed
echo Startup failed. Review the error above or see README.md.
pause
exit /b 1
