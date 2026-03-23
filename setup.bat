@echo off
setlocal

set "PROJECT_DIR=%~dp0"
set "SETUP_SCRIPT=%PROJECT_DIR%scripts\setup_env.py"

echo.
echo ========================================
echo   EmbeddedGUI Environment Setup
echo ========================================
echo.

if not exist "%SETUP_SCRIPT%" (
    echo [!!] Setup script not found:
    echo      %SETUP_SCRIPT%
    exit /b 1
)

where py.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    py -3 "%SETUP_SCRIPT%" %*
    exit /b %ERRORLEVEL%
)

where python.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    python "%SETUP_SCRIPT%" %*
    exit /b %ERRORLEVEL%
)

echo [!!] Python 3.8+ was not found.
echo      Install Python from:
echo        https://www.python.org/downloads/
echo      Then run setup.bat again.
exit /b 1
