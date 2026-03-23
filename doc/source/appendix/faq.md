# 常见问题 (FAQ)

本文整理 EmbeddedGUI 开发中最常见的环境、构建和运行问题。

---

## 环境搭建

### Q: `setup.bat` / `setup.sh` 执行失败，提示找不到 Python

**问题描述：** 脚本启动后立即失败，提示未找到 Python。

**解决方案：**

1. 安装 Python 3.8+
2. Windows 确保 `py` 或 `python` 可执行
3. Linux / macOS 确保 `python3` 或 `python` 可执行
4. 重新运行脚本

---

### Q: Python 依赖安装失败

**问题描述：** `setup_env.py` 在安装 `requirements.txt` 或 `requirements-desktop.txt` 时失败。

**原因分析：**

- 网络不稳定
- 镜像源不可用
- 某些包需要手动重试

**解决方案：**

1. 直接按脚本打印出的手动命令执行
2. 先升级 `pip`
3. 必要时分步安装

示例：

```bash
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
python -m pip install -r scripts/ui_designer/requirements-desktop.txt
python -m pip install playwright
python -m playwright install chromium
```

---

### Q: UI Designer 依赖装了，但 `python scripts/ui_designer/main.py` 还是启动失败

**问题描述：** 提示 `PyQt5`、`qfluentwidgets` 或 `ui_designer` 导入失败。

**解决方案：**

1. 确认你使用的是项目虚拟环境里的 Python
2. 重新安装完整依赖
3. 用一条命令验证导入链路

```bash
python -m pip install -r scripts/ui_designer/requirements-desktop.txt
python -m pip install playwright
python -c "import os, sys; sys.path.insert(0, 'scripts'); import ui_designer.main"
```

---

### Q: Windows 下自动下载 `w64devkit` 失败

**问题描述：** 脚本提示下载 `w64devkit` 失败，可能是网络或 SSL 错误。

**解决方案：**

1. 手动下载 `w64devkit`
2. 解压到项目根目录下的 `tools/w64devkit/`
3. 重新运行 `setup.bat`

下载地址：

```text
https://github.com/skeeto/w64devkit/releases
```

---

## 构建问题

### Q: 找不到 `make` 命令

**问题描述：** 执行 `make all` 时提示 `make` 不存在。

**解决方案：**

1. Windows 重新运行 `setup.bat`
2. Linux / macOS 安装系统编译工具链
3. 确认 `make --version` 能输出版本号

---

### Q: 找不到 `gcc`

**问题描述：** 构建时报 `gcc: No such file or directory`。

**解决方案：**

1. Windows 重新运行 `setup.bat`，让脚本检查或安装 `w64devkit`
2. Linux / macOS 通过系统包管理器安装 `gcc`
3. 确认 `gcc --version` 正常

---

### Q: `libwinpthread-1.dll` 找不到

**问题描述：** 之前某些 Windows 环境会在构建或运行时提示 `libwinpthread-1.dll` 缺失。

**说明：**

当前构建系统已经调整为：

- 仅当该 DLL 实际存在时才复制
- 对项目内 `w64devkit` 场景不再强制依赖这个文件

如果你仍然遇到这个问题：

1. 先删除 `output/` 下旧的 DLL 残留
2. 重新编译一次
3. 若你使用自己的 MinGW 工具链，再检查对应运行时 DLL 是否在 PATH 中

---

### Q: `SDL2.h: No such file or directory`

**问题描述：** 编译 PC 模拟器时找不到 SDL2 头文件。

**解决方案：**

- Windows：确认仓库内 `porting/pc/sdl2/` 目录完整
- Linux：安装 `libsdl2-dev`
- macOS：安装 `sdl2`

---

## 运行问题

### Q: 程序编译成功，但窗口不显示

**解决方案：**

1. 先执行 `make run`
2. 检查 `app_egui_config.h` 中的屏幕尺寸是否合理
3. 检查 `egui_polling_work()` 是否在主循环中被调用
4. 用运行时检查脚本验证

```bash
python scripts/code_runtime_check.py --app HelloSimple --timeout 10
```

---

### Q: 程序卡死或无响应

**解决方案：**

1. 检查定时器回调里是否做了阻塞操作
2. 检查是否存在死循环
3. 先在 PC 模拟器下复现
4. 再用调试器定位

---

## 资源问题

### Q: 字体或图片显示不正确

**解决方案：**

1. 重新生成资源

```bash
make resource_refresh APP=<APP>
```

2. 检查资源配置文件
3. 确认字体字符集是否覆盖所需文本

---

## 其他

### Q: 想完全跳过 Python 安装，只检查工具链

可以执行：

```bash
python scripts/setup_env.py --python-mode none
```

或：

```bash
setup.bat --python-mode none
```

### Q: 想只安装基础 Python 依赖，不装 UI Designer

可以执行：

```bash
python scripts/setup_env.py --python-mode basic
```

或：

```bash
setup.bat --python-mode basic
```
