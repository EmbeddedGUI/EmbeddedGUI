@echo off
setlocal

cd /d "%~dp0"

set "PYTHON_CMD="

py -3 --version >nul 2>nul
if not errorlevel 1 (
    set "PYTHON_CMD=py -3"
)

if not defined PYTHON_CMD (
    python --version >nul 2>nul
    if not errorlevel 1 (
        set "PYTHON_CMD=python"
    )
)

if not defined PYTHON_CMD (
    echo Python was not found. Please install Python 3 or run scripts\setup_env.py first.
    set EXIT_CODE=1
    goto done
)

%PYTHON_CMD% scripts\start_app.py %*
set EXIT_CODE=%ERRORLEVEL%

:done
echo.
pause
exit /b %EXIT_CODE%
