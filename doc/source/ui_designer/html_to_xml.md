# HTML/JSX 转换文档已迁移

`html2egui_helper.py` 及其对应的 HTML/JSX/Figma Make 转换文档，已迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的常用命令

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python html2egui_helper.py extract-layout --input design.html
python html2egui_helper.py figmamake-extract --input figma_make_project --list-pages
python html2egui_helper.py scaffold --app MyApp --width 320 --height 480
python html2egui_helper.py generate-code --app MyApp
python html2egui_helper.py gen-resource --app MyApp
python html2egui_helper.py verify --app MyApp
```

## 当前仓库保留内容

当前仓库不再维护这条设计转换链路，只保留被 Designer 复用的 SDK、控件、示例和运行时基础设施。
