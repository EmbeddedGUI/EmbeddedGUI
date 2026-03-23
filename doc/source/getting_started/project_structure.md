# 项目目录结构

本文介绍 EmbeddedGUI 项目的目录组织方式，帮助你快速定位代码和资源文件。

## 顶层目录总览

```
EmbeddedGUI/
|-- src/                    # 核心库源代码
|-- example/                # 示例应用
|-- porting/                # 平台移植层
|-- scripts/                # 构建工具和辅助脚本
|-- doc/                    # Sphinx 文档源码
|-- web/                    # WebAssembly 在线演示
|-- tools/                  # 内置开发工具 (w64devkit 等)
|-- build/                  # CMake 构建目录
|-- output/                 # 编译产物输出目录
|-- Makefile                # 顶层 Makefile (配置 APP/PORT 等参数)
|-- CMakeLists.txt          # CMake 构建配置
|-- setup.bat               # Windows 环境入口脚本（转发到 setup_env.py）
|-- setup.sh                # Linux / macOS 环境入口脚本
|-- requirements.txt        # Python 依赖列表
|-- VERSION                 # 版本号文件
|-- LICENSE                 # MIT 许可证
`-- README.md               # 项目说明
```

## src/ -- 核心库源代码

这是 EGUI 框架的核心实现，所有模块均以 `egui_` 为前缀命名。

```
src/
|-- egui.h                  # 统一头文件，包含所有模块
|-- build.mk                # 源码构建模块定义
|-- core/                   # 核心框架
|-- widget/                 # UI 控件
|-- anim/                   # 动画系统
|-- font/                   # 字体渲染
|-- image/                  # 图片处理
|-- mask/                   # 遮罩支持
|-- style/                  # 样式系统
|-- shadow/                 # 阴影效果
|-- background/             # 背景绘制
|-- app/                    # 应用框架 (Activity/Dialog/Toast)
|-- resource/               # 内置资源
|-- utils/                  # 工具函数
`-- test/                   # 测试代码
```

### core/ -- 核心框架

核心模块提供了 EGUI 框架的运行基础:

| 文件 | 说明 |
|------|------|
| `egui_core.c/h` | 框架主循环，驱动渲染和事件处理 |
| `egui_canvas.c/h` | 画布绘制引擎，支持基本图形、抗锯齿等 |
| `egui_canvas_circle_hq.c` | 高质量抗锯齿圆形渲染 |
| `egui_canvas_line.c/line_hq.c` | 直线和高质量抗锯齿直线 |
| `egui_canvas_bezier.c` | 贝塞尔曲线渲染 |
| `egui_canvas_gradient.c/h` | 渐变色渲染 |
| `egui_canvas_polygon.c` | 多边形渲染 |
| `egui_canvas_triangle.c` | 三角形渲染 |
| `egui_canvas_ellipse.c` | 椭圆渲染 |
| `egui_pfb_manager.c/h` | PFB (局部帧缓冲) 管理器 |
| `egui_timer.c/h` | 定时器系统 |
| `egui_input.c/h` | 输入事件处理 |
| `egui_motion_event.c/h` | 触摸/运动事件 |
| `egui_key_event.c/h` | 按键事件 |
| `egui_touch_driver.c/h` | 触摸驱动接口 |
| `egui_display_driver.c/h` | 显示驱动接口 |
| `egui_theme.c/h` | 主题系统 |
| `egui_config.h` | 编译期配置 |
| `egui_config_default.h` | 默认配置值 |
| `egui_region.c/h` | 区域计算 (脏矩形) |
| `egui_scroller.c/h` | 滚动物理引擎 |
| `egui_velocity_tracker.c/h` | 速度追踪器 (惯性滑动) |
| `egui_focus.c/h` | 焦点管理 |
| `egui_rotation.c/h` | 屏幕旋转支持 |
| `egui_oop.h` | OOP 辅助宏 (类型转换) |
| `egui_typedef.h` | 基础类型定义 |
| `egui_api.c/h` | 通用 API 函数 |

### widget/ -- UI 控件

包含 60+ UI 控件的实现，每个控件由一对 `.c/.h` 文件组成。主要控件分类:

**基础控件**: label (标签), button (按钮), image (图片), image_button (图片按钮)

**输入控件**: switch (开关), checkbox (复选框), radio_button (单选按钮), slider (滑块), arc_slider (弧形滑块), toggle_button (切换按钮), combobox (下拉框), roller (滚轮选择), number_picker (数字选择器), spinner (旋转选择), textblock (文本块)

**布局控件**: linearlayout (线性布局), gridlayout (网格布局), scroll (滚动视图), viewpage (分页视图), tileview (瓦片视图), list (列表)

**进度控件**: progress_bar (进度条), circular_progress_bar (圆形进度条), activity_ring (活动环)

**图表控件**: chart_line (折线图), chart_bar (柱状图), chart_pie (饼图), chart_scatter (散点图)

**时钟控件**: analog_clock (模拟时钟), digital_clock (数字时钟), stopwatch (秒表), mini_calendar (迷你日历)

**其他控件**: card (卡片), divider (分割线), page_indicator (页面指示器), notification_badge (通知徽章), led (LED 指示灯), gauge (仪表盘), compass (罗盘), heart_rate (心率), scale (刻度), button_matrix (按钮矩阵), table (表格), animated_image (动画图片), spangroup (富文本), window (窗口), menu (菜单), line (线条), keyboard (键盘), mp4 (视频)

### anim/ -- 动画系统

提供类 Android 的动画框架:

| 文件 | 说明 |
|------|------|
| `egui_animation.c/h` | 动画基类 |
| `egui_animation_translate.c/h` | 平移动画 |
| `egui_animation_alpha.c/h` | 透明度动画 |
| `egui_animation_scale_size.c/h` | 缩放动画 |
| `egui_animation_resize.c/h` | 尺寸变化动画 |
| `egui_animation_color.c/h` | 颜色渐变动画 |
| `egui_animation_set.c/h` | 动画组合 (顺序/并行) |
| `egui_interpolator.c/h` | 插值器基类 |

支持 9 种插值器: linear (线性), accelerate (加速), decelerate (减速), accelerate_decelerate (先加速后减速), anticipate (回弹启动), overshoot (过冲), anticipate_overshoot (回弹过冲), bounce (弹跳), cycle (循环)。

### font/ -- 字体渲染

支持点阵字体和标准字体渲染，提供 UTF-8 编码支持。

### image/ -- 图片处理

图片解码和渲染，支持带 Alpha 通道的图片。

### mask/ -- 遮罩支持

提供圆形遮罩、圆角矩形遮罩、渐变遮罩和图片遮罩，用于实现圆角图片、异形裁剪等效果。

### utils/ -- 工具函数

| 文件 | 说明 |
|------|------|
| `egui_fixmath.c/h` | 定点数运算 (三角函数、平方根等) |
| `egui_dlist.h` | 双向链表 |
| `egui_slist.h` | 单向链表 |
| `egui_utils.c/h` | 通用工具函数 |
| `simple_ringbuffer/` | 环形缓冲区实现 |

## example/ -- 示例应用

每个示例位于独立子目录中，包含 `uicode.c` (UI 代码)、`build.mk` (构建配置) 和可选的 `resource/` (资源文件) 目录。

| 示例 | 说明 |
|------|------|
| `HelloSimple` | 最简单的入门示例 (标签 + 按钮) |
| `HelloBasic` | 控件演示集合，包含 50+ 子应用 |
| `HelloActivity` | Activity 生命周期和页面管理 |
| `HelloAPP` | 多 Activity 应用示例 |
| `HelloStyleDemo` | 样式和主题演示 |
| `HelloChart` | 图表控件演示 |
| `HelloBattery` | 电池监控界面 |
| `HelloPFB` | PFB 机制演示 |
| `HelloEasyPage` | 简易分页应用 |
| `HelloViewPageAndScroll` | 分页和滚动组合 |
| `HelloPerformance` | 性能测试基准 |
| `HelloResourceManager` | 资源管理器示例 |
| `HelloCanvas` | 画布绘图 API 演示 |
| `HelloGradient` | 渐变效果演示 |
| `HelloTest` | 功能测试 |
| `HelloUnitTest` | 单元测试 |

其中 `HelloBasic` 是最丰富的示例，通过 `APP_SUB` 参数选择不同的子应用来演示各种控件。

## porting/ -- 平台移植层

每个平台对应一个子目录，实现平台相关的显示驱动、触摸驱动和定时器:

| 平台 | 说明 |
|------|------|
| `pc/` | PC 模拟器 (基于 SDL2，支持 Windows/Linux/macOS) |
| `stm32g0/` | STM32G0 系列 MCU (ARM Cortex-M0+) |
| `stm32g0_empty/` | STM32G0 空模板 (最小移植参考) |
| `qemu/` | QEMU ARM 虚拟化环境 (自动化测试和性能基准) |
| `emscripten/` | WebAssembly 编译 (浏览器在线演示) |
| `designer/` | UI Designer 专用移植 |
| `pc_test/` | PC 测试专用移植 |

## scripts/ -- 构建工具和辅助脚本

| 脚本 | 说明 |
|------|------|
| `code_compile_check.py` | 完整编译检查 (CI 使用，含示例 icon font 检查) |
| `code_runtime_check.py` | 运行时验证 (截图对比) |
| `code_format.py` | 代码格式化 |
| `check_example_icon_font.py` | 示例图标字体显式配置检查 |
| `code_perf_check.py` | 性能测试 |
| `release_check.py` | 发布前多步骤一键检查 |
| `setup_env.py` | 跨平台环境配置主脚本，负责 Python 依赖和工具链检查 |
| `utils_analysis_elf_size.py` | ELF 二进制大小分析 |
| `wasm_build_demos.py` | WebAssembly 演示构建 |
| `gif_recorder.py` | GIF 录制工具 |
| `ui_designer/` | UI 可视化设计器 (基于 PyQt5) |
| `tools/` | 资源生成工具 |

## doc/ -- 文档

基于 Sphinx 构建的项目文档:

```
doc/source/
|-- index.rst               # 文档首页
|-- introduction/           # 项目介绍
|-- getting_started/        # 快速入门 (本系列文档)
|-- architecture/           # 架构说明
|-- widgets/                # 控件参考
|-- animation/              # 动画系统
|-- resource/               # 资源管理
|-- porting/                # 移植指南
|-- performance/            # 性能优化
|-- ui_designer/            # UI 设计器
`-- conf.py                 # Sphinx 配置
```

## web/ -- WebAssembly 在线演示

通过 Emscripten 将 EGUI 示例编译为 WebAssembly，可在浏览器中直接体验:

```
web/
|-- index.html              # 演示首页
|-- demos/                  # 各示例的 WASM 编译产物
|-- lib/                    # JavaScript 依赖库
`-- style.css               # 页面样式
```

## 下一步

- [构建系统详解](build_system.md): 深入了解 Makefile 和 build.mk 的工作原理
- [第一个应用](first_app.md): 从代码层面理解 EGUI 应用结构
