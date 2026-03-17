# skeleton 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`Skeleton`
- 本次保留状态：`wave`、`pulse`、`static`
- 删减效果：复杂渐变、Acrylic、真实内容切换动画、长列表无限骨架
- EGUI 适配说明：使用固定骨架块模板、轻量定时 shimmer 和静态快照切换，在 `240 x 320` 下先保证结构可读与低噪音表达

## 1. 为什么需要这个控件

`skeleton` 用来在真实内容尚未到达时，先把页面结构、重点区域和内容密度表达出来。相比单纯的旋转 loading，它更能说明“内容将会长什么样”，适合列表、文章、设置页和卡片面板这些通用页面。

## 2. 为什么现有控件不够用

- `spinner` 只能表达“正在加载”，不能表达页面骨架结构
- `progress_bar` 更适合线性进度，不适合内容占位
- 旧 `skeleton_loader` 是深色 showcase 骨架卡，视觉更重，也更偏演示页
- 当前主线缺少一版接近 Fluent 的浅色、低噪音 skeleton reference

因此这里不在旧控件上修补，而是按 `Fluent 2 / WPF UI` 的参考方向重做一个更标准的 `skeleton`。

## 3. 目标场景与示例概览

- 主卡展示标准 `wave` skeleton，轮换 `Article / Feed / Settings` 三类页面骨架
- 左下 `Pulse` 预览展示更紧凑的小尺寸骨架和脉冲强调
- 右下 `Static` 预览展示只读静态骨架，验证在无动画时依然能看出结构层级
- 页面通过点击主卡和 `Pulse` 预览切换不同骨架快照，观察页内 loading 结构变化

目录：

- `example/HelloCustomWidgets/feedback/skeleton/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> `Wave` 标签 -> 主骨架卡 -> 分隔线 -> `Pulse / Static` 双预览
- 主骨架卡：`196 x 124`
- 底部双预览容器：`214 x 86`
- `Pulse` 预览：`106 x 60`
- `Static` 预览：`106 x 60`
- 视觉规则：
  - 使用浅灰白 page panel + 白底轻边框容器
  - 骨架块采用浅灰填充，不做重阴影
  - `Wave` 用轻量 shimmer，`Pulse` 用局部脉冲强调，`Static` 保持无动画弱化展示
  - 当前强调块只做轻微 accent，不做高饱和强描边

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Skeleton` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap cards to switch loading` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Wave` | 主骨架标签 |
| `skeleton_primary` | `egui_view_skeleton_t` | 196 x 124 | `Feed` | 主 `wave` 骨架 |
| `section_divider` | `egui_view_line_t` | 132 x 2 | visible | 主区与预览区分隔线 |
| `pulse_label` | `egui_view_label_t` | 106 x 11 | `Pulse` | 脉冲预览标签 |
| `skeleton_pulse` | `egui_view_skeleton_t` | 106 x 60 | `Pulse tile` | 紧凑脉冲预览 |
| `static_label` | `egui_view_label_t` | 106 x 11 | `Static` | 静态预览标签 |
| `skeleton_static` | `egui_view_skeleton_t` | 106 x 60 | `Static` | 静态弱化预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主骨架 | Pulse | Static |
| --- | --- | --- | --- |
| 默认态 | `Feed` wave | `Pulse tile` | `Static` |
| 点击主骨架 | 轮换 `Article / Feed / Settings` | 保持当前 pulse 预览 | 保持静态 |
| 点击 Pulse | 主骨架保持当前快照 | 在 `Pulse row / Pulse tile` 间切换 | 保持静态 |
| 静态弱化 | 不适用 | 不适用 | 保持无动画、弱对比、只做结构提示 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 录制阶段使用三段 `wait`，优先抓取当前默认 `Feed` reference 状态
2. 主卡与 `Pulse` 的其他快照主要通过手动点击验证
3. runtime 侧重点放在结构完整性、层级对比和低噪音呈现是否稳定

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=feedback/skeleton PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub feedback/skeleton --timeout 10 --keep-screenshots
```

验收重点：

- 标题、主骨架和底部双预览都必须完整可见
- 主骨架里的 shimmer 不能太花，要保持轻量
- `Pulse` 与 `Static` 的差异要能从截图中直接辨认
- 骨架短块、圆形头像位和小胶囊位要检查真实居中与左右留白
- 不能回到旧 `skeleton_loader` 的深色 showcase 风格

## 9. 已知限制与下一轮迭代计划

- 当前 shimmer 仍是简化版条带扫光，不是完整渐变波浪
- 骨架模板仍使用固定快照，未做运行时自由拼装
- 现阶段只覆盖小型页面骨架，不包含更长列表和复杂表单
- 先完成 reference 版 30 轮收敛，再决定是否沉入框架层

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `skeleton_loader`：本控件更浅、更轻、更标准，不再走深色 showcase 卡路线
- 相比 `spinner`：本控件表达内容结构，不只是等待状态
- 相比 `progress_bar`：本控件表达页面骨架，不承担数值进度反馈
- 相比 `card_panel` 候选方向：本控件只表达“加载前占位”，不承载真实内容卡片

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`Skeleton`
- 本次保留状态：
  - `wave`
  - `pulse`
  - `static`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做真实内容淡入切换
- 不做复杂渐变和高频 shimmer 动画
- 不做长列表无限滚动骨架
- 不做阴影、Acrylic、背景模糊和系统级转场

## 14. EGUI 适配时的简化点与约束

- 使用固定骨架块模板，优先保证 `240 x 320` 下的审阅效率
- shimmer 用轻量定时刷新实现，避免引入重型动画系统
- 骨架块使用统一浅灰白配色，保持 Fluent 风格的低噪音参考页
- 先完成示例级 reference 版，再决定是否上升到框架公共控件
