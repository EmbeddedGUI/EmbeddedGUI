# teaching_tip 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`WinUI TeachingTip`
- 对应组件名：`TeachingTip`
- 保留状态：anchored target、callout surface、top / bottom placement、compact / read-only 对照
- 删除效果：系统级 popup 定位、复杂阴影、Acrylic、真实图标资源、入场动画
- EGUI 适配说明：保留“目标锚点 + 提示气泡 + 轻量动作行”的核心语义，压缩到 `240 x 320` 页面，改成页内稳定锚定的 reference 版本

## 1. 为什么需要这个控件

`teaching_tip` 用来表达“围绕某个页面目标做上下文引导”的提示语义，适合首次引导、功能提示、快捷键提示、发布前提醒等场景。

## 2. 为什么现有控件不够用

- `message_bar` 是页内横向反馈条，不围绕具体目标锚定
- `dialog_sheet` 是收口型对话层，不是贴近控件的上下文提示
- `toast_stack` 偏临时通知，不承担目标指引语义
- `menu_flyout` 是命令面板，不是教学提示

## 3. 目标场景与示例概览

- 主区域展示标准 `TeachingTip`：顶部目标 pill，下方或上方为 callout surface
- 左下 `Compact` 预览展示缩小版 coachmark
- 右下 `Read Only` 预览展示只读弱化版提示
- guide 点击后循环切换不同 state（placement / tone / closed snapshot），并带出不同的水平锚点偏移
- target、primary、secondary、close 都可触摸并更新状态文本
- 主卡支持键盘 focus：`Left / Right / Up / Down / Tab / Home / End / Esc / Enter / Space`
- `HelloCustomWidgets` 示例页已开启 focus / key 支持，便于在 PC 录制里验证主卡的键盘闭环
- 关闭态主卡会保留一条页内 helper hint，提示用户再次点击 target 重新展开
- 关闭态会保留最近一次被关闭的 target 标签、tone 和水平锚点，避免 closed state 跳成无关目标

目录：

- `example/HelloCustomWidgets/feedback/teaching_tip/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 300`
- 页面结构：标题 -> guide -> `Anchored coachmark` -> 主卡 -> 状态文本 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`196 x 132`
- 底部双预览容器：`218 x 94`
- 单个预览：`106 x 84`
- 视觉规则：
  - 使用浅色 page panel 和低噪音边框
  - 主 bubble 要和 page panel 保持轻微分层，不能淡到像直接印在背景上
  - 目标 pill 与提示气泡必须保持明确锚定关系
  - 标准态 target 周围保留一圈轻量 halo，避免锚点在大留白区域里显得过孤
  - 标准态动作行保持主次层级：secondary 较窄、primary 较宽，避免两个按钮权重过于接近
  - read-only 预览保持弱化，但边框与正文仍需可读，不能因为去交互化而退化成一片灰雾
  - close affordance 要保持轻量但可辨认，不能淡到像装饰噪点
  - 标准气泡正文区要保留足够呼吸感，标题、正文、footer 与动作行不能挤成一团
  - footer 应明显弱于正文，承担辅助说明而不是和正文抢同一层级
  - 底部 `Compact / Read Only` 双预览要留出足够间距，同时保证标签与卡片都有最基本的横向余量
  - 顶部标题、guide 与 section label 要偏安静，作为页面 framing，不应和主卡抢注意力
  - 中段状态反馈应作为轻量 bridge，帮助串起主卡与底部对照区，而不是形成新的视觉断点
  - 中段状态 bridge 可以轻量跟随当前 tone，但只能做淡色提示，不能长成新的强调条
  - closed state 的 helper hint 应形成一个完整的小提示块，并尽量与 target 居中对齐成一个稳定 cluster
  - 底部 compact / read-only 的文案要刻意短一些，必要时收成单词级短词，优先保证低密度和不截断
  - 通过小箭头表达 `top / bottom placement`
  - 小箭头需要足够可辨认，不能弱到只剩“看起来像上下留白不同”
  - compact 版本保留 target + bubble 轮廓，不追求完整文本密度
  - compact 小卡里的主按钮需要可见，但不应因为面积太小而显得比整张卡更重

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 300 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Teaching Tip` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主卡 snapshot |
| `tip_primary` | `egui_view_teaching_tip_t` | 196 x 132 | accent / bottom / open | 标准 teaching tip |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Focus Pin tip` | 当前焦点状态 |
| `tip_compact` | `egui_view_teaching_tip_t` | 106 x 84 | compact | 紧凑预览 |
| `tip_locked` | `egui_view_teaching_tip_t` | 106 x 84 | read only | 只读预览 |

## 6. 状态覆盖矩阵

- `bottom placement`：target 在上，tip bubble 在下
- `top placement`：target 在下，tip bubble 在上
- `warning tip`：保留更强提醒语义
- `closed tip`：保留最近一次被关闭的 target pill，并切到隐藏后的 helper 提示
- `closed helper hint`：关闭态在 target 下方保留轻量文字提示
- `offset anchor`：target 在 snapshot 之间左右偏移，标准气泡也会轻微跟随，箭头与 target 保持对应
- `primary / secondary / close / target`：触摸后状态文本切换
- 键盘 focus：支持 target 与 bubble 内动作位之间的顺序切换与返回 target
- 当前焦点高亮：被点击或键盘选中的 target / action 会出现更明确的 ring 与底部指示线
- 标准气泡动作区：正文 / footer 与底部按钮之间保留轻量分隔线，降低信息区与动作区的粘连感
- dismiss / reopen：关闭后的 target 再次点击可以重新展开 tip
- compact 态：压缩布局与文字密度
- read-only 态：弱化 palette，不响应交互

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 默认 `bottom placement` 截图
2. 点击主卡 target，确认状态文本切到 `Quick filters`
3. 点击主卡 primary，确认状态文本回到 `Pin tip`
4. 切到 `top placement` 后点击 secondary，确认状态文本切到 `Tips`
5. 切到 `warning tip` 后点击 primary，确认状态文本切到 `Review`
6. 点击 close，确认主卡进入 `closed tip`
7. 再次点击关闭态 target，确认主卡重新展开并进入 `Reopen Quick filters`
8. 录制阶段通过控件内部导航 helper 依次回放 `Right / Right / Esc`，确认截图里能直接看到 `Focus Later`、`Focus Pin tip`、`Focus Quick filters`
9. 切换底部 compact 预览并点击 target，确认状态文本切到 `Compact Search`

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=feedback/teaching_tip PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/teaching_tip --timeout 10 --keep-screenshots`
- 验收重点：
  - target 与 bubble 的锚定关系必须清楚
  - `top / bottom placement` 必须能从截图中直接辨认
  - compact 与 read-only 对照必须明显区分
  - 文本与动作行不能贴边
  - 主卡键盘 focus 切换不能跳到无效动作位
  - 录制序列里必须至少出现 1 组明确可见的键盘 focus 迁移截图
  - target / primary / secondary / close 的触摸命中必须真实生效，不能只停留在静态默认态

## 9. 已知限制与下一轮迭代计划

- 当前版本是页内固定锚点 reference，不做真实 popup 跟随
- 当前不做自动避让和屏幕边缘翻转策略
- 当前不做多段正文换行和真实图标资源
- 录制层目前仍没有原生键盘 action 类型，因此键盘截图通过控件内部导航 helper 复用同一套 `on_key_event` 规则
- 下一轮继续细化主卡细节和标准卡视觉留白

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `message_bar`：这里是围绕目标的上下文提示，不是横向反馈条
- 相比 `dialog_sheet`：这里强调 anchored callout，不是居中确认层
- 相比 `toast_stack`：这里强调教学引导，不是短暂消息
- 相比 `menu_flyout`：这里不呈现命令列表，核心是提示与动作收口

## 11. 参考设计系统与开源母本

- `Fluent 2`：提供低噪音教学提示语义
- `WPF UI`：提供 Windows Fluent 风格参考
- `WinUI TeachingTip`：补充 placement 与 target 锚定关系

## 12. 对应组件名与保留核心状态

- 对应组件名：`TeachingTip`
- 本次保留的核心状态：
  - target pill
  - open callout surface
  - top / bottom placement
  - primary / secondary / close 动作位
  - compact 对照态
  - read-only 对照态

## 13. 相比参考原型删掉的效果或装饰

- 不做系统级 popup 定位与边界翻转
- 不做复杂阴影和入场动画
- 不做 Acrylic 与真实图标资源
- 不做多目标链式引导

## 14. EGUI 适配时的简化点与约束

- 固定在 `240 x 320` 下优化，优先保证锚定关系和文本可读性
- 用固定 target / bubble 组合表达 reference 语义
- compact 与 read-only 固定放在底部双列预览
- 先完成 `HelloCustomWidgets` 版本，后续再决定是否沉淀到框架层
