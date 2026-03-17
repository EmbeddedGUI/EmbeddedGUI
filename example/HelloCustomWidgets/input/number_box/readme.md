# number_box 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`NumberBox`
- 本次保留状态：`standard`、`compact`、`read only`
- 删减效果：键盘输入校验、鼠标滚轮、错误提示气泡、图标前后缀、Acrylic 与复杂焦点动画
- EGUI 适配说明：使用固定范围、轻量步进按钮和静态单位后缀，在 `240 x 320` 下优先保证数字输入框的可读性与可点击性

## 1. 为什么需要这个控件

`number_box` 用来在表单、设置页和属性面板里输入离散数字，比如边距、延迟、字号和数量限制。它应该比 `slider` 更精确，比 `number_picker` 更贴近表单控件，也比纯 `textinput` 更适合低风险的整数步进输入。

## 2. 为什么现有控件不够用

- `number_picker` 是滚轮式选择，适合列表值滚动，不适合标准表单里的轻量数字输入
- `slider` 更偏连续拖动，难以表达精确步进
- `textinput` 是通用文本编辑，不自带数值范围与加减步进语义
- 当前主线缺少一版接近 `Fluent 2 / WPF UI` 的标准 `NumberBox` reference

因此这里单独实现一个新的 `number_box`，而不是继续在旧控件上打补丁。

## 3. 目标场景与示例概览

- 主区域展示标准 `number_box`，用于调节 `Spacing`
- 左下 `Compact` 预览展示窄宽度下的轻量数字输入
- 右下 `Read only` 预览展示只读弱化版数字框
- 页面通过点击 `- / +` 按钮切换值，观察单位后缀、步进反馈和弱化态层级是否稳定

目录：

- `example/HelloCustomWidgets/input/number_box/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主数字框 -> 当前值文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主数字框：`196 x 68`
- 底部双预览容器：`216 x 64`
- `Compact` 预览：`106 x 42`
- `Read only` 预览：`106 x 42`
- 视觉规则：
  - 使用浅灰白 page panel + 白底轻边框容器
  - 中央数值区保持清爽白底，强调“表单输入框”而不是 showcase 卡片
  - `- / +` 按钮只做轻量填充和描边，不做重装饰
  - 只读态移除步进按钮，只保留弱化数值展示
  - 不引入工业风饰件、重阴影或过强的状态胶囊

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Number Box` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap minus or plus to step` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主数字框标签 |
| `box_primary` | `egui_view_number_box_t` | 196 x 68 | `Spacing = 24 px` | 标准数字框 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Spacing 24 px` | 当前值状态文案 |
| `section_divider` | `egui_view_line_t` | 144 x 2 | visible | 分隔主区与预览区 |
| `compact_label` | `egui_view_label_t` | 106 x 11 | `Compact` | 紧凑预览标签 |
| `box_compact` | `egui_view_number_box_t` | 106 x 42 | `12 ms` | 紧凑数字框 |
| `locked_label` | `egui_view_label_t` | 106 x 11 | `Read only` | 只读预览标签 |
| `box_locked` | `egui_view_number_box_t` | 106 x 42 | `16 px` | 只读弱化数字框 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主数字框 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `24 px` | `12 ms` | `16 px` |
| 点击 `+` | 按 step 增加并更新状态文案 | 在小范围内递增 | 不响应 |
| 点击 `-` | 按 step 减少并更新状态文案 | 在小范围内递减 | 不响应 |
| 只读弱化 | 不适用 | 不适用 | 移除步进按钮，仅保留弱化数值展示 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待，记录默认 `24 px`
2. 点击主数字框 `+`，切到更大的 spacing
3. 再次点击主数字框 `+`，验证连续步进
4. 点击主数字框 `-`，验证回退
5. 点击 `Compact` 预览的 `+`，记录小尺寸步进
6. 末尾等待一帧，供 runtime 抓最终截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/number_box PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/number_box --timeout 10 --keep-screenshots
```

验收重点：

- 主数字框、状态文案和底部双预览都必须完整可见
- 主数字框必须看起来像标准表单数字输入，而不是滚轮或 slider
- `- / +` 按钮在标准和 compact 下都要可辨识、可点击
- 数值与单位后缀要保持真实居中，不能贴边
- `Read only` 必须明显弱化，并与可交互态形成边界

## 9. 已知限制与后续方向

- 当前版本只覆盖整数步进，不做键盘文本编辑
- 现阶段不做错误校验提示、placeholder 和复杂焦点环
- 本轮先完成 reference 版 30 次收敛，后续再看是否需要沉入框架层
- 如果未来需要更完整的表单输入，再考虑与 `textinput` 做更深联动

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `number_picker`：本控件是标准表单数字输入，不是滚轮选择器
- 相比 `slider`：本控件表达离散步进，不是连续拖动
- 相比 `textinput`：本控件有明确范围、步长与加减按钮语义
- 相比 `stepper`：本控件编辑数字值，不表达流程步骤

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`NumberBox`
- 本次保留状态：
  - `standard`
  - `current value`
  - `compact`
  - `read only`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做键盘输入、选中文本和光标编辑
- 不做错误态图标、浮层提示和复杂校验动画
- 不做鼠标滚轮输入、hover 光效和系统级焦点环
- 不做图标前后缀与背景特效

## 14. EGUI 适配时的简化点与约束

- 使用固定整数范围和步进，优先保证 `240 x 320` 下的可审阅性
- 通过轻量 `- / +` 按钮承载交互，不引入复杂文本输入状态机
- 保持浅色低噪音 reference 风格，避免回到 showcase 语法
- 先完成示例级数字框，再决定是否上升到框架公共控件
