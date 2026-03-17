# menu_flyout 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`MenuFlyout / ContextMenu`
- 本次保留状态：`standard`、`submenu`、`shortcut`、`danger`、`compact`、`disabled`
- 删减效果：Acrylic、桌面级阴影扩散、真实图标资源、级联弹出动画、完整键盘导航
- EGUI 适配说明：保留轻量弹出菜单卡、submenu 箭头、shortcut 右对齐、danger/disabled 语义和 compact 对照，在 `240 x 320` 中优先保证弹出菜单节奏、层级和留白稳定

## 1. 为什么需要这个控件？
`menu_flyout` 用来表达轻量弹出菜单，而不是全屏菜单页。它适合右键菜单、工具栏溢出菜单、卡片局部操作菜单等场景，强调短命令、级联入口、快捷键提示以及 danger / disabled 状态。

## 2. 为什么现有控件不够用
- `menu` 是整页层级菜单，偏页面导航，不是 Fluent 风格的轻量 flyout
- `nav_panel` 是常驻导航面板，不承载局部命令弹层
- `breadcrumb_bar`、`tab_strip` 都是导航结构，不是临时命令列表
- 现有 showcase 类控件更偏装饰或场景化，不适合作为标准 popup menu reference

因此需要单独实现 `menu_flyout`，作为 `navigation` 目录下贴近 Fluent `MenuFlyout / ContextMenu` 的 reference 控件。

## 3. 目标场景与示例概览
- 主区域展示标准 `menu_flyout`，覆盖 submenu、state、danger、layout command 四组 snapshot
- 左下 `Compact` 预览展示窄尺寸轻量弹出菜单
- 右下 `Disabled` 预览展示不可用命令的弱化语义
- 点击主卡轮换 4 组 snapshot，点击 compact 卡轮换 2 组 snapshot，验证 submenu 箭头、shortcut 右对齐、danger 和 disabled 的视觉边界

目录：
- `example/HelloCustomWidgets/navigation/menu_flyout/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主 `menu_flyout` -> 状态文案 -> 分隔线 -> `Compact / Disabled` 双预览
- 主卡区域：`188 x 118`
- 底部双预览容器：`214 x 92`
- `Compact` 预览：`104 x 78`
- `Disabled` 预览：`104 x 78`
- 视觉规则：
  - 使用浅灰 page panel + 白底 popup surface
  - 保留轻量圆角、细边框和弱阴影，不做重 showcase 装饰
  - 命令行对齐统一：左侧 glyph、标题、右侧 shortcut / state、最右 submenu 箭头
  - 通过 group separator 区分命令分组，不添加额外头部 chrome

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Menu Flyout` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap flyouts to review commands` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主菜单标签 |
| `flyout_primary` | `egui_view_menu_flyout_t` | 188 x 118 | `submenu focus` | 标准 flyout |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Submenu command focused` | 当前 focus 语义说明 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 分隔主区域与底部预览 |
| `compact_label` | `egui_view_label_t` | 105 x 11 | `Compact` | compact 标签 |
| `flyout_compact` | `egui_view_menu_flyout_t` | 104 x 78 | `compact action` | 紧凑预览 |
| `disabled_label` | `egui_view_label_t` | 105 x 11 | `Disabled` | disabled 标签 |
| `flyout_disabled` | `egui_view_menu_flyout_t` | 104 x 78 | `disabled preview` | 禁用态预览 |

## 6. 状态覆盖矩阵
| 状态 / 区域 | 主卡 | Compact | Disabled |
| --- | --- | --- | --- |
| 默认态 | `submenu` | `compact action` | `disabled commands` |
| 轮换 1 | `state command` | 保持 | 保持 |
| 轮换 2 | `danger command` | 保持 | 保持 |
| 轮换 3 | `layout command` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `compact state` | 保持 |
| 禁用弱化 | 不适用 | 不适用 | 所有命令整体降噪并保留层级 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取默认 submenu snapshot
2. 切换到 state snapshot
3. 切换到 danger snapshot
4. 切换到底部 compact 的第二组 snapshot
5. 切换到 layout snapshot
6. 末尾等待一帧，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，避免依赖点击命中，保证回放稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/menu_flyout PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/menu_flyout --timeout 10 --keep-screenshots
```

验收重点：
- 主 flyout 和底部双预览都必须完整可见，不能裁切
- 菜单行的左右留白、separator、shortcut 对齐必须稳定
- submenu 箭头不能贴边，shortcut 不能压住标题
- danger 项要明显区别于普通命令，但不能过度装饰
- disabled 预览要一眼可辨，同时仍保留弹出菜单结构
- compact 预览不能退化成纯文本列表

## 9. 已知限制与下一轮迭代计划
- 当前是固定尺寸 reference 实现，未覆盖超长 shortcut 或超多菜单项
- 当前不做真实 popup 定位、级联子菜单展开和 hover 交互
- 当前 glyph 采用双字母占位，不接入真实图标资源
- 若后续需要沉入框架层，再单独评估和 `src/widget/menu` 的职责边界

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `menu`：这里不是整页菜单导航，而是局部命令 flyout
- 相比 `nav_panel`：这里不是常驻导航容器，而是临时操作菜单
- 相比 `breadcrumb_bar` / `tab_strip`：这里不承担导航路径或页签切换
- 相比旧 showcase 控件：这里强调标准 Fluent popup 语义，而非场景化视觉叙事

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`MenuFlyout / ContextMenu`
- 本次保留状态：
  - `standard`
  - `submenu`
  - `shortcut`
  - `danger`
  - `compact`
  - `disabled`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做 Acrylic、真实桌面阴影和复杂弹出动画
- 不做完整图标资源、checkbox / radio glyph 体系
- 不做 hover、pressed、focus ring 等完整桌面交互细节
- 不做真实级联子菜单展开，仅保留 submenu 入口语义

## 14. EGUI 适配时的简化点与约束
- 用固定 snapshot + item 数组驱动，先保证示例稳定
- 用统一的行高、separator 和右对齐 meta 文本，确保 `240 x 320` 下可审阅
- compact 与 disabled 固定放在底部双列，便于与主卡直接对照
- 先完成示例级 `menu_flyout`，后续再决定是否需要和框架层 `menu` 做抽象收敛
