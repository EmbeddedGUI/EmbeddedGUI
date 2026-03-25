---
name: figmamake-to-egui
description: Figma Make conversion flow has moved to EmbeddedGUI_Designer
---

# Figma Make 转换流程已迁移

`figmamake/` 目录下的抓图、解析、回归验证和统一入口脚本，已经迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的入口

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python figmamake/figmamake2egui.py --project-dir figma_make_project --app MyApp
python figmamake/figmamake_capture.py --tsx-dir figma_make_project --output-dir reference_frames
python figmamake/figmamake_regression.py --reference-dir reference_frames --rendered-dir runtime_check_output
```

## 当前仓库的职责

当前仓库只提供被 Designer 复用的 SDK 能力，例如：

- `src/` 下的运行时与控件实现
- `example/` 下的示例工程结构
- `scripts/code_runtime_check.py` 等通用验证基础能力

如果任务涉及 Figma Make 设计稿转换、参考帧采集、像素回归或动效对齐，请切换到 `EmbeddedGUI_Designer`。
