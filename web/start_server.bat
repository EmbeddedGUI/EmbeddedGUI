@echo off
chcp 65001 >nul 2>&1

set "WEB_DIR=%~dp0"
set "PORT=8080"

if not "%1"=="" set "PORT=%1"

if not exist "%WEB_DIR%index.html" (
    echo [!!] index.html 不存在
    echo     请先构建 WASM demos:
    echo       python scripts/wasm_build_demos.py
    pause
    exit /b 1
)

echo ========================================
echo   EmbeddedGUI Web Demos
echo ========================================
echo.
echo   http://localhost:%PORT%
echo   按 Ctrl+C 停止
echo.

start http://localhost:%PORT%
python -m http.server %PORT% --directory "%WEB_DIR%"
