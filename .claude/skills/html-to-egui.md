---
name: html-to-egui
description: HTML and Figma conversion flow has moved to EmbeddedGUI_Designer
---

# HTML/JSX 转换流程已迁移

`html2egui_helper.py`、`figmamake-extract` 和相关 HTML/Figma 转换工作流已从当前仓库迁移到 `EmbeddedGUI_Designer`。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 现在应该怎么用

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python html2egui_helper.py extract-layout --input design.html
python html2egui_helper.py scaffold --app MyApp --width 320 --height 480
python html2egui_helper.py generate-code --app MyApp
python html2egui_helper.py gen-resource --app MyApp
python html2egui_helper.py verify --app MyApp
```

如果是 Figma Make 全流程：

```bash
python figmamake/figmamake2egui.py --project-dir figma_make_project --app MyApp
```

## 当前仓库的职责

当前仓库只维护 EmbeddedGUI SDK、本地示例、控件实现和运行时能力，不再维护 HTML/Figma 转换脚本本体。

涉及以下任务时，应切换到 `EmbeddedGUI_Designer`：

- Stitch HTML 转 XML / C
- Figma Make JSX/TSX 转 XML / C
- Figma MCP / REST 导入
- 设计稿转换后的资源导出与验证
