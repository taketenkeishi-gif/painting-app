@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "POWERSHELL=powershell"
set "RUN_PS1=%SCRIPT_DIR%scripts\run.ps1"

if not exist "%RUN_PS1%" (
  echo run script not found: %RUN_PS1%
  exit /b 1
)

%POWERSHELL% -NoProfile -ExecutionPolicy Bypass -File "%RUN_PS1%" -BuildDir "build_release_run" -Config "Release"
set "EXITCODE=%ERRORLEVEL%"

if not "%EXITCODE%"=="0" (
  echo launch failed with exit code %EXITCODE%
  exit /b %EXITCODE%
)

exit /b 0
