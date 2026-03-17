# tab_strip 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`TabView` / `TabStrip`
- 本次保留状态：`standard`、`compact`、`read only`
- 删减效果：hover 动画、关闭按钮、拖拽重排、Acrylic、图标页签和复杂阴影
- EGUI 适配说明：使用纯文本页签、固定宽度容器和轻量 underline 指示器，在 `240 x 320` 下优先保证页内导航的可读性

## 1. 为什么需要这个控件

`tab_strip` 用来承载同一页面上下文中的平级 section 切换，例如设置页里的 `Overview / Usage / Access`、文档页里的 `Home / Logs / Audit`。它比 `breadcrumb` 更偏平级导航，也比 `tab_expose` 这种总览页签更轻，适合直接放在页面内容上方作为标准页内导航条。

## 2. 为什么现有控件不够用

- `src/widget/egui_view_tab_bar.*` 采用均分宽度，适合简单样例，但不够接近 Fluent 的轻量页签条
- `tab_expose` 更像“多页签总览面板”，不是正文内的标准 tab strip
- `button_matrix`、`menu` 更偏入口按钮或菜单，不是当前 section 导航
- 当前主线缺少一版贴近 `Fluent 2 / WPF UI` 的低噪音标签导航控件

因此这里不在旧 `tab_bar` 上做小修小补，而是单独实现一个 reference 版 `tab_strip`。

## 3. 目标场景与示例概览

- 主区域展示标准 `tab_strip`，提供 `Overview / Usage / Access` 三个 section
- 左下 `Compact` 预览展示窄宽度下的轻量 tab strip
- 右下 `Read only` 预览展示只读弱化版标签条
- 页面通过点击实际标签切换当前项，观察 variable width、当前项文字强调和 underline 指示器是否稳定

目录：

- `example/HelloCustomWidgets/navigation/tab_strip/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主标签条 -> 当前 section 文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主标签条：`204 x 46`
- 底部双预览容器：`216 x 76`
- `Compact` 预览：`106 x 36`
- `Read only` 预览：`106 x 36`
- 视觉规则：
  - 使用浅灰白 page panel + 白底轻边框容器
  - 页签不做等宽分栏，按文字内容宽度和留白自然排布
  - 当前项只保留轻量文字强调、淡色面片和 underline 指示器
  - 非当前项保持低噪音灰度文字，不做强描边 pill
  - 不引入深色卡片、工业风装饰件或总览式缩略图

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Tab Strip` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap tabs to switch focus` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主标签条标签 |
| `bar_primary` | `egui_view_tab_strip_t` | 204 x 46 | `Overview` | 标准标签条 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Overview pane ready` | 当前主 section 状态文案 |
| `section_divider` | `egui_view_line_t` | 144 x 2 | visible | 分隔主区与预览区 |
| `compact_label` | `egui_view_label_t` | 106 x 11 | `Compact` | 紧凑预览标签 |
| `bar_compact` | `egui_view_tab_strip_t` | 106 x 36 | `Home` | 窄宽度标签条 |
| `locked_label` | `egui_view_label_t` | 106 x 11 | `Read only` | 只读预览标签 |
| `bar_locked` | `egui_view_tab_strip_t` | 106 x 36 | `Usage` | 只读弱化标签条 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主标签条 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `Overview` | `Home` | `Usage` |
| 点击主标签条 | 切换到被点中的 tab，并更新状态文案 | 保持当前 compact 选择 | 保持只读 |
| 点击 Compact | 主标签条不变 | 在 `Home / Logs` 间切换 | 保持只读 |
| 只读弱化 | 不适用 | 不适用 | 标签仍可辨识，但整体对比度降低且不响应点击 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧通过 setter 固定为默认 `Overview`
2. 第二帧切到主标签条 `Usage`
3. 第三帧切到主标签条 `Access`
4. 第四帧切到 `Compact` 预览的 `Logs`
5. 每次切换时请求精确快照，确保 runtime 直接抓到稳定的焦点状态

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/tab_strip PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/tab_strip --timeout 10 --keep-screenshots
```

验收重点：

- 主标签条、状态文案和底部双预览都必须完整可见
- variable width tab 不能退化成 `tab_bar` 的均分视觉
- 当前项 underline 要清晰，但不能过厚或过花
- `Compact` 要在窄宽度下仍保持可读
- `Read only` 弱化后仍能看出当前页签，不可灰到失真

## 9. 已知限制与后续方向

- 当前版本只覆盖文本页签，不做图标、关闭按钮和拖拽重排
- 文字宽度仍采用轻量估算，没有接入真实字体测量
- 本轮先完成 reference 版 30 次收敛，后续再看是否需要沉入框架层
- 如果后续要对齐更完整的 `TabView`，再补内容面板联动和更多交互状态

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `tab_bar`：本控件按内容宽度排布，不走等宽分栏
- 相比 `tab_expose`：本控件是页内导航条，不是多页签总览面板
- 相比 `breadcrumb_bar`：本控件表达平级切换，不表达层级路径
- 相比 `button_matrix`：本控件强调当前 section 和 underline 语义，不是按钮阵列

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`TabView` / `TabStrip`
- 本次保留状态：
  - `standard`
  - `current tab`
  - `compact`
  - `read only`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做 hover、按压、焦点环和复杂切换动画
- 不做图标页签、关闭按钮和拖拽排序
- 不做阴影、Acrylic、背景模糊和系统级特效
- 不做与 `tab_expose` 类似的页面缩略图和总览卡

## 14. EGUI 适配时的简化点与约束

- 使用纯文本 tab 和轻量 underline，优先保证 `240 x 320` 下的审阅效率
- 通过近似字符宽度和省略号控制 tab 宽度，不引入复杂布局系统
- 保持浅色低噪音 reference 方向，避免回到 showcase 风格
- 先完成示例级页内导航条，再决定是否上升到框架公共控件
