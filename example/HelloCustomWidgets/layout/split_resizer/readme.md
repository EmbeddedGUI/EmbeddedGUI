# split_resizer

## 1. 为什么需要这个控件

`split_resizer` 用来表达 IDE、编辑器、监控台、设计工具里的多栏分割布局。它不是内容容器本身，而是一个把 pane 比例、拖拽柄和锁定态集中可视化的 dashboard 预览控件。

## 2. 为什么现有控件不够用

- `window_snap_grid` 表达的是整屏吸附预设，不是 pane ratio 和 resizer handle。
- `kanban_board` 关注列内内容组织，不关注横竖分割与比例切换。
- `layer_stack` 关注前后层叠深度，不是多 pane 分栏结构。

## 3. 目标场景与示例概览

- 主卡：展示三段横向 split 的主布局与当前状态。
- 左下 compact 卡：展示纵向 column stack 预览。
- 右下 compact 卡：展示 locked/read-only 的横向 grid 预览。
- 点击三张卡分别轮换各自的 snapshot，运行时截图中要能看出状态差异。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 标题区：居中标题 + guide，两行顶部文案需要保持上下留白平衡。
- 主卡：`220 x 125`，作为主视觉锚点，内部包含 title、status pill、summary、layout preview、footer。
- 底部双 compact 卡：`102 x 87` + `102 x 87`，强调次级预览与锁定态差异。
- 中段状态行与 divider：用于连接主卡与底部双卡，避免中部留白过大。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `resizer_dashboard` | `egui_view_split_resizer_t` | `240 x 280` | `primary=0, column=0, locked=0` | 整个 split dashboard |
| `primary_snapshots` | `egui_view_split_resizer_snapshot_t[3]` | - | `EDIT` | 主卡横向分栏组 |
| `column_snapshots` | `egui_view_split_resizer_snapshot_t[3]` | - | `STACK` | 左下纵向分栏组 |
| `locked_snapshots` | `egui_view_split_resizer_snapshot_t[3]` | - | `LIVE` | 右下 locked 预览组 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | 摘要 | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `EDIT` | `Three panes ready` | `primary pane live` |
| 主卡 | 1 | `FOCUS` | `Center pane tuned` | `focus pane lock` |
| 主卡 | 2 | `SAFE` | `Balanced panes calm` | `layout seal safe` |
| 左下卡 | 0 | `STACK` | `Column set` | `column group` |
| 左下卡 | 1 | `SCAN` | `Tall panes` | `scan group` |
| 左下卡 | 2 | `SAFE` | `Column calm` | `seal stack` |
| 右下卡 | 0 | `LIVE` | `Dock set` | `render grid` |
| 右下卡 | 1 | `WARN` | `Queue panes` | `guard split` |
| 右下卡 | 2 | `LOCK` | `Export calm` | `archive split` |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 初始等待一帧稳定输出。
- 点击主卡两次，覆盖主卡 `EDIT -> FOCUS -> SAFE`。
- 点击左下卡两次，覆盖 `STACK -> SCAN -> SAFE`。
- 点击右下卡两次，覆盖 `LIVE -> WARN -> LOCK`。
- 结尾保留等待，确保最终收口态被截图。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=layout/split_resizer PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/split_resizer --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、断言、裁切。
- 顶部标题与 guide 要视觉居中，顶部留白不能失衡。
- `EDIT`、`FOCUS`、`SAFE`、`STACK`、`LIVE`、`LOCK` 等短词胶囊不能贴边，左右内边距要均衡。
- 中央 preview 的 separator 和 grip 要清晰，但不能压过文本层级。
- 底部 compact 卡的 footer 与边框之间必须保留安全距离。

## 9. 已知限制与下一轮迭代计划

- 当前版本是 HelloCustomWidgets 层的自绘 dashboard，还未下沉到 `src/widget/` 公共层。
- snapshot 仍是静态预设切换，不包含真实拖拽动画。
- 如果后续需要升级为框架公共控件，再规划参数化 API 和 UI Designer 注册。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `window_snap_grid`：这里不表达整屏吸附，而是 pane ratio 与 resizer handle。
- 区别于 `kanban_board`：这里不承载内容卡片，只表达 split 结构和状态。
- 区别于 `layer_stack`：这里没有前后层叠深度，核心是横竖分栏切换与锁定预览。
