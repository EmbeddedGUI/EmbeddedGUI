# boxplot_chart 控件说明

## 1. 为什么需要这个控件

`boxplot_chart` 用最小值、Q1、中位数、Q3、最大值这组五数概括表达分布形态，适合在小屏里快速比较离散程度、上下须范围和中位趋势。

## 2. 为什么现有控件不够用

- `chart_line` 和 `chart_bar` 更适合连续值或单值对比，不能在一个柱位里同时表达五数概括。
- `candlestick_chart` 语义偏向 OHLC 时序数据，不等同于统计分位分布。
- `heatmap_chart`、`treemap_chart` 强调密度或占比，不直接表达分位结构。
- 本控件的差异边界是“统计分位结构 + 主卡/紧凑卡/只读卡的分层预览”。

## 3. 目标场景与示例概览

- primary：主箱线图卡片，展示 5 组数据，并可在 `Profile A / Profile B` 之间切换。
- compact：底部左侧紧凑预览卡，重点验证小尺寸下的箱体、须线和顶部 badge 是否仍可读。
- locked：底部右侧只读预览卡，验证 disabled 语义下仍能保持结构辨识。
- 页面入口文案为 `Tap cards to cycle`，录制动作依次点击 primary、compact、locked。

## 4. 视觉与布局规格

- 目标屏幕：240 x 320。
- 根布局：`230 x 308`，整体垂直居中。
- 主卡：`188 x 142`，包含阴影、外描边、内描边、居中 header pill 和居中 footer pill。
- 底部双卡：每列 `108 x 110`，内部箱线图卡 `108 x 94`。
- 关键检查项：
  - `Profile A / Profile B` 顶部胶囊必须真实视觉居中。
  - `Range A / Range B` footer 胶囊必须左右留白对称，文字不能贴边。
  - compact 顶部 `A / B` badge、locked 顶部 `LK` badge 必须居中。
  - `Compact`、`Locked` 标题与卡片边框之间保持稳定安全距离。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 230 x 308 | enabled | 页面根布局 |
| `boxplot_primary` | `egui_view_boxplot_chart_t` | 188 x 142 | `Profile A` | 主箱线图卡片 |
| `boxplot_compact` | `egui_view_boxplot_chart_t` | 108 x 94 | `A` | 紧凑预览卡 |
| `boxplot_locked` | `egui_view_boxplot_chart_t` | 108 x 94 | `LK` / disabled | 只读预览卡 |
| `status_label` | `egui_view_label_t` | 230 x 13 | `Core A` | 外部状态反馈 |
| `compact_label` | `egui_view_label_t` | 108 x 13 | `Compact` | 紧凑卡标题 |
| `locked_label` | `egui_view_label_t` | 108 x 13 | `Locked` | 只读卡标题 |

## 6. 状态覆盖矩阵

| 区域 | 默认态 | 切换态 | 只读态 |
| --- | --- | --- | --- |
| primary | `Profile A` + `Range A` + `Core A` | 点击后切到 `Profile B` + `Range B` + `Core B` | - |
| compact | `A` badge，标签默认收敛 | 点击后切到 `B` badge，并点亮暖色强调 | - |
| locked | `LK` badge，灰阶结构保持可读 | 点击不响应 | 始终为 disabled 预览 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待 400ms，截取默认态。
2. 点击 primary，切换到 `Profile B`。
3. 等待 300ms，截取主卡切换态。
4. 点击 compact，切换到 `B` badge 的焦点态。
5. 等待 300ms，截取紧凑卡切换态。
6. 点击 locked，确认只读卡不响应。
7. 再等待 800ms，输出最终收口截图。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=chart/boxplot_chart PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub chart/boxplot_chart --timeout 10 --keep-screenshots
```

验收要求：

- `make` 必须通过。
- runtime 必须为 `ALL PASSED`。
- 每轮截图都要归档到 `iteration_log/images/iter_xx/`。
- 必须逐轮检查 header pill、footer pill、compact/locked badge、`Compact` / `Locked` 标题的视觉居中与边距。
- 必须逐轮检查短文本和边框的安全距离，不能只看“没裁切”。

## 9. 已知限制与下一轮迭代计划

- 当前版本只覆盖两组 profile，没有加入 outlier 散点和更多分组数。
- 坐标轴刻度没有展开为更完整的统计标尺，当前优先保证小屏识别度。
- 如果后续升级到框架级公共控件，可继续抽象异常点、网格线和更多主题配色接口。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `candlestick_chart`：`boxplot_chart` 表达的是统计分位，不是开高低收。
- 相比 `chart_line` / `chart_bar`：`boxplot_chart` 强调分布宽度和中位位置，不是趋势线或单值高低。
- 相比 `heatmap_chart` / `treemap_chart`：`boxplot_chart` 不做面积编码或热区编码，而是围绕五数概括建立阅读路径。
- 相比普通卡片类控件：本控件的核心不是容器造型，而是“统计结构可读性 + 多态预览一致性”。
