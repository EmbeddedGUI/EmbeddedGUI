# treemap_chart 控件说明

## 1. 为什么需要这个控件

`treemap_chart` 用矩形面积直接表达总量固定场景下的资源分配关系，适合在 240x320 这种小屏里快速看出谁占比更大、谁被压缩得更明显。

## 2. 为什么现有控件不够用

- `chart_bar` 更适合长度对比，不擅长表达“总面积切分”。
- `chart_pie` 在小屏里标签容易拥挤，弱小分区的阅读效率不高。
- `table` 适合精确数值，不适合先看整体分配结构。
- `radar_chart` 表达多维轮廓，不表达总量固定下的空间分配。

本控件的差异边界是：用矩形面积编码占比，并在主卡、紧凑卡、只读卡三层预览里保持一致的阅读路径。

## 3. 目标场景与示例概览

- primary：主 treemap 卡片，展示 5 个模块在 `Profile A / Profile B` 两组配置下的面积分配差异。
- compact：底部左侧紧凑预览卡，重点验证小尺寸下矩形块、顶部 badge 和外部标题是否仍可读。
- locked：底部右侧只读预览卡，验证 disabled 语义下仍能保持块结构辨识。
- 页面引导文案为 `Tap cards to cycle`，录制动作依次点击 primary、compact、locked。

## 4. 视觉与布局规格

- 目标屏幕：240 x 320。
- 根布局：`230 x 300`，整体垂直居中。
- 主卡：`188 x 138`，包含阴影、外描边、内描边、居中 header pill 和居中 footer pill。
- 状态栏：`230 x 13`，使用 `Core A / Core B / Compact A / Compact B` 这类短词，避免贴边。
- 底部双卡：整体 `230 x 104`，每列 `108 x 106`，内部 treemap 卡 `108 x 90`。
- 关键检查项：
  - 顶部 `Profile A / Profile B` 胶囊必须真实视觉居中。
  - 底部 `Load A / Load B` 胶囊必须左右留白对称。
  - compact 顶部 `A / B` badge、locked 顶部 `LK` badge 必须真实居中。
  - `Compact`、`Locked` 标题与卡片边框之间必须保留安全距离，不能贴边。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 230 x 300 | enabled | 页面根布局 |
| `treemap_primary` | `egui_view_treemap_chart_t` | 188 x 138 | `Profile A` | 主 treemap 卡片 |
| `treemap_compact` | `egui_view_treemap_chart_t` | 108 x 90 | `A` | 紧凑预览卡 |
| `treemap_disabled` | `egui_view_treemap_chart_t` | 108 x 90 | `LK` / disabled | 只读预览卡 |
| `status_label` | `egui_view_label_t` | 230 x 13 | `Core A` | 外部状态反馈 |
| `compact_label` | `egui_view_label_t` | 108 x 13 | `Compact` | 紧凑卡标题 |
| `disabled_label` | `egui_view_label_t` | 108 x 13 | `Locked` | 只读卡标题 |

## 6. 状态覆盖矩阵

| 区域 | 默认态 | 切换态 | 只读态 |
| --- | --- | --- | --- |
| primary | `Profile A` + `Load A` + `Core A` | 点击后切到 `Profile B` + `Load B` + `Core B` | - |
| compact | `A` badge，标签默认收敛 | 点击后切到 `B` badge，并点亮暖色强调 | - |
| locked | `LK` badge，灰阶结构保持可读 | 点击不响应 | 始终为 disabled 预览 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待 400ms，截取默认态。
2. 点击 primary，切换到 `Profile B`。
3. 等待 300ms，截取主卡切换态。
4. 点击 compact，切换到 `Compact B`。
5. 等待 300ms，截取紧凑卡切换态。
6. 点击 locked，确认只读卡不响应。
7. 再等待 800ms，输出最终收口截图。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=chart/treemap_chart PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub chart/treemap_chart --timeout 10 --keep-screenshots
```

验收要求：

- `make` 必须通过。
- runtime 必须为 `ALL PASSED`。
- 关键截图必须归档到 `iteration_log/images/iter_xx/`。
- 逐轮检查 header pill、footer pill、compact/locked badge、`Compact` / `Locked` 标题的视觉居中与边距。
- 逐轮检查短文本与边框之间是否保留了安全距离，不能只看“没有裁切”。

## 9. 已知限制与下一轮迭代计划

- 当前切分算法仍是轻量的 slice-and-dice 递归分割，没有升级到更复杂的平衡 treemap 算法。
- 当前块内标签只保留简写策略，没有额外叠加数值文本。
- 如果后续升级到框架级公共控件，可以继续补异常高亮、更多主题配色和块内摘要接口。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `chart_bar`：`treemap_chart` 表达的是面积占比，不是长度对比。
- 相比 `chart_pie`：`treemap_chart` 在小屏里更适合做矩形块阅读和紧凑态预览。
- 相比 `table`：`treemap_chart` 先强调整体分配结构，再做局部阅读。
- 相比 `radar_chart`：`treemap_chart` 的核心不是多维轮廓，而是总量固定下的面积切分关系。
