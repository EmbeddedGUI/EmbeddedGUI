# conversion_funnel

## 1. 为什么需要这个控件
`conversion_funnel` 用来表达从上层曝光到中层筛选再到最终收敛的单路径阶段式流失结构。它强调的是阶段体量逐级收窄、当前焦点层突出，以及主卡和双小卡之间的对照关系。

## 2. 为什么现有控件不够用
- `sankey_flow` 强调多路径分流和带宽流向，不适合表达单路径漏斗收窄。
- `chart_bar`、`chart_line` 只能表达数值，不表达阶段收窄结构。
- `treemap_chart` 强调面积切分，不强调阶段式转化与收窄关系。

## 3. 目标场景与示例概览
- 主卡：展示四层 funnel 预览，在 `LIVE / MIX / SAFE` 三个状态间切换。
- 左下 compact 卡：展示较轻量的 queue funnel，在 `SCAN / LOAD / SAFE` 三个状态间切换。
- 右下 locked 卡：展示审计态 funnel，在 `LOCK / HOLD / SYNC` 三个状态间切换。
- 中间状态带：展示最近一次交互区域的简短反馈。
- 顶部 guide：使用 `Tap cards to shift funnel` 说明交互方式。

## 4. 视觉与布局规格
- 根控件尺寸：`240 x 280`
- 顶部标题区：居中标题 + guide
- 主卡尺寸：`220 x 120`
- 底部双卡尺寸：`104 x 87` + `104 x 87`
- 主卡与底部双卡之间保留独立状态带和分隔线
- 关键短词胶囊必须检查视觉居中、左右内边距和平衡留白
- summary、footer 与边框之间必须保持安全距离，不能贴边

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `funnel_dashboard` | `egui_view_conversion_funnel_t` | `240 x 280` | `primary=0, compact=0, locked=0` | 整个 conversion funnel dashboard |
| `primary_snapshots` | `egui_view_conversion_funnel_snapshot_t[3]` | - | `LIVE` | 主卡 funnel 预览 |
| `compact_snapshots` | `egui_view_conversion_funnel_snapshot_t[3]` | - | `SCAN` | 左下 compact funnel 预览 |
| `locked_snapshots` | `egui_view_conversion_funnel_snapshot_t[3]` | - | `LOCK` | 右下 locked funnel 预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | 摘要 | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Acquire stage wide` | `live focus` |
| 主卡 | 1 | `MIX` | `Consider stage bends` | `mix bend` |
| 主卡 | 2 | `SAFE` | `Close stage calm` | `seal safe` |
| 左下卡 | 0 | `SCAN` | `Queue top open` | `queue ready` |
| 左下卡 | 1 | `LOAD` | `Middle drop set` | `load tuned` |
| 左下卡 | 2 | `SAFE` | `Queue close calm` | `seal calm` |
| 右下卡 | 0 | `LOCK` | `Audit top set` | `audit ready` |
| 右下卡 | 1 | `HOLD` | `Hold band set` | `hold steady` |
| 右下卡 | 2 | `SYNC` | `Sync close set` | `sync safe` |

## 7. `egui_port_get_recording_action()` 录制动作设计
- 初始等待，确保首页稳定出图。
- 点击主卡两次，覆盖 `LIVE -> MIX -> SAFE`。
- 点击左下卡两次，覆盖 `SCAN -> LOAD -> SAFE`。
- 点击右下卡两次，覆盖 `LOCK -> HOLD -> SYNC`。
- 结尾等待，确保最终状态被截图归档。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=chart/conversion_funnel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub chart/conversion_funnel --timeout 10 --keep-screenshots
```

验收重点：
- 不能黑屏、白屏、断言或裁切。
- 标题与 guide 必须视觉居中。
- 主卡和底部双卡的状态胶囊短词必须检查真实居中与左右内边距。
- 主卡、双卡的 summary 与 footer 不能贴边。
- 主卡与底部双卡之间的状态带必须可辨，不能与 footer 重复或互相挤压。
- 关键截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log.md` 里用相对路径记录。

## 9. 已知限制与下一轮迭代计划
- 当前仍是 `HelloCustomWidgets` 层的专用 dashboard，不是框架级公共控件。
- funnel 的桥接段采用简化矩形桥接，没有复杂曲线。
- 如果后续需要沉入公共控件层，再单独规划参数化 API 和 widget 注册。

## 10. 与现有控件的重叠分析与差异化边界
- 区别于 `sankey_flow`：这里强调单路径阶段收窄，不强调多分流。
- 区别于 `chart_bar` / `chart_line`：这里表达的是阶段结构与焦点层，不是连续数值趋势。
- 区别于 `treemap_chart`：这里强调转化漏斗，不是面积切分。
