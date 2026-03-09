# window_snap_grid

## 1. 为什么需要这个控件

`window_snap_grid` 用于表达桌面多窗口吸附、分屏预设和当前布局焦点。它需要同时展示屏幕预览、分区划分、当前高亮区域和布局摘要。

## 2. 为什么现有控件不够用

- `gridlayout` 只是通用布局容器，不负责布局预览语义。
- `seat_map` 是座位矩阵，不表达窗口吸附和屏幕分区。
- `kanban_board` 侧重泳道卡片，不处理分屏比例。
- `dock_launcher` 是入口停靠，不是窗口布局预设。

## 3. 目标场景与示例概览

- 主卡展示完整屏幕吸附预览，支持 tile / wide / quad 三种布局。
- 左侧 compact 卡展示保存的布局预设。
- 右侧 compact 卡展示最近使用的吸附布局。
- runtime 通过多组 snapshot 检查高亮分区、状态胶囊和摘要切换。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 顶部标题：`220 x 18`
- 顶部说明：`220 x 11`
- 主卡：`192 x 118`
- 状态说明：`220 x 12`
- 分隔线：`142 x 2`
- 底部容器：`216 x 86`
- 左侧 compact：`108 x 80`
- 右侧 compact：`104 x 80`

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Window Snap Grid` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap cards to rotate snap layouts` | 页面说明 |
| `snap_primary` | `egui_view_window_snap_grid_t` | `192 x 118` | snapshot 0 | 主布局预览 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary tile focus` | 状态说明 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `216 x 86` | 横向布局 | 承载两个 compact 卡 |
| `snap_saved` | `egui_view_window_snap_grid_t` | `108 x 80` | compact snapshot 0 | 保存布局预览 |
| `snap_recent` | `egui_view_window_snap_grid_t` | `104 x 80` | locked snapshot 0 | 最近布局预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态文案 | 摘要 | 底部文案 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `TILE` | `Dual stage armed` | `top focus on` | 双栏预设 |
| 主卡 | 1 | `WIDE` | `Triple lane spread` | `center strip on` | 三栏预设 |
| 主卡 | 2 | `QUAD` | `Four zone recall` | `quad map sealed` | 四宫格预设 |
| 左卡 | 0 | `PIN` | `Pinned split` | `saved preset` | 保存布局 |
| 左卡 | 1 | `SCAN` | `Review pack` | `sorting lanes` | 扫描布局 |
| 左卡 | 2 | `PIN` | `Pinned quad` | `safe hold` | 固定四宫格 |
| 右卡 | 0 | `LAST` | `Last dock` | `recent preset` | 最近布局 |
| 右卡 | 1 | `WARN` | `Hot restore` | `guard split` | 风险恢复 |
| 右卡 | 2 | `LAST` | `Recall quad` | `recent calm` | 最近恢复 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`。
2. 点击主卡，切换主卡 snapshot。
3. 等待 `300ms`。
4. 点击左侧 compact 卡，切换保存预设 snapshot。
5. 等待 `300ms`。
6. 点击右侧 compact 卡，切换最近预设 snapshot。
7. 等待 `800ms` 输出关键截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=layout/window_snap_grid PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/window_snap_grid --timeout 10 --keep-screenshots
```

### 验收标准

- 运行不能黑屏、卡死或崩溃。
- 主卡和两张 compact 卡的屏幕预览、分区块、状态胶囊必须完整可见。
- 顶部标题、状态短词、屏幕预览和底部提示都要检查视觉居中和留白。
- 每轮截图都要复制到 `iteration_log/images/iter_xx/` 并写入结论。

## 9. 已知限制与下一轮迭代计划

- 首版先跑通静态渲染和 snapshot 切换。
- 后续优先打磨分区块之间的间距和 compact 卡的纵向节奏。
- 若后续需要更强交互，再考虑拖拽式分区反馈。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `gridlayout`：这里强调的是布局预设和屏幕分区语义。
- 区别于 `seat_map`：这里不是座位矩阵，而是窗口吸附区域。
- 区别于 `kanban_board`：这里没有卡片泳道结构。
- 区别于 `dock_launcher`：这里不是入口导航，而是布局预览和焦点分区。
