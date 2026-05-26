# 项目总索引

本文档用于快速定位 EmbeddedGUI 仓库中的核心入口，帮助新用户、维护者和贡献者更快找到合适的文档、示例和源码。

## 先从哪里开始

如果你是第一次接触本项目，建议按下面顺序阅读：

1. `getting_started/index` - 环境搭建、首个示例、项目结构
2. `README.md` - 项目定位、核心特性、快速开始
3. `architecture/index` - 了解框架设计思路
4. `widgets/index` - 查看控件能力与用法
5. `performance/index` - 性能测试与分析
6. `size/index` - 体积分析与资源占用

## 按目标查找

### 想快速跑起来

- 文档：[`getting_started/overview`](../getting_started/overview.md)
- 环境：[`getting_started/environment_setup`](../getting_started/environment_setup.md)
- 首个应用：[`getting_started/first_app`](../getting_started/first_app.md)
- 构建系统：[`getting_started/build_system`](../getting_started/build_system.md)

### 想看框架架构

- PFB：[`architecture/pfb_design`](../architecture/pfb_design.md)
- 脏矩形：[`architecture/dirty_rect`](../architecture/dirty_rect.md)
- 视图系统：[`architecture/view_system`](../architecture/view_system.md)
- 布局系统：[`architecture/layout_system`](../architecture/layout_system.md)
- 事件系统：[`architecture/event_system`](../architecture/event_system.md)
- Activity / Page / Virtual Stage：[`architecture/virtual_stage`](../architecture/virtual_stage.md)

### 想看控件

- 基础控件：[`widgets/basic_widgets`](../widgets/basic_widgets.md)
- 输入控件：[`widgets/input_widgets`](../widgets/input_widgets.md)
- 容器控件：[`widgets/container_widgets`](../widgets/container_widgets.md)
- 滚动控件：[`widgets/scroll_widgets`](../widgets/scroll_widgets.md)
- 进度控件：[`widgets/progress_widgets`](../widgets/progress_widgets.md)
- 图表控件：[`widgets/chart_widgets`](../widgets/chart_widgets.md)
- 媒体控件：[`widgets/media_widgets`](../widgets/media_widgets.md)
- 虚拟控件：[`widgets/virtual_widgets`](../widgets/virtual_widgets.md)

### 想看应用场景

- `example/HelloSimple` - 最小运行示例
- `example/HelloBasic` - 基础控件合集
- `example/HelloActivity` - Activity 生命周期
- `example/HelloAPP` - 多页面应用
- `example/HelloVirtual` - Virtual 控件
- `example/HelloShowcase` - 全量展示
- `example/HelloPerformance` - 性能分析
- `example/HelloSizeAnalysis` - 体积分析
- `example/HelloSVGSpec` - SVG 规范验证
- `example/HelloUnitTest` - 单元测试

### 想看资源系统

- 概览：[`resource/index`](../resource/index.rst)
- 资源生成：[`resource/resource_generate_introduction`](../resource/resource_generate_introduction.md)
- 字体生成：[`resource/font_generate`](../resource/font_generate.md)
- 图片生成：[`resource/image_generate`](../resource/image_generate.md)
- 资源优化：[`resource/optimization_tips`](../resource/optimization_tips.md)

### 想看性能与体积

- 性能概览：[`performance/perf_overview`](../performance/perf_overview.md)
- 性能调优：[`performance/perf_tuning`](../performance/perf_tuning.md)
- 体积概览：[`size/size_overview`](../size/size_overview.md)
- 体积选型：[`size/size_selection_guide`](../size/size_selection_guide.md)
- 低内存配置：[`architecture/core_ram_guide`](../architecture/core_ram_guide.md)

### 想看移植与调试

- 移植总览：[`porting/index`](../porting/index.rst)
- PC 移植：[`porting/porting_pc`](../porting/porting_pc.md)
- STM32 移植：[`porting/porting_stm32`](../porting/porting_stm32.md)
- 自定义移植：[`porting/porting_custom`](../porting/porting_custom.md)
- 调试总览：[`debug/index`](../debug/index.rst)
- 调试脚本：[`debug/debug_scripts`](../debug/debug_scripts.md)

### 想看 UI Designer

- 入口：[`ui_designer/index`](../ui_designer/index.rst)
- 说明：Designer 已迁移到独立仓库，当前仓库保留运行时、示例与预览端口

## 按源码模块查找

### 核心运行时

- `src/core/` - 事件循环、输入、定时器、焦点、显示管理
- `src/widget/` - 视图控件实现
- `src/canvas/` - 图形绘制
- `src/anim/` - 动画与插值器
- `src/app/` - Activity、Dialog、Toast
- `src/image/` - 图片解码与图片对象
- `src/resource/` - 内置字体与资源
- `src/style/` - 主题与样式
- `src/mask/` - 遮罩实现
- `src/background/` - 背景绘制
- `src/font/` - 字体系统
- `src/utils/` - 工具类与基础数据结构

### 平台移植

- `porting/pc/` - PC SDL 模拟器
- `porting/pc_test/` - 无 SDL 的测试端口
- `porting/qemu/` - QEMU 端口
- `porting/emscripten/` - WebAssembly 端口
- `porting/stm32g0/` - STM32G0 示例移植
- `porting/designer/` - Designer 预览端口

## 按目标查找脚本

- 构建检查：`scripts/code_compile_check.py`
- 运行验证：`scripts/code_runtime_check.py`
- 体积分析：`scripts/size_analysis/main.py`
- 性能分析：`scripts/perf_analysis/main.py`
- 资源生成：`scripts/setup_resvg.py`
- Web 构建：`scripts/web/`
- 录制与验证：`scripts/recording/`

## 建议维护方式

如果你新增了文档、示例或 API，请同步更新本页的对应入口，让“看文档、找示例、查源码”始终保持一致。
