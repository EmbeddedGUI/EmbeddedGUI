# 项目概述与特点

## 一句话了解 EmbeddedGUI

EmbeddedGUI (EGUI) 是一个面向资源受限嵌入式系统的轻量级 C 语言 GUI 框架，主打 **PFB 局部帧缓冲、脏矩形刷新、纯 C99、无 FPU 依赖**，适合 RAM / ROM 都非常紧张的 MCU 场景。

项目仓库地址：

- Gitee: [EmbeddedGUI (gitee.com)](https://gitee.com/embeddedgui/EmbeddedGUI)
- GitHub: [EmbeddedGUI (github.com)](https://github.com/EmbeddedGUI/EmbeddedGUI)

项目采用 **MIT 许可证**，可自由用于商业和非商业项目。

## 先看什么

如果你是第一次接触这个项目，建议按下面顺序阅读：

1. [环境搭建](environment_setup.md)
2. [第一个应用](first_app.md)
3. [项目目录结构](project_structure.md)
4. [构建系统详解](build_system.md)
5. [架构原理总览](../architecture/index.rst)
6. [控件参考总览](../widgets/index.rst)

## 为什么选 EmbeddedGUI

EGUI 的核心目标，是在很小的资源预算里仍然保持完整的 GUI 能力。

### 局部帧缓冲 (PFB)

传统 GUI 往往依赖整屏帧缓冲，这对许多 MCU 来说代价太高。EGUI 采用局部帧缓冲方案，只使用一小块缓冲区分块渲染整个屏幕，从而显著降低 RAM 占用。

### 脏矩形机制

框架只重绘发生变化的区域，而不是整屏刷新。这能减少 CPU 负载、降低功耗，并让静态界面几乎没有额外开销。

### 定点数运算

所有核心数学计算都使用定点数实现，避免浮点依赖，使框架能在没有 FPU 的芯片上稳定运行。

### 类 Android API

EGUI 提供 Activity、ViewPage、LinearLayout、Dialog、Toast 等熟悉的 API 组织方式，降低从 Android 或现代 UI 框架迁移过来的学习成本。

## 核心特点

- **资源占用低**：适合小 RAM、小 ROM 的 MCU
- **纯 C 实现**：C99 风格，易集成、易移植
- **PFB + 脏矩形**：适合局部刷新屏幕与低功耗场景
- **控件丰富**：按钮、输入、布局、图表、时间、列表、虚拟控件等
- **动画完整**：支持常见动画类型和插值器
- **主题系统**：统一管理视觉风格
- **多平台支持**：PC、QEMU、STM32、WebAssembly、Designer 预览端口
- **配套工具链**：资源生成、编译检查、运行验证、性能分析、体积分析

## 适合谁

- 需要在资源受限 MCU 上实现图形界面的工程师
- 需要兼顾“可用性”和“资源占用”的产品团队
- 需要 PC 预览、自动验证和多平台移植能力的嵌入式项目
- 需要快速搭建原型，同时后续可落到量产平台的场景

## 不太适合谁

- 需要非常复杂的桌面级图形效果和大规模资源管理的项目
- 完全不在意内存占用、也不需要嵌入式落地的通用桌面应用
- 需要高度依赖 GPU / 浮点 / 大内存的场景

## 支持的平台

EGUI 采用分层架构，应用代码与平台无关，主要由 Porting 层适配底层能力：

- **PC 模拟器**：Windows / Linux / macOS，适合日常开发和调试
- **PC Test**：无 SDL 的测试端口，适合单元测试和 CI
- **QEMU**：适合自动化测试和性能基准
- **STM32G0**：裸机移植参考
- **WebAssembly**：浏览器在线体验
- **Designer Port**：设计器预览端口

## 快速体验

如果你已经配置好了开发环境，可以直接编译最小示例：

```bash
make all APP=HelloSimple
make run
```

想快速看几个典型能力，可以再试：

```bash
make all APP=HelloBasic APP_SUB=button
make all APP=HelloBasic APP_SUB=slider
make all APP=HelloChart
```

## 与其他框架的定位

EGUI 介于“极简绘图层”与“功能完整大框架”之间：

- 比极简绘图层更完整，提供控件、布局、动画和应用组织能力
- 比资源更重的大型框架更轻，适合小 RAM / 小 ROM 的 MCU

> 具体选型仍建议根据目标芯片、屏幕大小和项目复杂度来决定。

## 下一步

- [环境搭建](environment_setup.md) - 配置开发环境
- [第一个应用](first_app.md) - 理解最小示例
- [项目目录结构](project_structure.md) - 了解仓库组织
- [架构原理总览](../architecture/index.rst) - 了解设计思路
- [控件参考总览](../widgets/index.rst) - 查看控件能力
