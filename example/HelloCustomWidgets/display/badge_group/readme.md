# badge_group 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`Badge`
- 本次保留状态：`standard`、`compact`、`read only`、`accent`、`success`、`warning`、`neutral`、`mixed group`
- 删除效果：真实图标、复杂阴影、拖拽排序、hover/focus ring、长列表滚动、桌面级动画
- EGUI 适配说明：保留多 badge 组合、tone 混合、focus badge 和 footer summary，在 `240 x 320` 页面内优先保证结构稳定和对照阅读

## 1. 为什么需要这个控件？
`badge_group` 用来展示一组语义相关的 badge，并让其中一个 focus badge 驱动整张卡片的 tone 和 footer summary。它适合出现在概览页、审阅页和状态面板里，表达“这一组标签是同一条信息的多个维度”，而不是单个数字提醒或交互筛选器。

## 2. 为什么现有控件不够用
- `notification_badge` 只解决单个角标或计数，不解决多 badge 并列展示
- `chips` 偏交互筛选和选中态，不适合作为静态信息组合
- `tag_cloud` 强调权重分布和散点云布局，不强调 focus badge 与 summary
- `card_panel` 更偏结构化信息卡，主次层级更重，不适合做轻量 badge 集群

因此单独实现 `badge_group`，作为 `display` 目录下更贴近 Fluent 2 `Badge / Badge Group` 语义的 reference 控件。

## 3. 目标场景与示例概览
- 主区域展示标准 `badge_group`，覆盖 accent / success / warning / neutral 四组 snapshot
- 左下 `Compact` 预览展示小尺寸 badge 组合在单行里的压缩布局
- 右下 `Read only` 预览展示 tone 弱化后的被动展示态
- 点击主卡轮换 4 组 snapshot，点击 compact 卡轮换 2 组 snapshot，验证 mixed tone、focus badge、footer summary 与 compact/read only 对照

目录：
- `D:\workspace\gitee\EmbeddedGUI\example\HelloCustomWidgets\display\badge_group\`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 286`
- 页面结构：标题 -> 引导文案 -> 主 `badge_group` -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡区域：`196 x 118`
- 底部双预览容器：`212 x 98`
- `Compact` 预览：`104 x 84`
- `Read only` 预览：`104 x 84`
- 视觉规则：
  - 使用浅灰 page panel + 白底低噪音卡片
  - 用 focus badge 的 tone 驱动顶部细条、eyebrow pill 和 footer strip
  - 主体 badge 允许 filled / outlined 混排，但整体不做过强装饰
  - footer 只保留 focus label + summary，避免回到 `card_panel` 的重卡片结构

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 286 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Badge Group` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap groups to review states` | 引导文案 |
| `group_primary` | `egui_view_badge_group_t` | 196 x 118 | `accent` | 标准 badge 组合卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Accent focus badge active` | 当前 focus tone 说明 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 分隔主区和底部预览 |
| `compact_label` | `egui_view_label_t` | 104 x 11 | `Compact` | compact 标题 |
| `group_compact` | `egui_view_badge_group_t` | 104 x 84 | `accent compact` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 104 x 11 | `Read only` | 只读标题 |
| `group_locked` | `egui_view_badge_group_t` | 104 x 84 | `neutral locked` | 只读静态预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `accent + mixed badges` | `accent compact` | `neutral locked` |
| 轮换 1 | `success focus` | 保持 | 保持 |
| 轮换 2 | `warning focus` | 保持 | 保持 |
| 轮换 3 | `neutral focus` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `warning compact` | 保持 |
| 只读弱化 | 不适用 | 不适用 | tone 弱化、对比降低、无交互强调 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧等待并抓取默认 accent 状态
2. 切到 success focus snapshot
3. 切到 warning focus snapshot
4. 切到底部 compact 的第二组 snapshot
5. 切到 neutral focus snapshot
6. 末尾等待一帧，给 runtime 抓最终截图

录制采用 setter + `recording_request_snapshot()`，避免依赖点击命中，保证回放稳定。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=display/badge_group PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub display/badge_group --timeout 10 --keep-screenshots
```

验收重点：
- 主卡和底部双预览必须完整可见，不能被裁切
- 主卡的 badge 组合最多两行，不能压到 footer strip
- focus badge label、eyebrow、footer 文案这些短文本必须检查视觉居中和左右留白
- badge 内部 label 与 meta 胶囊之间要有稳定空隙，不能贴边
- compact 态要保持 badge 组合语义，不能退化成普通文字卡
- read only 态要明显弱化，但仍能看清 badge 关系和 footer 总结

## 9. 已知限制与下一轮迭代计划
- 当前是固定尺寸 reference 实现，未覆盖超长文案和超过 6 个 badge 的数据
- 当前不做真实图标、可关闭按钮、键盘导航和 hover/focus ring
- 当前 badge 宽度估算基于简化字符宽度，不是完整文本测量系统
- 若后续要沉入框架层，再单独评估 `src/widget/` 抽象和更通用的数据接口

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `notification_badge`：这里不是单个计数泡，而是一组可混合 tone 的 badge 集群
- 相比 `chips`：这里不是交互筛选条，不强调选中、取消和筛选结果
- 相比 `tag_cloud`：这里不是权重词云，不做自由散点布局和字号权重编码
- 相比 `card_panel`：这里更轻、更扁平，重点在 badge 组合与 focus summary，而不是结构化信息卡

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`Badge`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `accent`
  - `success`
  - `warning`
  - `neutral`
  - `mixed group`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做 Fluent 原型里的图标徽记、头像、关闭动作和上下文菜单
- 不做真实阴影层叠、桌面级 hover 动效和高频过渡动画
- 不做动态换行列表、拖拽排序和可滚动 badge 池
- 不做复杂页面联动，只保留 focus badge 驱动整体 summary 的核心语义

## 14. EGUI 适配时的简化点与约束
- 用固定 snapshot + item 数组驱动，先保证示例稳定
- 每个 snapshot 最多 6 个 badge，先满足参考展示，不追求无限扩展
- compact 与 read only 固定放底部双列，便于与主卡直接对照
- 先完成示例级 `badge_group`，后续再决定是否抽象为通用框架控件
