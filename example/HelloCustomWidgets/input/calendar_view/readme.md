# calendar_view 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`WinUI CalendarView`
- 对应组件名：`CalendarView`
- 保留状态：常驻月历面板、month navigation、range anchor、compact / read-only 对照
- 删除效果：系统级阴影、浮层动画、多视图切换、桌面级多选快捷手势
- EGUI 适配说明：保留“常驻月历 + 区间选择 + 浏览月份”的核心语义，压缩到 `240 x 320` 页面；不做年份层级切换，只保留单月浏览与区间闭环

## 1. 为什么需要这个控件

`calendar_view` 用来表达“在一个始终可见的月历面板里浏览月份并选择日期区间”的标准日历语义，适合排期、预订、冻结窗口、值班表和活动窗口等页面。

## 2. 为什么现有控件不够用

- `date_picker` 更偏表单字段 + 展开面板，不是常驻月历视图
- `mini_calendar` 更偏展示型月历，不强调范围选择与键盘焦点闭环
- `textinput` 不适合低噪音日期区间输入
- `time_picker` 只覆盖时分，不覆盖月历浏览与日期范围

## 3. 目标场景与示例概览

- 主区域展示标准 `CalendarView`：始终可见的标题、月份导航、weekday header 和 6x7 day grid
- 左下 `Compact` 预览只保留标题与范围摘要，作为紧凑态参考
- 右下 `Read Only` 预览展示弱化的只读摘要，不响应 touch / key
- 主卡支持键盘 `Tab / Left / Right / Up / Down / Home / End / +/- / Enter / Space / Esc`
- 主卡支持 touch 点击日期开始/结束范围，点击前后按钮浏览月份

目录：

- `example/HelloCustomWidgets/input/calendar_view/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文本 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`196 x 144`
- 底部双预览容器：`216 x 66`
- `Compact` 预览：`104 x 50`
- `Read Only` 预览：`104 x 50`
- 视觉规则：
  - 使用浅色卡面、低噪音边框和弱强调顶部 accent 条
  - 主卡保留标准月份标题、左右浏览按钮、weekday header 和日期网格
  - 日期区间用起止点 + 中段桥接填充表达
  - compact 版本退化成标题 + 区间摘要，不再绘制完整网格

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Calendar View` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主卡示例预设 |
| `calendar_primary` | `egui_view_calendar_view_t` | 196 x 144 | `Mar 2026 / 09-13` | 标准常驻月历主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Sprint Mar 09-13` | 当前状态反馈 |
| `calendar_compact` | `egui_view_calendar_view_t` | 104 x 50 | `May 05-08` 紧凑 | 紧凑态预览 |
| `calendar_locked` | `egui_view_calendar_view_t` | 104 x 50 | `Jul 18-22` 只读 | 只读态预览 |

## 6. 状态覆盖矩阵

- 标准态：常驻月份标题、weekday header 和完整 day grid
- `range preview`：第一次确认 anchor 后，方向键或第二次点击前的范围预览
- `range commit`：第二次确认后提交起止日
- `browse month`：通过左右按钮或 `+/-` 浏览月份，不强制提交选择范围
- `focus day`：主卡网格内保持键盘焦点日
- `today marker`：当前日期用 outline + dot 弱强调
- compact 态：仅标题 + 范围摘要
- read-only 态：仅摘要展示，不响应交互
- 键盘态：
  - `Tab` 在 `grid / prev / next` 之间循环
  - `Left / Right / Up / Down` 在 grid 中移动 focus day
  - `Home / End` 跳到当月首日 / 末日
  - `Enter / Space`：第一次进入 anchor，第二次提交区间
  - `+ / -` 浏览前后月份
  - `Esc` 取消当前区间预览并恢复已提交范围

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始帧：主卡显示 `Mar 2026 / 09-13`
2. 主卡进入 `Anchor` 状态
3. 键盘向右扩展到 `09-15`
4. 输出 anchor 预览帧
5. 提交范围
6. 浏览到 `Apr 2026`
7. 输出 browse 帧
8. 点击 guide 切到下一个主卡预设
9. 输出切换后的主卡帧
10. 点击 `Compact` 标签切换紧凑态预设
11. 输出最终收尾帧

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/calendar_view PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/calendar_view --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - 月份标题、week header 和网格必须层级清楚
  - 区间起止与中段桥接必须可辨识
  - compact / read-only 对照必须明显区分

## 9. 已知限制与下一轮迭代计划

- 当前只保留单月视图，不做 year / decade 层级切换
- 当前范围仅在单月内编辑，不做跨月连续范围
- 暂不做 locale 文案切换，只保留英文月缩写和单字母 weekday
- 若后续沉淀到框架层，可补年份跳转、跨月区间和更通用的选择模式

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `date_picker`：这里是常驻月历视图，不是字段 + 展开面板
- 相比 `mini_calendar`：这里强调区间 anchor、month browse 和键盘部件切换，不只是展示型月历
- 相比 `textinput`：这里不做自由日期文本输入
- 相比 `time_picker`：这里处理日期网格和月份浏览，不处理时分段选择

## 11. 参考设计系统与开源母本

- `Fluent 2`：提供低噪音日历视图方向
- `WPF UI`：提供 Windows Fluent 风格 `CalendarView` 参考
- `WinUI CalendarView`：补充常驻月历与范围选择语义

## 12. 对应组件名与保留核心状态

- 对应组件名：`CalendarView`
- 本次保留的核心状态：
  - 常驻月历面板
  - month navigation
  - range anchor / preview / commit
  - selected range / focus day / today marker
  - compact 对照态
  - read-only 对照态

## 13. 相比参考原型删掉的效果或装饰

- 不做系统级浮层、阴影和多层动效
- 不做年份/年代层级切换
- 不做跨月连续选择
- 不做复杂手势和系统区域格式联动

## 14. EGUI 适配时的简化点与约束

- 固定在 `240 x 320` 下优化，优先保证月历网格可读性
- compact 版本退化为标题 + 范围摘要
- 区间选择通过 anchor + 第二次确认闭环实现
- 只暴露 `grid / prev / next` 三个键盘交互部件
- 读取状态时区分 `selection month` 与 `display month`，支持“先浏览，再决定是否切换预设”的示例反馈
