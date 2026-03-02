# 环境搭建

本文介绍如何在 PC 上搭建 EmbeddedGUI 的开发环境。EGUI 支持在 PC 上编译调试，等 UI 交互行为测试成功后，再将代码部署到嵌入式芯片中。PC 端与嵌入式端使用同一份应用代码，只是 Porting 层不同。

## 依赖概览

EGUI 在 PC 上编译运行需要以下工具:

| 工具 | 用途 | 是否必需 |
|------|------|----------|
| GCC | C 编译器 (PC 模拟器) | 必需 |
| GNU Make | 构建工具 | 必需 |
| SDL2 | PC 模拟器显示和输入 | 必需 (项目已内置 Windows 版) |
| Python 3.8+ | 资源生成、脚本工具 | 仅修改资源时需要 |
| ARM GCC | 嵌入式交叉编译 (STM32/QEMU) | 可选 |
| QEMU | ARM 仿真器，性能基准测试 | 可选 |
| Emscripten | WASM Web demo 构建 | 可选 |
| Playwright | Figma 设计自动截图 | 可选 |

## Windows 环境搭建 (推荐)

### 方式一: setup.bat 一键配置 (推荐)

项目根目录提供了 `setup.bat` 自动化配置脚本，可自动检测并安装所需工具链。

```bash
# 在项目根目录双击运行，或在命令行中执行:
setup.bat
```

脚本执行流程:

**步骤 1 - 检测 make.exe**

脚本会依次在以下位置查找 `make.exe`:
- 系统 PATH 环境变量
- `tools/w64devkit/bin/` (项目内置工具链目录)
- `tools/win32/` (项目内置备用目录)

**步骤 2 - 检测 gcc.exe**

与 make 类似，脚本会检查系统 PATH 和项目内置工具链目录。

如果 make 或 gcc 缺失，脚本会提供自动下载选项:

```
缺少编译工具链 (make/gcc)

可自动下载 w64devkit (约 37 MB) 获得完整 C 工具链:
  包含: GCC 15.2 + GNU Make 4.4 + GDB + Binutils
  来源: https://github.com/skeeto/w64devkit
  免安装，解压即用

1. 自动下载并安装 w64devkit (推荐)
2. 跳过，稍后手动安装
```

选择选项 1 即可自动下载并解压 w64devkit 到 `tools/w64devkit/` 目录。

**步骤 3 - 检测 Python**

检查系统中是否安装了 Python。Python 用于资源生成 (`make resource`) 和各种辅助脚本。如果只需编译已有示例 (不修改资源文件)，可跳过此步骤。

**步骤 4 - 配置 Python 依赖**

提供三种安装模式:

```
1. 基础模式 - 仅安装构建所需依赖
   (freetype_py, json5, numpy, Pillow, pyelftools)
2. 完整模式 - 安装构建 + UI Designer 依赖
   (包含 PyQt5, PyQt-Fluent-Widgets 等)
3. 跳过 Python 依赖安装
```

- **基础模式**: 安装字体处理、图片处理、ELF 分析等构建相关依赖，满足日常开发。
- **完整模式**: 额外安装 PyQt5 和 UI Designer 所需的界面组件库，适合需要使用可视化设计器的用户。

**步骤 5 - 检测 ARM GCC 工具链**

检查 `ARM_GCC_PATH` 环境变量或 PATH 中是否有 `arm-none-eabi-gcc`。此工具链用于编译 STM32 和 QEMU 固件。

**步骤 6 - 检测 QEMU**

检查 `QEMU_PATH` 环境变量或 PATH 中是否有 `qemu-system-arm`。QEMU 用于运行性能基准测试。

**步骤 7 - 检测 Emscripten**

检查 PATH 中是否有 `emcc` 或 `EMSDK` 环境变量。Emscripten 用于将示例编译为 WebAssembly 在浏览器中运行。

**步骤 8 - 检测 Playwright**

检查 Python 环境中是否安装了 Playwright。用于 Figma 设计稿的自动截图和渲染对比。

**验证构建**

脚本会自动尝试编译 HelloSimple 示例来验证环境是否配置成功:

```
正在尝试编译 HelloSimple ...
[OK] HelloSimple 编译成功!
```

**环境变量提示**

setup.bat 仅在当前命令行会话中临时添加工具链路径。如需永久生效，请将以下路径添加到系统环境变量 PATH:

```
<项目路径>\tools\w64devkit\bin
```

### 环境变量汇总

以下环境变量用于配置可选工具链路径，可在系统环境变量中设置，也可在 `setup.bat` 运行后自动检测:

| 环境变量 | 用途 | 示例值 |
|----------|------|--------|
| `ARM_GCC_PATH` | ARM GCC 工具链安装目录 | `D:\Arm GNU Toolchain\12.2` |
| `QEMU_PATH` | QEMU 安装目录 | `C:\Program Files\qemu` |
| `EMSDK` | Emscripten SDK 根目录 | `D:\emsdk` |
| `SEGGER_JLINK_PATH` | J-Link 调试器安装目录 (STM32 烧录) | `C:\Program Files\SEGGER\JLink` |

### 可选环境安装指南

**ARM GCC (嵌入式交叉编译)**

用于编译 STM32 和 QEMU 固件。下载地址: [ARM GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

安装后设置环境变量:
```bash
set ARM_GCC_PATH=D:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1
```

**QEMU (ARM 仿真器)**

用于运行性能基准测试。下载地址: [QEMU Downloads](https://www.qemu.org/download/#windows)

安装后设置环境变量:
```bash
set QEMU_PATH=C:\Program Files\qemu
```

**Emscripten (WASM 构建)**

用于将示例编译为 WebAssembly 在浏览器中运行。安装指南: [Emscripten Getting Started](https://emscripten.org/docs/getting_started/downloads.html)

```bash
# 安装并激活
emsdk install latest
emsdk activate latest
```

**Playwright (Figma 设计截图)**

用于 Figma 设计稿的自动截图和渲染对比:

```bash
pip install playwright
playwright install chromium
```

### 方式二: 手动安装

如果不使用 setup.bat，可以手动安装以下工具:

1. **GCC + Make**: 安装 [MSYS2](https://www.msys2.org/) 并在 MSYS2 终端执行:

   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
   ```

   将 `C:\msys64\mingw64\bin` 添加到系统 PATH。

   或者下载 [w64devkit](https://github.com/skeeto/w64devkit/releases)，解压后将 `bin` 目录添加到 PATH。

2. **Python 3**: 从 [python.org](https://www.python.org/downloads/) 下载安装，安装时勾选 "Add Python to PATH"。

3. **Python 依赖**: 在项目根目录执行:

   ```bash
   python -m pip install -r requirements.txt
   ```

4. **SDL2**: 项目已内置 Windows 版 SDL2 静态库 (位于 `porting/pc/sdl2/`)，无需额外安装。

## Linux 环境搭建

在 Linux (Ubuntu/Debian) 上，安装非常简单:

```bash
# 安装编译工具链和 SDL2 开发库
sudo apt-get update && sudo apt-get install -y build-essential libsdl2-dev

# 安装 Python 3 和 pip (大多数发行版已预装)
sudo apt-get install -y python3 python3-pip

# 安装 Python 依赖
python3 -m pip install -r requirements.txt
```

对于其他 Linux 发行版:

```bash
# Fedora / RHEL
sudo dnf install gcc make SDL2-devel python3 python3-pip

# Arch Linux
sudo pacman -S gcc make sdl2 python python-pip
```

## macOS 环境搭建

首先安装 [Homebrew](https://brew.sh/):

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

然后安装依赖:

```bash
# 安装编译工具链和 SDL2
brew install gcc make sdl2

# 安装 Python 依赖
python3 -m pip install -r requirements.txt
```

> **提示**: macOS 上 SDL2 的安装可能需要额外配置，可参考 [SDL2 安装指南](https://www.bilibili.com/opus/940636995053420548)。

## 验证安装

无论使用哪种操作系统，安装完成后都可以通过以下命令验证环境:

```bash
# 编译 HelloSimple 示例
make all APP=HelloSimple

# 运行
make run
```

如果一切正常，将弹出一个窗口显示 "Hello World!" 标签和一个 "Click me!" 按钮。

也可以尝试编译其他示例:

```bash
# 编译并运行 HelloBasic 的按钮示例
make all APP=HelloBasic APP_SUB=button
make run

# 编译并运行 HelloActivity
make all APP=HelloActivity
make run
```

## CMake 构建方式

EGUI 也支持 CMake 构建，适合习惯 CMake 工作流或需要集成到 CMake 项目中的用户:

```bash
# 创建构建目录
mkdir build && cd build

# 生成 Makefile
cmake ..

# 编译
make all
```

> **注意**: CMake 支持目前仍在完善中，推荐优先使用 Makefile 方式构建。

## VSCode 开发调试

EGUI 项目预配置了 VSCode 的编译和调试任务。用 VSCode 打开项目根目录后:

1. 确保已安装 C/C++ 扩展 (ms-vscode.cpptools)
2. 使用 `F5` 或调试面板启动调试
3. 设置断点即可进行单步调试

## 常见问题

### freetype_py 安装后提示 "Freetype library not found"

这是 Windows 上的常见问题。可以尝试:

```bash
pip install freetype-py --force-reinstall
```

或参考 [解决方案](https://blog.csdn.net/wyx100/article/details/73527117)。

### make 命令提示 "not recognized"

确认编译工具已正确添加到系统 PATH 环境变量。如果使用 setup.bat 安装了 w64devkit，需要将 `tools\w64devkit\bin` 的完整路径添加到系统 PATH。

### 编译报错 "SDL2.h: No such file or directory"

- Windows: 确认项目 `porting/pc/sdl2/` 目录下存在对应位数 (32/64) 的 SDL2 库文件。
- Linux: 执行 `sudo apt-get install libsdl2-dev`。
- macOS: 执行 `brew install sdl2`。

## 下一步

环境搭建完成后，请继续阅读:

- [第一个应用](first_app.md): 深入理解 HelloSimple 示例代码
- [构建系统详解](build_system.md): 了解 Makefile 参数和构建流程
