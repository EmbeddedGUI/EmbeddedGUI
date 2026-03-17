# settings_panel 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`SettingCard` / `SettingsGroup`
- 本次保留状态：`standard`、`compact`、`read only`、`accent`、`success`、`warning`、`neutral`
- 删除效果：真实图标、Acrylic、长列表滚动、复杂 hover/focus ring、桌面级展开动效
- EGUI 适配说明：保留设置卡分组、focus row、value pill / switch / chevron 尾部语义，在 `240 x 320` 页面里优先保证卡片节奏和信息密度稳定

## 1. 为什么需要这个控件？
`settings_panel` 用来表达一组统一风格的设置卡行，适合出现在设置页、偏好面板和系统页中。它强调分组卡片、row focus、尾部值/开关语义，以及 compact/read only 对照，而不是通用列表或普通卡片容器。

## 2. 为什么现有控件不够用
- `card_panel` 更偏摘要卡，不是标准设置页的多行 setting cards
- `list` 只有列表语义，没有 Fluent 设置卡层级和尾部控件表达
- `nav_panel` 偏导航，不适合承载 value pill、switch 和 read only 设置行
- `number_box` 只解决单个输入，不解决设置分组布局

因此单独实现 `settings_panel`，作为 `layout` 目录下贴近 Fluent `SettingCard / SettingsGroup` 的 reference 控件。

## 3. 目标场景与示例概览
- 主区域展示标准 `settings_panel`，覆盖 accent / success / warning / neutral 四组 snapshot
- 左下 `Compact` 预览展示小尺寸 settings rows 的高密度布局
- 右下 `Read only` 预览展示弱化后的只读设置面板
- 点击主卡轮换 4 组 snapshot，点击 compact 卡轮换 2 组 snapshot，验证 value pill、switch、chevron 和 focus row 对照

目录：
- `D:\workspace\gitee\EmbeddedGUI\example\HelloCustomWidgets\layout\settings_panel\`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 300`
- 页面结构：标题 -> 引导文案 -> 主 `settings_panel` -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡区域：`196 x 132`
- 底部双预览容器：`212 x 98`
- `Compact` 预览：`104 x 84`
- `Read only` 预览：`104 x 84`
- 视觉规则：
  - 使用浅灰 page panel + 白底低噪音卡片
  - focus row 的 tone 驱动顶部细条、eyebrow pill 和 footer strip
  - 中部 group area 承载 2-3 行 setting cards，维持统一边距和分隔节奏
  - 尾部控件只保留 `value pill`、`switch`、`chevron` 三种最小语义

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 300 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Settings Panel` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap panels to review states` | 引导文案 |
| `panel_primary` | `egui_view_settings_panel_t` | 196 x 132 | `accent` | 标准设置卡分组 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Accent settings row active` | 当前 focus tone 说明 |
| `section_divider` | `egui_view_line_t` | 148 x 2 | visible | 分隔主区和底部预览 |
| `compact_label` | `egui_view_label_t` | 104 x 11 | `Compact` | compact 标题 |
| `panel_compact` | `egui_view_settings_panel_t` | 104 x 84 | `accent compact` | 紧凑预览 |
| `locked_label` | `egui_view_label_t` | 104 x 11 | `Read only` | 只读标题 |
| `panel_locked` | `egui_view_settings_panel_t` | 104 x 84 | `neutral locked` | 只读静态预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `accent value row` | `accent compact` | `neutral locked` |
| 轮换 1 | `success switch row` | 保持 | 保持 |
| 轮换 2 | `warning update row` | 保持 | 保持 |
| 轮换 3 | `neutral privacy row` | 保持 | 保持 |
| 紧凑轮换 | 保持 | `warning compact` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 对比降低、切换只保留被动展示 |

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
make all APP=HelloCustomWidgets APP_SUB=layout/settings_panel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/settings_panel --timeout 10 --keep-screenshots
```

验收重点：
- 主卡和底部双预览必须完整可见，不能被裁切
- 中部 setting rows 的上下节奏必须稳定，不能压住 footer strip
- value pill、switch、chevron 这些短尾部控件必须检查视觉居中和左右留白
- compact 态要保持设置卡语义，不能退化成普通列表
- read only 态要明显弱化，但仍能看清分组和尾部信息

## 9. 已知限制与下一轮迭代计划
- 当前是固定尺寸 reference 实现，未覆盖超长标题和超过 4 行 setting cards
- 当前不做真实图标、真实可编辑输入和键盘导航
- 当前 switch 只是语义性绘制，不接入真实开关状态逻辑
- 若后续要沉入框架层，再单独评估 `src/widget/` 抽象和设置页数据接口

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `card_panel`：这里不是摘要卡，而是多行 settings cards 分组
- 相比 `list`：这里不是通用列表，重点在 setting card 节奏和尾部控件语义
- 相比 `nav_panel`：这里不是导航侧栏，不强调分组入口和 rail 选中
- 相比 `number_box`：这里不是单个输入控件，而是设置面板容器语义

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`SettingCard` / `SettingsGroup`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `accent`
  - `success`
  - `warning`
  - `neutral`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做真实 Fluent 图标和完整设置页滚动容器
- 不做 hover、focus ring、展开/折叠和大段说明文本
- 不做桌面级输入控件嵌入，只保留尾部最小语义
- 不做复杂页面组装，只保留单组 settings panel 的核心 reference

## 14. EGUI 适配时的简化点与约束
- 用固定 snapshot + item 数组驱动，先保证示例稳定
- 每个 snapshot 最多 4 行 setting cards，先满足参考展示
- compact 与 read only 固定放底部双列，便于与主卡直接对照
- 先完成示例级 `settings_panel`，后续再决定是否抽象为通用框架控件
