# 构建系统详解

EmbeddedGUI 使用 GNU Make 作为主要构建系统。本文详细介绍 Makefile 的参数配置、常用命令、模块化构建机制和交叉编译方法。

## Makefile 参数说明

项目根目录的 `Makefile` 定义了所有可配置参数。可通过命令行传参覆盖默认值，也可以直接修改 `Makefile` 中的默认配置。

### APP -- 选择示例应用

`APP` 参数指定要编译的示例应用，对应 `example/` 目录下的子目录名。默认值为 `HelloSimple`。

```bash
# 编译 HelloSimple (默认)
make all

# 编译 HelloActivity
make all APP=HelloActivity

# 编译 HelloStyleDemo
make all APP=HelloStyleDemo
```

可用的示例应用:

| APP 值 | 说明 |
|--------|------|
| `HelloSimple` | 最简单的入门示例 |
| `HelloBasic` | 控件演示集合 (需配合 APP_SUB) |
| `HelloVirtual` | Virtual / ListView / GridView / Stage 示例集合 (需配合 APP_SUB) |
| `HelloActivity` | Activity 生命周期演示 |
| `HelloAPP` | 多 Activity 应用 |
| `HelloStyleDemo` | 样式和主题演示 |
| `HelloChart` | 图表控件 |
| `HelloBattery` | 电池界面 |
| `HelloPFB` | PFB 机制演示 |
| `HelloEasyPage` | 简易分页 |
| `HelloViewPageAndScroll` | 分页和滚动 |
| `HelloPerformance` | 性能测试 |
| `HelloResourceManager` | 资源管理器 |
| `HelloCanvas` | 画布绘图 |
| `HelloGradient` | 渐变效果 |
| `HelloTest` | 功能测试 |
| `HelloUnitTest` | 单元测试 |

### APP_SUB -- 选择 HelloBasic / HelloVirtual 子应用

`APP_SUB` 适用于 `HelloBasic` 和 `HelloVirtual` 这类多子应用示例：

- `HelloBasic`: 基础控件演示，当前包含 58 个子应用
- `HelloVirtual`: virtual/list/grid/stage 示例，当前包含 19 个子应用

```bash
# 编译按钮演示
make all APP=HelloBasic APP_SUB=button

# 编译滑块演示
make all APP=HelloBasic APP_SUB=slider

# 编译折线图演示
make all APP=HelloBasic APP_SUB=chart_line

# 编译模拟时钟演示
make all APP=HelloBasic APP_SUB=analog_clock

# 编译 HelloVirtual basic stage 演示
make all APP=HelloVirtual APP_SUB=virtual_stage_basic

# 编译 HelloVirtual showcase 对照演示
make all APP=HelloVirtual APP_SUB=virtual_stage_showcase
```

常用 `HelloBasic` 子应用: `button`、`slider`、`combobox`、`textinput`、`table`、`activity_ring`、`mini_calendar`、`button_matrix`、`window`、`menu` 等。

常用 `HelloVirtual` 子应用: `virtual_viewport_basic`、`virtual_page_basic`、`virtual_grid_basic`、`list_view_basic`、`grid_view_basic`、`virtual_stage_basic`、`virtual_stage_showcase`、`virtual_stage` 等。

### PORT -- 选择目标平台

`PORT` 参数指定目标平台，对应 `porting/` 目录下的子目录名。默认值为 `pc`。

```bash
# PC 模拟器 (默认)
make all APP=HelloSimple PORT=pc

# STM32G0 交叉编译
make all APP=HelloSimple PORT=stm32g0

# QEMU 虚拟化环境
make all APP=HelloSimple PORT=qemu

# WebAssembly
make all APP=HelloSimple PORT=emscripten
```

### 其他参数

```bash
# 优化级别 (默认 -O0 无优化)
make all COMPILE_OPT_LEVEL=-O2     # 速度优化
make all COMPILE_OPT_LEVEL=-Os     # 大小优化

# 调试信息 (默认 -g 包含调试信息)
make all COMPILE_DEBUG=            # 去掉调试信息 (发布构建)

# 显示完整编译命令 (默认静默)
make all V=1

# 指定编译器位数 (通常自动检测)
make all BITS=64
make all BITS=32
```

## 常用命令

### 编译

```bash
# 编译当前配置的应用
make all

# 等价于指定所有默认参数
make all APP=HelloSimple PORT=pc

# 并行编译 (加速)
make all -j
make all -j8      # 指定 8 个并行任务
```

### 运行

```bash
# 编译并运行
make run

# 注意: 如果编译时修改了参数，运行时也需要带上
make run APP=HelloBasic APP_SUB=slider
make run APP=HelloVirtual APP_SUB=virtual_stage_basic
```

`make run` 实际上先执行 `make all`，然后运行 `output/main.exe` (Windows) 或 `output/main` (Linux/macOS)。

### 清理

```bash
# 删除 output/ 目录下的所有编译产物
make clean
```

### 资源生成

```bash
# 生成当前应用的资源文件 (字体、图片等)
make resource

# 强制重新生成所有资源
make resource_refresh
```

资源生成依赖 Python，会调用 `scripts/tools/app_resource_generate.py` 脚本处理 `example/<APP>/resource/` 目录下的资源文件，输出到 `output/` 目录。

### 查看信息

```bash
# 显示当前配置
make info
# 输出: Current Configuration: APP=HelloSimple PORT=pc

# 查看二进制大小
make size

# 生成反汇编和段列表
make dasm
```

## build.mk 模块系统

EGUI 的构建系统基于模块化设计，每个组件目录包含一个 `build.mk` 文件，通过两个变量定义自己的源文件和头文件路径:

- `EGUI_CODE_SRC`: 源文件目录列表
- `EGUI_CODE_INCLUDE`: 头文件搜索路径列表

### 模块包含流程

根目录的 `Makefile` 按以下顺序包含各模块:

```makefile
# 1. 包含示例应用的 build.mk
include $(EGUI_APP_PATH)/build.mk

# 2. 包含核心库的 build.mk
include $(EGUI_PATH)/build.mk

# 3. 包含平台移植的 build.mk
include $(EGUI_PORT_PATH)/build.mk
```

### 示例: HelloSimple 的 build.mk

```makefile
EGUI_CODE_SRC     += $(EGUI_APP_PATH)
EGUI_CODE_SRC     += $(EGUI_APP_PATH)/resource

EGUI_CODE_INCLUDE += $(EGUI_APP_PATH)
```

这告诉构建系统: 将 `example/HelloSimple/` 及其 `resource/` 子目录下的所有 `.c` 文件加入编译，并将 `example/HelloSimple/` 加入头文件搜索路径。

### 示例: HelloBasic 的 build.mk

```makefile
EGUI_CODE_SRC     += $(EGUI_APP_PATH)
EGUI_CODE_INCLUDE += $(EGUI_APP_PATH)

# 根据 APP_SUB 选择子应用
APP_SUB ?= button
EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

EGUI_CODE_SRC     += $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC     += $(EGUI_APP_SUB_PATH)/resource
EGUI_CODE_SRC     += $(EGUI_APP_SUB_PATH)/resource/img
EGUI_CODE_SRC     += $(EGUI_APP_SUB_PATH)/resource/font

EGUI_CODE_INCLUDE += $(EGUI_APP_SUB_PATH)
EGUI_CODE_INCLUDE += $(EGUI_APP_SUB_PATH)/resource
```

`HelloVirtual` 也使用同样的 `APP_SUB` 机制，但每个子应用通常带有自己的 `app_egui_config.h`，因此其 `build.mk` 还会设置独立的 `APP_OBJ_SUFFIX`，避免不同子应用复用错误的编译产物。

### 示例: 核心库的 build.mk

```makefile
EGUI_CODE_SRC     += $(EGUI_PATH)
EGUI_CODE_SRC     += $(EGUI_PATH)/app
EGUI_CODE_SRC     += $(EGUI_PATH)/anim
EGUI_CODE_SRC     += $(EGUI_PATH)/core
EGUI_CODE_SRC     += $(EGUI_PATH)/widget
EGUI_CODE_SRC     += $(EGUI_PATH)/font
EGUI_CODE_SRC     += $(EGUI_PATH)/image
EGUI_CODE_SRC     += $(EGUI_PATH)/mask
EGUI_CODE_SRC     += $(EGUI_PATH)/utils
# ... 更多模块

EGUI_CODE_INCLUDE += $(EGUI_PATH)
```

### 添加新模块

如果你需要添加自己的模块，只需在 `build.mk` 中追加路径即可:

```makefile
# 在示例的 build.mk 中添加自定义模块
EGUI_CODE_SRC     += $(EGUI_APP_PATH)/my_module
EGUI_CODE_INCLUDE += $(EGUI_APP_PATH)/my_module
```

## 应用配置

每个示例应用可以通过 `app_egui_config.h` 文件覆盖框架的默认配置:

```c
#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

// 屏幕分辨率
#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320

// 色深: 16-bit RGB565
#define EGUI_CONFIG_COLOR_DEPTH  16

// PFB 大小 (宽/高必须是屏幕尺寸的整数约数)
#define EGUI_CONFIG_PFB_WIDTH    (240 / 8)   // 30 像素
#define EGUI_CONFIG_PFB_HEIGHT   (320 / 8)   // 40 像素

// 抗锯齿圆的最大半径
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 20

#endif
```

框架的所有默认配置定义在 `src/core/egui_config_default.h` 中，应用的 `app_egui_config.h` 会在其之前被包含，因此可以覆盖任何配置项。

## CMake 构建

EGUI 也提供了 CMake 构建支持:

```bash
mkdir build && cd build
cmake ..
make all
```

CMake 构建主要用于 IDE 集成和代码索引。日常开发和 CI 流程推荐使用 Makefile 方式。

## 交叉编译

### ARM GCC 工具链

编译到 STM32 等 ARM MCU 时，需要安装 ARM GCC 工具链:

```bash
# Ubuntu/Debian
sudo apt-get install gcc-arm-none-eabi

# 或从 ARM 官网下载:
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
```

然后使用对应的 PORT 参数编译:

```bash
make all APP=HelloSimple PORT=stm32g0
```

STM32 平台的 `build.mk` 会自动设置交叉编译器前缀 `CROSS_COMPILE` 和链接脚本。

### 编译产物

编译完成后，产物位于 `output/` 目录:

| 文件 | 说明 |
|------|------|
| `output/main.exe` | PC 可执行文件 (Windows) |
| `output/main` | PC 可执行文件 (Linux/macOS) |
| `output/main.elf` | ARM ELF 文件 |
| `output/main.bin` | ARM 二进制固件 (通过 `make dasm` 生成) |
| `output/SDL2.dll` | SDL2 动态库 (Windows，自动拷贝) |
| `output/app_egui_resource_merge.bin` | 合并后的资源文件 |
| `output/obj/` | 中间目标文件 (.o) |

## 辅助脚本

除了 `make` 命令，项目还提供了多个 Python 辅助脚本:

```bash
# 完整编译检查 (CI 使用，编译所有示例，并包含示例 icon font 约定检查)
python scripts/code_compile_check.py --full-check --bits64

# 运行时验证 (截图对比)
python scripts/code_runtime_check.py --app HelloBasic --app-sub button --timeout 10

# HelloVirtual 子应用运行时验证
python scripts/code_runtime_check.py --app HelloVirtual --timeout 10
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_stage_basic --timeout 10

# 代码格式化
python scripts/code_format.py

# 示例图标字体显式配置检查
python scripts/check_example_icon_font.py

# 如果要把本地未跟踪目录也一起扫描
python scripts/check_example_icon_font.py --include-untracked

# 发布前一键检查
python scripts/release_check.py --skip perf,perf_doc,wasm,doc,ui_package

# ELF 二进制大小分析
python scripts/utils_analysis_elf_size.py

# 资源生成
python scripts/tools/app_resource_generate.py -r example/HelloSimple/resource -o output
```

说明：

- `check_example_icon_font.py` 默认只扫描 git 已跟踪的 `example/` 源文件，避免本地临时目录影响 CI 风格检查
- `code_compile_check.py --full-check` 默认会包含这一步；如果你在外层已经单独跑过 icon font 检查，可用 `--skip-icon-font-check` 避免重复执行

## 下一步

- [项目概述](overview.md): 了解 EGUI 的设计理念和核心特点
- [第一个应用](first_app.md): 从代码层面理解 EGUI 应用结构
- [项目目录结构](project_structure.md): 了解源代码的组织方式
