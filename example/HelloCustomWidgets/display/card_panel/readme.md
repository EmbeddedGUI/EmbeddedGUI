# card_panel 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`Card`
- 本次保留状态：`standard`、`compact`、`read only`、`accent`、`success`、`warning`、`neutral`
- 删除效果：Acrylic、复杂阴影、系统级 hover、真实图标资源、桌面级过渡动画
- EGUI 适配说明：保留结构化卡片、右侧 summary slot、底部 detail strip 和轻量 action pill，在 `240 x 320` 里优先保证页内排版稳定和主/副卡对照可读

## 1. 为什么需要这个控件？
`card_panel` 用来承载一张结构化信息卡：顶部 badge、主标题、正文摘要、右侧 summary slot、底部 detail strip，以及可选 action pill。它适合放在设置页、概览页、详情页里，作为比纯 `card` 更完整的信息块。

## 2. 为什么现有控件不够用
- `card` 是通用容器，不负责结构化信息层级
- `layer_stack` 强调叠层和深度，不适合做标准信息卡
- `message_bar`、`toast_stack` 属于反馈语义，不是常驻内容卡
- 旧 showcase 控件普遍更偏装饰或场景化，不适合作为新的 Fluent reference 主线

因此单独实现 `card_panel`，作为 `display` 目录下贴近 Fluent 2 / WPF UI 的结构化卡片参考。

## 3. 目标场景与示例概览
- 主区域展示标准 `card_panel`，覆盖 accent / warning / success / neutral 四组 snapshot
- 左下 `Compact` 预览展示小尺寸卡片如何保留结构
- 右下 `Read only` 预览展示无动作、弱化 tone 的被动展示状态
- 页面通过点击主卡和 compact 卡轮换 snapshot，验证 badge、title、body、value、detail、footer 与 action 的组合

目录：
- `example/HelloCustomWidgets/display/card_panel/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> 引导文案 -> 主 `card_panel` -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡区域：`196 x 122`
- 底部双预览容器：`212 x 104`
- `Compact` 预览：`104 x 90`
- `Read only` 预览：`104 x 90`
- 视觉规则：
  - 使用浅灰 page panel + 白底低噪音卡片
  - 卡片顶部只保留轻量 badge 和可选 action pill，不做复杂 chrome
  - 右侧 summary slot 用 value + label 表达辅助信息
  - 底部 detail strip 用于补充第二层说明，footer 维持更弱的收尾信息

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Card Panel` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap cards to review states` | 引导文案 |
| `panel_primary` | `egui_view_card_panel_t` | 196 x 122 | `accent` | 标准结构化卡片 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Accent card active` | 当前状态说明 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 分隔主区和底部预览 |
| `compact_label` | `egui_view_label_t` | 104 x 11 | `Compact` | compact 标题 |
| `panel_compact` | `egui_view_card_panel_t` | 104 x 90 | `accent compact` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 104 x 11 | `Read only` | 只读标题 |
| `panel_locked` | `egui_view_card_panel_t` | 104 x 90 | `neutral locked` | 只读静态预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `accent + emphasized` | `accent compact` | `neutral locked` |
| 轮换 1 | `warning + emphasized` | 保持 | 保持 |
| 轮换 2 | `success` | 保持 | 保持 |
| 轮换 3 | `neutral` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `warning compact` | 保持 |
| 只读弱化 | 不适用 | 不适用 | tone 弱化、隐藏 action |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取默认 accent 状态
2. 切到 warning
3. 切到 success
4. 切到底部 compact 的另一组 snapshot
5. 切到 neutral 主卡
6. 末尾等待一帧，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，避免依赖点击命中造成不稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=display/card_panel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/card_panel --timeout 10 --keep-screenshots
```

验收重点：
- 主卡、底部双预览必须完整可见
- 右侧 summary slot 不能挤压标题和正文
- detail strip 与 footer 之间要保留稳定留白
- badge / action / value / footer 这些短文本不能贴边，也不能视觉偏心
- `Compact` 在小尺寸下仍要保留结构，不退化成单纯文字框
- `Read only` 需要明显区别于标准态，但不能脏、灰、发糊

## 9. 已知限制与下一轮迭代计划
- 当前版本是固定尺寸 reference 实现，未覆盖超长标题和超长数值
- 当前不做 hover、焦点环、键盘导航等桌面细节
- 当前不接入真实图标资源，只保留文本和块面层级
- 若后续要沉入框架层，再单独评估 `src/widget/` 抽象和 UI Designer 接入

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `card`：这里不是通用容器，而是结构化信息卡
- 相比 `layer_stack`：这里强调标准卡片信息组织，不强调叠层深度
- 相比 `message_bar`：这里是常驻内容块，不是提示反馈条
- 相比 `toast_stack`：这里不表达临时通知队列，而是稳定展示内容摘要

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`Card`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `accent`
  - `success`
  - `warning`
  - `neutral`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做 Acrylic、真实阴影扩散和桌面级系统效果
- 不做真实图标资源和复合命令栏
- 不做 hover、pressed、focus ring 等完整桌面交互细节
- 不做多列内容模板和任意嵌套布局

## 14. EGUI 适配时的简化点与约束
- 使用固定 summary slot 和 detail strip，先保证 `240 x 320` 可审阅性
- 用少量 tone 混色表达层级，不回到旧 showcase 的花哨装饰
- `Compact` 与 `Read only` 固定放底部双列，便于和主卡直接对照
- 先完成示例级 `card_panel`，后续再决定是否沉入通用框架控件
