@echo off
setlocal

set "BUILD_DIR=%~dp0build"
set "EXE=%BUILD_DIR%\src\Release\LayeredPaintApp.exe"
set "WINDEPLOYQT=C:\Qt\6.7.2\msvc2019_64\bin\windeployqt.exe"
set "PROC_NAME=LayeredPaintApp.exe"

:: 1. Kill existing process
tasklist /fi "imagename eq %PROC_NAME%" 2>nul | find /i "%PROC_NAME%" >nul
if not errorlevel 1 (
    echo [launch] Stopping %PROC_NAME%...
    taskkill /f /im "%PROC_NAME%" >nul 2>&1
    timeout /t 1 /nobreak >nul
)

:: 2. Configure (ensure PAINT_DEBUG_SERVER=ON in cmake cache)
cmake -B "%BUILD_DIR%" -DPAINT_DEBUG_SERVER=ON >nul 2>&1

:: 3. Incremental build (cmake skips unchanged files automatically)
echo [launch] Building...
cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo [launch] ERROR: Build failed.
    pause
    exit /b 1
)

:: 4. Deploy Qt DLLs if missing
if not exist "%BUILD_DIR%\src\Release\Qt6Core.dll" (
    echo [launch] Deploying Qt DLLs...
    "%WINDEPLOYQT%" "%EXE%" >nul 2>&1
)

:: 5. Launch with Dev_Bridge debug server enabled (port 9223)
echo [launch] Starting with --debug-server...
start "" "%EXE%" --debug-server
endlocal
