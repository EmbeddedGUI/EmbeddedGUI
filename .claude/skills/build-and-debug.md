---
name: build-and-debug
description: Use when building EmbeddedGUI projects, diagnosing compile errors, or debugging runtime issues on PC simulator
---

# Build & Debug Skill

EmbeddedGUI 构建系统说明和常见问题排查指南。

## 构建系统架构

```
Makefile (根)
├── example/{APP}/build.mk      ← 应用源码和资源目录
├── src/build.mk                ← 核心库源码
├── porting/{PORT}/build.mk     ← 平台移植层
└── porting/pc/Makefile.base    ← 核心构建逻辑（编译/链接/资源生成）
```

每个 `build.mk` 定义 `EGUI_CODE_SRC`（源文件）和 `EGUI_CODE_INCLUDE`（头文件路径）。

## Make 命令速查

```bash
# 基本构建（默认 APP=HelloSimple PORT=pc）
make all APP={APP} PORT=pc

# 64位构建（推荐，避免指针截断问题）
make all APP={APP} PORT=pc BITS=64

# HelloBasic 子应用
make all APP=HelloBasic APP_SUB={SUB} PORT=pc BITS=64

# 清理
make clean

# 生成资源（有resource/目录时 make all 自动触发）
make resource

# 强制重新生成资源（忽略缓存）
make resource_refresh

# 运行
make run
```

### 关键参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `APP` | 应用名 | HelloSimple, HelloAPP, HelloStyleDemo |
| `APP_SUB` | HelloBasic子应用 | button, label, viewpage, chart_line |
| `PORT` | 平台 | pc, stm32g0, qemu |
| `BITS` | 位宽 | 64（推荐PC用64位） |
| `COMPILE_OPT_LEVEL` | 优化级别 | -O0（快速编译）, -O2 |
| `USER_CFLAGS` | 自定义编译标志 | `-DEGUI_CONFIG_SCEEN_WIDTH=320` |

## 全量编译检查

```bash
# 快速检查（默认应用）
python scripts/code_compile_check.py

# 全量检查（所有应用 + HelloBasic所有子应用）
python scripts/code_compile_check.py --full-check --bits64

# 清理后检查
python scripts/code_compile_check.py --clean --full-check --bits64
```

特点：使用快速编译标志（`-O0`，无调试符号），per-app OBJDIR 避免应用间 clean，自动运行单元测试。

## 常见编译错误诊断

| 错误信息 | 原因 | 修复 |
|----------|------|------|
| `undefined reference to 'egui_res_image_xxx'` | 资源未生成或配置不匹配 | `make resource_refresh` 或检查 `app_resource_config.json` |
| `undefined reference to 'egui_res_font_xxx'` | 字体资源缺失 | 检查字体配置，确认 ttf 文件存在 |
| `conflicting types for 'xxx'` | 头文件声明与实现不一致，或旧 obj 残留 | `make clean` 后重新编译 |
| `SDL2.dll not found` (Windows) | SDL2 库缺失 | 构建系统自动从 `porting/pc/sdl2/{BITS}/bin/` 复制 |
| `implicit declaration of function` | 缺少 `#include` 或函数签名变更 | 添加正确的头文件引用 |
| `multiple definition of 'xxx'` | 全局变量/函数在头文件中定义而非声明 | 头文件用 `extern` 声明，`.c` 文件中定义 |
| `No rule to make target` | build.mk 中源文件路径错误 | 检查 `EGUI_CODE_SRC` 路径 |

## 构建系统要点

- **Per-app OBJDIR**：`output/obj/{APP}/`，切换应用无需 clean
- **资源自动生成**：`make all` 检测到 `resource/` 目录时自动调用资源生成
- **资源缓存**：存在 `app_egui_resource_merge.bin` 时跳过生成，用 `resource_refresh` 强制重新生成
- **Recording 模式**：`app_egui_config.h` 中 `EGUI_CONFIG_RECORDING_TEST=1` 启用录制测试

## 调试策略

### PC 模拟器调试（推荐）

PC 模拟器基于 SDL2，可直接用 GDB/LLDB 或 IDE 调试：

```bash
# 编译带调试符号
make all APP={APP} PORT=pc BITS=64 COMPILE_DEBUG=-g COMPILE_OPT_LEVEL=-O0

# GDB 调试
gdb output/main.exe
```

### Printf 调试

框架提供 `EGUI_LOG_INF/WRN/ERR` 宏（定义在 `egui_log.h`）：

```c
#include "egui_log.h"
EGUI_LOG_INF("value: %d\n", some_value);
```

### 常见运行时问题

| 症状 | 可能原因 | 排查方向 |
|------|----------|----------|
| 启动即崩溃 | 空指针、未初始化控件 | 检查 init 调用顺序 |
| 黑屏无渲染 | PFB 配置错误、draw 函数未注册 | 检查 `app_egui_config.h` 中 PFB 尺寸 |
| 触控无响应 | 事件未注册、控件不可见 | 确认 `set_on_click_listener` 已调用 |
| 内存溢出 | PFB 过大、控件树过深 | 减小 PFB 尺寸，简化布局层级 |
| 画面撕裂 | 刷新率与绘制不同步 | PC 模拟器正常则忽略（硬件问题） |

## 文件参考

| 文件 | 说明 |
|------|------|
| `Makefile` | 根构建配置 |
| `porting/pc/Makefile.base` | 核心构建逻辑 |
| `scripts/code_compile_check.py` | 全量编译检查 |
| `scripts/code_format.py` | clang-format 代码格式化 |
| `example/{APP}/build.mk` | 应用构建模块 |
| `example/{APP}/app_egui_config.h` | 应用配置（屏幕尺寸、PFB、颜色深度） |
