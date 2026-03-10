# alert_banner 自定义控件

## 1. 为什么需要这个控件

`alert_banner` 用来在紧凑屏幕里表达告警队列、严重级别、确认状态和只读对照态，适合需要快速扫读多条横幅告警的场景。

## 2. 为什么现有控件不够用

- `notification_stack` 更强调卡片层叠，不适合固定行高的横幅扫读。
- `status_timeline` 强调阶段链路，不适合并列告警列表。
- `chips` 和 `list` 缺少严重级别色条、右侧 badge 和焦点横幅语义。
- `tag_cloud` 适合离散权重词，不适合高密度告警行。

因此需要一个同时具备横幅队列、严重级别、badge、focus 行和 compact / locked 对照态的复合控件。

## 3. 目标场景与示例概览

- 主卡展示 `Queue A / Queue B` 两组告警队列，点击后轮换焦点告警。
- 左下 compact 卡展示 `Compact A / Compact B`，用于快速浏览短告警摘要。
- 右下 locked 卡展示禁用态，提供审计或只读对照。
- 页面顶部文案为 `Alert Banner` 和 `Tap banners to rotate queue`，中间状态行为 `Queue A focus`、`Queue B focus`、`Compact A alerts`、`Compact B alerts`。

目录：

- `example/HelloCustomWidgets/feedback/alert_banner/`

## 4. 视觉与布局规格

- 画布为 `240 x 320`。
- 根布局 `root_layout` 为 `220 x 304`，整体垂直居中摆放。
- 主告警卡 `banner_primary` 为 `176 x 132`，包含标题、4 条横幅、右侧 badge 和焦点描边。
- 状态文本 `status_label` 为 `220 x 14`，位于主卡下方并与主卡保持 4px 间距。
- 分隔线 `section_divider` 为 `148 x 2`，用于切开主卡与底部双卡区域。
- 底部双列各为 `106 x 108`，上方标题 12px，高度与左右边距保持平衡。
- `Queue A/B`、`Compact A/B`、`Locked` 以及 `OBS`、`WRN`、`HOT`、`INF`、`Lag`、`Burst`、`Ack` 等短词都必须检查视觉居中和左右留白。
- 横幅标题与右侧 badge 之间需要保留合理空隙，不能出现 badge 贴边或文字挤压。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 220 x 304 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 220 x 18 | `Alert Banner` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 220 x 12 | `Tap banners to rotate queue` | 引导说明 |
| `banner_primary` | `egui_view_alert_banner_t` | 176 x 132 | `Queue A` | 主告警卡 |
| `status_label` | `egui_view_label_t` | 220 x 14 | `Queue A focus` | 当前状态摘要 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 主卡与底部区域分隔 |
| `bottom_row` | `egui_view_linearlayout_t` | 220 x 104 | enabled | 底部双列容器 |
| `compact_label` | `egui_view_label_t` | 106 x 12 | `Compact A` | compact 标题 |
| `banner_compact` | `egui_view_alert_banner_t` | 106 x 92 | `Compact A` | compact 告警预览 |
| `locked_label` | `egui_view_label_t` | 106 x 12 | `Locked` | locked 标题 |
| `banner_locked` | `egui_view_alert_banner_t` | 106 x 92 | disabled | 只读告警预览 |

## 6. 状态覆盖矩阵

| 状态 / 页面 | 主告警卡 | compact 告警卡 | locked 告警卡 |
| --- | --- | --- | --- |
| 默认态 | `Queue A`，焦点行为 `DB lag peak` | `Compact A`，焦点行为 `Lag` | disabled |
| 点击主卡 | 切换到 `Queue B`，焦点切换为 `Queue drain` | 保持当前 compact 快照，但恢复普通配色 | disabled |
| 点击 compact 卡 | 主卡保持当前快照 | 切换到 `Compact B` 并切换高亮配色 | disabled |
| 只读对照 | 不响应 | 不响应 | 始终保留灰化和斜线遮罩效果 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧静置 400ms，确认默认态完整渲染。
2. 点击主告警卡，切换到 `Queue B`。
3. 等待 300ms，确认主卡焦点和状态文本已更新。
4. 点击 compact 卡，切换到 `Compact B`。
5. 再等待 300ms，确认 compact 告警高亮配色已生效。
6. 保持 locked 卡不交互，只验证只读态稳定渲染。
7. 最后停留 800ms，供 runtime 截图抓取最终状态。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/alert_banner PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/alert_banner --timeout 10 --keep-screenshots
```

验收标准：

- `make` 必须通过。
- runtime 必须输出 `ALL PASSED`。
- 主卡、compact 卡、locked 卡都必须完整可见，不能裁切。
- `Alert Banner`、`Queue A/B`、`Compact A/B`、`Locked` 以及 `OBS/WRN/HOT/INF` 等短词都要逐项确认视觉居中。
- 横幅标题、badge 和边框之间要保留合理留白，不能出现 badge 贴右边或标题被挤压。
- 关键截图必须归档到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中使用相对路径记录。

## 9. 已知限制与下一轮迭代计划

- 当前只提供两组主卡快照和两组 compact 快照，后续可扩展到更多告警模板。
- 目前 badge 宽度采用固定布局，后续可考虑更长短词的自适应宽度。
- locked 卡只做静态只读展示，后续可补充维护模式或升级态标记。
- 若后续增加更多短词 badge，仍需继续检查文字视觉居中与边框留白。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `notification_stack`，本控件强调多条横幅队列，而不是卡片前后层级。
- 相比 `status_timeline`，本控件强调并列告警扫描，而不是阶段顺序链路。
- 相比 `list`，本控件强调严重级别、badge 和焦点横幅语义，而不是普通文本行。
- 相比 `chips`，本控件强调整条告警横幅与队列关系，而不是独立标签集合。
