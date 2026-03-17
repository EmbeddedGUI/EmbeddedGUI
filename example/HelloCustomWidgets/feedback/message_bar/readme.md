# message_bar 自定义控件设计说明

## 1. 为什么需要这个控件
`message_bar` 用来表达页面级或容器级的轻量反馈信息，覆盖 `info / success / warning / error` 四种常见状态，适合设置页、表单页、同步页和后台管理页顶部的提示场景。

## 2. 为什么现有控件不够用

- `alert_banner` 更偏队列化告警条和多行扫描，不是 Fluent 风格的标准消息反馈条
- `notification_stack` 更偏多卡片堆叠，而不是单条页面反馈
- `skeleton_loader` 是内容占位，不承担反馈语义
- `chips`、`badge` 一类缺少标题、正文、动作按钮和关闭语义

因此需要一个更接近 Fluent `MessageBar / InfoBar` 的通用反馈控件，作为后续标准主线。

## 3. 目标场景与示例概览

- 主卡展示 4 个主状态：`Info`、`Success`、`Warning`、`Error`
- 左下 compact 卡展示紧凑版 `Warning / Error`
- 右下 persistent 卡展示只读、非关闭式的持久提示
- 页面通过点击主卡和 compact 卡切换状态，验证文本、动作按钮、关闭位和颜色语义

目录：

- `example/HelloCustomWidgets/feedback/message_bar/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> 主消息条 -> 状态文案 -> 分隔线 -> compact / persistent 双预览
- 主卡：`196 x 96`
- compact 卡：`104 x 82`
- persistent 卡：`104 x 82`，右下角带 `Pin` 持久态标识
- 视觉规则：
  - 浅色 page panel + 白底 message bar
  - 左侧 severity accent 条
  - leading icon 使用小圆形状态符号
  - 标题、正文、动作按钮层级清晰
  - closable 状态仅在主卡保留，compact 与 persistent 默认不显示关闭位

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Message Bar` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap bars to rotate severity` | 引导文案 |
| `bar_primary` | `egui_view_message_bar_t` | 196 x 96 | `Info` | 主消息条 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Info state active` | 当前状态文案 |
| `section_divider` | `egui_view_line_t` | 152 x 2 | visible | 分隔上下区域 |
| `bottom_row` | `egui_view_linearlayout_t` | 212 x 96 | enabled | 底部双列容器 |
| `compact_label` | `egui_view_label_t` | 108 x 12 | `Compact` | 紧凑版标题 |
| `bar_compact` | `egui_view_message_bar_t` | 104 x 82 | `Warning` | 紧凑预览 |
| `persistent_label` | `egui_view_label_t` | 108 x 12 | `Persistent` | 持久版标题 |
| `bar_persistent` | `egui_view_message_bar_t` | 104 x 82 | pinned | 持久只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | compact | persistent |
| --- | --- | --- | --- |
| 默认态 | `Info` | `Warning` | `Info pinned` |
| 点击主卡 | `Info -> Success -> Warning -> Error` 轮换 | 保持当前紧凑态 | 保持只读 |
| 点击 compact | 主卡保持当前态 | `Warning <-> Error` 轮换 | 保持只读 |
| 只读预览 | 不适用 | 不适用 | 持续显示 pinned 状态、无动作按钮、无关闭位 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 400ms，记录默认 `Info`
2. 点击主卡一次，切到 `Success`
3. 等待 250ms
4. 再点主卡，切到 `Warning`
5. 等待 250ms
6. 第三次点主卡，切到 `Error`
7. 等待 250ms
8. 点击 compact 卡，切换 compact 到 `Error`
9. 末尾等待 800ms，供 runtime 抓最终截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/message_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/message_bar --timeout 10 --keep-screenshots
```

验收重点：

- `Message Bar` 标题、主卡、底部双卡都必须完整可见
- `Info / Success / Warning / Error` 四态要一眼可分
- 标题、正文、动作按钮之间留白必须稳定，不能拥挤
- 主卡关闭位不能贴边，也不能压到标题
- compact 卡在小尺寸下仍需保持标题、正文和动作层次
- persistent 卡必须明显区别于可交互态

## 9. 已知限制与下一轮迭代计划

- 当前版本是固定尺寸布局，还未覆盖更长正文
- 关闭位目前只做视觉表现，不响应单独关闭动作
- 动作按钮宽度采用近似字符宽度估算，后续可继续微调
- 当前已完成 30 轮本地迭代验收；若后续需要上升到框架公共控件，再单独规划 `src/widget/` 与 UI Designer 侧接入

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `alert_banner`：这里强调标准消息反馈条，而不是多条告警队列
- 相比 `notification_stack`：这里不是卡片堆叠，而是单条页面反馈
- 相比 `skeleton_loader`：这里强调反馈状态和动作，不是占位
- 相比 `chips` / `badge`：这里有完整的标题、正文、动作和关闭位语义

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`MessageBar / InfoBar`
- 本次保留状态：
  - `info`
  - `success`
  - `warning`
  - `error`
  - `compact`
  - `persistent`

## 13. 相比参考原型删掉了哪些效果或装饰

- 删掉 Acrylic、阴影和更复杂的桌面动效
- 不做真实图标资源，仅保留圆形 severity glyph
- 不做多按钮组合，只保留单动作按钮
- 不做可展开正文与复杂布局换行

## 14. EGUI 适配时的简化点与约束

- 用固定尺寸和固定排版保证 `240 x 320` 下可读
- 使用 Montserrat 内置字体，不引入额外字体资源
- 颜色和线条表达尽量克制，避免回到 showcase 风格的重装饰路线
- 先完成最小可运行版本，再继续做留白与状态收敛
