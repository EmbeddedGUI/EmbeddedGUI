# level_meter 控件说明

## 1. 为什么需要这个控件

`level_meter` 用于展示多通道电平、峰值保持线和当前焦点通道。它适合调音、媒体监看和设备面板里的多路反馈场景。

## 2. 为什么现有控件不够用

- `progress_bar` 只能表达单值进度，不能同时展示多通道和 peak hold。
- `chart_bar` 偏静态统计图，不强调实时焦点和状态切换。
- `waveform_strip` 强调时间轴波形，不适合做瞬时柱状电平监看。
- `fader_bank` 是控制器语义，而 `level_meter` 是监看反馈语义。

## 3. 目标场景与示例概览

- primary: `Bus A / Bus B` 主卡，显示五通道电平柱、peak hold 和 footer 短词。
- compact: 紧凑预览卡，用 `A / B` badge 标识快照。
- locked: 只读预览卡，保留结构，但用更弱的层级表达 disabled 语义。

## 4. 视觉与布局规格

- 目标屏幕：240 x 320。
- 根布局尺寸：230 x 310。
- primary 主卡尺寸：190 x 142，包含阴影、内描边、顶部标题胶囊和底部 footer pill。
- compact / locked 列尺寸：110 x 112，内部 meter 卡为 110 x 96。
- 必须检查标题胶囊、footer 短词、compact badge、`Compact` / `Locked` 标签的视觉居中与边距。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 230 x 310 | enabled | 页面主布局 |
| `meter_primary` | `egui_view_level_meter_t` | 190 x 142 | `Bus A` | 主电平监看卡 |
| `meter_compact` | `egui_view_level_meter_t` | 110 x 96 | `Compact A` | 紧凑预览卡 |
| `meter_locked` | `egui_view_level_meter_t` | 110 x 96 | disabled | 只读预览卡 |
| `status_label` | `egui_view_label_t` | 230 x 13 | `Bus A` | 外部状态反馈 |

## 6. 状态覆盖矩阵

| 区域 | 默认态 | 切换态 | 只读态 |
| --- | --- | --- | --- |
| primary | `Bus A` 聚焦 `VX` | `Bus B` 聚焦 `BS` | - |
| compact | badge `A` | badge `B` | - |
| locked | 保持固定快照 | 不响应点击 | 弱化叉线和 muted 配色 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待 400ms，截取默认态。
2. 点击 primary，切到 `Bus B`。
3. 等待 300ms，截取主卡切换态。
4. 点击 compact，切到 `Compact B`。
5. 等待 300ms，截取 compact 切换态。
6. 点击 locked，确认其仍为只读。
7. 再等待 800ms，截取最终收口帧。

## 8. 编译、runtime 、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=media/level_meter PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub media/level_meter --timeout 10 --keep-screenshots
```

- 编译必须通过。
- runtime 必须 `ALL PASSED`。
- 关键截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中使用相对路径引用。
- 必须逐轮检查标题胶囊、footer 短词、compact badge、状态词的居中与边距。

## 9. 已知限制与下一轮计划

- 当前录制动作覆盖的是预设切换路径，还没有拆出更多 peak 策略。
- 如果后续升级为框架级控件，可以继续抽象峰值衰减和通道告警策略。

## 10. 与现有控件的重叠分析

- 与 `progress_bar` 的差异：`level_meter` 强调多通道和 peak hold。
- 与 `chart_bar` 的差异：`level_meter` 强调实时监看和 focus channel。
- 与 `waveform_strip` 的差异：`waveform_strip` 是时间连续波形，`level_meter` 是瞬时柱状反馈。