# priority_matrix

## 1. 为什么需要这个控件

`priority_matrix` 用来表达任务分诊、工作台排队和行动优先级判断。它不是简单的二维表格，而是把四象限决策、焦点象限、任务密度和锁定复核状态集中可视化的 dashboard 控件。

## 2. 为什么现有控件不够用

- `heatmap_chart` 强调的是连续格点热度，不是离散任务在四象限里的决策分布。
- `seat_map` 关注固定座位网格，不表达“重要/紧急”这类语义轴。
- `window_snap_grid` 与 `split_resizer` 关注布局切分，不关心任务优先级和象限切换。
- `kanban_board` 是泳道内容组织，不是四象限判断面板。

## 3. 目标场景与示例概览

- 主卡：展示一张大尺寸四象限优先级面板，点击后在 `NOW / PLAN / HAND` 三种焦点状态间切换。
- 左下 compact 卡：展示团队侧的紧凑队列视角，强调 cluster scan 与 grouped lane 的变化。
- 右下 locked 卡：展示只读归档视角，强调 locked / hold / sync 三种复核结果。
- 顶部 guide 当前使用 `Tap cards to shift focus`，用于明确说明交互入口。
- 录制动作会依次点击三张卡，确保截图里能看到主卡、compact 卡、locked 卡都发生状态变化。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 顶部标题区：居中标题 + guide，两行文案需保持左右留白平衡。
- 主卡尺寸：`220 x 125`
- 底部双卡尺寸：`102 x 87` + `102 x 87`
- 主卡与底部双卡之间保留状态行和分隔线，避免上下节奏松散。
- 状态胶囊短词必须单独检查视觉居中与左右留白，不能出现文字贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `priority_matrix_dashboard` | `egui_view_priority_matrix_t` | `240 x 280` | `primary=0, compact=0, locked=0` | 整个 priority matrix dashboard |
| `primary_snapshots` | `egui_view_priority_matrix_snapshot_t[3]` | - | `NOW` | 主卡四象限焦点预览 |
| `compact_snapshots` | `egui_view_priority_matrix_snapshot_t[3]` | - | `SCAN` | 左下 compact 队列预览 |
| `locked_snapshots` | `egui_view_priority_matrix_snapshot_t[3]` | - | `LOCK` | 右下 locked 归档预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | 摘要 | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `NOW` | `Urgent lane owns focus` | `priority lane live` |
| 主卡 | 1 | `PLAN` | `Schedule lane leads next` | `plan sweep set` |
| 主卡 | 2 | `HAND` | `Delegate lane stays calm` | `handoff matrix safe` |
| 左下卡 | 0 | `SCAN` | `Cluster scan open` | `compact scan set` |
| 左下卡 | 1 | `LOAD` | `Hand-off queue fills` | `compact load tuned` |
| 左下卡 | 2 | `SAFE` | `Grouped lane settled` | `compact seal set` |
| 右下卡 | 0 | `LOCK` | `Review grid parked` | `locked board set` |
| 右下卡 | 1 | `HOLD` | `Backlog hold visible` | `locked hold steady` |
| 右下卡 | 2 | `SYNC` | `Archive bins aligned` | `locked sync safe` |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 初始等待，保证首帧稳定输出。
- 点击主卡两次，覆盖 `NOW -> PLAN -> HAND`。
- 点击左下卡两次，覆盖 `SCAN -> LOAD -> SAFE`。
- 点击右下卡两次，覆盖 `LOCK -> HOLD -> SYNC`。
- 结尾增加等待，保证最终收口状态被截图。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=layout/priority_matrix PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/priority_matrix --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、断言、裁切。
- 顶部标题与 guide 必须视觉居中。
- `NOW`、`PLAN`、`HAND`、`LOCK`、`HOLD`、`SYNC` 等短词胶囊必须检查左右内边距是否均衡。
- 主卡四象限的中心十字、焦点象限填充、四角锚点和底部四段 badge 必须清晰可辨。
- 底部 compact / locked 卡的 footer 文字与边框之间必须保留安全距离。

## 9. 已知限制与下一轮迭代计划

- 当前版本先实现为 HelloCustomWidgets 层的自绘 dashboard，还未下沉到 `src/widget/` 公共层。
- 当前 snapshot 是离散切换，不包含真实拖拽或任务重排动画。
- 后续若要升级为框架公共控件，再规划参数化 API 和 UI Designer 注册。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `heatmap_chart`：这里不是热区数值矩阵，而是四象限决策面板。
- 区别于 `kanban_board`：这里不展示泳道内容卡片，重点是象限聚类与优先级判断。
- 区别于 `seat_map`：这里没有固定座位语义，核心是任务标记在四象限里的分布与焦点切换。
- 区别于 `split_resizer`：这里不表达 pane ratio，而是表达优先级语义轴和任务 triage。
