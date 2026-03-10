# pin_cluster 自定义控件

## 1. 为什么需要这个控件

`pin_cluster` 用于表达地图或平面示意图上的“点位聚合 + 当前焦点点位 + compact / locked 对照预览”，适合在小屏里快速确认区域、热点和选中点位信息。

## 2. 为什么现有控件不够用

- `avatar_stack` 强调头像重叠，不适合表达二维区域点位与聚合计数。
- `tag_cloud` 强调词权重，不适合表达空间位置语义。
- `node_topology` 强调连线依赖，不适合做点位聚合与选中信息卡。
- `notification_stack` 强调消息卡层级，不适合空间点位概览。

因此需要一个同时具备区域标题、聚合点、选中 pin、计数 badge 以及 compact / locked 预览的装饰型控件。

## 3. 目标场景与示例概览

- 主卡：展示 `Zone A / Zone B` 两组点位快照，点击后轮换焦点点位和 badge。
- 左下 compact：展示 `Compact A / Compact B` 点位预览，用于对照小尺寸信息可读性。
- 右下 locked：展示禁用态，只读对照。
- 中部状态行为 `Zone A pin` / `Zone B pin` / `Compact A pins` / `Compact B pins` 等摘要。

目录：

- `example/HelloCustomWidgets/decoration/pin_cluster/`

## 4. 视觉与布局规格

- 画布为 `240 x 320`。
- 根布局 `root_layout` 为 `220 x 304`，整体垂直居中摆放。
- 主卡 `cluster_primary` 为 `176 x 132`，包含区域标题、聚合点位、焦点 pin 与计数 badge。
- compact / locked 卡为 `106 x 92`，标题行 12px，高度与左右边距保持平衡。
- `Zone A/B`、`Compact A/B`、`Locked`、点位 badge 数字等短词必须检查视觉居中与左右留白，不能出现贴边或内边距失衡。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 220 x 304 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 220 x 18 | `Pin Cluster` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 220 x 12 | `Tap pins to rotate zone` | 引导说明 |
| `cluster_primary` | `egui_view_pin_cluster_t` | 176 x 132 | `Zone A` | 主点位聚合卡 |
| `status_label` | `egui_view_label_t` | 220 x 14 | `Zone A pin` | 当前状态摘要 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 主卡与底部区域分隔 |
| `bottom_row` | `egui_view_linearlayout_t` | 220 x 104 | enabled | 底部双列容器 |
| `compact_label` | `egui_view_label_t` | 106 x 12 | `Compact A` | compact 标题 |
| `cluster_compact` | `egui_view_pin_cluster_t` | 106 x 92 | `Compact A` | compact 点位预览 |
| `locked_label` | `egui_view_label_t` | 106 x 12 | `Locked` | locked 标题 |
| `cluster_locked` | `egui_view_pin_cluster_t` | 106 x 92 | disabled | 只读点位预览 |

## 6. 状态覆盖矩阵

| 状态 / 页面 | 主卡 | compact 卡 | locked 卡 |
| --- | --- | --- | --- |
| 默认态 | `Zone A`，焦点点位为 `N3` | `Compact A` | disabled |
| 点击主卡 | 切换到 `Zone B`，聚合 badge 与焦点点位变化 | 恢复普通配色 | disabled |
| 点击 compact 卡 | 主卡保持当前快照 | 切换到 `Compact B` 并切换高亮配色 | disabled |
| 只读对照 | 不响应 | 不响应 | 始终灰化并保持只读覆盖效果 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧静置 400ms，确认默认态完整渲染。
2. 点击主卡，切换到 `Zone B`。
3. 等待 300ms，确认主卡焦点 pin 与状态文本已更新。
4. 点击 compact 卡，切换到 `Compact B`。
5. 等待 300ms，确认 compact 预览高亮配色已生效。
6. 点击 locked 卡（disabled），验证无响应。
7. 最后停留 800ms，供 runtime 截图抓取最终状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=decoration/pin_cluster PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub decoration/pin_cluster --timeout 10 --keep-screenshots
```

验收标准：

- `make` 必须通过。
- runtime 必须输出 `ALL PASSED`。
- 主卡、compact 卡、locked 卡都必须完整可见，不能裁切。
- 重点检查：区域标题、badge 数字、短词标题的视觉居中；文字与边框之间必须保留合理留白。
- 关键截图必须归档到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中使用相对路径记录。

## 9. 已知限制与下一轮迭代计划

- 当前聚合点和 pin 布局由快照固定驱动，后续可扩展到更多区域模板或运行时数据输入。
- 目前 badge 为固定宽度绘制，后续可评估更大数字的自适应。
- locked 卡为静态只读展示，后续可加入更明确的锁定标记或审计信息。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `avatar_stack`，本控件强调二维点位与聚合语义，而不是头像重叠。
- 相比 `tag_cloud`，本控件强调空间位置和焦点点位，而不是词权重。
- 相比 `node_topology`，本控件强调点位聚合与选中 pin，而不是连线依赖网络。
- 相比 `notification_stack`，本控件强调点位概览，而不是消息卡层级。
