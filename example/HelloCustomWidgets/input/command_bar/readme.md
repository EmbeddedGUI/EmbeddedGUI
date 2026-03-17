# command_bar 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`CommandBar / Toolbar`
- 本次保留状态：`standard`、`edit / review / layout / publish`、`compact`、`disabled`
- 删除效果：Acrylic、真实图标资源、复杂溢出菜单动画、完整 hover / pressed / focus ring 体系、响应式隐藏算法
- EGUI 适配说明：保留常驻命令栏的主命令、scope pill、overflow 入口和 compact / disabled 对照；在 `240 x 320` 中优先保证低噪音、稳定留白和命令层级可读

## 1. 为什么需要这个控件？
`command_bar` 用来表达页内常驻工具栏，而不是弹出菜单或整页导航。它适合编辑、审核、布局、发布等需要“一组高频命令长期停留在页面顶部”的场景，强调主命令、当前 scope 和 overflow 入口的分层。

## 2. 为什么现有控件不够用
- `menu_flyout` 是局部弹出菜单，不是常驻命令栏
- `nav_panel`、`breadcrumb_bar`、`tab_strip` 属于导航结构，不承担主操作语义
- `message_bar`、`dialog_sheet` 偏反馈层，不是工具条
- 旧的 showcase / HMI 风格控件噪音偏高，不适合作为标准 Fluent 命令栏参考

因此需要单独实现 `command_bar`，作为 `input` 目录下更贴近 Fluent 2 / WPF UI 的常驻命令栏 reference。

## 3. 目标场景与示例概览
- 主区域展示标准 `command_bar`，覆盖 `edit / review / layout / publish` 四组 snapshot
- 左下 `Compact` 预览展示紧凑 icon-first rail
- 右下 `Disabled` 预览展示禁用态命令栏
- 主卡支持点按命令切换 focus
- 主卡支持 `Left / Right / Tab / Home / End` 键盘切焦点
- 手动查看时，点按 guide 文案可轮换主卡 snapshot；点按 `Compact` 标题可轮换 compact snapshot

目录：
- `example/HelloCustomWidgets/input/command_bar/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 296`
- 页面结构：标题 -> guide -> `Standard bar` -> 主 `command_bar` -> 状态文案 -> 分隔线 -> `Compact / Disabled` 双预览
- 主卡区域：`196 x 88`
- 底部双预览容器：`218 x 92`
- `Compact` 预览：`106 x 80`
- `Disabled` 预览：`106 x 80`
- 视觉规则：
  - 使用浅灰 page panel + 白色工具栏卡片，避免 HMI / 工业风装饰
  - 标题与 eyebrow 维持低对比、轻描边、轻阴影的 Fluent reference 语义
  - rail 内固定保留 `scope pill + 主命令 + overflow` 三段
  - compact 预览改为 icon-first，disabled 预览弱化色彩和交互

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 296 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Command Bar` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 轮换主卡 snapshot 的 guide |
| `bar_primary` | `egui_view_command_bar_t` | 196 x 88 | `edit` | 标准命令栏 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Edit: Save / Canvas` | 当前 focus 状态文案 |
| `section_divider` | `egui_view_line_t` | 136 x 2 | visible | 分隔主卡与底部预览 |
| `bar_compact` | `egui_view_command_bar_t` | 106 x 80 | `compact edit` | 紧凑预览 |
| `bar_disabled` | `egui_view_command_bar_t` | 106 x 80 | `disabled` | 禁用预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Disabled |
| --- | --- | --- | --- |
| 默认态 | `edit` | `compact edit` | `disabled` |
| 切换 1 | `review` | 保持 | 保持 |
| 切换 2 | `layout` | 保持 | 保持 |
| 切换 3 | `publish` | 保持 | 保持 |
| 紧凑切换 | 保持 | `compact review` | 保持 |
| 点按命令 | 更新 focus | 更新 focus | 不可交互 |
| 键盘焦点 | `Left / Right / Tab / Home / End` | 同步支持 | 禁用 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取 `edit` snapshot
2. 切到 `review`
3. 切到 `layout`
4. 切到底部 `Compact` 第二组 snapshot
5. 回到主卡 `publish`
6. 尾帧等待，供 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，不依赖触摸命中，保证回放稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=input/command_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/command_bar --timeout 10 --keep-screenshots
```

验收重点：
- 主卡、compact、disabled 三个命令栏都必须完整可见
- `scope pill`、命令按钮、overflow 按钮之间留白稳定，不能贴边
- 标题、短 pill、overflow `...` 需要单独检查视觉居中
- 焦点切换时，当前命令要有清晰但不过度的高亮
- disabled 预览要一眼可辨，但仍保留命令栏结构而不是整块发灰

## 9. 已知限制与下一轮迭代计划
- 当前是固定尺寸 reference 实现，未覆盖真实响应式隐藏与测量
- 当前 overflow 仅保留入口语义，没有真实弹出菜单
- 当前 glyph 使用双字母占位，不接真实图标资源
- 若后续需要沉入框架层，再评估与 `menu_flyout`、`button`、`toolbar` 抽象的复用边界

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `menu_flyout`：这里是常驻 rail，不是弹出式命令面板
- 相比 `nav_panel` / `breadcrumb_bar` / `tab_strip`：这里是操作命令，不是导航结构
- 相比 `number_box` 等输入控件：这里承载命令组，不是单一编辑值输入
- 相比旧 showcase / HMI 页面：这里强调低噪音 Fluent reference，而非场景化大装饰

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`CommandBar / Toolbar`
- 本次保留状态：
  - `edit`
  - `review`
  - `layout`
  - `publish`
  - `compact`
  - `disabled`

## 13. 相比参考原型删除了哪些效果或装饰
- 不做 Acrylic、阴影扩散和复杂菜单转场
- 不做真实图标、快捷键标签、下拉箭头菜单系统
- 不做完整 responsive overflow 隐藏逻辑
- 不做 hover、keyboard focus ring、checked / toggle 的完整桌面端细节

## 14. EGUI 适配时的简化点与约束
- 使用固定 snapshot 驱动，先保证 `240 x 320` 下的稳定 reference
- 主卡保留 `eyebrow + title + scope + command rail + footer` 五段结构
- compact 版本只保留 icon-first 语义，不再追求完整文字信息密度
- disabled 版本通过统一 palette 弱化，而不是引入额外状态层
