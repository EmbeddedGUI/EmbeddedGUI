# expander 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`ModernWpf`
- 对应组件名：`Expander` / `CardExpander`
- 保留状态：标准展开态、折叠态、`compact` 对照、`read-only` 对照
- 删除效果：Acrylic、投影、Reveal/Hover 光效、系统级转场动画、真实图标资源
- EGUI 适配说明：保留“标题行 disclosure + 展开 body”的核心语义，压缩到 `240 x 320` 页面内；主卡展示标准 accordion 节奏，底部保留更轻量的 compact / read-only 对照

## 1. 为什么需要这个控件

`expander` 用来表达“点击标题行后展开说明内容，再次点击可收起”的标准 disclosure 结构。它适合设置说明、同步规则、帮助段落、审核说明等需要按需展开正文的页面内信息组织。

## 2. 为什么现有控件不够用

- `settings_panel` 强调设置行和 trailing value / switch，不负责正文展开
- `tree_view` 强调层级导航，不是单层 disclosure
- `master_detail` / `split_view` 强调双栏联动，不适合内联展开说明
- `card_panel` 只能展示固定摘要，缺少收放状态

## 3. 目标场景与示例概览

- 主区域展示标准 `expander`：3 个 disclosure rows，其中当前项展开 body
- 左下 `Compact` 预览展示紧凑版展开语义
- 右下 `Read Only` 预览展示只读对照，不响应 touch / key
- 支持 touch 点击标题行切换展开项
- 支持 `Up / Down / Home / End / Enter / Space`

目录：

- `example/HelloCustomWidgets/layout/expander/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 296`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`194 x 110`
- 底部双预览容器：`222 x 88`
- `Compact` 预览：`108 x 72`
- `Read Only` 预览：`108 x 72`
- 视觉规则：
  - 使用浅色 page panel 和低噪音边框，不回到 HMI / dashboard 语法
  - 标题行保留 chevron、标题和短 meta pill
  - 展开 body 保留 eyebrow / body text / footer strip
  - compact 版本压缩正文，只保留一行 body + footer

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 296 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Expander` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 推进主卡演示状态 |
| `panel_primary` | `egui_view_expander_t` | 194 x 110 | 第 1 项展开 | 标准 expander 主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Workspace policy: Expanded` | 当前状态反馈 |
| `panel_compact` | `egui_view_expander_t` | 108 x 72 | 第 1 项展开 | 紧凑态预览 |
| `panel_read_only` | `egui_view_expander_t` | 108 x 72 | 第 2 项展开 | 只读态对照 |

## 6. 状态覆盖矩阵

- 标准展开态：当前项展开 body
- 折叠态：当前项仅保留标题行
- compact 态：正文压缩后仍保持 disclosure 结构
- read-only 态：不响应交互，但保留展开结构
- 按压态：点击标题行时显示 pressed 反馈
- 键盘态：
  - `Up / Down` 切换焦点项
  - `Home / End` 跳转首尾
  - `Enter / Space` 展开或收起当前项

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 初始化后抓取主卡首项展开的基线状态
2. 切换到主卡第 2 项展开
3. 折叠当前项，覆盖 collapse 状态
4. 切换到主卡第 3 项展开，覆盖 warning 语义
5. 切换 compact 预览到第 2 项展开
6. 折叠 compact 当前项，保留 compact collapsed 画面
7. read-only 预览保持固定，不在录制中响应状态变化

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=layout/expander PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/expander --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - 标题行与展开 body 都完整可见，不黑屏、不截断
  - chevron、短 meta、footer strip 需要人工复核居中与留白
  - compact 预览必须仍能看出展开 / 折叠区别
  - read-only 预览点击后不应变化

## 9. 已知限制与下一轮迭代计划

- 当前仍以静态 item 数据驱动，不接真实数据绑定
- 正文仅支持两行摘要，不做长文本换行
- 没有加入动画，只保留结构状态切换
- 如后续沉淀到框架层，可继续补充更通用的数据模型和过渡动画

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `settings_panel`：这里强调 inline disclosure，而不是设置行 trailing controls
- 相比 `tree_view`：这里没有多层级树结构，只有单层 accordion
- 相比 `master_detail` / `split_view`：这里不是双栏结构，而是页内纵向展开
- 相比 `card_panel`：这里自带展开 / 收起状态，不是固定摘要卡

## 11. 参考设计系统与开源母本

- `Fluent 2`：提供低噪音 disclosure / accordion 方向
- `WPF UI`：提供 `Expander` / `CardExpander` 的标准页面内语义
- `ModernWpf`：补充 Windows Fluent 风格下的展开容器表达

## 12. 对应组件名与保留核心状态

- 对应组件名：`Expander` / `CardExpander`
- 本次保留的核心状态：
  - 标准展开态
  - 折叠态
  - compact 对照态
  - read-only 对照态
  - touch / keyboard 切换态

## 13. 相比参考原型删除的效果或装饰

- 不做桌面级阴影和材质层
- 不做复杂 hover / reveal / transition 动效
- 不做真实图标资源和桌面命令栏整合
- 不做多重嵌套 expander，仅保留单层 accordion 语义

## 14. EGUI 适配时的简化点与约束

- 固定在 `240 x 320` 下调优，优先保证标题行与 body 可读性
- item 数量限制为 `4`
- compact 版本只保留一行正文，避免小卡片过载
- 颜色和圆角维持 Fluent 方向的低噪音表达，不再引入 showcase 装饰零件
