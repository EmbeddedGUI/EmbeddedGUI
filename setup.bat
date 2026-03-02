@echo off
setlocal enabledelayedexpansion

echo.
echo ========================================
echo   EmbeddedGUI Development Environment Setup
echo ========================================
echo.

set "PROJECT_DIR=%~dp0"
set "TOOLS_DIR=%PROJECT_DIR%tools\win32"
set "DEVKIT_DIR=%PROJECT_DIR%tools\w64devkit"
set "HAS_MAKE=0"
set "HAS_GCC=0"
set "HAS_PYTHON=0"
set "HAS_ARM_GCC=0"
set "HAS_QEMU=0"
set "HAS_EMSDK=0"
set "HAS_PLAYWRIGHT=0"


set "ARM_GCC_PATH=D:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1"
set "QEMU_PATH=C:\Program Files\qemu"

:: ========================================
:: Step 1: Check make.exe
:: ========================================
echo [1/8] Checking make.exe ...

where make.exe >nul 2>&1
if !ERRORLEVEL! == 0 (
    set "HAS_MAKE=1"
    echo       [OK] make.exe found in PATH
    for /f "tokens=*" %%i in ('make --version 2^>^&1 ^| findstr "GNU Make"') do echo            %%i
) else if exist "%DEVKIT_DIR%\bin\make.exe" (
    set "HAS_MAKE=1"
    set "PATH=%DEVKIT_DIR%\bin;%PATH%"
    echo       [OK] Using w64devkit built-in make.exe
) else if exist "%TOOLS_DIR%\make.exe" (
    set "HAS_MAKE=1"
    set "PATH=%TOOLS_DIR%;%PATH%"
    echo       [OK] Using project built-in make.exe
    for /f "tokens=*" %%i in ('"%TOOLS_DIR%\make.exe" --version 2^>^&1 ^| findstr "GNU Make"') do echo            %%i
) else (
    echo       [!!] make.exe not found
)
echo.

:: ========================================
:: Step 2: Check gcc.exe
:: ========================================
echo [2/8] Checking gcc.exe ...

where gcc.exe >nul 2>&1
if !ERRORLEVEL! == 0 (
    set "HAS_GCC=1"
    echo       [OK] gcc.exe found in PATH
    for /f "tokens=*" %%i in ('gcc --version 2^>^&1 ^| findstr "gcc"') do echo            %%i
) else if exist "%DEVKIT_DIR%\bin\gcc.exe" (
    set "HAS_GCC=1"
    set "PATH=%DEVKIT_DIR%\bin;%PATH%"
    echo       [OK] Using w64devkit built-in gcc.exe
    for /f "tokens=*" %%i in ('"%DEVKIT_DIR%\bin\gcc.exe" --version 2^>^&1 ^| findstr "gcc"') do echo            %%i
) else (
    echo       [!!] gcc.exe not found
)

:: If make or gcc missing, offer to install w64devkit
if "!HAS_MAKE!"=="0" goto :offer_devkit
if "!HAS_GCC!"=="0" goto :offer_devkit
goto :skip_devkit

:offer_devkit
echo.
echo ----------------------------------------
echo   Missing build toolchain (make/gcc)
echo ----------------------------------------
echo.
echo   Auto-download w64devkit (~37 MB) for a complete C toolchain:
echo     Includes: GCC 15.2 + GNU Make 4.4 + GDB + Binutils
echo     Source:   https://github.com/skeeto/w64devkit
echo     No install needed, just extract and use
echo.

where python.exe >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo   [!!] Python is required for auto-download, but Python not found
    echo       Please download w64devkit manually:
    echo         https://github.com/skeeto/w64devkit/releases
    echo       Extract to: %DEVKIT_DIR%
    echo       Then re-run this script
    echo.
    goto :skip_devkit
)

echo   1. Auto-download and install w64devkit (recommended)
echo   2. Skip, install manually later
echo.
set /p "DEVKIT_CHOICE=      Choose [1/2]: "

if "%DEVKIT_CHOICE%"=="1" (
    python "%PROJECT_DIR%scripts\setup_env.py" --install-toolchain --mode 0
    if !ERRORLEVEL! == 0 (
        :: Re-check tools after install
        if exist "%DEVKIT_DIR%\bin\gcc.exe" (
            set "HAS_GCC=1"
            set "HAS_MAKE=1"
            set "PATH=%DEVKIT_DIR%\bin;%PATH%"
            echo.
            echo       [OK] w64devkit installed successfully
        )
    ) else (
        echo       [!!] w64devkit installation failed
    )
) else (
    echo.
    echo   To install manually:
    echo     Option 1: Download w64devkit - https://github.com/skeeto/w64devkit/releases
    echo               Extract to: %DEVKIT_DIR%
    echo     Option 2: Install MSYS2 - https://www.msys2.org/
    echo               pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    echo               Add C:\msys64\mingw64\bin to PATH
)

:skip_devkit

:: Show PATH hint if using tools dir
if not exist "%DEVKIT_DIR%\bin\gcc.exe" goto :skip_devkit_hint
echo.
echo       w64devkit added to PATH for this session only.
echo       To make permanent, add to system PATH:
echo         %DEVKIT_DIR%\bin
:skip_devkit_hint
echo.

:: ========================================
:: Step 3: Check python.exe
:: ========================================
echo [3/8] Checking python.exe ...

where python.exe >nul 2>&1
if !ERRORLEVEL! == 0 (
    set "HAS_PYTHON=1"
    echo       [OK] python.exe found in PATH
    for /f "tokens=*" %%i in ('python --version 2^>^&1') do echo            %%i
) else (
    echo       [!!] python.exe not found
    echo       Please install Python 3.8+
    echo       Download: https://www.python.org/downloads/
    echo       Check "Add Python to PATH" during installation
    echo.
    echo       Python is used for resource generation ^(make resource^) and scripts.
    echo       If you only need to compile existing examples, you can skip this.
    goto :skip_python
)

:: ========================================
:: Step 4: Python dependencies
:: ========================================
echo.
echo [4/8] Python dependencies ...
echo.
echo       Select install mode:
echo         1. Basic  - build dependencies only
echo            (freetype_py, json5, numpy, Pillow, pyelftools)
echo         2. Full   - build + UI Designer dependencies
echo            (includes PyQt5, PyQt-Fluent-Widgets, etc.)
echo         3. Skip Python dependency installation
echo.
set /p "SETUP_MODE=      Choose [1/2/3]: "

if "%SETUP_MODE%"=="3" goto :skip_python
if "%SETUP_MODE%"=="" set "SETUP_MODE=1"

python "%PROJECT_DIR%scripts\setup_env.py" --mode %SETUP_MODE%
if !ERRORLEVEL! neq 0 (
    echo.
    echo       [!!] Python dependency installation failed, check messages above
)

:skip_python

:: ========================================
:: Step 5: Check ARM GCC toolchain
:: ========================================
echo [5/8] Checking ARM GCC cross-compiler ...

if defined ARM_GCC_PATH (
    if exist "!ARM_GCC_PATH!\bin\arm-none-eabi-gcc.exe" (
        echo       [OK] ARM GCC configured: !ARM_GCC_PATH!
        set "HAS_ARM_GCC=1"
    ) else (
        echo       [!!] ARM_GCC_PATH set but invalid: !ARM_GCC_PATH!
    )
) else (
    where arm-none-eabi-gcc.exe >nul 2>&1
    if !ERRORLEVEL! == 0 (
        echo       [OK] arm-none-eabi-gcc found in PATH
        set "HAS_ARM_GCC=1"
    ) else (
        echo       [  ] ARM GCC not found ^(needed for embedded cross-compilation^)
        echo            Download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
        echo            Then set: ARM_GCC_PATH=install_path
    )
)
echo.

:: ========================================
:: Step 6: Check QEMU
:: ========================================
echo [6/8] Checking QEMU ARM emulator ...

if defined QEMU_PATH (
    if exist "!QEMU_PATH!\qemu-system-arm.exe" (
        echo       [OK] QEMU configured: !QEMU_PATH!
        set "HAS_QEMU=1"
    ) else (
        echo       [!!] QEMU_PATH set but invalid: !QEMU_PATH!
    )
) else (
    where qemu-system-arm.exe >nul 2>&1
    if !ERRORLEVEL! == 0 (
        echo       [OK] qemu-system-arm found in PATH
        set "HAS_QEMU=1"
    ) else (
        echo       [  ] QEMU not found ^(needed for performance testing^)
        echo            Download: https://www.qemu.org/download/#windows
        echo            Then set: QEMU_PATH=install_path
    )
)
echo.

:: ========================================
:: Step 7: Check Emscripten (WASM)
:: ========================================
echo [7/8] Checking Emscripten ^(WASM^) ...

where emcc.bat >nul 2>&1
if !ERRORLEVEL! == 0 (
    echo       [OK] emcc found in PATH
    set "HAS_EMSDK=1"
) else if defined EMSDK (
    if exist "!EMSDK!\emsdk_env.bat" (
        echo       [OK] EMSDK configured: !EMSDK!
        echo            Run first: !EMSDK!\emsdk_env.bat
        set "HAS_EMSDK=1"
    ) else (
        echo       [!!] EMSDK set but invalid: !EMSDK!
    )
) else (
    echo       [  ] Emscripten not found ^(needed for WASM builds^)
    echo            Install: https://emscripten.org/docs/getting_started/downloads.html
)
echo.

:: ========================================
:: Step 8: Check Playwright
:: ========================================
echo [8/8] Checking Playwright ...

if "!HAS_PYTHON!"=="1" (
    python -c "from playwright.sync_api import sync_playwright; print('ok')" >nul 2>&1
    if !ERRORLEVEL! == 0 (
        echo       [OK] Playwright installed
        set "HAS_PLAYWRIGHT=1"
    ) else (
        echo       [  ] Playwright not found ^(needed for Figma design screenshots^)
        echo            Install: pip install playwright ^&^& playwright install chromium
    )
) else (
    echo       [  ] Skipped ^(requires Python^)
)
echo.

:: ========================================
:: Verification build
:: ========================================
echo.
echo ========================================
echo   Build Verification
echo ========================================
echo.

if "!HAS_MAKE!"=="0" (
    echo [!!] make.exe missing, skipping build verification
    goto :summary
)
if "!HAS_GCC!"=="0" (
    echo [!!] gcc.exe missing, skipping build verification
    goto :summary
)

echo Compiling HelloSimple ...
make -C "%PROJECT_DIR%." -j APP=HelloSimple PORT=pc COMPILE_DEBUG= COMPILE_OPT_LEVEL=-O0 2>&1
if !ERRORLEVEL! == 0 (
    echo.
    echo [OK] HelloSimple compiled successfully!
) else (
    echo.
    echo [!!] Build failed, check error messages above
)

:: ========================================
:: Summary
:: ========================================
:summary
echo.
echo ========================================
echo   Environment Summary
echo ========================================
echo.

where make.exe >nul 2>&1 && (echo   [OK] make    ) || (echo   [!!] make     - missing)
where gcc.exe >nul 2>&1 && (echo   [OK] gcc     ) || (echo   [!!] gcc      - missing)
where python.exe >nul 2>&1 && (echo   [OK] python  ) || (echo   [!!] python   - missing)
if exist "%PROJECT_DIR%.venv\Scripts\python.exe" (echo   [OK] .venv   ) else (echo   [  ] .venv    - not created)
if "!HAS_ARM_GCC!"=="1" (echo   [OK] arm-gcc ) else (echo   [  ] arm-gcc  - not configured ^(optional: embedded builds^))
if "!HAS_QEMU!"=="1" (echo   [OK] qemu    ) else (echo   [  ] qemu     - not configured ^(optional: perf testing^))
if "!HAS_EMSDK!"=="1" (echo   [OK] emsdk   ) else (echo   [  ] emsdk    - not configured ^(optional: WASM builds^))
if "!HAS_PLAYWRIGHT!"=="1" (echo   [OK] playwright) else (echo   [  ] playwright - not configured ^(optional: Figma screenshots^))

echo.
echo ----------------------------------------
echo   Usage:
echo     make all APP=HelloSimple          Build example
echo     make run                          Run
echo     make all APP=HelloBasic APP_SUB=button
echo.
echo   Activate venv before using Python scripts:
echo     .venv\Scripts\activate.bat
echo ----------------------------------------
echo.
pause
