# dock_launcher

## 1. 为什么需要这个控件

`dock_launcher` 用于表达桌面端、车机首页或工作台中的高频入口区域。它不仅要展示一排核心入口，还需要同时承载焦点放大、运行态指示、通知徽标与快速状态摘要。

## 2. 为什么现有控件不够用

- `breadcrumb_trail` 解决层级路径，不负责高频入口聚焦。
- `radial_menu` 适合中心触发和扇区选择，不适合横向停靠条。
- `button_matrix` 更像规则网格，不具备 dock 的焦点放大和运行态语义。
- `menu` 偏文本列表，不强调图标化停靠入口与当前焦点。

## 3. 目标场景与示例概览

- 主卡展示一条带焦点放大效果的停靠栏，中央入口高亮并带运行指示。
- 左侧 compact 卡展示收藏组的简化预览。
- 右侧 compact 卡展示最近任务组的只读预览。
- runtime 通过多组 snapshot 检查焦点图标、徽标、运行点和标题摘要的切换。

## 4. 视觉与布局规格

- 根布局：`220 x 306`
- 顶部标题：`220 x 18`
- 顶部说明：`220 x 11`
- 主卡：`192 x 118`
- 主卡状态说明：`220 x 12`
- 中部分隔线：`142 x 2`
- 底部容器：`212 x 86`
- 左侧 compact：`108 x 80`
- 右侧 compact：`100 x 80`
- 验收重点：
  - 顶部标题、状态胶囊、徽标与边框之间必须保留合理空隙。
  - 主卡焦点入口需要视觉居中，左右入口缩放节奏要平衡。
  - compact 卡的标题、徽标和底部摘要不能贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `220 x 306` | 垂直布局 | 页面骨架 |
| `title_label` | `egui_view_label_t` | `220 x 18` | `Dock Launcher` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `220 x 11` | `Tap cards to rotate launcher states` | 页面说明 |
| `dock_primary` | `egui_view_dock_launcher_t` | `192 x 118` | snapshot 0 | 主停靠栏 |
| `status_label` | `egui_view_label_t` | `220 x 12` | `Primary dock focus` | 主卡说明 |
| `section_divider` | `egui_view_line_t` | `142 x 2` | 静态 | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | `212 x 86` | 横向布局 | 承载两个 compact 卡 |
| `dock_favorites` | `egui_view_dock_launcher_t` | `108 x 80` | compact snapshot 0 | 收藏预览 |
| `dock_recent` | `egui_view_dock_launcher_t` | `100 x 80` | locked snapshot 0 | 最近任务预览 |

## 6. 状态覆盖矩阵

| 区域 | Snapshot | 状态文案 | 摘要 | 底部文案 | 说明 |
| --- | --- | --- | --- | --- | --- |
| 主卡 | 0 | `FOCUS` | `Creative lane armed` | `center tools online` | 默认焦点态 |
| 主卡 | 1 | `LIVE` | `Capture route running` | `stream queue active` | 运行中 |
| 主卡 | 2 | `READY` | `Playback lane standby` | `recent launch pinned` | 待命态 |
| 左卡 | 0 | `STAR` | `Pinned tools` | `starred` | 收藏预览 |
| 左卡 | 1 | `SCAN` | `Review lane` | `sorting` | 扫描中 |
| 左卡 | 2 | `STAR` | `Pinned set` | `saved` | 已保存 |
| 右卡 | 0 | `SAFE` | `Last launch` | `sealed` | 最近任务安全态 |
| 右卡 | 1 | `WARN` | `Return queue` | `guard` | 最近任务告警态 |
| 右卡 | 2 | `SAFE` | `Recall dock` | `sealed` | 回调预览 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 `400ms`。
2. 点击主卡，切换主卡 snapshot。
3. 等待 `300ms`。
4. 点击左侧 compact 卡，切换收藏预览 snapshot。
5. 等待 `300ms`。
6. 点击右侧 compact 卡，切换最近任务预览 snapshot。
7. 等待 `800ms` 输出关键截图。

## 8. 编译、runtime、截图验收标准

### 编译

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/dock_launcher PORT=pc
```

### Runtime

```bash
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/dock_launcher --timeout 10 --keep-screenshots
```

### 验收标准

- 运行不能黑屏、白屏、卡死或崩溃。
- 主卡和两个 compact 卡必须完整可见，不能被裁切。
- 焦点入口、运行态小点、通知徽标、标题与摘要都要检查视觉居中。
- 文字与圆角边框、胶囊、徽标之间必须保留合理空隙。
- 每轮截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中记录结论。

## 9. 已知限制与下一轮迭代计划

- 首版先聚焦静态渲染与 snapshot 切换，后续再看是否补充更强的缩放动画。
- 若状态文案继续增长，compact 卡需要重新评估标题和胶囊的比例。
- 如需沉淀到框架层，再单独规划通用 launcher item 数据结构。

## 10. 与现有控件的重叠分析与差异化边界

- 区别于 `breadcrumb_trail`：这里强调高频入口停靠和焦点放大，不是层级路径。
- 区别于 `radial_menu`：这里是横向停靠条，不是中心展开式菜单。
- 区别于 `button_matrix`：这里强调聚焦入口、运行态与徽标，不是规则矩阵按钮。
- 区别于 `notification_stack`：这里的核心是入口导航，通知只是附属状态层。
