---
name: figma-mcp-to-egui
description: Figma MCP import flow has moved to EmbeddedGUI_Designer
---

# Figma MCP 导入流程已迁移

Figma MCP / REST 导入、设计稿转 XML 以及相关对比验证，已经迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的入口

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python html2egui_helper.py scaffold --app MyApp --width 320 --height 480
python html2egui_helper.py figma-mcp --input figma_node.json --app MyApp --page main_page
python html2egui_helper.py generate-code --app MyApp
python html2egui_helper.py gen-resource --app MyApp
python html2egui_helper.py verify --app MyApp
```

如果需要 Figma 视觉对比：

```bash
python figmamake/figma_visual_compare.py --design design.png --rendered frame_0000.png --output comparison.png
```

## 当前仓库的职责

当前仓库不再直接承载 Figma 导入流程，只作为 Designer 的 SDK 提供方。涉及 Figma MCP、Figma REST、设计稿截图对比等任务时，请切换到 `EmbeddedGUI_Designer`。
