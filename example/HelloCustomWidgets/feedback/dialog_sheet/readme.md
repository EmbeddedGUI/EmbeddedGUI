# dialog_sheet 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`ContentDialog / Dialog Sheet`
- 本次保留状态：`accent`、`success`、`warning`、`error`、`compact`、`read only`
- 删除效果：系统级模糊、真实图标资源、复杂阴影、动画进出场、拖拽手势关闭
- EGUI 适配说明：保留低噪音遮罩、sheet 化卡片、hero area、tag、footer summary、primary / secondary action row，在 `240 x 320` 下优先保证主卡与底部对照预览的可读性

## 1. 为什么需要这个控件？
`dialog_sheet` 用来表达轻量弹层确认语义，适合设置页、发布页、同步页里的二次确认场景。它不是全屏对话框，也不是页内横幅，而是更接近 Fluent 2 `ContentDialog` 的低噪音 sheet 收口。

## 2. 为什么现有控件不够用
- `message_bar` 偏页内单条反馈，不承担明确的动作确认
- `toast_stack` 偏临时通知叠卡，不适合做主次动作确认
- `card_panel` 偏信息结构展示，不包含遮罩语义和动作收口
- 现有通用按钮 / 卡片组合缺少统一的 hero area、footer tag、主次动作焦点切换

因此这里单独实现 `dialog_sheet`，作为 `feedback` 目录下更贴近 Fluent 2 / WPF UI 的确认层 reference。

## 3. 目标场景与示例概览
- 主区域展示标准 `dialog_sheet`，覆盖 `warning / error / accent / success`
- 左下 `Compact` 预览展示更窄尺寸下的单动作 sheet
- 右下 `Read only` 预览展示弱化 palette 与只读态
- 页面通过录制动作切换 snapshot，通过点击 action pill 改变当前焦点动作

目录：
- `example/HelloCustomWidgets/feedback/dialog_sheet/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 300`
- 页面结构：标题 -> 引导文案 -> 主 `dialog_sheet` -> 状态文案 -> 分隔线 -> `Compact / Read-only` 双预览
- 主卡区域：`196 x 132`
- 底部双预览容器：`218 x 96`
- `Compact` 预览：`106 x 86`
- `Read-only` 预览：`106 x 86`
- 视觉规则：
  - 使用浅灰 page panel + 低对比 backdrop，避免旧 HMI / showcase 风格
  - sheet 卡片保留 handle、hero circle、eyebrow、title、body、tag、footer summary、action row
  - `Compact` 简化 close 位和 footer 文案
  - `Read-only` 保留结构但弱化 tone 和按钮强调

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 300 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Dialog Sheet` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap actions to shift focus` | 引导文案 |
| `sheet_primary` | `egui_view_dialog_sheet_t` | 196 x 132 | `warning` | 标准 dialog sheet |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Focus Reconnect` | 当前动作状态说明 |
| `section_divider` | `egui_view_line_t` | 141 x 2 | visible | 分隔主区和底部预览 |
| `sheet_compact` | `egui_view_dialog_sheet_t` | 106 x 86 | `warning compact` | 紧凑预览 |
| `sheet_locked` | `egui_view_dialog_sheet_t` | 106 x 86 | `neutral locked` | 只读静态预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read-only |
| --- | --- | --- | --- |
| 默认态 | `warning / primary focus` | `warning / primary focus` | `neutral / locked` |
| 轮换 1 | `error / secondary focus` | 保持 | 保持 |
| 轮换 2 | `accent / primary focus` | 保持 | 保持 |
| 轮换 3 | `success / primary focus` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `accent / primary focus` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 弱化 tone、禁止触摸和键盘切换 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取默认 `warning` 状态
2. 切到 `error`
3. 切到 `accent`
4. 切到底部 `Compact`
5. 回到主卡 `success`
6. 末尾等待一帧，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，避免依赖点击命中造成状态不稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/dialog_sheet PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/dialog_sheet --timeout 10 --keep-screenshots
```

验收重点：
- backdrop、sheet、hero area、footer tag、action row 都必须完整可见
- `warning / error / accent / success` 四态差异要清晰
- 主次按钮切换时，焦点按钮要有稳定高亮
- `Compact` 不允许出现明显的文字贴边和按钮截断
- `Read-only` 需要与可交互态明显区分，但不能做成脏灰色块

## 9. 已知限制与下一轮迭代计划
- 当前版本是固定尺寸 reference 实现，还未覆盖更长正文和更长动作文案
- 当前不做真实 modal 动画、遮罩渐变和关闭行为
- 当前不做真实图标资源，仅保留 tone glyph
- 若后续需要沉入框架层，再单独评估 `src/widget/` 抽象和 UI Designer 接入

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `message_bar`：这里强调遮罩下的确认层，而不是页内反馈条
- 相比 `toast_stack`：这里强调主次动作收口，而不是通知叠卡
- 相比 `card_panel`：这里保留 modal sheet 语义和 footer action row
- 相比 `notification_stack`：这里彻底去掉旧 showcase 风格，改成 Fluent 2 低噪音 reference

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`ContentDialog / Dialog Sheet`
- 本次保留状态：
  - `warning`
  - `error`
  - `accent`
  - `success`
  - `compact`
  - `read only`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做系统级模糊和 Acrylic
- 不做复杂阴影扩散和动效进出场
- 不做真实 close 交互和拖拽下拉关闭
- 不做图标资源、复选框、辅助链接等扩展内容

## 14. EGUI 适配时的简化点与约束
- 用固定尺寸 sheet 和低对比 backdrop 表达对话层，优先保证 `240 x 320` 下可审阅
- 以 `hero + title + body + footer + actions` 五段式结构表达语义，不引入额外装饰
- `Compact` 和 `Read-only` 固定放到底部双列，方便与主卡直接对照
- 先完成示例级 `dialog_sheet`，后续再决定是否沉入通用框架控件
