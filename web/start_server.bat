@echo off
setlocal
chcp 65001 >nul 2>&1

for %%i in ("%~dp0.") do set "WEB_DIR=%%~fi"
set "PYTHON_EXE="
set "PYTHON_ARGS="

if defined VIRTUAL_ENV (
    if exist "%VIRTUAL_ENV%\Scripts\python.exe" set "PYTHON_EXE=%VIRTUAL_ENV%\Scripts\python.exe"
)

if not defined PYTHON_EXE (
    if exist "%WEB_DIR%\..\.venv\Scripts\python.exe" set "PYTHON_EXE=%WEB_DIR%\..\.venv\Scripts\python.exe"
)

if not defined PYTHON_EXE (
    where py.exe >nul 2>&1
    if not errorlevel 1 (
        set "PYTHON_EXE=py"
        set "PYTHON_ARGS=-3"
    )
)

if not defined PYTHON_EXE (
    where python.exe >nul 2>&1
    if not errorlevel 1 set "PYTHON_EXE=python"
)

if not defined PYTHON_EXE (
    echo([!!] Python 未找到，无法启动 HTTP 服务器
    echo(    请先运行 setup.bat 或激活 .venv:
    echo(      .venv\Scripts\activate.bat
    pause
    exit /b 1
)

"%PYTHON_EXE%" %PYTHON_ARGS% "%WEB_DIR%\start_server.py" %*
exit /b %errorlevel%
