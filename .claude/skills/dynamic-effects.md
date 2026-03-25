---
name: dynamic-effects
description: Figma-driven dynamic effects guidance has moved to EmbeddedGUI_Designer
---

# 动态特效流程已迁移

基于 Figma Make TSX 源码补齐动态特效和交互的流程，已经迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 适用范围

以下任务应在 `EmbeddedGUI_Designer` 中处理：

- 根据 TSX 动效定义补齐页面动画
- 将 Figma Make 的 `motion` / `animate` / `transition` 映射到 EGUI
- 结合设计稿回归结果修正交互与动态行为

当前仓库仍然提供底层能力，例如定时器、动画 API、控件和页面运行时；但面向设计稿的动效工作流不再在这里维护。
