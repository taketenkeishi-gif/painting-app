@echo off
setlocal

set "BUILD_DIR=%~dp0build"
set "EXE=%BUILD_DIR%\src\Release\LayeredPaintApp.exe"
set "PROC_NAME=LayeredPaintApp.exe"

:: Kill existing
tasklist /fi "imagename eq %PROC_NAME%" 2>nul | find /i "%PROC_NAME%" >nul
if not errorlevel 1 (
    echo [launch-dev] Stopping %PROC_NAME%...
    taskkill /f /im "%PROC_NAME%" >nul 2>&1
    timeout /t 1 /nobreak >nul
)

:: Build
echo [launch-dev] Building...
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [launch-dev] ERROR: Build failed.
    pause
    exit /b 1
)

:: Launch with debug server on port 9223
echo [launch-dev] Starting with --debug-server (port 9223)...
start "" "%EXE%" --debug-server
echo [launch-dev] Dev Bridge available at http://localhost:9223/debug/health
endlocal
