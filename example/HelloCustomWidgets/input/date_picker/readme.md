# date_picker 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`WinUI DatePicker`
- 对应组件名：`DatePicker`
- 保留状态：标准日期字段、内联 calendar surface、month navigation、compact / read-only 对照
- 删除效果：系统级 flyout 定位、Acrylic、复杂阴影、桌面级弹出层动画
- EGUI 适配说明：保留“日期字段 + 月历面板”的核心语义，压缩到 `240 x 320` 页面；不实现真实浮层，改成卡片内展开的 inline calendar surface

## 1. 为什么需要这个控件

`date_picker` 用来表达“先查看当前日期值，再按需展开月历面板选择某一天”的标准日期输入语义，适合交付日期、截止日期、出行日期、预约日期等页面内设置场景。

## 2. 为什么现有控件不够用

- `mini_calendar` 更偏展示型月历，不具备标准表单字段 + 展开面板的输入语义
- `textinput` 需要手动输入日期字符串，不适合低噪音日期选择页面
- `combobox` / `auto_suggest_box` 更强调下拉列表或建议项，不适合月历网格选择
- `time_picker` 只覆盖时分语义，不覆盖年月日与月历切换

## 3. 目标场景与示例概览

- 主区域展示标准 `DatePicker`：顶部字段，展开后显示 month title、weekday header 和 day grid
- 左下 `Compact` 预览只保留日期字段，作为紧凑态参考
- 右下 `Read Only` 预览展示弱化的只读字段，不响应 touch / key
- 底部两个预览在示例页里作为被动参考区使用，点击时只用于帮助主卡失焦收口，不参与主交互
- 支持 touch 点击字段展开或收起
- 支持 touch 点击左右月切换按钮浏览月份，再点击日期网格提交日期
- 支持跨年 browse：字段值保持旧日期，panel 可先浏览到下一年月份，再决定是否提交
- 支持键盘 `Left / Right / Up / Down / Home / End / +/- / Enter / Space / Esc`

目录：

- `example/HelloCustomWidgets/input/date_picker/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 308`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文本 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：展开态 `194 x 180`，收起态自动压缩到 `194 x 82`
- 底部双预览容器：`218 x 60`
- `Compact` 预览：`106 x 48`
- `Read Only` 预览：`106 x 48`
- 视觉规则：
  - 使用浅色 page panel 和低噪音边框，不回到 HMI / showcase 风格
  - 字段区域保留标准日期字段的表单语义
  - 展开面板保留 month title、weekday header 和 6x7 day grid
  - browse month 时 month title 会切到 accent 色，帮助区分“正在浏览的月份”和已提交日期
  - compact 版本只保留字段，不展开 calendar surface

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 308 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Date Picker` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主卡示例状态 |
| `picker_primary` | `egui_view_date_picker_t` | 194 x 180（展开） / 194 x 82（收起） | `2026-03-18` 展开 | 标准日期选择器主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Launch 2026-03-18 / Open` | 当前状态反馈 |
| `picker_compact` | `egui_view_date_picker_t` | 106 x 48 | `Mar 18` 紧凑 | 紧凑态预览 |
| `picker_read_only` | `egui_view_date_picker_t` | 106 x 48 | `Apr 05` 只读 | 只读态预览 |

## 6. 状态覆盖矩阵

- 标准展开态：字段 + calendar surface 同时可见
- 标准收起态：仅保留日期字段
- `month navigation`：切换前后月份，但字段日期在真正选中某一天之前不变化
- `browse feedback`：当 panel 仅浏览其他月份时，示例页状态文本切到 `Browse`，和 committed date 分离
- `cross-year browse`：字段日期与 panel 月份可以跨年分离，例如字段仍是 `2026-11-27`，panel 已浏览到 `Jan 2027`
- `browse anchor`：当 panel 浏览别的月份时，用轻量 anchor 高亮提示键盘导航会从哪一天继续移动
- `selected day`：当前选中日高亮
- `today marker`：当前月份中的今日弱强调，使用轮廓 + 小圆点辅助提示
- `day press feedback`：日期格按下后会显示即时 press 反馈，拖动到其他有效日期格时会跟随更新
- compact 态：字段压缩，不展开
- read-only 态：不响应交互，但保留日期值展示
- `helper feedback`：helper 文案会根据展开 / 浏览 / 提交状态动态切换
- 键盘态：
  - `Left / Right` 切换前后日期；若当前 panel 浏览的是别的月份，则以 display month 为锚点提交
  - `Up / Down` 按周移动；若当前 panel 浏览的是别的月份，则以 display month 为锚点提交
  - `Home / End` 跳到当前显示月份的首日 / 末日
  - `+ / -` 在展开态下浏览前后月份，但不提交字段日期
  - `Enter / Space`：关闭态时展开；展开且 panel 正在浏览别的月份时提交 anchor day；展开且 panel 已与 committed date 对齐时收起
  - `Esc` 收起展开面板
  - 失焦后自动收起面板，回到标准表单字段态；页面空白区和底部预览区都可用于 dismiss

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始帧：主卡 `2026-03-18` 展开，panel 也在 `Mar 2026`
2. 浏览帧：字段仍是 `2026-03-18`，但 panel 切到 `Apr 2026`
3. 选中帧：主卡提交为 `2026-04-02`
4. 主卡切到 `2026-11-27` 收起
5. 主卡进入 `Cross Year`：字段仍是 `2026-11-27`，panel 浏览到 `Jan 2027`
6. 主卡切到 `2027-01-05` 展开
7. compact 切到 `Jun 09`
8. 输出 read-only 对照态作为收尾

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/date_picker PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/date_picker --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - 日期字段中的年月日必须清晰可读，不贴边
  - 展开 surface 的 month title、weekday header 和 day grid 必须层级清楚
  - `Compact` 与 `Read Only` 对照必须明显区分
- 收起态时主卡高度要自动收紧，字段、helper 与主卡边框之间的留白要均衡

## 9. 已知限制与下一轮迭代计划

- 当前仍是页内 inline surface，不是真实浮层
- 暂不做 locale 文案切换，只保留英文月缩写和单字母 weekday
- 暂不做月份外日期的弱显示，仅展示当前月有效日
- 若后续沉淀到框架层，可继续补充 placeholder、格式化策略和更通用的 locale 支持

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `mini_calendar`：这里是标准输入字段 + 展开面板，不是纯展示月历
- 相比 `textinput`：这里不做自由文本编辑，核心是日期输入语义
- 相比 `combobox` / `auto_suggest_box`：这里不做候选列表，核心是月历网格选择
- 相比 `time_picker`：这里处理年月日和月切换，不处理时分段选择
- 相比纯月历浏览器：这里保留 field value 与 panel browse month 的分离，只有真正选中日期时才提交字段值

## 11. 参考设计系统与开源母本

- `Fluent 2`：提供低噪音表单字段和日期选择器方向
- `WPF UI`：提供 `DatePicker` 的 Windows Fluent 风格参考
- `WinUI DatePicker`：补充标准日期字段和月历面板语义

## 12. 对应组件名与保留核心状态

- 对应组件名：`DatePicker`
- 本次保留的核心状态：
  - 标准字段态
  - 展开 calendar surface
  - month navigation（browse month 与 committed date 分离）
  - selected day / today marker
  - compact 对照态
  - read-only 对照态
  - touch / keyboard 选日态

## 13. 相比参考原型删掉的效果或装饰

- 不做系统级弹出层动画和定位逻辑
- 不做 Acrylic、复杂阴影和桌面级浮层过渡
- 不做月份外日期的完整连续显示
- 不做复杂 locale 和系统区域格式联动

## 14. EGUI 适配时的简化点与约束

- 固定在 `240 x 320` 下优化，优先保证字段和 calendar surface 可读性
- compact 版本只保留字段，不展开 surface
- calendar surface 固定为 month title + weekday header + 6x7 day grid
- 以 touch 点击字段 / 月切换按钮 / 日期格，以及键盘日期移动作为最小可运行版本
- 对外暴露 display month getter / setter，并补 `display month changed` 回调，方便页面层同步浏览态反馈
- 关闭态下若先调用 `set_display_month()` 再调用 `set_opened(1)`，会保留预设的 browse month，便于页面层先配置再展开
- 若用户只是 browse 到其他月份但未提交日期，关闭后 display month 会回到 committed date 对应月份；再次打开时保持标准 `DatePicker` 语义
- 切到 compact / read-only 模式时会主动收起 panel，避免在 draw 阶段偷偷改写 open state
- 主字段在 focus 态下提供轻量 focus ring，browse month 时额外显示 navigation anchor，帮助解释键盘语义
- 日期格补了按下与拖动反馈，帮助 touch 选择过程更自然
