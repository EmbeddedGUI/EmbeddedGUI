# signal_matrix

## 1. 为什么需要这个控件

`signal_matrix` 用来表达多通道信号格阵的离散开关、焦点列和只读限制态。它强调的是 5x4 矩阵里的列焦点、行级激活数量，以及主卡与 compact/locked 双预览之间的对照关系。

## 2. 为什么现有控件不够用

- `heatmap_chart` 强调连续热区，不强调离散门格和焦点列。
- `status_timeline` 强调纵向阶段顺序，不表达并行矩阵监看。
- `node_topology` 强调节点和连线，不表达固定列阵列的通道状态。

## 3. 目标场景与示例概览

- 主卡：展示 `GRID` 主矩阵，在 `LIVE / SCAN / SAFE` 三态之间切换。
- 左下 compact 卡：展示 `MESH` 监看矩阵，在 `SCAN / LOAD / SAFE` 之间切换。
- 右下 locked 卡：展示 `LOCK` 只读矩阵，在 `HOLD / SYNC / SAFE` 之间切换。
- 中部状态带：展示最近一次交互区域的反馈词组，例如 `Grid live`、`Load set`、`Lock set`。

## 4. 视觉与布局规格

- 根控件尺寸：`240 x 280`
- 顶部标题区：居中标题 + 居中 guide
- 主卡尺寸：`220 x 118`
- 底部双卡尺寸：`105 x 87` + `105 x 87`
- 主卡预览：四列五行矩阵，焦点列带底部 marker，允许四列满亮
- 短词胶囊：`LIVE / SAFE / HOLD / SCAN` 必须检查真实视觉居中和左右内边距
- 文本安全距离：summary、footer 与边框之间必须保留安全距离，不允许贴边

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `signal_dashboard` | `egui_view_signal_matrix_t` | `240 x 280` | `primary=0, compact=0, locked=0` | 整个 signal matrix dashboard |
| `primary_snapshots` | `egui_view_signal_matrix_snapshot_t[3]` | - | `LIVE` | 主卡矩阵状态 |
| `compact_snapshots` | `egui_view_signal_matrix_snapshot_t[3]` | - | `SCAN` | 左下 compact 预览 |
| `locked_snapshots` | `egui_view_signal_matrix_snapshot_t[3]` | - | `HOLD` | 右下 locked 预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态词 | Summary | Footer |
| --- | --- | --- | --- | --- |
| 主卡 | 0 | `LIVE` | `Five lanes in sync` | `grid live` |
| 主卡 | 1 | `SCAN` | `Focus column tracks` | `scan sweep` |
| 主卡 | 2 | `SAFE` | `Guard matrix calm` | `guard safe` |
| 左下卡 | 0 | `SCAN` | `Mesh lane set` | `mesh set` |
| 左下卡 | 1 | `LOAD` | `Load cell rise` | `load tuned` |
| 左下卡 | 2 | `SAFE` | `Mesh calm` | `seal calm` |
| 右下卡 | 0 | `HOLD` | `Lock lane set` | `lock set` |
| 右下卡 | 1 | `SYNC` | `Sync cell set` | `sync set` |
| 右下卡 | 2 | `SAFE` | `Audit calm` | `audit safe` |

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧等待，确保页面稳定渲染。
- 点击主卡两次，覆盖 `LIVE -> SCAN -> SAFE`。
- 点击左下卡三次，覆盖 `SCAN -> LOAD -> SAFE -> SCAN`。
- 点击右下卡三次，覆盖 `HOLD -> SYNC -> SAFE -> HOLD`。
- 录制序列最终保留 compact 和 locked 回到默认态的截图，便于 review 短词胶囊和 footer 居中。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=display/signal_matrix PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/signal_matrix --timeout 10 --keep-screenshots
```

验收重点：

- 不能黑屏、白屏、崩溃或全空白
- 主卡、compact、locked 三张卡都必须完整可见
- 5x4 矩阵必须真实渲染，不能出现“外框有了但内部空白”
- 标题、guide、中部状态带和 footer 都必须检查视觉居中
- 短词胶囊必须检查左右内边距和平衡感
- 关键截图要复制到 `iteration_log/images/iter_xx/` 并在 `iteration_log.md` 中引用

## 9. 已知限制与下一轮迭代计划

- 当前仍是 `HelloCustomWidgets` 层的专用自绘控件，还未升级为框架通用控件。
- 当前矩阵使用静态 snapshot 切换，没有连续动画或实时信号采样。
- 如果后续要沉入公共控件层，需要单独规划参数化 API、主题扩展和 widget 注册。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `heatmap_chart`：这里是离散格阵和焦点列，不是连续热区值。
- 区别于 `status_timeline`：这里是并行通道监看，不是阶段顺序链路。
- 区别于 `node_topology`：这里是固定列阵矩阵，不是节点连线网络。
