# kanban_board 控件设计说明

## 1. 为什么需要这个控件？

`kanban_board` 用多列泳道展示任务在不同阶段的堆叠状态，适合在 240x320 小屏上快速查看工作流负载、焦点列和收敛状态。它强调“列结构 + 卡片堆叠 + 焦点列”的组合信息，而不是单值、趋势线或表格明细。

## 2. 为什么现有控件不够用？

- `table` 更适合规则网格数据，不适合表达列间流转和不同泳道密度。
- `list` 适合单列纵向浏览，不适合并列对比多个阶段。
- `gridlayout` 只能负责布局，不能直接表达卡片堆叠、WIP 提示和焦点列语义。
- 现有 chart 类控件强调统计或趋势，不表达任务编排关系。

差异化边界：`kanban_board` 的核心是“多泳道任务流 + 卡片堆叠 + 焦点列强调 + compact/locked 对照预览”，不是表格、列表或图表的换皮。

## 3. 目标场景与示例概览

- primary：主看板，展示三列泳道，点击后在 `Sprint A / Sprint B` 间切换。
- compact：紧凑预览，保留列结构但压缩文案密度，用于验证小尺寸可读性。
- locked：只读预览，验证禁用态下仍能看出泳道结构，但不会响应点击。
- 页面文案：标题为 `Kanban Board`，引导语为 `Tap boards to rotate focus`，中部状态文案在 `Sprint A focus / Sprint B focus / Compact A lane / Compact B lane` 之间切换。

目录：`example/HelloCustomWidgets/layout/kanban_board/`

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 根布局：`220 x 304`，垂直居中摆放。
- 主看板：`176 x 132`，重点展示完整三列泳道结构。
- 中部状态文案：`220 x 14`，用于反馈当前焦点来源。
- 底部分割线：`148 x 2`，区分主看板和预览区。
- 底部双列：整体 `220 x 104`，左右列各 `106 x 108`，内部卡片尺寸 `106 x 92`。
- 每列包含顶部短词标题、右上角 WIP chip 和最多 3 张卡片；compact / locked 预览沿用三列结构。
- 视觉验收重点：
  - `Sprint A / Sprint B`、`Compact A / Compact B`、`Locked` 这些短词必须居中且与边框保留安全距离。
  - lane 顶部标题与 WIP chip 不能贴边，左右内边距需要平衡。
  - compact / locked 两个底部预览必须完整可见，不能裁切。
  - card 标题、`+1 / +2` 等短词不能贴边，焦点列描边要清晰但不过度压迫。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 220 x 304 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 220 x 18 | `Kanban Board` | 标题 |
| `guide_label` | `egui_view_label_t` | 220 x 12 | `Tap boards to rotate focus` | 交互提示 |
| `board_primary` | `egui_view_kanban_board_t` | 176 x 132 | `Sprint A` | 主看板 |
| `status_label` | `egui_view_label_t` | 220 x 14 | `Sprint A focus` | 中部状态反馈 |
| `compact_label` | `egui_view_label_t` | 106 x 12 | `Compact A` | 左下预览标题 |
| `board_compact` | `egui_view_kanban_board_t` | 106 x 92 | `Preview A` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 106 x 12 | `Locked` | 右下预览标题 |
| `board_locked` | `egui_view_kanban_board_t` | 106 x 92 | disabled | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 能力 | 覆盖方式 | 预期结果 |
| --- | --- | --- |
| 默认态 | 初始渲染 | 主看板显示 `Sprint A`，焦点列在中间泳道 |
| primary 切换态 | 点击主看板 | 在 `Sprint A / Sprint B` 间切换，并切换焦点列 |
| compact 切换态 | 点击 compact 看板 | 切到 `Compact B lane`，同时点亮不同列 |
| locked 只读态 | 点击 locked 看板 | 无状态变化，保持禁用遮罩与交叉线 |
| 居中与边距复核 | runtime 截图检查 | 标题短词、WIP chip、卡片短词都保持合理留白 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始等待 400ms，输出默认态截图。
2. 点击主看板，切到 `Sprint B`。
3. 等待 300ms，记录主看板切换后的稳定帧。
4. 点击紧凑看板，切到 `Compact B`。
5. 等待 300ms，记录 compact 切换后的稳定帧。
6. 点击 locked 看板，验证 disabled 预览无响应。
7. 最后等待 800ms，输出最终稳定帧。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=layout/kanban_board PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/kanban_board --timeout 10 --keep-screenshots
```

验收要求：
- `make` 必须通过，无编译或链接错误。
- runtime 必须输出 `ALL PASSED`。
- 截图中主看板、compact 看板、locked 看板均完整可见。
- lane 标题、WIP chip、`Compact A/B`、`Locked`、`Sprint A/B focus` 等关键短词必须经过居中和边距复核。
- 焦点列、卡片堆叠、禁用态交叉线语义必须可辨认。
- 关键截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中用相对路径引用。

## 9. 已知限制与下一轮迭代计划

- 当前以固定列数和固定可见卡片数为主，不做滚动和拖拽排序。
- 紧凑态使用短文案，不覆盖长任务名自动折行。
- locked 预览只验证禁用语义，不展示失败列或阻塞列。
- 后续可扩展列折叠、拖拽排序、超长泳道滚动和泳道百分比提示。

## 10. 与现有控件的重叠分析与差异化边界

- 与 `table`：`kanban_board` 不是规则单元格矩阵，而是强调阶段流转与卡片堆叠。
- 与 `list`：`kanban_board` 不是单列浏览，而是多列并列比较。
- 与 `gridlayout`：`gridlayout` 仅处理摆放；`kanban_board` 还承担泳道状态、焦点列和卡片摘要语义。
- 与 chart 类控件：`kanban_board` 展示任务流和密度，不展示趋势或统计值。
