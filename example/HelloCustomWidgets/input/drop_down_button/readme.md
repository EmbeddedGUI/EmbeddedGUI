# drop_down_button 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`DropDownButton`
- 本次保留状态：`default`、`accent`、`warning`、`compact`、`read-only`
- 删除效果：Acrylic、真实图标资源、桌面级 hover/pressed 动画、多级真实 flyout 弹出定位、复杂阴影扩散
- EGUI 适配说明：保留“整按钮触发下拉命令”的核心语义，重点验证标题、辅助文案、展开入口提示、紧凑态与只读态对照；在 `240 x 320` 下优先保证按钮轮廓、文案节奏和弹出内容摘要清晰可辨

## 1. 为什么需要这个控件？

`drop_down_button` 用来表达“单一主入口 + 展开更多动作”的标准页内按钮语义。它适合过滤、排序、视图切换、导出选项、快捷布局等场景：用户点击整个按钮即可打开下拉动作集，而不是先点击主动作、再单独点击菜单段。

## 2. 为什么现有控件不够用？

- `split_button` 强调“主动作 + 菜单段”双入口，不等于 `DropDownButton` 的整按钮展开语义
- `toggle_split_button` 带 checked state，重点是复合切换，不是单入口命令展开
- `menu_flyout` 是独立弹出面板，本身不是按钮控件
- `command_bar` 承担的是常驻工具栏语义，不是单个标准下拉按钮

## 3. 目标场景与示例概览

- 主区域展示标准 `drop_down_button`，覆盖 `Sort`、`Layout`、`Theme` 三组 snapshot
- 左下 `Compact` 预览展示紧凑宽度下的轻量版本
- 右下 `Read-only` 预览展示可见但不可交互的只读版本
- guide 文案点击后轮换主卡 snapshot
- compact 标题点击后轮换紧凑态 snapshot
- 状态文案同步回显当前主卡标题、当前展开项和语义说明

目录：
- `example/HelloCustomWidgets/input/drop_down_button/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> guide -> `Standard` 标签 -> 主卡按钮 -> 状态文案 -> 分隔线 -> `Compact / Read-only` 双预览
- 主卡区域：`194 x 74`
- 底部双预览容器：`218 x 84`
- `Compact` 预览：`106 x 66`
- `Read-only` 预览：`106 x 66`
- 视觉规则：
  - 使用浅灰 page panel + 白色按钮表面，保持 Fluent 2 的低噪音页内控件语气
  - 保留按钮标题、右侧展开提示和底部 helper 文案三段信息层
  - 主卡强调按钮整体是单一点击面，而不是左右分段
  - compact 版压缩标题长度与 helper 文案，但保留清晰边界
  - read-only 版保留结构和信息层，不做额外装饰性禁用遮罩

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Drop Down Button` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 轮换主卡 snapshot |
| `primary_button` | `egui_view_drop_down_button_t` | 194 x 74 | `sort` | 标准主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Sort menu ready` | 状态回显 |
| `compact_button` | `egui_view_drop_down_button_t` | 106 x 66 | `compact` | 紧凑预览 |
| `readonly_button` | `egui_view_drop_down_button_t` | 106 x 66 | `read-only` | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read-only |
| --- | --- | --- | --- |
| 默认态 | `Sort` | `Compact sort` | `Locked layout` |
| 切换 1 | `Layout` | 保持 | 保持 |
| 切换 2 | `Theme` | 保持 | 保持 |
| 紧凑切换 | 保持 | `Compact filter` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 始终只读 |
| guide 点击 | 主卡轮换 | 不影响 | 不影响 |
| compact 标签点击 | 不影响 | 紧凑态轮换 | 不影响 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待并抓取默认 `Sort` snapshot
2. 切换到 `Layout`
3. 切换到 `Theme`
4. 切换到底部 `Compact` 的第二组 snapshot
5. 回到主卡默认 `Sort`
6. 末尾等待一帧，供 runtime 抓最终截图

录制优先采用 setter + `recording_request_snapshot()`，避免依赖真实点击命中，提高回放稳定性。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/drop_down_button PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/drop_down_button --timeout 10 --keep-screenshots
```

验收重点：
- 主卡、compact、read-only 三个按钮都必须完整可见
- 按钮整体必须被感知为单一点击面，不能误读成 split button
- 标题、helper、状态文案要逐项检查水平居中和左右留白
- 展开提示区不能贴边，也不能压住标题
- read-only 预览要一眼可辨，但仍保留按钮层级和文案节奏

## 9. 已知限制与下一轮迭代计划

- 当前先做示例级 `DropDownButton`，不实现真实 flyout 布局系统
- 当前 glyph 先用文本占位，不接真实图标资源
- 当前优先固定尺寸 reference，不覆盖更大宽度和多分辨率自适应
- 如果后续要沉入框架层，再评估和 `button`、`menu_flyout` 的复用边界

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `split_button`：这里是整按钮展开，不是主段 / 菜单段双入口
- 相比 `toggle_split_button`：这里没有 checked 复合语义
- 相比 `menu_flyout`：这里是按钮控件，不是独立弹出菜单容器
- 相比 `command_bar`：这里是单控件入口，不承担整条工具栏布局职责

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`DropDownButton`
- 本次保留状态：
  - `sort`
  - `layout`
  - `theme`
  - `compact`
  - `read-only`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做桌面级 hover、pressed、focus ring 动画
- 不做真实弹层定位、滚动列表和多级 submenu
- 不做真实图标资产、快捷键标签和额外徽标
- 不做 Acrylic、复杂阴影和系统主题联动

## 14. EGUI 适配时的简化点与约束

- 先用固定 snapshot 驱动，优先保证 `240 x 320` 下的 reference 可审阅性
- 主卡保留 `title + trigger hint + helper` 三段结构
- compact 版只保留最必要的标题和提示层
- read-only 版通过统一 palette 降噪，而不是新增装饰层
