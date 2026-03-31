# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

EmbeddedGUI 是一个轻量级的 C 语言 GUI 框架，专为资源受限的嵌入式系统设计（RAM <8KB，ROM <64KB，CPU ~100MHz，无需浮点运算单元）。采用局部帧缓冲（PFB）设计，在最小化内存使用的同时支持触控、动画和类 Android 的 UI 模式。

## 构建命令

```bash
# 构建并运行（PC 模拟器）
make all APP=HelloSimple
make run

# 指定平台构建
make all APP=HelloActivity PORT=pc
make all APP=HelloSimple PORT=stm32g0

# 构建 HelloBasic 并选择子应用
make all APP=HelloBasic APP_SUB=button
make all APP=HelloBasic APP_SUB=anim

# 清理构建产物
make clean

# 生成资源文件
make resource

# 分析二进制大小（生成 output/README.md）
python scripts/utils_analysis_elf_size.py

# 格式化代码
python scripts/code_format.py

# 完整编译检查（CI 使用）
python scripts/code_compile_check.py --full-check

# 使用 CMake 构建
cmake -B build_cmake/HelloSimple -DAPP=HelloSimple -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloSimple -j

# CMake 构建 HelloBasic 子应用
cmake -B build_cmake/HelloBasic_button -DAPP=HelloBasic -DAPP_SUB=button -DPORT=pc -G "MinGW Makefiles"
cmake --build build_cmake/HelloBasic_button -j

# 使用 CMake 做完整编译检查
python scripts/code_compile_check.py --cmake --full-check
```

**可用示例：** HelloActivity, HelloAPP, HelloBasic, HelloCanvas, HelloChart, HelloEasyPage, HelloGradient, HelloLayer, HelloPerformance, HelloPFB, HelloResourceManager, HelloShowcase, HelloSimple, HelloStyleDemo, HelloTest, HelloUnitTest, HelloViewPageAndScroll

**HelloBasic 子应用：** activity_ring, analog_clock, anim, animated_image, arc_slider, button, button_img, button_matrix, card, checkbox, circular_progress_bar, combobox, compass, digital_clock, divider, enhanced_widgets, gauge, gridlayout, heart_rate, image, image_button, label, led, line, linearlayout, list, mask, menu, mini_calendar, mp4, notification_badge, number_picker, page_indicator, progress_bar, radio_button, roller, rotation, scale, scroll, slider, spangroup, spinner, stopwatch, switch, table, tab_bar, textblock, textinput, tileview, toggle_button, viewpage, viewpage_cache, window

## 架构说明

### 目录结构
- `src/` - 核心库（控件、动画、字体、图片、遮罩）
- `example/` - 示例应用
- `porting/` - 平台移植（pc, stm32g0, stm32g0_empty）
- `scripts/` - 构建自动化和资源生成工具

### 面向对象的 C 语言模式
使用基于结构体的"类"和函数指针实现面向对象。基类为 `egui_view_t`，通过组合实现继承：
```c
static egui_view_button_t button;
egui_view_button_init(&button);
egui_view_set_on_click_listener(&button, callback);
```

### OOP 辅助宏（egui_oop.h）

项目提供了一套 OOP 辅助宏，用于简化类型转换，零运行时开销，兼容 C99：

| 宏 | 用途 | 示例 |
|----|------|------|
| `EGUI_LOCAL_INIT(_type)` | self 指针向下转换 | `EGUI_LOCAL_INIT(egui_view_label_t);` |
| `EGUI_VIEW_OF(_ptr)` | 向上转换为 egui_view_t* | `EGUI_VIEW_OF(&local->container)` |
| `EGUI_ANIM_OF(_ptr)` | 向上转换为 egui_animation_t* | `EGUI_ANIM_OF(&local->anim)` |
| `EGUI_VIEW_PARENT(_view)` | 获取父视图指针 | `EGUI_VIEW_PARENT(self)` |
| `EGUI_CAST_TO(_type, _ptr)` | 显式向下转换 | `EGUI_CAST_TO(egui_view_label_t, ptr)` |

**使用示例**：
```c
// 改造前
void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    egui_view_label_t *local = (egui_view_label_t *)self;
    local->text = text;
}

// 改造后
void egui_view_label_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_label_t);
    local->text = text;
}
```

### 核心源码模块
- `src/core/` - 事件循环、画布、定时器、输入处理、配置
- `src/widget/` - UI 控件（view, button, label, scroll, viewpage 等）
- `src/app/` - Activity、Dialog、Toast（类 Android 生命周期）
- `src/anim/` - 动画和 9 种插值器
- `src/utils/` - 定点数运算、链表、环形缓冲区

### 配置系统
每个示例可通过 `app_egui_config.h` 覆盖默认配置：
```c
#define EGUI_CONFIG_SCEEN_WIDTH 240
#define EGUI_CONFIG_SCEEN_HEIGHT 320
#define EGUI_CONFIG_COLOR_DEPTH 16
#define EGUI_CONFIG_PFB_WIDTH (240/8)
#define EGUI_CONFIG_PFB_HEIGHT (320/8)
```

### 构建模块系统
每个组件都有一个 `build.mk` 文件，定义 `EGUI_CODE_SRC` 和 `EGUI_CODE_INCLUDE`。根目录的 Makefile 包含来自 src/、example/ 和 porting/ 的模块。

## 核心设计理念

1. **局部帧缓冲（PFB）**：不使用全屏帧缓冲，而是使用小块缓冲（如 30x40 像素 = RGB565 下 2400 字节）。宽度/高度必须是屏幕尺寸的整数约数。
2. **脏矩形机制**：仅重绘变化的区域，降低功耗。
3. **定点数运算**：所有计算使用定点数（无浮点），实现在 `egui_fixmath.h`。
4. **类 Android API**：Activity 管理页面，ViewPage 实现滑动切页，LinearLayout 进行布局。

## 交互说明

- 交互都用简体中文

- 代码用utf-8英文，代码里面不要出现中文。

- 文档使用utf-8简体中文

## 代码风格

使用基于 LLVM 的代码风格，自定义配置见 `.clang-format`：
- 4 空格缩进，不使用 Tab
- 160 列宽限制
- 函数和控制语句的大括号换行

## 运行与渲染验证

每次修改代码并构建成功后，必须执行运行时验证，确保程序不会卡死、崩溃，且渲染结果正确：

```bash
# 对具体示例进行运行验证
python scripts/code_runtime_check.py --app HelloBasic --app-sub textinput --timeout 10

# 验证所有示例
python scripts/code_runtime_check.py --full-check
```

验证要点：
- 构建成功后必须运行 `code_runtime_check.py` 进行运行时验证
- 检查截图输出（`runtime_check_output/` 目录下的 PNG 文件）确认渲染正确
- 检查文本、图标、中心按钮等关键元素的视觉居中是否准确，左右/上下留白是否平衡；不能只以“没截断”作为通过标准
- 检查文字与按钮/圆形/胶囊等边框之间是否保留合理空隙，不能出现文字贴边、视觉压迫或左右内边距明显失衡
- HelloCustomWidgets 逐轮迭代时，必须在各应用目录下维护 iteration_log/iteration_log.md，并把关键截图复制到 iteration_log/images/ 后使用相对路径引用，作为后续 review 依据
- iteration_log/ 属于本地审阅产物，不纳入 git 提交
- 如果运行时检查失败（卡死、崩溃），必须排查修复后重新验证
- **多页面应用必须验证所有页面渲染**：对于包含多个页面的应用，运行时验证必须覆盖每一个页面，不能只验证首页。代码生成器会自动为多页面项目生成 `egui_port_get_recording_action()` 录制动作，通过 `uicode_switch_page()` 依次切换所有页面。验证时需检查截图确认每个页面都有正确渲染输出
- **性能测试必须使用 QEMU 验证**：PC 模拟器的计时器精度只有 1ms，不适合做性能基准测试。性能数据必须通过 QEMU 运行获取（使用微秒级计时器 `qemu_get_tick_us`）。PC 运行仅用于方便查看渲染效果，不作为性能数据依据

## UI Designer Widget 注册与渐进式扩展

设计转换管道（HTML/JSX → XML → C）依赖 widget 注册系统。当转换新设计遇到不支持的属性或控件时，**必须扩展 widget 注册而非手写 C 代码**。

### 架构

```
scripts/ui_designer/custom_widgets/   ← Widget 注册插件（每控件一个 .py）
scripts/ui_designer/model/widget_registry.py  ← 注册中心
scripts/ui_designer/generator/        ← 代码生成器（读注册信息生成 C）
src/widget/                           ← C 层控件实现（头文件中有实际 API）
```

### Widget 注册文件格式

每个 `custom_widgets/*.py` 描述一个控件的代码生成规则：

```python
WidgetRegistry.instance().register(
    type_name="progress_bar",          # 内部类型名
    descriptor={
        "c_type": "egui_view_progress_bar_t",           # C 结构体类型
        "init_func": "egui_view_progress_bar_init_with_params",  # 初始化函数
        "params_macro": "EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT",   # 参数宏
        "params_type": "egui_view_progress_bar_params_t",        # 参数类型
        "is_container": False,         # 是否是容器控件
        "properties": {
            "value": {
                "type": "int", "default": 50,
                "code_gen": {"kind": "setter", "func": "egui_view_progress_bar_set_process"},
            },
        },
    },
    xml_tag="ProgressBar",             # XML 中的标签名
    display_name="ProgressBar",        # 显示名称
)
```

### code_gen kind 类型

| kind | 说明 | 示例 |
|------|------|------|
| `setter` | 调用 `func(view, value)` | `egui_view_switch_set_checked` |
| `text_setter` | 设置静态字符串 | `egui_view_label_set_text` |
| `multi_setter` | 多参数调用，用 `args` 模板 | `egui_view_label_set_font_color` |
| `derived_setter` | 从资源派生参数 | `egui_view_label_set_font` |

### 遇到不支持属性时的扩展流程

当设计转换过程中遇到构建失败（如 `undefined reference`）或渲染异常，按以下步骤排查扩展：

1. **确认 C 层 API**：在 `src/widget/egui_view_*.h` 中查找实际的函数签名
2. **对比 widget 注册**：检查 `custom_widgets/*.py` 中 `code_gen.func` 是否与 C 层一致
3. **修复或新增属性**：
   - 函数名错误 → 修正 `func` 字段（如 `set_value` → `set_process`）
   - 缺少属性 → 在 `properties` 中新增条目
   - 缺少控件 → 新建 `custom_widgets/xxx.py` 注册文件
4. **重新生成验证**：`generate-code` → `gen-resource` → `verify`

### 新增控件的检查清单

- [ ] `src/widget/` 中存在对应的 C 实现（`egui_view_xxx.h/c`）
- [ ] `custom_widgets/xxx.py` 中 `c_type`、`init_func`、`params_macro` 与 C 头文件一致
- [ ] 所有 `code_gen.func` 函数名在 C 头文件中有声明
- [ ] `xml_tag` 与 `EmbeddedGUI_Designer` 仓库中的 HTML 转换文档示例保持一致
- [ ] 构建通过且运行时验证截图正确

### 已注册控件一览

| XML 标签 | type_name | 注册文件 |
|----------|-----------|----------|
| `Group` | group | group.py |
| `Card` | card | card.py |
| `Label` | label | label.py |
| `Image` | image | image.py |
| `Button` | button | button.py |
| `Switch` | switch | switch.py |
| `ProgressBar` | progress_bar | progress_bar.py |
| `CircularProgressBar` | circular_progress_bar | circular_progress_bar.py |
| `Slider` | slider | slider.py |
| `LinearLayout` | linearlayout | linearlayout.py |
| `GridLayout` | gridlayout | gridlayout.py |
| `Scroll` | scroll | scroll.py |
| `ViewPage` | viewpage | viewpage.py |
| `Checkbox` | checkbox | checkbox.py |
| `RadioButton` | radio_button | radio_button.py |
| `Spinner` | spinner | spinner.py |
| `Led` | led | led.py |
| `ToggleButton` | toggle_button | toggle_button.py |
| `ImageButton` | image_button | image_button.py |
| `Divider` | divider | divider.py |
| `Textblock` | textblock | textblock.py |
| `Textinput` | textinput | textinput.py |
| `DynamicLabel` | dynamic_label | dynamic_label.py |
| `NumberPicker` | number_picker | number_picker.py |
| `ArcSlider` | arc_slider | arc_slider.py |
| `Combobox` | combobox | combobox.py |
| `Roller` | roller | roller.py |
| `Gauge` | gauge | gauge.py |
| `TabBar` | tab_bar | tab_bar.py |
| `PageIndicator` | page_indicator | page_indicator.py |
| `Keyboard` | keyboard | keyboard.py |
| `Mp4` | mp4 | mp4.py |
| `ChartLine` | chart_line | chart_line.py |
| `ChartScatter` | chart_scatter | chart_scatter.py |
| `ChartBar` | chart_bar | chart_bar.py |
| `ChartPie` | chart_pie | chart_pie.py |
| `AnalogClock` | analog_clock | analog_clock.py |
| `DigitalClock` | digital_clock | digital_clock.py |
| `Stopwatch` | stopwatch | stopwatch.py |
| `ActivityRing` | activity_ring | activity_ring.py |
| `HeartRate` | heart_rate | heart_rate.py |
| `Compass` | compass | compass.py |
| `NotificationBadge` | notification_badge | notification_badge.py |
| `MiniCalendar` | mini_calendar | mini_calendar.py |
| `Line` | line | line.py |
| `Scale` | scale | scale.py |
| `ButtonMatrix` | button_matrix | button_matrix.py |
| `Table` | table | table.py |
| `AnimatedImage` | animated_image | animated_image.py |
| `List` | list | list.py |
| `Spangroup` | spangroup | spangroup.py |
| `TileView` | tileview | tileview.py |
| `ViewPageCache` | viewpage_cache | viewpage_cache.py |
| `Window` | window | window.py |
| `Menu` | menu | menu.py |

## Figma/HTML 设计转换迁移说明

`html2egui_helper.py`、Figma Make 工具链以及围绕 Figma/HTML 的设计转换与动效工作流，已迁移到独立仓库 `EmbeddedGUI_Designer` 维护。

当前仓库仅保留 SDK、运行时、示例、控件与通用验证能力。凡是涉及以下场景，优先切换到 `EmbeddedGUI_Designer`：

- Stitch HTML / Figma Make / Figma MCP 导入
- XML 布局生成与 Designer 工程搭建
- 基于 TSX 源码补齐动效与交互
- 设计稿参考帧、像素回归与视觉对比

## 资源生成（图片 & 字体）

为应用添加图片、字体、图标时，必须通过资源生成管线，**不要直接把 `.c` 文件放到 `src/resource/`**。详见 `.claude/skills/resource-generation.md`。

核心要点：
- 资源放在 `example/{APP}/resource/src/`，配置文件为 `app_resource_config.json`
- 公用字体库位于 `scripts/tools/build_in/`，直接从那里复制到 `resource/src/`
- **新增中文或图标字符后**，运行 `python scripts/tools/extract_font_text.py --app {APP}` 自动提取字符串到 text 文件（每行一个字符串，按字体分文件）
- 运行 `make resource_refresh APP={APP}` 生成 C 源文件
- 代码中通过 `#include "app_egui_resource_generate.h"` 引用资源

## GitHub Pages 在线 Demo 站点

在线 Demo 站点以 `web/` 目录为根，通过 GitHub Actions 自动构建 WASM 并部署。

添加 demo、新建 Web 页面、修改布局等操作，详见 `.claude/skills/github-pages-web.md`。

关键约束：
- **WASM 构建强制禁用录制模式**（`EGUI_CONFIG_RECORDING_TEST=0`），用户直接与 canvas 交互
- 所有页面内容由 `web/demos/demos.json` 驱动，**不要手动编辑 `index.html` / `basic.html` 的 demo 列表**
- 响应式布局通过 CSS 变量令牌（`--content-max-width` 等）统一控制，新增页面直接套用已有 class

## Buffer 分配约束

涉及以下任一维度变化的 buffer：
- 字体大小
- 图片大小
- 屏幕尺寸
- `PFB` 尺寸

必须使用 `heap` 动态申请与释放/复用，不能为了维持 `heap=0` 而改成宏固定大小、静态全局/静态局部，或超大的栈上局部数组。

换句话说：
- **尺寸相关 buffer 禁止用“宏展开 + 固定 RAM 占用”代替 heap**
- **尺寸相关 buffer 禁止用“大栈数组”代替 heap**
- 做 RAM 优化时，只有与上述尺寸无关、且上界稳定明确的小型 scratch/cache，才允许继续评估是否放在栈或静态区

## Skills 技能文件说明

项目技能文件位于 `.claude/skills/`，**在当前 VS Code + Copilot 环境下不会自动触发**——需要 AI 根据任务类型主动阅读对应文件。下表列出所有技能及触发时机：

| 技能文件 | 触发时机 |
|----------|----------|
| `resource-generation.md` | 添加图片/字体/图标；资源构建报 undefined reference；需要配置 `app_resource_config.json` |
| `runtime-verification.md` | 修改代码后验证；截图检查渲染；诊断黑屏/控件缺失/布局错位 |
| `html-to-egui.md` | 已迁移到 `EmbeddedGUI_Designer`，用于 HTML / JSX / TSX 设计转换 |
| `figmamake-to-egui.md` | 已迁移到 `EmbeddedGUI_Designer`，用于 Figma Make 全流程转换 |
| `figma-mcp-to-egui.md` | 已迁移到 `EmbeddedGUI_Designer`，用于 Figma MCP / REST 导入 |
| `dynamic-effects.md` | 已迁移到 `EmbeddedGUI_Designer`，用于设计稿驱动的动效补齐 |
| `performance-analysis.md` | 性能测试、帧率分析、QEMU 基准 |
| `recording-simulation.md` | 配置录制动作用于 CI 运行时验证 |
| `build-and-debug.md` | 构建失败排查、链接错误、平台移植问题 |
| `github-pages-web.md` | 添加/修改 GitHub Pages Demo 页面 |

**使用方式**：遇到上述场景时，先判断任务是否属于设计稿转换链路；如果属于，切换到 `EmbeddedGUI_Designer` 并阅读那里的迁移说明执行。

## Plan说明

每次进行Plan完成时，需要将Plan整理成文档，放入.claude路径下，文档按照Plan简写来，用中文utf-8编码。

