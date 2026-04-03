# Figma Make 管道已迁移

Figma Make 到 EGUI 的整条自动化转换管道已经迁移到 `EmbeddedGUI_Designer` 仓库维护。

## 新位置

- 本地仓库: `D:\workspace\gitee\EmbeddedGUI_Designer`
- 远端仓库: `https://github.com/EmbeddedGUI/EmbeddedGUI_Designer.git`

## 迁移后的入口

在 `EmbeddedGUI_Designer` 仓库根目录执行：

```bash
python figmamake/figmamake2egui.py --project-dir figma_make_project --app MyApp --width 320 --height 240
python figmamake/figmamake_capture.py --tsx-dir figma_make_project --output-dir reference_frames
python figmamake/figmamake_regression.py --reference-dir reference_frames --rendered-dir runtime_check_output
```

## 说明

当前仓库不再维护以下内容：

- Figma Make 参考帧采集
- TSX 解析与动效提取
- 设计稿到 XML/C 的统一入口
- 像素级 SSIM 回归验证

这些能力现在由 `EmbeddedGUI_Designer` 统一维护，当前仓库只继续提供其依赖的 SDK 侧能力。
