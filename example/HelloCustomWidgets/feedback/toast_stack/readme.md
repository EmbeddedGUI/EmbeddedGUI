# toast_stack 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`Toast / Snackbar`
- 本次保留状态：`info`、`success`、`warning`、`error`、`compact`、`read only`
- 删除效果：系统级阴影、Acrylic、自动入场/退场动画、真实图标资源、复杂手势关闭
- EGUI 适配说明：保留轻量叠卡、左侧 severity accent、标题/正文/动作/时间层级，在 `240 x 320` 下优先保证页内预览和双列对照的可读性

## 1. 为什么需要这个控件？
`toast_stack` 用来表达页内临时通知的叠卡语义，适合设置页、同步页、桌面入口页里展示最近 2 到 3 条轻量反馈。它不是全屏弹窗，也不是单条横幅，而是更接近 Fluent 的轻量 toast / snackbar 组合。

## 2. 为什么现有控件不够用
- `message_bar` 偏页内单条反馈，不强调连续 toast 的前后层级
- `notification_stack` 更接近旧版 showcase 风格，视觉语言偏重，不适合作为新的 reference 主线
- `alert_banner` 偏横向告警条，不适合做卡片堆叠式的临时消息
- `notification_badge`、`chips` 一类控件缺少标题、正文、动作和时间信息

因此这里单独实现 `toast_stack`，作为 `feedback` 目录下更贴近 Fluent 2 的轻量叠卡参考控件。

## 3. 目标场景与示例概览
- 主区域展示标准 `toast_stack`，覆盖 `info / success / warning / error`
- 左下 `Compact` 预览展示窄尺寸下的双态 toast stack
- 右下 `Read only` 预览展示弱化颜色、无动作按钮、无关闭位的静态预览
- 页面通过点击主卡和 compact 卡轮换 snapshot，验证标题、正文、动作按钮、meta pill、close glyph 与后两层卡片摘要

目录：
- `example/HelloCustomWidgets/feedback/toast_stack/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> 主 `toast_stack` -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡区域：`196 x 106`
- 底部双预览容器：`212 x 96`
- `Compact` 预览：`104 x 82`
- `Read only` 预览：`104 x 82`
- 视觉规则：
  - 使用浅灰 page panel + 白底低噪音 toast card
  - 前卡保留左侧 severity accent strip、状态圆点、标题、正文、动作 pill、meta pill
  - 后两层卡只保留摘要标题与轻量占位线，避免过度装饰
  - `Read only` 仅弱化 accent 和边框，不做额外交互动效

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Toast Stack` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap stacks to rotate snapshots` | 引导文案 |
| `stack_primary` | `egui_view_toast_stack_t` | 196 x 106 | `info` | 标准 toast stack |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Info toast active` | 当前状态说明 |
| `section_divider` | `egui_view_line_t` | 144 x 2 | visible | 分隔主区和底部预览 |
| `compact_label` | `egui_view_label_t` | 104 x 11 | `Compact` | compact 标题 |
| `stack_compact` | `egui_view_toast_stack_t` | 104 x 82 | `warning` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 104 x 11 | `Read only` | 只读标题 |
| `stack_locked` | `egui_view_toast_stack_t` | 104 x 82 | `locked` | 只读静态预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `info` | `warning` | `success locked` |
| 轮换 1 | `success` | 保持 | 保持 |
| 轮换 2 | `warning` | 保持 | 保持 |
| 轮换 3 | `error` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `warning <-> error` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 弱化 accent、隐藏 action / close |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取默认 `info` 状态
2. 切到 `success`
3. 切到 `warning`
4. 切到 `error`
5. 切到底部 `Compact` 的另一组 snapshot
6. 末尾等待一帧，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，避免依赖点击命中造成状态不稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/toast_stack PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/toast_stack --timeout 10 --keep-screenshots
```

验收重点：
- 主卡、后两层卡、底部双预览都必须完整可见
- `info / success / warning / error` 四态颜色差异要一眼可辨
- 标题、正文、action pill、meta pill 之间要保留稳定留白
- close glyph 不能贴边，也不能挤压标题
- `Compact` 在小尺寸下仍需保留前卡语义与后两层层级
- `Read only` 需要与可交互态明显区分，但不能做成发灰脏污效果

## 9. 已知限制与下一轮迭代计划
- 当前版本是固定尺寸 reference 实现，还未覆盖更长正文与更长动作文案
- 当前不做真实弹入/弹出动画，也不做手势关闭
- 当前不做真实图标资源，仅保留 severity glyph + accent strip
- 若后续需要沉入框架层，再单独评估 `src/widget/` 抽象和 UI Designer 接入

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `message_bar`：这里强调多条 toast 的叠卡关系，而不是单条页内反馈条
- 相比 `notification_stack`：这里改成轻量 Fluent 2 语言，去掉旧 showcase 风格的重装饰
- 相比 `alert_banner`：这里是卡片堆叠预览，不是横向告警横幅
- 相比 `card`：这里保留通知语义、severity、动作和 meta，而不是通用容器

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`Toast / Snackbar`
- 本次保留状态：
  - `info`
  - `success`
  - `warning`
  - `error`
  - `compact`
  - `read only`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做系统级弹入/弹出动画
- 不做 Acrylic、模糊、阴影扩散等桌面特效
- 不做真实图标资源和手势滑动关闭
- 不做队列计数、堆叠折叠、复杂 hover 状态

## 14. EGUI 适配时的简化点与约束
- 使用固定叠卡偏移量，优先保证 `240 x 320` 下的可审阅性
- 用低噪音边框和少量色彩混合表达层级，避免回到 HMI / showcase 风格
- `Compact` 与 `Read only` 统一放在底部双列，方便和主卡直接对照
- 先完成示例级 `toast_stack`，后续再决定是否沉入通用框架控件
