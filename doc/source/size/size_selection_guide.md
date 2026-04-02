# Size Selection Guide

本页不是新的测量结果，而是把当前 `size/` 目录里的几份报告整理成一套更容易执行的取舍顺序。具体数字请以各报告页表格为准。

建议配合以下文档一起看：

- `size_report.md`
- `hq_size_report.md`
- `canvas_path_size_report.md`
- `canvas_feature_size_report.md`
- `widget_feature_size_report.md`

## 1. 不要把所有 Delta 直接相加

`EmbeddedGUI` 的 size 不是“单项线性累加”。

原因很直接：

- 不同 widget 会复用 `egui_view / theme / canvas / font / fixmath / image` 等公共代码。
- 不同 canvas path 之间也会共享底层几何和绘制辅助逻辑。
- 因此单项 delta 更适合看“边际成本”，组合项和最终应用报告才适合看“真实落地成本”。

实际阅读时，请把几份报告的口径分开：

- `canvas_path_size_report.md` 和 `hq_size_report.md` 看的是“单条路径被链接进来会多多少”。
- `canvas_feature_size_report.md` 和 `widget_feature_size_report.md` 看的是“一组能力或一组控件真正一起启用时的组合成本”。
- `size_report.md` 看的是“真实应用最终要付出的总体成本”。

## 2. 先看 feature，再看 widget，最后看 HQ

推荐顺序：

1. 先决定全局 feature 要不要开
2. 再决定真实 widget 要不要引入
3. 最后才决定某些场景是否需要显式调用 `_hq` 路径

这样做的原因是：

- feature 决定的是底层能力有没有进入固件，影响面最大。
- widget 决定的是业务层真实落地成本。
- HQ 是锦上添花项，适合最后做精细化开关，而不是一开始就全局打开。

## 3. 小 ROM 项目怎么选

如果目标是小 ROM，建议这样做：

- 从 `size_preset_profiles.md` 的 `tiny_rom` 或 `basic_ui` 起步。
- 只在资源实际使用时打开 `QOI` 或 `RLE` codec。
- 不要默认打开 `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ`。
- 优先确认是否真的需要重控件，而不是先引入再回头裁。

这类项目最应该优先复查的通常是：

- `MASK`
- `QOI / RLE`
- `WIDGET_ENHANCED_DRAW`
- `_hq` 调用路径

## 4. 通用 UI 项目怎么选

如果目标是常规设置页、表单页、简单交互页，建议：

- 从 `basic_ui` 模板起步。
- 先选基础交互控件，再逐步增加视觉增强项。
- codec 只按资源格式打开，不要因为“可能以后会用”而提前保留。
- 最终以 `size_report.md` 验证真实应用，而不是只看 probe 增量。

## 5. 仪表盘和展示项目怎么选

如果目标是看板、仪表、演示项目，建议：

- 从 `dashboard` 或 `full_feature` 起步。
- 接受更高的视觉相关 ROM 成本。
- 先确认是否确实需要 `ring / gauge / chart / clock` 这类重控件，再决定组合。
- 即使使用高配模板，也尽量把 HQ 保持为“按场景显式启用”，不要全局默认打开。

## 6. HQ 路径建议

当前 `hq_size_report.md` 的结论可以直接用于开关判断：

- 当前 qemu probe 报告里，`LINE_HQ`、`CIRCLE_HQ`、`ARC_HQ` 的隔离增量都在 `+15200B` 量级。
- `ARC_ROUND_CAP_HQ` 和 `ALL_HQ` 当前都落在 `+13388B`。
- 这说明真实链接结果要看依赖共享关系，不能只看公开符号本身的体积。

实用建议：

- 不要把 `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ` 直接改成全局默认 `1`。
- `_hq` API 只在明确需要画质收益的 widget 或场景里显式调用。
- 如果关心的是 `activity_ring` 这类真实业务场景，优先看 `ARC_ROUND_CAP_HQ`，不要只看 `ARC_HQ`。

## 7. 选型时最值得先看的开关

优先级较高的开关包括：

- `EGUI_CONFIG_FUNCTION_SUPPORT_MASK`
- `EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE`
- `EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE`
- `EGUI_CONFIG_WIDGET_ENHANCED_DRAW`
- `EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565_8`
- `EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_ALPHA_8`

这些开关的特点是：

- 要么决定整类功能是否进入固件；
- 要么决定资源格式支持范围；
- 要么会影响后续大量 widget 或绘制路径是否被保留。

## 8. 推荐实际流程

建议按下面的顺序做一次完整判断：

1. 先看 `size_preset_profiles.md`，选一个最接近目标的模板。
2. 再看 `canvas_feature_size_report.md`，决定全局 feature。
3. 再看 `widget_feature_size_report.md`，决定业务层真实控件。
4. 最后看 `hq_size_report.md`，只给必要场景加 HQ。
5. 用 `size_report.md` 验证真实应用的最终 `ROM/RAM/heap/stack`。

如果需要跑一轮统一回归，直接用：

```bash
python scripts/size_analysis/run_size_suite.py --quick
python scripts/size_analysis/run_size_suite.py --full
python scripts/size_analysis/run_size_suite.py --only hq,canvas_feature,widget
```
