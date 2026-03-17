# nav_panel 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`NavigationView / NavView`
- 本次保留状态：`standard`、`compact`、`read only`
- 删除效果：pane 展开/收起动画、分层树形导航、Acrylic、图标资源、系统级焦点与键盘快捷操作
- EGUI 适配说明：以轻量常驻导航面板为核心，保留 header、selection indicator、底部 settings 语义，在 `240 x 320` 下优先保证页内侧栏的可读性与点击区域

## 1. 为什么需要这个控件

`nav_panel` 用来承载页内常驻导航，而不是弹出菜单或一次性的命令入口。它适合设置页、管理页和工具页左侧的一级切换，让用户能持续看到“当前在哪个分区”。

## 2. 为什么现有控件不够用

- `menu` 更偏弹出式命令列表，不适合作为常驻导航面板
- `breadcrumb_bar` 表达的是路径层级，不负责平级导航切换
- `tab_strip` 更像页签条，通常横向承载少量页面
- 当前仓库缺少一版贴近 `Fluent NavigationView` 语义的轻量侧边导航

因此这里单独实现 `nav_panel`，而不是继续在现有控件上打补丁。

## 3. 目标场景与示例概览

- 主区域展示标准 `nav_panel`，对应桌面或平板页面里的左侧常驻导航
- 左下 `Compact` 预览展示窄宽度下的图标化 rail
- 右下 `Read only` 预览展示只读弱化态，用于静态预览或受限场景
- 页面通过点选导航项切换选中态，并观察 selection indicator、badge、settings footer 与弱化态层级

目录：

- `example/HelloCustomWidgets/navigation/nav_panel/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 284`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主导航面板 -> 当前状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主导航面板：`196 x 110`
- 底部双预览容器：`216 x 88`
- `Compact` 预览：`56 x 72`
- `Read only` 预览：`56 x 72`
- 视觉规则：
  - 使用浅灰 page panel + 白底低噪音边框
  - 标准态保留 header、小 badge、选中背景与左侧 selection indicator
  - compact 态压缩成窄 rail，只保留 badge 与选中条
  - 只读态弱化 accent 与文字，不做额外交互装饰

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 284 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Nav Panel` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap items to move focus` | 引导文案 |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主导航标签 |
| `panel_primary` | `egui_view_nav_panel_t` | 196 x 110 | `Overview` 选中 | 标准导航面板 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Overview pane ready` | 当前选中状态文案 |
| `section_divider` | `egui_view_line_t` | 144 x 2 | visible | 分隔主区域与预览区 |
| `compact_label` | `egui_view_label_t` | 106 x 11 | `Compact` | 紧凑预览标签 |
| `panel_compact` | `egui_view_nav_panel_t` | 56 x 72 | `Home` 选中 | compact rail 预览 |
| `locked_label` | `egui_view_label_t` | 106 x 11 | `Read only` | 只读预览标签 |
| `panel_locked` | `egui_view_nav_panel_t` | 56 x 72 | `Teams` 选中 | 只读弱化 rail |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主导航面板 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `Overview` | `Home` | `Teams` |
| 点击第 2 项 | 切到 `Library` | 切到 `Files` | 不响应 |
| 点击第 3 项 | 切到 `People` | 切到 `Rules` | 不响应 |
| 只读弱化 | 不适用 | 不适用 | 保留选中态但取消交互 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待，记录默认 `Overview`
2. 切到主导航第 2 项
3. 切到主导航第 3 项
4. 回到主导航第 2 项，验证中间状态回切
5. 切到 compact rail 第 3 项
6. 末尾等待一帧，供 runtime 抓最终截图

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/nav_panel PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/nav_panel --timeout 10 --keep-screenshots
```

验收重点：

- 主导航面板必须明显是常驻侧边导航，而不是 list 或 popup menu
- header、选中背景、左侧 indicator、badge 与底部 settings 要同时可辨识
- compact rail 需要在小尺寸下保持当前项可读
- `Read only` 必须和可交互态拉开层级，但不能发灰发脏
- 文字与 badge、边框、选中条之间要保留合理留白

## 9. 已知限制与后续方向

- 当前版本不做展开/收起 pane、树形层级与二级导航
- 当前不做真实图标资源，使用 badge 字母替代
- 当前不做键盘导航、搜索框和顶栏命令区
- 本轮先完成 reference 版 30 次收敛，后续再看是否需要沉入框架层

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `menu`：本控件是页内常驻导航，不是弹出命令菜单
- 相比 `breadcrumb_bar`：本控件表达分区切换，不表达路径层级
- 相比 `tab_strip`：本控件是纵向侧栏，不是横向页签
- 相比 `list`：本控件有明确 selection indicator、header 与 footer 语义

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`NavigationView / NavView`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `selected`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做顶部搜索框、pane toggle 与 command bar
- 不做系统图标资源与复杂分组图标
- 不做树形层级展开、二级导航和过渡动画
- 不做 Acrylic、阴影与大面积桌面窗口装饰

## 14. EGUI 适配时的简化点与约束

- 使用固定项数与固定 badge 文本，优先保证 `240 x 320` 下的可审阅性
- 通过 selection indicator + 轻量填充表达选中态，不引入复杂 hover 动画
- compact 与 read only 统一用窄 rail 表达，降低小尺寸布局压力
- 先完成示例级导航面板，再决定是否抽象成通用框架控件
