# server_rack 自定义控件

## 1. 为什么需要这个控件

`server_rack` 用来表达机架式设备的槽位、负载和告警分布，适合在很小的屏幕里同时展示设备层级、热点槽位和只读对照态。

## 2. 为什么现有控件不够用

- `list` 只能顺序列项，缺少机架槽位和焦点机位语义。
- `progress_bar` 只能表达单条数值，无法同时表现多槽位负载和状态灯。
- `notification_stack` 强调消息层级，不适合表现固定机架结构。
- `node_topology` 强调网状依赖关系，不适合垂直机架单元浏览。

因此需要一个同时具备机架外框、槽位列表、负载条、状态灯和 compact / locked 对照态的复合控件。

## 3. 目标场景与示例概览

- 主卡展示 `Rack A / Rack B` 两组机架快照，点击后轮换负载布局。
- 左下 compact 卡展示 `Compact A / Compact B`，用于快速预览焦点机位。
- 右下 locked 卡展示禁用态，提供审计或只读对照。
- 页面顶部文案为 `Server Rack` 和 `Tap racks to rotate load`，中间状态行为 `Rack A load`、`Rack B load`、`Compact A rack`、`Compact B rack`。

目录：

- `example/HelloCustomWidgets/display/server_rack/`

## 4. 视觉与布局规格

- 画布为 `240 x 320`。
- 根布局 `root_layout` 为 `220 x 304`，整体垂直居中摆放。
- 主机架卡 `rack_primary` 为 `176 x 132`，包含机架标题、5 个槽位、负载条和状态灯。
- 状态文本 `status_label` 为 `220 x 14`，位于主卡下方并与主卡保持 4px 间距。
- 分隔线 `section_divider` 为 `148 x 2`，用于切开主卡与底部双卡区域。
- 底部双列各为 `106 x 108`，上方标题 12px，高度与左右边距保持平衡。
- `Rack A / Rack B`、`Compact A / Compact B`、`Locked` 这类短词必须检查视觉居中，不能出现左右留白不均或文字贴边。
- 机架内缩写标签如 `GW`、`API`、`DB`、`AUTH`、`IDX`、`JOB` 需要保证与负载条、状态灯之间留有安全距离。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 220 x 304 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 220 x 18 | `Server Rack` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 220 x 12 | `Tap racks to rotate load` | 引导说明 |
| `rack_primary` | `egui_view_server_rack_t` | 176 x 132 | `Rack A` | 主机架卡 |
| `status_label` | `egui_view_label_t` | 220 x 14 | `Rack A load` | 当前状态摘要 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 主卡与底部区域分隔 |
| `bottom_row` | `egui_view_linearlayout_t` | 220 x 104 | enabled | 底部双列容器 |
| `compact_label` | `egui_view_label_t` | 106 x 12 | `Compact A` | compact 标题 |
| `rack_compact` | `egui_view_server_rack_t` | 106 x 92 | `Compact A` | compact 机架预览 |
| `locked_label` | `egui_view_label_t` | 106 x 12 | `Locked` | locked 标题 |
| `rack_locked` | `egui_view_server_rack_t` | 106 x 92 | disabled | 只读机架预览 |

## 6. 状态覆盖矩阵

| 状态 / 页面 | 主机架 | compact 机架 | locked 机架 |
| --- | --- | --- | --- |
| 默认态 | `Rack A`，焦点机位为 `DB` | `Compact A`，焦点机位为第 3 槽 | disabled |
| 点击主卡 | 切换到 `Rack B`，焦点切换为 `AUTH` | 保持当前 compact 快照，但取消高亮态 | disabled |
| 点击 compact 卡 | 主卡保持当前快照 | 切换到 `Compact B` 并切换高亮配色 | disabled |
| 只读对照 | 不响应 | 不响应 | 始终保留遮罩、斜线和灰化效果 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧静置 400ms，确认默认态完整渲染。
2. 点击主机架，切换到 `Rack B`。
3. 等待 300ms，确认主卡负载和状态文本已更新。
4. 点击 compact 卡，切换到 `Compact B`。
5. 再等待 300ms，确认 compact 机架高亮配色已生效。
6. 保持 locked 卡不交互，只验证只读态稳定渲染。
7. 最后停留 800ms，供 runtime 截图抓取最终状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=display/server_rack PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/server_rack --timeout 10 --keep-screenshots
```

验收标准：

- `make` 必须通过。
- runtime 必须输出 `ALL PASSED`。
- 主卡、compact 卡、locked 卡都必须完整可见，不能裁切。
- `Server Rack`、`Tap racks to rotate load`、`Rack A/B`、`Compact A/B`、`Locked` 需要逐项确认视觉居中。
- 机架缩写、状态文本与机架边框之间要保留合理留白，不能出现贴边或某一侧内边距明显偏小。
- 关键截图必须归档到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中使用相对路径记录。

## 9. 已知限制与下一轮迭代计划

- 当前只提供两组主卡快照和两组 compact 快照，后续可扩展到更多机架模板。
- 目前负载条为离散宽度映射，后续可考虑接入实时数据。
- locked 卡只做静态只读展示，后续可补充告警闪烁或维护标记。
- 若后续引入更多短词标题，仍需继续检查文字视觉居中与边框留白。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `progress_bar`，本控件强调多槽位机架结构，而不是单一数值条。
- 相比 `node_topology`，本控件强调垂直机架单元和负载分层，而不是网状连线关系。
- 相比 `list`，本控件强调固定设备槽位布局，而不是纯文本列表。
- 相比 `notification_stack`，本控件强调机架健康态和焦点机位，而不是多张消息卡层级。
