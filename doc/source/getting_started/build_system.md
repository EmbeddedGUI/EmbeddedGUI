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
| `HelloBasic` | 基础控件演示集合（62 个子应用，需配合 `APP_SUB`） |
| `HelloVirtual` | Virtual / ListView / GridView / Stage 示例集合（19 个子应用，需配合 `APP_SUB`） |
| `HelloCustomWidgets` | 已迁移到独立仓库 `EmbeddedGUI_Widgets`，主仓不再本地构建 |
| `HelloActivity` | Activity 生命周期演示 |
| `HelloAPP` | 基于 `egui_page_base_t` 的多页面应用 |
| `HelloCanvas` | 画布绘图 API 演示 |
| `HelloChart` | 图表控件演示 |
| `HelloDirty` | 脏区刷新局部重绘演示 |
| `HelloEasyPage` | 简易分页应用 |
| `HelloGradient` | 渐变效果演示 |
| `HelloLayer` | Layer 机制演示 |
| `HelloMultiDisplay` | 主屏和副屏同为 240x320，演示多屏 Activity 切换与副屏独立输入 |
| `HelloMultiDisplayHetero` | 主屏 240x320，副屏 128x64，演示异构副屏状态面板和跨屏状态联动 |
| `HelloPerformance` | 性能测试基准 |
| `HelloPFB` | PFB 机制演示 |
| `HelloResourceManager` | 资源管理器示例 |
| `HelloShowcase` | 综合展示示例 |
| `HelloSizeAnalysis` | 体积分析 probe / 配置模板集合（需配合 `APP_SUB`） |
| `HelloStyleDemo` | 样式和主题演示 |
| `HelloSVGSpec` | SVG 规范对比基座 |
| `HelloTest` | 功能测试 |
| `HelloUnitTest` | 单元测试 |
| `HelloViewPageAndScroll` | 分页和滚动组合 |

### APP_SUB -- 选择带子应用的示例

`APP_SUB` 适用于 `HelloBasic`、`HelloVirtual` 和 `HelloSizeAnalysis` 这类多子应用示例：

- `HelloBasic`: 基础控件演示，当前包含 62 个子应用
- `HelloVirtual`: virtual/list/grid/stage 示例，当前包含 19 个子应用
- `HelloSizeAnalysis`: probe / preset 目录集合，通常配合 `PORT=qemu`

```bash
# 编译按钮演示
make all APP=HelloBasic APP_SUB=button

# 编译 Stepper 演示
make all APP=HelloBasic APP_SUB=stepper

# 编译 HelloVirtual basic stage 演示
make all APP=HelloVirtual APP_SUB=virtual_stage_basic

# HelloCustomWidgets 已迁移到独立仓库 EmbeddedGUI_Widgets
# 请在该仓库中编译对应 widget

# 编译 HelloSizeAnalysis 的 widget probe
make all APP=HelloSizeAnalysis APP_SUB=widget_feature_probe PORT=qemu
```

常用 `HelloBasic` 子应用: `button`、`slider`、`combobox`、`textinput`、`table`、`activity_ring`、`mini_calendar`、`button_matrix`、`autocomplete`、`chips`、`stepper`、`pattern_lock` 等。

常用 `HelloVirtual` 子应用: `virtual_viewport_basic`、`virtual_page_basic`、`virtual_grid_basic`、`list_view_basic`、`grid_view_basic`、`virtual_stage_basic`、`virtual_stage_showcase`、`virtual_stage` 等。

`HelloCustomWidgets` 已迁移到独立仓库 `EmbeddedGUI_Widgets`。

常用 `HelloSizeAnalysis` 子应用: `widget_feature_probe`、`canvas_path_probe`、`hq_path_probe`、`preset_validation`。

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
# 优化级别 (根目录 Makefile 默认 -O2)
make all COMPILE_OPT_LEVEL=-O0     # 快速调试构建
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

`HelloBasic`、`HelloVirtual` 和 `HelloSizeAnalysis` 这类多子应用示例都会设置独立的 `APP_OBJ_SUFFIX`，避免不同子应用复用错误的编译产物。

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
#define EGUI_CONFIG_SCREEN_WIDTH  240
#define EGUI_CONFIG_SCREEN_HEIGHT 320

// 色深: 16-bit RGB565
#define EGUI_CONFIG_COLOR_DEPTH  16

// PFB 大小 (宽/高建议优先选为屏幕尺寸的整数约数)
#define EGUI_CONFIG_PFB_WIDTH    (240 / 8)   // 30 像素
#define EGUI_CONFIG_PFB_HEIGHT   (320 / 8)   // 40 像素

// 抗锯齿圆的最大半径
#define EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE 20

#endif
```

框架默认配置现在分成两层：

- `src/config/egui_config_default.h`：主屏基础默认值
- `src/config/egui_config_multi_default.h`：多屏默认值，例如 `EGUI_CONFIG_MAX_DISPLAY_COUNT`、`EGUI_CONFIG_SCREEN_1_*`、`EGUI_CONFIG_PFB_1_*`

应用的 `app_egui_config.h` 会先于这些默认头被包含，因此可以覆盖任意配置项。

## CMake 构建

EGUI 也提供了 CMake 构建支持，适合 IDE 集成和代码索引。当前仓库已经提供 CMake 移植层的平台是 `pc` 和 `pc_test`：

```bash
# 构建独立示例
cmake -B build_cmake/HelloSimple -DAPP=HelloSimple -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloSimple -j

# 构建 HelloBasic 子应用
cmake -B build_cmake/HelloBasic_button -DAPP=HelloBasic -DAPP_SUB=button -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloBasic_button -j

# 构建 HelloVirtual 子应用
cmake -B build_cmake/HelloVirtual_virtual_stage_basic -DAPP=HelloVirtual -DAPP_SUB=virtual_stage_basic -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloVirtual_virtual_stage_basic -j

# 构建单元测试（pc_test）
cmake -B build_cmake/HelloUnitTest_pc_test -DAPP=HelloUnitTest -DPORT=pc_test -G "MinGW Makefiles"
cmake --build build_cmake/HelloUnitTest_pc_test -j
```

Windows 下也可以用仓库内置的 Visual Studio + SDL2 preset 生成 CMake 工程并调试 PC 模拟器：

```powershell
cmake --preset visual_studio_sdl
cmake --build --preset visual_studio_sdl
.\build_vs\visual_studio_sdl\output\Debug\HelloSimple.exe
```

`visual_studio_sdl` 默认使用 Visual Studio 2022、MSVC x64 和仓库内置 SDL2。若使用 Visual Studio 2026，可改用 `visual_studio_sdl_vs2026`。需要切换示例时，在 CMake configure 参数中覆盖 `APP` / `APP_SUB` / `PORT`，例如 `-DAPP=HelloBasic -DAPP_SUB=button -DPORT=pc`。

如果只想双击打开工程，可直接打开 `porting/pc/EmbeddedGUI_PC_Simulator.sln`。该方案参考 `lv_port_pc_visual_studio` 的组织方式：PC 模拟器相关的 Visual Studio 工程文件集中放在 `porting/pc/`，仓库只维护一个 `EmbeddedGUI_PC_Simulator` 工程，公共源码和 SDL2/MSVC 配置固定在工程中，常用示例通过 Visual Studio 顶部的解决方案配置下拉框选择。该工程只维护 x64 平台配置，避免 Visual Studio 打开时自动补全 Win32 映射并重写 `.sln`。

常用配置包括：

- `HelloSimple_Debug|x64`
- `HelloBasic_button_Debug|x64`
- `HelloBasic_slider_Debug|x64`
- `HelloVirtual_virtual_viewport_basic_Debug|x64`
- `HelloVirtual_virtual_stage_basic_Debug|x64`
- `HelloGame_snake_Debug|x64`
- `HelloSizeAnalysis_canvas_path_probe_Debug|x64`

其中普通示例使用 `APP_Debug` 命名，带子应用的示例使用 `APP_APP_SUB_Debug` 命名。选择配置后直接生成或按 F5 调试即可，输出位于 `build_vs/<APP 或 APP_APP_SUB>/x64/<配置名>/`，例如 `build_vs/HelloBasic_button/x64/HelloBasic_button_Debug/HelloBasic_button.exe`。Visual Studio 方案使用 Windows 子系统运行 PC 模拟器，不弹出额外控制台窗口；`Hello, egui!`、录制状态、shutdown 检查、普通 `printf()` 和 `EGUI_LOG_*` 日志会显示在 Visual Studio 的“输出”窗口中，查看来源选择“调试”。Debug 配置会在应用未显式配置日志级别时默认打开 INFO 及以上框架日志。

`porting/pc/EmbeddedGUI.VisualStudio.props` 仍保留为高级自定义入口。没有预置到下拉框里的示例，可以选择 `Debug|x64` 或 `Release|x64`，再修改下面的默认值：

```xml
<EguiApp>HelloSimple</EguiApp>
<EguiAppSub></EguiAppSub>
<EguiPort>pc</EguiPort>
```

需要切换到带子应用的示例时，修改 `porting/pc/EmbeddedGUI.VisualStudio.props` 后重新生成即可，例如：

```xml
<EguiApp>HelloBasic</EguiApp>
<EguiAppSub>button</EguiAppSub>
<EguiPort>pc</EguiPort>
```

`HelloBasic`、`HelloVirtual`、`HelloGame`、`HelloSizeAnalysis` 在 `EguiAppSub` 留空时会使用各自默认子应用。Visual Studio 调试工作目录为仓库根目录，方便 `file_image`、`deferred_image`、`HelloPerformance` 这类示例读取仓库内的示例文件。

Visual Studio C++ 工程不直接使用 `*.c`、`**/*.h` 或 `$(EguiAppPath)` 这类通配符项目项，避免 IDE 加载时禁用项目缓存并长时间停在“正在加载项目”。`.vcxproj` 中的源码和头文件列表由脚本生成，`.vcxproj.filters` 中的 `Source Files` / `Header Files` 也会按仓库原始目录结构自动生成，例如 `Source Files\src\core`、`Header Files\example\HelloBasic\button`。

新增示例后，如果希望它出现在 Visual Studio 配置下拉框中，需要同步 `porting/pc/` 下的 `.sln` 和 `.vcxproj`。直接运行脚本会自动扫描 `example/` 下所有含 `build.mk` 的 app，以及使用 `APP_SUB` 的 app 下所有含 `app_egui_config.h` 的一级子目录，并刷新 x64 配置和显式工程项列表：

```powershell
python scripts\platform\update_visual_studio_sln.py
```

如果只想同步某个指定示例，也可以显式指定 app 或 sub app。脚本可重复执行，已有配置不会重复插入：

```powershell
# 新增普通示例
python scripts\platform\update_visual_studio_sln.py --app HelloDirty

# 新增带 APP_SUB 的示例
python scripts\platform\update_visual_studio_sln.py --app HelloBasic --app-sub checkbox

# 一次添加多个配置
python scripts\platform\update_visual_studio_sln.py --entry HelloDirty --entry HelloBasic/checkbox
```

如果只是新增、删除或重命名了某个已有源码目录下的 `.c/.h` 文件，不需要手动修改 `.sln`、`.vcxproj` 或 `.vcxproj.filters`，直接运行脚本刷新工程项和目录过滤器即可：

```powershell
python scripts\platform\update_visual_studio_sln.py
```

如果新增的是全新的源码目录，先按 Makefile 规则把目录加入对应 `build.mk`，再运行上面的脚本同步 Visual Studio 工程视图。

发布前检查也会覆盖 Visual Studio 工程同步和轻量编译检查。若只想单独检查这一项，可以运行：

```powershell
python scripts\release_check.py --only visual_studio
```

该步骤使用 `--check` 模式，不会自动改写工程文件；如果失败，先运行 `python scripts\platform\update_visual_studio_sln.py` 更新 `porting/pc/` 下的 `.sln/.vcxproj/.vcxproj.filters`，再重新执行 release check。

CMake 产物默认输出到对应 `build_cmake/<target>/output/` 目录。对于 `stm32g0`、`qemu`、`emscripten` 这类非 CMake 移植层，日常开发仍推荐使用 Makefile。

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

# 多屏专项快速回归
python scripts/code_compile_check.py --scope multi-display --case-jobs 2
python scripts/code_runtime_check.py --scope multi-display --jobs 2 --timeout 10 --keep-screenshots

# 代码格式化
python scripts/code_format.py

# 示例图标字体显式配置检查
python scripts/checks/check_example_icon_font.py

# 如果要把本地未跟踪目录也一起扫描
python scripts/checks/check_example_icon_font.py --include-untracked

# 发布前一键检查
python scripts/release_check.py --skip perf,wasm,doc

# 多屏专项一键回归
python scripts/release_check.py --scope multi-display

# ELF 二进制大小分析
python scripts/size_analysis/main.py

# 资源生成
python scripts/tools/app_resource_generate.py -r example/HelloSimple/resource -o output
```

说明：

- `scripts/checks/check_example_icon_font.py` 默认只扫描 git 已跟踪的 `example/` 源文件，避免本地临时目录影响 CI 风格检查
- `code_compile_check.py --full-check` 默认会包含这一步；如果你在外层已经单独跑过 icon font 检查，可用 `--skip-icon-font-check` 避免重复执行
- 修改多屏入口、descriptor、线程模型或副屏录制链路后，优先补跑 `--scope multi-display`
- `release_check.py --scope multi-display` 会把多屏 compile/runtime scope 和文档校验串成一条命令，适合本地快速收口

## 下一步

- [项目概述](overview.md): 了解 EGUI 的设计理念和核心特点
- [第一个应用](first_app.md): 从代码层面理解 EGUI 应用结构
- [项目目录结构](project_structure.md): 了解源代码的组织方式
