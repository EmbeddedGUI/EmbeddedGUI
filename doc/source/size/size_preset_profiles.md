# Size Preset Profiles

本页把当前提供的几档 size 模板整理成可直接复用的起始配置，目标不是给出唯一答案，而是帮助用户快速选一个合理起点。

建议和以下文档一起看：

- `size_selection_guide.md`
- `canvas_feature_size_report.md`
- `widget_feature_size_report.md`
- `hq_size_report.md`
- `size_preset_validation.md`

对应模板文件位于：

- `example/HelloSizeAnalysis/ConfigProfiles/tiny_rom/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/basic_ui/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/dashboard/app_egui_config.h`
- `example/HelloSizeAnalysis/ConfigProfiles/full_feature/app_egui_config.h`

## 1. 使用原则

这些模板只负责“全局配置起点”，不负责替你决定业务代码里是否真的引用某个 widget 或绘制路径。

因此需要区分两层：

- 第一层：`EGUI_CONFIG_*` 这类全局开关，由模板直接给出。
- 第二层：具体 widget、canvas path、HQ path 是否被引用，由应用代码自己决定。

也就是说：

- 有全局总开关的能力，优先用模板做裁剪。
- 没有全局总开关的能力，优先通过“不引用对应 API 或 widget”让编译器和链接器回收代码。

## 2. 四档模板总览

| Profile | 目标 | 核心特点 |
|---------|------|---------|
| `tiny_rom` | 极限 ROM 裁剪 | 关闭 mask、codec、增强绘制、阴影、图层、触摸、默认 HQ 和额外图片格式 |
| `basic_ui` | 常规 UI 页面 | 保留触摸、滚动条、快速文字绘制，关闭 mask、codec、图层和默认 HQ |
| `dashboard` | 看板 / 仪表 / 展示页 | 打开 `QOI/RLE`、增强绘制和阴影，保留触摸，仍然关闭 mask 和默认 HQ |
| `full_feature` | 高配设备 / 演示工程 | 打开 mask、双 codec、图层、多点触控、焦点、额外图片格式，默认 HQ 仍保持关闭 |

## 3. tiny_rom

适合场景：

- 小屏
- 小 ROM
- 以状态显示和基础交互为主

当前模板直接关闭了这些开关：

- `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`
- `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE`
- `EGUI_CONFIG_WIDGET_ENHANCED_DRAW`
- `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW`
- `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER`
- `EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR`
- `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH`
- `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS`
- `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ`
- `EGUI_CONFIG_FONT_STD_FAST_DRAW_ENABLE`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`

仍然保留：

- `EGUI_CONFIG_FUNCTION_SUPPORT_KEY`

建议：

- 从这档开始时，不要默认引入重控件。
- 只有业务明确需要时，再向上加功能。

## 4. basic_ui

适合场景：

- 设置页
- 表单页
- 常规触摸交互

当前模板的特点：

- 打开 `SCROLLBAR`、`TOUCH`、`FONT_STD_FAST_DRAW_ENABLE`
- 关闭 `MASK`、`QOI`、`RLE`、`LAYER`、`FOCUS`、默认 HQ
- 仍然保持增强绘制和阴影关闭

建议：

- 这档适合作为绝大多数普通 UI 应用的起点。
- 如果项目后续没有压缩图片资源，可以继续保持 codec 关闭。

## 5. dashboard

适合场景：

- 看板
- 仪表
- 展示型页面

当前模板的特点：

- 打开 `QOI`、`RLE`
- 打开 `WIDGET_ENHANCED_DRAW`
- 打开 `SHADOW`
- 保留 `TOUCH` 和 `SCROLLBAR`
- 仍然关闭 `MASK`、`LAYER`、`FOCUS`、默认 HQ

建议：

- 这档适合已经接受更高视觉成本的项目。
- 如果实际项目并不使用压缩图片资源，可以回退关闭 `QOI/RLE`。

## 6. full_feature

适合场景：

- 高配 MCU
- 演示工程
- 不以极限 ROM 为目标的项目

当前模板额外打开了：

- `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`
- `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER`
- `EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH`
- `EGUI_CONFIG_FUNCTION_SUPPORT_KEY`
- `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`

同时保留：

- `QOI / RLE`
- `WIDGET_ENHANCED_DRAW`
- `SHADOW`
- `TOUCH`
- `SCROLLBAR`

注意：

- 即使是 `full_feature`，当前模板也没有把 `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ` 改成 `1`。
- HQ 仍建议按场景显式启用，而不是作为全局默认行为。

## 7. 这些模板已经不再依赖 USER_CFLAGS

当前 size 分析链路已经改为：

- 把模板内容写入应用本地的 override 头文件
- 再由应用自己的 `app_egui_config.h` 体系接管

因此：

- 批量报告脚本不再依赖 `USER_CFLAGS` 做 profile 切换
- 模板的推荐用法是“复制到应用目录”或“作为应用本地 override 的来源”

## 8. 推荐使用顺序

建议这样使用这些模板：

1. 先从最接近目标的 profile 起步。
2. 先确认全局 `EGUI_CONFIG_*` 是否合理。
3. 再根据业务代码决定是否真正引入某些 widget、canvas path 和 `_hq` 路径。
4. 最后用 `size_report.md` 和 `size_preset_validation.md` 做验证。

如果你现在没有明确判断，默认建议：

- 普通 UI 项目从 `basic_ui` 起步
- 小 ROM 项目从 `tiny_rom` 起步
- 仪表 / 看板项目从 `dashboard` 起步
- 演示或高配设备项目从 `full_feature` 起步
