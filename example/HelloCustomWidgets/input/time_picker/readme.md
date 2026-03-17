# time_picker 设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`WinUI TimePicker`
- 对应组件名：`TimePicker`
- 保留状态：标准时间字段、展开 picker surface、`12h / 24h` 显示、`compact / read-only` 对照
- 删除效果：Acrylic、系统级弹出层动画、阴影模糊、桌面级 flyout 定位逻辑
- EGUI 适配说明：保留“时间字段 + 内联选择面板”的核心语义，压缩到 `240 x 320` 页面；不实现真实弹出层，而是使用页内 picker surface 表达 hour / minute / period 调整

## 1. 为什么需要这个控件
`time_picker` 用来表达“先查看当前时间值，再按需展开选择小时 / 分钟 / 上午下午”的标准时间输入语义，适合会议时间、同步窗口、静默时段、提醒时间等页内设置场景。

## 2. 为什么现有控件不够用
- `number_picker` 是单列数字选择，不具备标准时间字段和 `hour / minute / period` 组合语义
- `combobox` / `auto_suggest_box` 强调下拉选择和文本建议，不适合时间滚轮式调整
- `textinput` 需要手工输入字符串，不适合低噪音时间配置页
- `settings_panel` 只能承载设置行，不负责时间字段本身的交互和展开面板

## 3. 目标场景与示例概览
- 主区域展示标准 `TimePicker`：顶部时间字段，展开后出现三列 picker surface
- 左下 `Compact` 预览展示紧凑字段，切换到 `24h` 显示
- 右下 `Read Only` 预览展示只读时间值，不响应 touch / key
- 支持 touch 点击字段展开 / 收起
- 支持键盘 `Left / Right / Home / End / Up / Down / Enter / Space / Esc`

目录：
- `example/HelloCustomWidgets/input/time_picker/`

## 4. 视觉与布局规格
- 页面尺寸：`240 x 320`
- 根布局：`224 x 296`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`194 x 126`
- 底部双预览容器：`218 x 80`
- `Compact` 预览：`106 x 58`
- `Read Only` 预览：`106 x 58`
- 视觉规则：
  - 使用浅色 page panel 和低噪音边框，不回到 HMI / showcase 风格
  - 字段内部保留 hour / minute / period 的分段高亮
  - 展开面板保留三行可见值，中心行代表当前值
  - compact 版本仅保留时间字段，不展开 picker surface

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 296 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Time Picker` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主卡示例状态 |
| `picker_primary` | `egui_view_time_picker_t` | 194 x 126 | `08:30 AM` 展开 | 标准时间选择器主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Standup 08:30 AM / Open` | 当前状态反馈 |
| `picker_compact` | `egui_view_time_picker_t` | 106 x 58 | `13:30` 紧凑 | `24h` 紧凑预览 |
| `picker_read_only` | `egui_view_time_picker_t` | 106 x 58 | `07:15 AM` 只读 | 只读预览 |

## 6. 状态覆盖矩阵
- 标准展开态：字段 + picker surface 同时可见
- 标准收起态：仅保留时间字段
- `12h` 模式：显示 `AM / PM` 分段
- `24h` 模式：仅显示 hour / minute
- compact 态：字段压缩，不展开
- read-only 态：不响应交互，但保留时间值显示
- 键盘态：
  - `Left / Right` 切换 hour / minute / period 焦点
  - `Home / End` 跳转首尾分段
  - `Up / Down` 调整当前分段值
  - `Enter / Space` 展开或收起
  - `Esc` 收起展开面板

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 初始帧：主卡 `08:30 AM` 展开，compact 为 `13:30`
2. 切换主卡到 `10:45 AM` 展开
3. 切换主卡到 `06:00 PM` 收起
4. 切换主卡到 `09:15 PM` 展开并聚焦 `period`
5. 切换 compact 到 `18:00`
6. 输出 read-only 对照帧作为收尾

## 8. 编译、runtime、截图验收标准
- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/time_picker PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/time_picker --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - 时间字段内 hour / minute / period 必须可辨认，不能贴边
  - 展开 surface 的三列和中心行高亮必须清晰可见
  - `compact` 与 `read-only` 对照必须明显区分
  - 展开 / 收起后状态文案与截图一致

## 9. 已知限制与下一轮迭代计划
- 当前仍是页内内联 surface，不是真正浮层定位
- 分钟调整按固定 `minute_step` 跳变，不做平滑滚轮动画
- 未接入真实区域设置，仅覆盖 `12h / 24h` 两种展示
- 若后续沉淀到框架层，可继续补充更通用的 locale / step / placeholder 能力

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `number_picker`：这里是标准时间字段 + 组合式分段选择，不是单列数字滚轮
- 相比 `combobox` / `auto_suggest_box`：这里不做文本建议与列表项选择，核心是时间语义
- 相比 `settings_panel`：这里提供时间输入主体，而不是设置页容器
- 相比 `split_button` / `menu_flyout`：这里不是命令触发，而是值选择控件

## 11. 参考设计系统与开源母本
- `Fluent 2`：提供低噪音表单字段与时间选择器方向
- `WPF UI`：提供 `TimePicker` 的 Windows Fluent 样式参考
- `WinUI TimePicker`：补充 hour / minute / period 组合和键盘语义参考

## 12. 对应组件名与保留核心状态
- 对应组件名：`TimePicker`
- 本次保留的核心状态：
  - 标准字段态
  - 展开 picker surface
  - `12h / 24h` 模式
  - compact 对照态
  - read-only 对照态
  - touch / keyboard 调时态

## 13. 相比参考原型删除的效果或装饰
- 不做系统级弹出层动画与定位箭头
- 不做 Acrylic、阴影模糊和桌面级浮层过渡
- 不做真实滚轮惯性和无限滚动动画
- 不做复杂 locale 文案与系统日期联动

## 14. EGUI 适配时的简化点与约束
- 固定在 `240 x 320` 下优化，优先保证字段与 picker surface 可读性
- `compact` 版本只保留字段，不展开 surface
- picker surface 固定三行可见值，通过 top / middle / bottom 三段点击完成调整
- `24h` 模式下自动隐藏 period 分段，避免小卡片过挤
