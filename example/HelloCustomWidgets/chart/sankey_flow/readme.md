# sankey_flow

## 1. 为什么需要这个控件

`sankey_flow` 用来表达多阶段流量分配、资源去向和阶段吞吐变化。它强调的是“流向带宽”而不是单点数值，因此适合展示 source -> mix -> export 这类路径型信息。

## 2. 为什么现有控件不够用

- `chart_line`、`chart_bar`、`radar_chart` 只表达序列或维度值，不表达阶段间的流向宽度。
- `node_topology` 重点是节点连接关系，不强调链路宽度和吞吐量分配。
- `treemap_chart` 表达总量切分，不表达跨阶段路径。

## 3. 目标场景与示例概览

- 主卡：展示三阶段流量路径，在 `LIVE / MIX / SAFE` 间切换焦点阶段。
- 左下 compact 卡：展示紧凑队列视角，在 `SCAN / LOAD / SAFE` 间切换。
- 右下 locked 卡：展示只读审计视角，在 `LOCK / HOLD / SYNC` 间切换。
- 顶部 guide 使用 `Tap cards to shift flow`，提示通过点击三张卡切换路径状态。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 顶部标题区：居中标题 + guide，要求上下留白平衡。
- 主卡尺寸：`220 x 125`
- 底部双卡尺寸：`102 x 87` + `102 x 87`
- 主预览要能清楚看出三列节点、桥接流带和流带宽度差异。
- 短词胶囊如 `LIVE`、`SAFE`、`SYNC` 必须检查视觉居中和左右内边距。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `sankey_dashboard` | `egui_view_sankey_flow_t` | `240 x 280` | `primary=0, compact=0, locked=0` | 整个 sankey flow dashboard |
| `primary_snapshots` | `egui_view_sankey_flow_snapshot_t[3]` | - | `LIVE` | 主卡流向预览 |
| `compact_snapshots` | `egui_view_sankey_flow_snapshot_t[3]` | - | `SCAN` | 左下 compact 路由预览 |
| `locked_snapshots` | `egui_view_sankey_flow_snapshot_t[3]` | - | `LOCK` | 右下 locked 审计预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | 摘要 | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Source lanes push ahead` | `primary route live` |
| 主卡 | 1 | `MIX` | `Middle stage bends heavy` | `bridge mix set` |
| 主卡 | 2 | `SAFE` | `Split export stays calm` | `seal flow safe` |
| 左下卡 | 0 | `SCAN` | `Queue lanes open` | `route set` |
| 左下卡 | 1 | `LOAD` | `Mix lane gains` | `mix tuned` |
| 左下卡 | 2 | `SAFE` | `Queue flow rests` | `seal calm` |
| 右下卡 | 0 | `LOCK` | `Review route parked` | `audit set` |
| 右下卡 | 1 | `HOLD` | `Gate lane waits` | `locked hold steady` |
| 右下卡 | 2 | `SYNC` | `Export bins aligned` | `locked sync safe` |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 初始等待，保证首帧稳定。
- 点击主卡两次，覆盖 `LIVE -> MIX -> SAFE`。
- 点击左下卡两次，覆盖 `SCAN -> LOAD -> SAFE`。
- 点击右下卡两次，覆盖 `LOCK -> HOLD -> SYNC`。
- 结尾等待，确保最终状态被截图。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=chart/sankey_flow PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub chart/sankey_flow --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、断言、裁切。
- 标题与 guide 必须视觉居中。
- 主预览三列节点和流带必须完整可辨。
- 短词胶囊必须检查左右内边距和视觉居中。
- 底部 compact / locked 卡的 footer 与边框之间必须保留安全距离。

## 9. 已知限制与下一轮迭代计划

- 当前版本仍是 HelloCustomWidgets 层的自绘 dashboard，还未下沉到 `src/widget/`。
- 当前流带用简化桥接画法表达，不包含真实曲线插值。
- 若后续需要升级为公共控件，再规划参数化 API 和 UI Designer 注册。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `node_topology`：这里强调链路宽度和阶段吞吐，而不是纯连接关系。
- 区别于 `treemap_chart`：这里表达跨阶段路径，不是静态面积切分。
- 区别于 `chart_bar` / `chart_line`：这里不看单轴值，而看路径流向和合流分流。
