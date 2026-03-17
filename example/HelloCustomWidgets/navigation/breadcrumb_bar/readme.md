# breadcrumb_bar 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`Breadcrumb`
- 本次保留状态：`standard`、`narrow`、`read only`
- 删减效果：hover 动效、下拉溢出菜单、Acrylic、阴影与复杂图标
- EGUI 适配说明：使用固定尺寸、文字截断和静态折叠策略，在 `240 x 320` 下先保证可读性

## 1. 为什么需要这个控件

`breadcrumb_bar` 用来表达页面内的层级路径，是设置页、文档页、管理后台和资源浏览页里最常见的导航辅助控件之一。它要比 tab 更轻，比 menu 更贴近当前页面语义，也比旧 showcase 风格的路径卡更适合作为通用组件主线。

## 2. 为什么现有控件不够用

- `breadcrumb_trail` 更偏 showcase 卡片化表达，容器感和装饰感都更重
- `tab_bar` 强调平级切换，不表达层级路径
- `menu`、`button_matrix` 适合入口选择，不适合展示当前位置
- 现有示例缺少一版接近 Fluent `Breadcrumb` 的克制型标准导航条

因此需要一个新的 reference 版本，作为后续 `HelloCustomWidgets` 主线里的标准路径导航控件。

## 3. 目标场景与示例概览

- 主卡展示标准 breadcrumb，用于页面顶部路径导航
- 左下 narrow 预览展示窄宽度下的折叠策略：`首项 / ... / 当前项`
- 右下 read only 预览展示只读路径条，验证弱化对比下仍可辨识
- 页面通过点击主卡与 narrow 预览切换不同路径上下文，观察当前项高亮与文本截断

目录：

- `example/HelloCustomWidgets/navigation/breadcrumb_bar/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> standard 标签 -> 主路径条 -> 状态文案 -> 分隔线 -> narrow / read only 双预览
- 主路径条：`196 x 48`
- 底部双预览容器：`216 x 76`
- narrow 预览：`106 x 36`
- read only 预览：`106 x 36`
- 视觉规则：
  - 使用更接近浅灰白的 page panel + 白底路径条
  - 非当前项只保留文字与分隔符
  - 当前项使用轻量蓝色 pill 高亮
  - 窄宽度预览自动折叠中间层级
  - 不使用深色卡片、重阴影或科幻装饰件

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Breadcrumb Bar` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap bars to switch paths` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主路径条标签 |
| `bar_primary` | `egui_view_breadcrumb_bar_t` | 196 x 48 | docs path | 标准 breadcrumb |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Docs path` | 当前上下文状态文案 |
| `section_divider` | `egui_view_line_t` | 144 x 2 | visible | 分隔主区与预览区 |
| `compact_label` | `egui_view_label_t` | 106 x 11 | `Narrow` | 窄宽度预览标签 |
| `bar_compact` | `egui_view_breadcrumb_bar_t` | 106 x 36 | bills path | 窄宽度折叠预览 |
| `locked_label` | `egui_view_label_t` | 106 x 11 | `Read only` | 只读预览标签 |
| `bar_locked` | `egui_view_breadcrumb_bar_t` | 106 x 36 | audit path | 只读弱化预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主路径条 | narrow | read only |
| --- | --- | --- | --- |
| 默认态 | `Home / Docs / Nav / Details` | `Home / ... / Bills` | `Home / ... / Audit` |
| 点击主路径条 | 轮换 `Docs / Forms / Review` 三组上下文 | 保持当前 narrow 上下文 | 保持只读 |
| 点击 narrow | 主路径条保持当前上下文 | 在 `Bills / Access` 两组窄路径间切换 | 保持只读 |
| 只读弱化 | 不适用 | 不适用 | 路径仍清晰可读，但整体对比度降低 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待 400ms，记录默认路径
2. 点击主路径条一次，切换到 `Forms`
3. 等待 250ms
4. 再点击主路径条，切换到 `Review`
5. 等待 250ms
6. 点击 narrow 预览，切换到 `Access`
7. 末尾等待 800ms，供 runtime 抓最终截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/breadcrumb_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/breadcrumb_bar --timeout 10 --keep-screenshots
```

验收重点：

- 标题、主路径条、底部双预览都必须完整可见
- 当前项 pill 高亮要明显，但整体仍保持克制
- 分隔符不能太粗，也不能让路径显得拥挤
- narrow 预览要清楚体现折叠语义
- read only 预览在弱化后仍要可读，不能灰到看不清

## 9. 已知限制与后续方向

- 当前版本采用近似字符宽度估算，未接入真实字体测量
- 折叠策略仍是静态规则，没有实现真实的 `More` 溢出菜单
- 现阶段只覆盖英文短词路径，后续可再观察更长文本与更多层级
- reference 版已完成 30 轮留白和状态精修；如后续要上升到框架层，再考虑补齐更完整的交互状态

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `breadcrumb_trail`：本控件更轻、更平、更标准，不再走展示卡路线
- 相比 `tab_bar`：本控件表达层级路径，不承担平级切换
- 相比 `menu`：本控件用于当前位置说明，而不是入口展开
- 相比 `command_palette`：本控件是静态路径导航，不是搜索驱动操作面板

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`Breadcrumb`
- 本次保留状态：
  - `standard`
  - `current item`
  - `narrow`
  - `read only`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做 hover 动画和按压过渡
- 不做真实的 `More` 弹出菜单
- 不引入额外图标资源，只保留文字与 chevron
- 不做阴影、Acrylic、背景模糊和系统级特效

## 14. EGUI 适配时的简化点与约束

- 使用固定尺寸和静态路径样本，优先保证 `240 x 320` 下的审阅效率
- 用近似字符宽度和省略号做轻量级截断，不引入复杂测量逻辑
- 保持白底浅边框的低噪音视觉，避免回到旧 showcase 风格
- 先完成 reference 版路径条核心语义，再决定是否上升到框架层
