# UI Designer 迁移说明

UI Designer 的设计稿转换链路已经拆分到独立仓库 `EmbeddedGUI_Designer` 维护。

## 已迁移内容

- `html2egui_helper.py`
- `figmamake/` 下的 Figma Make 工具链
- Figma MCP / REST 导入
- 设计稿到 XML / C 的 AI workflow
- 设计稿参考帧与像素回归流程

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的使用方式

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python html2egui_helper.py scaffold --app MyApp --width 320 --height 480
python html2egui_helper.py generate-code --app MyApp
python html2egui_helper.py gen-resource --app MyApp
python html2egui_helper.py verify --app MyApp
python figmamake/figmamake2egui.py --project-dir figma_make_project --app MyApp
```

## 当前仓库保留内容

`EmbeddedGUI` 继续作为 Designer 的 SDK 提供方，维护以下内容：

- `src/` 运行时与控件实现
- `example/` 示例应用
- `porting/` 平台适配
- 通用构建与运行时验证脚本
