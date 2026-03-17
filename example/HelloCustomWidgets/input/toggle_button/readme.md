# toggle_button 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源母本：`WPF UI`
- 对应组件名：`ToggleButton`
- 次级实现参考：`WinUI ToggleButton`
- 参考链接：
  - Fluent 2 Button usage / Toggle button：[https://fluent2.microsoft.design/components/web/react/core/button/usage#toggle-button](https://fluent2.microsoft.design/components/web/react/core/button/usage#toggle-button)
  - WPF UI Button documentation：[https://wpfui.lepo.co/documentation/button](https://wpfui.lepo.co/documentation/button)
  - WinUI ToggleButton API：[https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.togglebutton](https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.xaml.controls.primitives.togglebutton)
- 本次保留状态：`on / off`、图标 + 文本、页内单入口切换、`compact / read-only` 对照
- 删减效果：Acrylic、阴影扩散、复杂 hover/pressed 动画、命令栏级别组合布局、多段 split 结构
- EGUI 适配说明：保留标准单按钮切换语义，用 icon + label + tone color 表达状态；在 `240 x 320` 里优先保证文字可读、主按钮居中与底部双预览对照，不引入复杂焦点环和系统动画

## 1. 为什么需要这个控件？

`toggle_button` 适合表达“单个按钮本身就是一个持续状态”的页内命令控件。它不是设置页里的拨杆，也不是带下拉入口的复合按钮，而是更接近 Fluent / WPF 语义里的单入口 on/off command，例如提醒开关、可见性切换、收藏态、固定态。

## 2. 为什么现有控件不够用？

- `switch` 更像设置项里的拨杆，强调开/关位置切换，而不是页内命令按钮。
- `split_button` 与 `drop_down_button` 都带有命令展开入口，不是纯单入口切换。
- `toggle_split_button` 保留 checked + menu 的双段复合结构，信息密度更高，不适合最基础的页内切换按钮。
- `button` 只有瞬时动作，不保留 checked state。

## 3. 目标场景与示例概览

- 主区域展示标准 `toggle_button`，支持 guide 切换三组 reference snapshot：`Alerts / Visible / Favorite`。
- 主按钮支持触摸切换 on/off，并补一个轻量键盘 `Enter / Space` 闭环。
- 左下 `Compact` 预览保留 icon + text 的紧凑态，对照两组小尺寸 snapshot。
- 右下 `Read only` 预览保留可见状态但吞掉触摸与键盘，不进入真实交互。

目录：
- `example/HelloCustomWidgets/input/toggle_button/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 254`
- 页面结构：标题 -> guide -> `Standard` -> 主按钮 -> 状态文案 -> 分割线 -> `Compact / Read only`
- 主按钮尺寸：`184 x 54`
- 底部双预览容器：`208 x 66`
- 预览按钮尺寸：`100 x 54`
- 视觉规则：
  - 主卡保持低噪音浅底，按钮本体使用实色 fill 对比 on/off
  - on 态保留 Fluent 风格强调色，off 态退回浅色 surface
  - 底部预览继续保留按钮主体，不再叠加多余说明胶囊

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 254 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 16 | `Toggle Button` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 10 | 可点击 | 切换主按钮 snapshot |
| `button_primary` | `egui_view_toggle_button_t` | 184 x 54 | `Alerts / On` | 标准主按钮 |
| `status_label` | `egui_view_label_t` | 224 x 9 | `Alerts: On` | 当前状态回显 |
| `button_compact` | `egui_view_toggle_button_t` | 100 x 54 | `Compact alerts / Off` | 紧凑预览 |
| `button_readonly` | `egui_view_toggle_button_t` | 100 x 54 | `Pinned / On` | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主按钮 | Compact | Read only |
| --- | --- | --- | --- |
| on / off | 是 | 是 | 是 |
| icon + text | 是 | 是 | 是 |
| guide 切换 snapshot | 是 | 否 | 否 |
| 标签点击切换 snapshot | 否 | 是 | 否 |
| 触摸切换 | 是 | 是 | 否 |
| 键盘 `Enter / Space` | 是 | 是 | 否 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 首帧等待并截取默认 `Alerts / On`。
2. 通过 `Space` 切换主按钮到 `Alerts / Off`。
3. 点击 guide 切到 `Visible` snapshot。
4. 用 `Enter` 再切换一次主按钮。
5. 点击 `Compact` 标签切到第二组紧凑预览。
6. 再点击一次 guide，保留最终 reference 状态用于收尾截图。

录制优先使用 setter / key helper / guide click，不依赖复杂命中路径，保证回放稳定。

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
make all APP=HelloCustomWidgets APP_SUB=input/toggle_button PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/toggle_button --timeout 10 --keep-screenshots
```

验收重点：
- 主按钮 on/off 两态必须一眼可辨，不允许退回普通按钮视觉。
- 图标、文本与左右留白要均衡，不能出现图标压字或文本贴边。
- `Compact` 与 `Read only` 必须保留同一语义，但亮度和交互边界明显分开。
- 运行截图不能黑屏、空白或出现 icon/font 缺失。

## 9. 已知限制与下一轮迭代计划

- 当前 `read-only` 通过吞掉输入事件实现，不额外引入独立 read-only mode。
- 当前示例只保留单按钮语义，不包含 toolbar group、checkbox group 等更复杂组合关系。
- 当前版本已完成 30 轮视觉收口；若后续升级到框架层，可再评估统一 `read-only` API、focus ring 和核心控件化。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `switch`：这里是页内命令按钮，不是设置页拨杆。
- 相比 `split_button` / `drop_down_button`：这里没有下拉入口，只有单按钮切换。
- 相比 `toggle_split_button`：这里去掉菜单段，只保留最小 checked button 语义。
- 相比普通 `button`：这里保留持久状态，点击后要继续表达 on/off。

## 11. 参考设计系统与开源母本

- 设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级参考：`WinUI ToggleButton`

本轮保留它们共有的“单按钮 + checked state + icon/text”核心结构，不扩展桌面端额外阴影和复杂交互动效。

## 12. 对应组件命名以及本次保留的核心状态

- 组件命名：`toggle_button`
- 保留状态：
  - `Alerts / Visible / Favorite` 三组主按钮 snapshot
  - `Compact` 两组预览 snapshot
  - `Read only` 固定摘要态
  - `On / Off`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做 hover 阴影、亚克力底、系统主题联动过渡。
- 不做 toolbar/command bar 级别的组装语义。
- 不做复杂焦点环动画，只保留最小键盘切换闭环。
- 不做真实图标资源包切换，仅使用现有 icon font。

## 14. EGUI 适配时的简化点与约束

- 示例基于现有 `src/widget/egui_view_toggle_button.c`，通过本目录的样式包装补充 Reference Track 页面。
- 颜色、圆角和 key helper 由 `example/HelloCustomWidgets/input/toggle_button/egui_view_toggle_button.c` 统一包装，避免侵入现有核心控件。
- `read-only` 通过吞掉输入事件来保持可见状态，适合当前 demo 验证，但不是框架层最终 API 形态。
