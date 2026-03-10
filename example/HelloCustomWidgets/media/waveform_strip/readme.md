# waveform_strip 控件设计说明

## 1. 为什么需要这个控件？

`waveform_strip` 用连续波形条和播放头表达音频片段的强弱分布、当前播放位置和 cue 区域，适合在小屏里快速预览音频片段、录音段落或采样片段。

## 2. 为什么现有控件不够用？

- `chart_bar` 更偏通用统计图，不带音频播放头和基线语义。
- `progress_bar` 只能表达进度，不表达波形振幅分布。
- `slider` 强调输入调节，不适合表达播放波形内容。
- `mp4` 偏视频播放，不适合做轻量级音频波形摘要。

差异化边界：`waveform_strip` 的核心是“波形振幅条 + 中轴线 + 播放头 + cue 高亮 + compact/locked 预览”，不是普通柱状图、进度条或输入滑杆。

## 3. 目标场景与示例概览

- primary：主波形条，展示完整片段并支持 `Track A / Track B` 切换。
- compact：紧凑预览，验证小尺寸下波形和播放头仍清晰。
- locked：只读预览，验证禁用态结构仍可辨认但不会响应点击。
- 页面文案：标题为 `Waveform Strip`，引导语为 `Tap strips to rotate cue`，中部状态文案在 `Track A cue / Track B cue / Compact A wave / Compact B wave` 之间切换。

目录：`example/HelloCustomWidgets/media/waveform_strip/`

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 根布局：`220 x 304`，垂直居中摆放。
- 主波形条：`176 x 132`，重点展示波形形态、播放头位置和 cue 提示。
- 中部状态文案：`220 x 14`，用于反馈当前主区或 compact 状态。
- 底部分割线：沿用页面中轴对齐，区分主波形条和底部预览区。
- 底部双列：左右列各 `106 x 92`，用于 compact / locked 对照。
- 视觉验收重点：
  - `Track A / Track B`、`Cue active`、`Compact A / Compact B`、`Locked` 这些短词必须居中且与边框保留安全距离。
  - 主波形条中的播放头竖线需要位于视觉中心节奏内，不能出现明显偏心。
  - compact / locked 两张底部预览必须完整可见，不能裁切。
  - 波形条、中轴线和交叉线的层级必须清楚，不能互相污染。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 220 x 304 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 220 x 18 | `Waveform Strip` | 标题 |
| `guide_label` | `egui_view_label_t` | 220 x 12 | `Tap strips to rotate cue` | 交互提示 |
| `waveform_primary` | `egui_view_waveform_strip_t` | 176 x 132 | `Track A` | 主波形条 |
| `status_label` | `egui_view_label_t` | 220 x 14 | `Track A cue` | 中部状态反馈 |
| `waveform_compact` | `egui_view_waveform_strip_t` | 106 x 92 | `Compact A` | 紧凑预览 |
| `waveform_locked` | `egui_view_waveform_strip_t` | 106 x 92 | disabled | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 能力 | 覆盖方式 | 预期结果 |
| --- | --- | --- |
| 默认态 | 初始渲染 | 主波形条显示 `Track A`，播放头位于中段 |
| primary 切换态 | 点击主波形条 | 切到 `Track B`，播放头和 cue 高亮同步变化 |
| compact 切换态 | 点击 compact 预览 | 切到 `Compact B`，底部波形焦点同步变化 |
| locked 只读态 | 点击 locked 预览 | 无状态变化，保持禁用遮罩与交叉线 |
| 居中与边距复核 | runtime 截图检查 | 标题短词、cue 文案、播放头和底部标签都保持合理留白 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始等待 400ms，输出默认态截图。
2. 点击主波形条，切到 `Track B`。
3. 等待 300ms，记录主波形条切换后的稳定帧。
4. 点击 compact 预览，切到 `Compact B`。
5. 等待 300ms，记录 compact 切换后的稳定帧。
6. 点击 locked 预览，验证 disabled 预览无响应。
7. 最后等待 800ms，输出最终稳定帧。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/waveform_strip PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/waveform_strip --timeout 10 --keep-screenshots
```

验收要求：
- `make` 必须通过，无编译或链接错误。
- runtime 必须输出 `ALL PASSED`。
- 主波形条、compact 预览、locked 预览完整可见。
- 波形条、中轴线、播放头和 cue 高亮必须明确可辨认。
- `Track A/B`、`Cue active`、`Compact A/B`、`Locked` 等短词必须经过居中和边距复核。
- 关键截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中用相对路径引用。

## 9. 已知限制与下一轮迭代计划

- 当前波形数据为静态数组，不接入真实音频解析。
- 不支持拖拽播放头或动态滚动波形。
- compact / locked 主要验证摘要态，不展示多段选区。
- 后续可扩展录音波形动画、时间刻度、选区和多轨叠加。

## 10. 与现有控件的重叠分析与差异化边界

- 与 `chart_bar`：这里强调音频波形和播放头，不是通用统计柱图。
- 与 `progress_bar`：这里有振幅形态，不只是进度。
- 与 `slider`：这里偏展示，不偏输入。
- 与 `mp4`：这里是轻量音频预览，不承担视频内容展示。
