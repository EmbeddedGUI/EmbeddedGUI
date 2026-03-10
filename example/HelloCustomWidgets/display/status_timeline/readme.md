# status_timeline 控件设计说明

## 1. 为什么需要这个控件？

`status_timeline` 用纵向节点时间线表达阶段推进顺序、当前焦点步骤和已完成进度，适合在小屏里展示任务流、审批流、发布流、巡检流这类带严格先后关系的状态链。

## 2. 为什么现有控件不够用？

- `list` 只能纵向罗列条目，缺少节点、连线和阶段推进语义。
- `table` 适合规则网格数据，不适合表达前后顺序和当前阶段。
- `stepper` 更偏输入型步骤控制，不强调展示型时间线和只读预览。
- chart 类控件强调趋势和统计，不强调流程节点与阶段切换。

差异化边界：`status_timeline` 的核心是“纵向节点 + 连线 + 当前阶段高亮 + 已完成/待处理区分 + compact/locked 双预览”，不是简单列表、表格或统计图。

## 3. 目标场景与示例概览

- primary：主时间线，展示完整四阶段流程，支持 `Flow A / Flow B` 点击切换。
- compact：紧凑预览，用更短文本验证小尺寸流程的可读性和 badge 居中。
- locked：只读预览，验证禁用态下仍能辨认流程结构，但不会响应点击。
- 页面文案：标题为 `Status Timeline`，引导语为 `Tap cards to cycle`，中部状态文案在 `Core A / Core B / Compact A / Compact B` 之间切换。

目录：`example/HelloCustomWidgets/display/status_timeline/`

## 4. 视觉与布局规格

- 屏幕基准：240 x 320。
- 根布局：`230 x 306`，垂直居中摆放。
- 主卡：`188 x 138`，带 panel shadow、outer border、inner border、顶部 header pill、底部 footer pill。
- 中部状态文案：`230 x 13`，用于反馈当前 primary/compact 状态。
- 底部分割线：`170 x 2`，区分主卡和预览区。
- 底部双列：整体 `230 x 106`，左右列各 `108 x 108`，内部卡片尺寸 `108 x 92`。
- compact 卡顶部使用居中 `A / B` badge，locked 卡顶部使用居中 `LK` badge，并带上下强调条。
- 视觉验收重点：
  - `Flow A / Flow B` header pill 必须真实居中。
  - `Stage A / Stage B` footer pill 必须真实居中。
  - compact `A / B` badge 与 locked `LK` badge 必须真实居中。
  - `Compact`、`Locked`、`Core A/B`、`Compact A/B` 与边框必须保留安全距离，不能贴边或左右内边距失衡。
  - 底部双卡必须完整可见，不能裁切。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 230 x 306 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 230 x 18 | `Status Timeline` | 标题 |
| `guide_label` | `egui_view_label_t` | 230 x 12 | `Tap cards to cycle` | 交互提示 |
| `timeline_primary` | `egui_view_status_timeline_t` | 188 x 138 | `Flow A` | 主时间线 |
| `status_label` | `egui_view_label_t` | 230 x 13 | `Core A` | 中部状态反馈 |
| `compact_label` | `egui_view_label_t` | 108 x 13 | `Compact` | 左下预览标题 |
| `timeline_compact` | `egui_view_status_timeline_t` | 108 x 92 | `Compact A` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 108 x 13 | `Locked` | 右下预览标题 |
| `timeline_locked` | `egui_view_status_timeline_t` | 108 x 92 | `LK` / disabled | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 能力 | 覆盖方式 | 预期结果 |
| --- | --- | --- |
| 默认态 | 初始渲染 | 主卡显示 `Flow A` / `Stage A` / `Core A` |
| primary 切换态 | 点击主卡 | 切到 `Flow B` / `Stage B` / `Core B`，焦点步骤同步变化 |
| compact 切换态 | 点击 compact 卡 | 切到 `Compact B`，badge 从 `A` 变成 `B`，状态文案切到 `Compact B` |
| locked 只读态 | 点击 locked 卡 | 无状态变化，保持禁用遮罩与交叉线 |
| 居中与边距复核 | runtime 截图检查 | header/footer/badge/短词文案都保持视觉居中和安全留白 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始等待 400ms，输出默认态截图。
2. 点击 `timeline_primary`，切到 `Flow B`。
3. 等待 300ms，记录 primary 切换后的稳定帧。
4. 点击 `timeline_compact`，切到 `Compact B`。
5. 等待 300ms，记录 compact 切换后的稳定帧。
6. 点击 `timeline_locked`，验证 disabled 预览不响应。
7. 最后等待 800ms，输出最终稳定帧。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=display/status_timeline PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/status_timeline --timeout 10 --keep-screenshots
```

验收要求：
- `make` 必须通过，无编译或链接错误。
- runtime 必须输出 `ALL PASSED`。
- 主卡、compact 卡、locked 卡必须完整可见，不允许裁切或黑屏。
- `Flow A / Flow B` header pill、`Stage A / Stage B` footer pill、`A / B / LK` badge 必须通过截图复核为真实居中。
- `Compact`、`Locked`、`Core A/B`、`Compact A/B` 这些短词必须与边框保留合理空隙，不能贴边或左右留白失衡。
- 关键截图必须复制到 `iteration_log/images/iter_xx/`，并在 `iteration_log/iteration_log.md` 中用相对路径引用。

## 9. 已知限制与下一轮迭代计划

- 当前只支持静态点击切换，不支持拖拽排序或长流程滚动。
- 节点标题按短文本设计，不处理超长标题折行。
- disabled 态主要验证灰化和不可交互，不展示失败态或异常态。
- 后续可扩展时间戳、阶段耗时、错误节点和展开详情卡片。

## 10. 与现有控件的重叠分析与差异化边界

- 与 `list`：`status_timeline` 强调节点、连线、阶段推进和焦点步骤，不是简单列表。
- 与 `table`：`status_timeline` 不做规则网格，而是做纵向流程链。
- 与 `stepper`：`status_timeline` 偏展示型流程摘要，不承担输入控制职责。
- 与 chart 类控件：`status_timeline` 展示流程推进，不展示数值趋势。
