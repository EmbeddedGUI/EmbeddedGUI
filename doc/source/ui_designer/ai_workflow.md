# AI 设计转换工作流已迁移

与 HTML/Figma/Figma Make 设计稿转换相关的 AI workflow，已经迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的入口

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python html2egui_helper.py extract-layout --input design.html
python html2egui_helper.py scaffold --app MyApp --width 320 --height 480
python html2egui_helper.py generate-code --app MyApp
python html2egui_helper.py verify --app MyApp
python figmamake/figmamake2egui.py --project-dir figma_make_project --app MyApp
```

## 当前仓库定位

当前仓库不再维护设计稿转换 AI workflow，本仓库现在主要承担：

- EmbeddedGUI SDK 和运行时能力
- 控件实现与示例应用
- 通用构建、格式化和运行时验证能力

Designer 仓库通过 SDK 路径或 `sdk/EmbeddedGUI` 子模块复用这里的能力。
