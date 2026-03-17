# menu_bar 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源母本：`WPF UI`、`ModernWpf`
- 对应组件名：`MenuBar`
- 本次保留状态：`standard`、`compact`、`read only`、`active menu`、`dropdown panel`
- 删减效果：不做系统级 menubar 接管，不做复杂多级子菜单，不做原生快捷键分发，不做窗口级菜单栏
- EGUI 适配说明：保留顶层菜单分类、当前菜单高亮和锚定下拉面板，用静态 snapshot 替代复杂桌面菜单状态机

## 1. 为什么需要这个控件
`menu_bar` 用来表达桌面应用或复杂信息页里的“顶层命令分类 + 下拉命令面板”语义。相比直接堆一排按钮，它更适合承载 `File / Edit / View / Tools` 这类分组明确、动作数量较多、层级关系清晰的命令结构。

## 2. 为什么现有控件不够用
- `menu_flyout` 是局部弹出命令面板，不承担顶层常驻菜单栏语义
- `command_bar` 更像工具栏，强调主命令和 overflow，不强调顶层分类菜单
- `nav_panel` 是页面导航，不是命令型菜单
- `menu` / `tab_strip` 也不具备“顶层菜单项 + 锚定下拉面板”的组合结构

## 3. 目标场景与示例概览
- 主卡展示标准桌面菜单栏：常驻顶部菜单、当前菜单高亮、下方命令面板
- 左下 `Compact` 预览保留 3 个顶层菜单，并支持 `File / View / Tools` 三组小面板
- 右下 `Read only` 预览弱化为不可交互的常驻菜单摘要
- 通过点击菜单标题或键盘方向键切换不同 snapshot，验证多组菜单内容

目标目录：
- `example/HelloCustomWidgets/navigation/menu_bar/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 292`
- 主卡尺寸：`196 x 112`
- 底部预览区：`214 x 88`
- 单列预览卡：`104 x 74`
- 视觉规则：
  - 页面维持浅灰 page panel + 白色菜单卡的 Fluent 风格
  - 顶层菜单项使用轻量 underline 和弱填充，不做厚重桌面阴影
  - 下拉面板强调层级，但保持边框克制、留白稳定
  - 当前 / 按压 row 使用细状态 rail，disabled row 用低噪音 lock badge 补充可辨识度
  - submenu row 在当前 / 按压态下补充右侧 ghost panel sliver，提示下一级菜单层级
  - `read only` 卡片补充 top rail lock seal 与 summary strip lock chip，强调锁定语义但不抢主内容
  - `read only` 当前菜单补充极轻的 current pip 与文字提亮，方便快速识别当前摘要归属
  - `compact / read only` 的 collapsed summary strip 顶边补极轻 anchor cue，并轻微提亮右侧 menu meta，方便看出当前菜单归属
  - `compact / read only` 的 collapsed summary 右侧 menu meta 使用极轻 meta chip 收口，让标题区和当前菜单标签更好区分
  - `compact / read only` 的 collapsed summary title 会跟随当前 tone 做极轻提亮，避免摘要主标题完全埋进中性灰里
  - `compact / read only` 的 collapsed summary title 与右侧 meta chip 之间保留稳定小间距，避免两段语义贴得太紧
  - `compact / read only` 的 collapsed summary 左侧 tone cue 需要做成更饱满的短竖条，避免在小卡里退化成过细的装饰线
  - `compact / read only` 的 collapsed summary meta chip 需要留出上下呼吸区，避免标签胶囊显得过满过重
  - `compact` 小卡里的 submenu affordance 需要进一步压缩体量，避免箭头胶囊和 ghost sliver 把小卡做花
  - `read only` 卡片里的 lock seal 与 summary lock chip 需要继续缩小并留白，避免锁定提示压过摘要文本
  - panel row 里的 meta 与 trailing 区要保留稳定小缝隙，避免快捷键文本和箭头/锁徽记挤在一起
  - 顶层当前菜单的 underline 需要再短一点，避免 current fill、边框和底线同时出现时显得过满
  - 当前菜单与 panel 之间的 anchor stem / cap 也要继续做细，避免锚定关系提示显得比菜单主体更重
  - `read only` 当前菜单的 current pip / 文本提亮还要再弱一点，避免锁定态里“当前项”语义盖过只读语义
  - collapsed summary strip 自身的底板也要继续降噪，避免背景胶囊比 cue、标题和 meta 标签更显眼
  - dropdown panel 的投影要继续保持很浅，避免面板像悬浮卡片一样压过顶层菜单本体
  - `read only` 用统一弱化 palette，而不是增加额外装饰

## 5. 控件清单
| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Menu Bar` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 点击/键盘提示 | 指示交互方式 |
| `menu_bar_primary` | `egui_view_menu_bar_t` | 196 x 112 | `File` 菜单展开 | 主菜单栏控件 |
| `status_label` | `egui_view_label_t` | 224 x 12 | 状态文案 | 当前 snapshot 说明 |
| `menu_bar_compact` | `egui_view_menu_bar_t` | 104 x 74 | compact | 底部紧凑对照 |
| `menu_bar_locked` | `egui_view_menu_bar_t` | 104 x 74 | read only | 底部只读对照 |

## 6. 状态覆盖矩阵
| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | 顶层菜单 + 下拉面板 | 3 个菜单 + 小面板 | 顶层菜单摘要 |
| current menu | 有 | 有 | 有 |
| panel focus row | 有 | 有 | 仅摘要 |
| disabled row | 有 | 可省略 | 仅弱化 |
| disabled top menu | 跳过不可达 snapshot | 跳过不可达 snapshot | 仅摘要 |
| submenu arrow | 有 | 有（轻量） | 无需展开 |
| submenu focus feedback | 有 | 有（轻量） | 无 |
| 锁定弱化 | 无 | 无 | 有 |
| keyboard left/right/home/end | 有 | 有 | 无 |
| keyboard up/down | 行间移动 | 行间移动 | 无 |
| keyboard enter/space | 激活当前 row | 激活当前 row | 无 |
| keyboard tab focus cycle | 与 compact 循环 | 与 primary 循环 | 不参与 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧记录 `File` 菜单展开态
2. 记录 `View` 顶层菜单按压态，验证 menu pressed 反馈
3. 记录 `Open recent` row 按压态，验证 submenu row 的 pressed 反馈
4. 切到 `Compact / View`，验证小卡 submenu cue、row 焦点与 focus ring
5. 切到 `Compact / Tools`，验证 row 激活后的状态文案联动
6. 最后切到 `Tools` 菜单，记录 disabled row 与同步动作激活后的结果态

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/menu_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/menu_bar --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

验收重点：
- 顶层菜单项居中、留白均匀，当前菜单的 underline 不偏心
- 下拉面板与顶部菜单的锚定关系清晰，不出现漂移
- panel row 的标题、meta、箭头三列对齐稳定
- compact / read only 区域仍然可读，不退化成拥挤小块
- primary / compact 获取焦点后，当前菜单项要出现清晰但不过度突兀的 focus ring
- submenu row 的箭头 affordance 在焦点态下要可辨识，但不能破坏低噪音布局
- submenu row 的 ghost panel sliver 要能提示层级，但不能把主卡右侧挤得杂乱
- 当前顶层菜单和 dropdown panel 之间要有轻量 anchor bridge，锚定关系清晰但不能显得厚重
- dropdown panel 顶边要保留一段很短的 accent cap，强化当前菜单归属但不能破坏简洁感
- compact / read only 的 summary strip 要保留细小的 tone cue，方便快速识别当前摘要语义
- compact / read only 的 summary strip 顶边要保留很轻的 anchor cue，右侧 menu meta 要有轻微 current emphasis，但不能把小卡做花
- compact / read only 的 summary strip 右侧 menu meta 要有轻量 meta chip，帮助区分 summary title 与当前菜单标签，但不能抢过主标题
- compact / read only 的 summary 主标题要带极轻的 tone emphasis，增强可读性，但不能把 collapsed 小卡变成高对比信息块
- compact / read only 的 summary 主标题和 meta chip 之间要留出稳定 gap，避免标题与当前菜单标签视觉粘连
- compact / read only 的 summary 左侧 tone cue 要保持足够厚度与上下留白，确保 collapsed 小卡里仍能稳定识别摘要语义
- compact / read only 的 summary meta chip 要上下居中并保留轻量呼吸区，避免 chip 看起来像把整条摘要压满
- compact 小卡里的 submenu cue 要更克制，能提示层级但不能让小卡显得花
- compact 小卡里的 submenu affordance 和 ghost sliver 要继续控制尺寸与透明度，确保 `Panels` 一类层级提示不压过正文
- panel row 的 meta 文本与 trailing 箭头/锁徽记之间要留出稳定 gap，保证右侧三列语义清楚分开
- disabled row 要有可辨识但不过度抢眼的锁定标记，current / pressed row 的左侧状态 rail 不能挤压文字留白
- read-only 卡的 lock seal / lock chip 要清晰但克制，不能和 `Work / Review` 文本互相抢空间
- read-only 卡的 lock seal / lock chip 还要保留额外呼吸区，避免短词摘要里出现“锁图标比文本更显眼”的问题
- read-only 卡当前菜单要保留很轻的 current pip / text emphasis，方便快速识别 `Review` 一类当前摘要，但不能盖过锁定语义
- read-only 卡里的 current pip / text emphasis 要继续偏弱，让锁定态仍然优先被读成“不可交互”而不是“选中态”
- 顶层当前菜单的 underline 要比 current fill 更克制，长度需保持在文字区内，避免贴近圆角边界
- 当前菜单与 panel 之间的 anchor stem / cap 要继续细化，保持归属关系清晰但不能把顶部区域做成装饰重点
- collapsed summary strip 的底板与描边要继续保持低噪音，让 cue、标题与 meta chip 成为主要可读信息
- dropdown panel 的 shadow 只能提供轻量分层，不能把菜单面板做成厚重的悬浮卡片
- `HelloUnitTest` 需要覆盖 snapshot 切换、row 选择/激活、键盘导航与 locked/disabled guard
- disabled top menu 不能通过 `set_current_snapshot()`、`Left/Right/Home/End` 进入
- 截图里能够清晰看出多组 snapshot 的差异

## 9. 已知限制与下一轮迭代计划
- 当前仍以静态 snapshot 驱动，不实现真实多级子菜单状态机
- 已补齐 panel row 命中、上下键行间移动与 Enter / Space 激活，但命令执行仍只联动状态文案，不做真实业务回调
- 已补齐 submenu row 的焦点 / 按压 affordance，但还不实现真实 nested submenu surface
- 当前 panel 宽度仍使用轻量估算，不做精确文本测量
- 本轮已完成 30 次迭代收口；若后续升级到框架层，可再评估真实 submenu surface 与精确文本测量

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `menu_flyout`：这里是常驻顶层菜单栏，不是局部弹出菜单
- 相比 `command_bar`：这里强调菜单分类和下拉层级，不是工具栏按钮集
- 相比 `nav_panel`：这里是命令结构，不是页面导航结构
- 相比 `tab_strip`：这里不是页面切换，而是命令分组入口

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 参考开源母本：`WPF UI`、`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`MenuBar`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `active menu`
  - `dropdown panel`
  - `focus row`
  - `row activation`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做窗口级系统菜单融合
- 不做复杂 hover、pressed、nested submenu 过渡
- 不做多级阴影和 acrylic 材质
- 不做原生快捷键事件转发，只展示 meta 文本

## 14. EGUI 适配时的简化点与约束
- 用 snapshot 数据驱动菜单与下拉内容，降低状态机复杂度
- 文本宽度先用近似规则控制，不引入复杂布局求解
- read only 版本保留摘要语义，避免小尺寸下重复绘制完整菜单
- 当前以 HelloCustomWidgets 示例优先，先验证结构、视觉与交互方向
