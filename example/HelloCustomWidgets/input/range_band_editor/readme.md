# range_band_editor

## 1. 为什么需要这个控件
`range_band_editor` 用来表达一个可调区间而不是单个值。它强调的是起点、终点、当前焦点刻度和带状选区之间的关系，适合做过滤带、阈值窗口、扫描区段和只读审计区间的可视表达。

## 2. 为什么现有控件不够用
- `slider` 主要是单值滑动，不表达双端区间。
- `progress_bar` 只表达完成度，不表达可调起止范围。
- `xy_pad` 是二维连续控制，不表达一维带状范围和刻度区段。

## 3. 目标场景与示例概览
- 主卡：展示双端范围带和焦点刻度，在 `LIVE / MIX / SAFE` 三个状态间切换。
- 左下 compact 卡：展示 queue band，在 `SCAN / LOAD / SAFE` 三个状态间切换。
- 右下 locked 卡：展示只读 audit band，在 `LOCK / HOLD / SYNC` 三个状态间切换。
- 中间状态带：展示最近一次切换区域的简短反馈。

## 4. 视觉与布局规格
- 根控件尺寸：`240 x 280`
- 顶部标题区：居中标题 + guide
- 主卡尺寸：`220 x 118`
- 底部双卡尺寸：`105 x 87` + `105 x 87`
- 主卡与底部双卡之间保留独立状态带和分隔线
- 预览区必须明确看到起点手柄、终点手柄、选区高亮和焦点刻度
- 状态胶囊短词必须检查真实居中、左右内边距和平衡留白
- summary、footer 与边框之间必须保持安全距离，不能贴边

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `range_dashboard` | `egui_view_range_band_editor_t` | `240 x 280` | `primary=0, compact=0, locked=0` | 整个 range band dashboard |
| `primary_snapshots` | `egui_view_range_band_editor_snapshot_t[3]` | - | `LIVE` | 主卡范围带预览 |
| `compact_snapshots` | `egui_view_range_band_editor_snapshot_t[3]` | - | `SCAN` | 左下 compact 预览 |
| `locked_snapshots` | `egui_view_range_band_editor_snapshot_t[3]` | - | `LOCK` | 右下 locked 预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | 摘要 | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Open span wide` | `range live` |
| 主卡 | 1 | `MIX` | `Middle band tight` | `mix band` |
| 主卡 | 2 | `SAFE` | `Guard span calm` | `guard safe` |
| 左下卡 | 0 | `SCAN` | `Queue span set` | `queue ready` |
| 左下卡 | 1 | `LOAD` | `Load gate set` | `load tuned` |
| 左下卡 | 2 | `SAFE` | `Queue calm` | `seal calm` |
| 右下卡 | 0 | `LOCK` | `Audit band set` | `audit ready` |
| 右下卡 | 1 | `HOLD` | `Hold limit set` | `hold steady` |
| 右下卡 | 2 | `SYNC` | `Sync band set` | `sync safe` |

## 7. `egui_port_get_recording_action()` 录制动作设计
- 初始等待，确保首帧稳定。
- 点击主卡两次，覆盖 `LIVE -> MIX -> SAFE`。
- 点击左下卡两次，覆盖 `SCAN -> LOAD -> SAFE`。
- 点击右下卡两次，覆盖 `LOCK -> HOLD -> SYNC`。
- 结尾等待，确保最终状态被截图归档。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=input/range_band_editor PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/range_band_editor --timeout 10 --keep-screenshots
```

验收重点：
- 不能黑屏、白屏、崩溃或裁切。
- 标题、guide、状态胶囊必须检查视觉居中。
- 起点手柄、终点手柄、带状高亮和焦点刻度必须完整可辨。
- summary、footer 与边框之间必须保留合理空隙。
- 关键截图要复制到 `iteration_log/images/iter_xx/` 并在 `iteration_log.md` 中记录。

## 9. 已知限制与下一轮迭代计划
- 当前仍是 `HelloCustomWidgets` 层的专用自绘控件，不是框架级公共控件。
- 当前区间手柄使用简化块状表达，没有做真实拖拽交互。
- 如果后续需要沉入公共控件层，再单独规划参数化 API 和 widget 注册。

## 10. 与现有控件的重叠分析与差异化边界
- 区别于 `slider`：这里表达双端范围带，不是单值位置。
- 区别于 `progress_bar`：这里表达可调区间和焦点刻度，不是完成度。
- 区别于 `xy_pad`：这里表达一维带状区间，不是二维平面定位。
