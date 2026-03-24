# 环境搭建

本文介绍如何为 EmbeddedGUI 配置开发环境。当前推荐入口已经统一为：

- Windows 使用 `setup.bat`
- Linux / macOS 使用 `setup.sh`
- 实际安装逻辑统一由 `scripts/setup_env.py` 负责

默认情况下，setup 脚本会创建虚拟环境、安装完整 Python 依赖，并准备构建与媒体处理所需的本地工具。

## 依赖概览

| 工具 | 用途 | 是否必需 |
|------|------|----------|
| GCC | C 编译器（PC 模拟器） | 必需 |
| GNU Make | 构建工具 | 必需 |
| SDL2 | PC 模拟器显示和输入 | 必需（Windows 版本已内置） |
| Python 3.8+ | 资源生成、脚本工具、UI Designer | 必需 |
| PyQt5 / PyQt-Fluent-Widgets | UI Designer 桌面界面 | 默认安装 |
| Playwright | 设计稿截图与自动化验证 | 默认安装 Python 包 |
| FFmpeg | MP4 转序列帧、GIF 录制等媒体处理 | 默认检查，Windows 缺失时自动下载 |
| ARM GCC | STM32 / QEMU 交叉编译 | 可选 |
| QEMU | ARM 仿真与性能测试 | 可选 |
| Emscripten | WASM 构建 | 可选 |

## Windows 环境搭建

### 推荐方式：运行 `setup.bat`

在项目根目录执行：

```bat
setup.bat
```

`setup.bat` 只负责：

- 检查系统里是否能找到 Python
- 调用 `scripts/setup_env.py`

真正的环境安装逻辑都在 Python 脚本中，后续维护以 Python 版本为准。

### 默认行为

直接执行：

```bat
setup.bat
```

等价于：

```bat
python scripts\setup_env.py --python-mode full
```

默认会完成以下动作：

1. 创建 `.venv`
2. 升级 `pip`
3. 安装 `requirements.txt`
4. 安装 `scripts/ui_designer/requirements-desktop.txt`
5. 安装 `playwright` Python 包
6. 校验 `json5`、`numpy`、`Pillow`、`freetype_py`、`pyelftools`
7. 校验 `PyQt5`、`qfluentwidgets`、`ui_designer.main`
8. 检查本地或系统中的 `make` / `gcc`
9. 在缺失时自动安装 `tools/w64devkit`
10. 检查 `ffmpeg`
11. 在缺失时自动安装 `tools/ffmpeg`
12. 默认编译一次 `HelloSimple` 做验证

### 常用参数

```bat
setup.bat --python-mode basic
setup.bat --python-mode none
setup.bat --skip-toolchain
setup.bat --skip-ffmpeg
setup.bat --skip-build-check
setup.bat --venv-dir .venv_custom
setup.bat --install-toolchain
setup.bat --install-ffmpeg
```

说明：

- `--python-mode full`
  默认值。安装完整 Python 依赖，适合绝大多数开发者。
- `--python-mode basic`
  只安装基础 Python 依赖，不安装 UI Designer 桌面依赖。
- `--python-mode none`
  跳过 Python 依赖安装，仅做工具链检查。
- `--skip-toolchain`
  跳过 `make` / `gcc` 检查和 `w64devkit` 自动安装。
- `--skip-ffmpeg`
  跳过 `ffmpeg` 检查和自动安装。仅在不需要 MP4 / GIF 相关流程时使用。
- `--skip-build-check`
  跳过 `HelloSimple` 编译验证。
- `--venv-dir`
  指定虚拟环境目录。
- `--install-toolchain`
  仅安装 Windows 工具链并退出。
- `--install-ffmpeg`
  仅安装 Windows 的 FFmpeg 本地包并退出。

### 关于 `w64devkit`

如果系统里没有 `make.exe` 或 `gcc.exe`，脚本会自动尝试安装 `w64devkit` 到：

```text
tools/w64devkit/
```

安装完成后，脚本会优先使用：

```text
tools/w64devkit/bin
```

注意：

- 该路径只会在当前脚本进程中注入
- 如果你想全局长期使用，需要手动加入系统 `PATH`

### 关于 `ffmpeg`

脚本会优先检查系统 `PATH` 中现有的 `ffmpeg`。如果在 Windows 下未找到，会自动下载并解压到：

```text
tools/ffmpeg/
```

安装完成后，脚本会优先使用：

```text
tools/ffmpeg/bin
```

这部分主要用于：

- `scripts/tools/app_mp4_image_generate.py` 的 MP4 转序列帧流程
- `scripts/gif_recorder.py` 的高质量 GIF 导出流程

如果你不需要这些媒体处理能力，可以显式传入 `--skip-ffmpeg`。

## Linux / macOS 环境搭建

### 推荐方式：运行 `setup.sh`

在项目根目录执行：

```bash
./setup.sh
```

默认行为与 Windows 一致，也会安装完整 Python 依赖，并检查系统中的 `make`、`gcc` 与 `ffmpeg`。

常用参数同样支持：

```bash
./setup.sh --python-mode basic
./setup.sh --python-mode none
./setup.sh --skip-ffmpeg
./setup.sh --skip-build-check
./setup.sh --venv-dir .venv_custom
```

### 先安装系统依赖

#### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential libsdl2-dev python3 python3-venv python3-pip ffmpeg
```

#### Fedora

```bash
sudo dnf install gcc make SDL2-devel python3 python3-pip ffmpeg
```

#### Arch Linux

```bash
sudo pacman -S base-devel sdl2 python python-pip ffmpeg
```

#### macOS

先安装 Homebrew，然后执行：

```bash
brew install gcc make sdl2 python ffmpeg
```

## 手动安装方式

如果不使用 `setup.bat` / `setup.sh`，也可以手动执行。

### 1. 创建虚拟环境

Windows：

```bat
python -m venv .venv
.venv\Scripts\activate.bat
```

Linux / macOS：

```bash
python3 -m venv .venv
source .venv/bin/activate
```

### 2. 安装 Python 依赖

基础依赖：

```bash
python -m pip install -r requirements.txt
```

完整依赖（包含 UI Designer）：

```bash
python -m pip install -r requirements.txt
python -m pip install -r scripts/ui_designer/requirements-desktop.txt
python -m pip install playwright
```

如需浏览器运行时，再执行：

```bash
python -m playwright install chromium
```

### 3. 安装编译工具链

Windows 可选：

- MSYS2
- MinGW-w64
- `w64devkit`

如果使用 `w64devkit`，解压后确保 `bin` 目录可被找到。

### 4. 准备 `ffmpeg`

Windows 可直接运行：

```bat
setup.bat --install-ffmpeg
```

Linux / macOS 通过系统包管理器安装后，可用以下命令确认：

```bash
ffmpeg -version
```

## 验证安装

无论使用哪种方式，建议至少验证一次：

```bash
make all APP=HelloSimple PORT=pc
make run
```

如果一切正常，会弹出 PC 模拟器窗口。

也可以继续验证：

```bash
python scripts/code_runtime_check.py --app HelloSimple --timeout 10
```

## 可选工具

### ARM GCC

用于 STM32 / QEMU 目标：

```text
ARM_GCC_PATH=<toolchain_root>
```

### QEMU

用于性能基准和仿真测试：

```text
QEMU_PATH=<qemu_install_dir>
```

### Emscripten

用于 WASM 构建：

```text
EMSDK=<emsdk_root>
```

## 常见问题

### `setup.bat` 或 `setup.sh` 提示 Python 未找到

先安装 Python 3.8+，并确保 `python` 或 `python3` 可执行。

### Python 依赖安装失败

脚本会先尝试镜像源，再回退到官方 PyPI。若仍失败，会打印手动恢复命令，按提示执行即可。

### `ffmpeg` 未找到

Windows 下重新运行 `setup.bat` 即可触发自动安装。Linux / macOS 请先通过系统包管理器安装，或在不需要相关流程时传入 `--skip-ffmpeg`。

## 下一步

环境搭建完成后，请继续阅读：

- [第一个应用](first_app.md)
- [构建系统详解](build_system.md)
