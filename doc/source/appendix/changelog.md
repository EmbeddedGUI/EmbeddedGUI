# 版本变更记录

本文档记录 EmbeddedGUI 项目的主要变更历史，格式参考 [Keep a Changelog](https://keepachangelog.com/)。

---

## [Unreleased]

### Added
- 文档体系重构：13 章目录结构，65 篇文档覆盖快速入门、架构原理、控件参考、动画系统、性能测试、资源管理、移植指南、附录
- Figma Make 到 EGUI 全自动转换管道（HTML/JSX -> XML -> C 代码生成）
- UI Designer Widget 注册系统，支持 50+ 控件的自动代码生成
- `build-and-debug`、`resource-generation` 技能，增强运行时验证渲染排错
- HelloStyleDemo 全页面动态效果增强（Timer/Animation 驱动）

### Changed
- 梳理 Figma 转 EGUI 的规则和脚本
- 优化 Activity 切换行为动效

---

## [Style System] - 2025

### Added
- ROM-First 风格/主题系统核心实现（`egui_style.h/c`）
- 风格过渡动画：颜色、透明度、尺寸插值
- 主题风格系统集成到 6 个核心控件
- HelloStyleDemo 示例：4 个主题演示页面（Dashboard、Settings、Charts、Widgets）
- 字体资源增强：度数符号与 Material 图标字体支持

### Fixed
- HelloStyleDemo 视觉问题修复（两轮迭代）
- 提升演示页面可读性：增大标题和关键标签字体

---

## [Chart Refactor] - 2025

### Added
- `egui_view_chart_axis` 基类：统一坐标轴管理
- 柱状图 X 轴分类轴模式，标签居中于柱组下方
- Scatter chart、Bar chart、Line chart 继承 axis base

### Fixed
- Chart 数据裁剪区域修正
- Velocity tracker `memmove` 修复
- Pie chart 角度累积舍入误差（改用累积比例）
- Bar chart 宽度下溢和零值柱修复
- Scatter chart 裁剪边距修复
- `INT16_MIN` 未定义行为和 tick 循环溢出修复

### Changed
- UI Designer widget 注册更新为使用 axis base setter

---

## [Visual Enhancement] - 2025

### Added
- `EGUI_CONFIG_WIDGET_ENHANCED_DRAW` 渐变绘制模式
- 控件视觉增强 Phase 1-5：覆盖 button、switch、slider、progress_bar、checkbox、card、radio_button、toggle_button、led、notification_badge、page_indicator、button_matrix、combobox、textinput、window、menu、mini_calendar、circular_progress_bar、arc_slider、gauge、activity_ring、spinner、chart_pie、chart_bar、chart_scatter、chart_line、analog_clock、compass、roller、tab_bar、textblock、image_button、divider
- Dithering 防色带 + 3-stop 高光渐变 + 边框描边 + 阴影效果
- 控件尺寸统一：XS/S/M/L 四档 + GridLayout 2 列布局

### Fixed
- 渐变圆角矩形 radius 裁剪过度导致 pill 形状缺口
- 圆角矩形渐变填充 HQ 子像素 AA 重构
- Switch 圆点垂直居中修正
- 渐变弧 AA 对称化 + 距离场平滑弧终点
- 渐变弧环 AA 边界修复 + 圆帽渲染 API
- Activity ring 两端圆帽渲染优化

---

## [Animation & Performance] - 2025

### Added
- 完善动画系统：HelloBasic/anim 增强为 4 种动画类型展示
- HelloPerformance 新增动画性能测试
- `performance-analysis` 技能：整理 QEMU 性能分析工作流
- 表盘控件（Gauge/AnalogClock 等）

### Changed
- 性能优化：编译时间优化、全量测试加速
- 优化 checkbox 和 slider 控件

---

## [Build & Tooling] - 2025

### Added
- 开发环境一键配置 `setup.bat`：自动下载 w64devkit 工具链
- 内置 `make.exe`，Python 依赖自动安装
- Web Demo 文档展示功能：marked.js 渲染 markdown，iframe 布局
- WASM 构建支持：per-app OBJDIR 增量编译

### Changed
- 构建优化：BITS 自动检测 + 跳过无资源目录的 resource 生成
- 编译检查脚本加速：禁用 debug 符号，显式指定 `-O0`
- WASM 构建加速：HelloBasic 子应用核心库只编译 1 次
- 简化 key input 系统：移除 key_driver/long press/repeat/combo，平台直接传入事件

### Fixed
- 修复 mp4/gradient 示例 emscripten 编译
- 修复单元测试 + 消除编译警告
- 修复 image resize 路径的 mask_get_row_range 优化

---

## [Rendering Optimization] - 2025

### Added
- Mask 渲染优化：新增 `mask_get_row_range` 行级查询，矩形填充场景提速 4.4x
- PFB 双缓冲管理器（ring queue 模式）
- DMA 异步刷新支持（`egui_pfb_notify_flush_complete`）
- 高质量抗锯齿绘图 API（`_hq` 系列函数）
- 贝塞尔曲线绘制（二次和三次）
- 三角形、椭圆、多边形绘制
- 圆帽线条和折线绘制

### Changed
- 画布 API 扩展：支持更多图形基元
- 定点数运算优化

---

## [Core Features] - 2024

### Added
- 外部资源加载：Image 支持从外部 Flash 加载模式
- `app_resource_generate.py` 资源管理工具
- HelloResourceManager 示例
- `img2c.py` 支持 ext bin 存储功能
- Font 管理系统
- `egui_view_viewpage_cache` 缓存翻页控件
- PFB 支持非整数倍屏幕尺寸
- 中断保护机制（Timer 等多线程场景）
- 64 位系统 SDL 编译支持

### Fixed
- ViewPage cache 指针管理问题
- List 与其他命名冲突
- STM32G0 编译问题
- 64 位系统编译报错
- PFB 无重叠区域进入 image 的问题
- Memory copy 相关问题

### Changed
- 编译结构调整，方便移植
- 资源文件移入 `src/` 目录
- 默认例程适应任意屏幕尺寸

---

## [Initial Release] - 2024

### Added
- EmbeddedGUI 核心框架：PFB（局部帧缓冲）渲染引擎
- 基础控件：View、Button、Label、Image、Switch、ProgressBar、Scroll、ViewPage
- 类 Android Activity 生命周期管理
- Dialog 和 Toast 支持
- 动画系统：9 种插值器
- 定点数运算库（无浮点依赖）
- 脏矩形刷新机制
- 触摸输入处理
- PC 模拟器（SDL）和 STM32G0 移植
- 示例应用：HelloSimple、HelloActivity、HelloBasic 等
- ReadTheDocs 文档框架
