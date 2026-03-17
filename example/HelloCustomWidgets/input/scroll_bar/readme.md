# scroll_bar 设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`WinUI ScrollBar`
- 对应组件名：`ScrollBar`
- 保留状态：`decrease button + track + thumb + increase button`、`content / viewport / offset`、`line/page step`、`compact / read-only` 对照
- 删除效果：系统级阴影、悬浮提示、自动隐藏滚动条、跨轴联动与容器级惯性滚动
- EGUI 适配说明：保留“独立滚动条 + viewport 预览 + 标准按钮/轨道/滑块语义”，压缩到 `240 x 320` 页面；不直接滚动容器内容，只表达 viewport 位置与尺寸

## 1. 为什么需要这个控件
`scroll_bar` 用来表达“在一个独立、可复用的标准滚动条里，用 thumb 的位置和尺寸表示 viewport 在长内容中的位置与可视比例”的语义，适合文档浏览、日志查看、时间线、属性面板与列表侧边轨道等场景。

## 2. 为什么现有控件不够用
- `slider` 更偏数值选择，thumb 大小固定，不表达 viewport 尺寸。
- `scroll` 是容器行为，直接驱动子内容滚动，不是独立标准控件。
- `progress_bar` 只有进度展示，没有按钮、轨道分页和拖拽定位语义。
- `number_box` / `stepper` 偏离散数值输入，不适合表达长内容浏览位置。

## 3. 目标场景与示例概览
- 主区展示标准 `ScrollBar`：左侧 viewport preview，右侧按钮 + track + thumb。
- 底部左侧 `Compact` 预览只保留比例摘要与精简轨道。
- 底部右侧 `Read only` 预览展示只读、去交互的静态摘要态。
- 主卡支持键盘 `Tab / Up / Down / Home / End / +/- / Enter / Space / Esc`。
- 主卡支持 touch：点击减/增按钮、点击 track 分页、拖拽 thumb。
- 目录：`example/HelloCustomWidgets/input/scroll_bar/`

## 4. 视觉与布局规格
- 页面尺寸：`240 x 320`
- 根布局：`224 x 288`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文本 -> 分隔线 -> `Compact / Read only`
- 主卡尺寸：`196 x 146`
- 底部双预览容器：`216 x 68`
- `Compact` 预览：`104 x 52`
- `Read only` 预览：`104 x 52`
- 视觉规则：
  - 主卡使用浅色卡片、低噪音边框和柔和 preview 蓝色。
  - 左侧 preview 明确展示 viewport 高度，避免退化成 slider。
  - 右侧滚动条保留标准上/下按钮、轨道与 thumb 抓手线。
  - compact 退化为比例 pill + 精简轨道；read-only 进一步弱化强调色。

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 288 | enabled | 页面根容器 |
| `scroll_bar_primary` | `egui_view_scroll_bar_t` | 196 x 146 | `840 / 220 / 168` | 标准独立滚动条主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Docs 168 / 620` | 当前 offset / max offset 状态反馈 |
| `scroll_bar_compact` | `egui_view_scroll_bar_t` | 104 x 52 | compact | 紧凑态比例预览 |
| `scroll_bar_locked` | `egui_view_scroll_bar_t` | 104 x 52 | compact + read-only | 只读态预览 |

## 6. 状态覆盖矩阵
- 标准态：完整 label / helper / viewport preview / 按钮 / track / thumb。
- `viewport preview`：根据 `viewport_length / content_length` 显示可视区高度。
- `thumb position`：根据 `offset / max_offset` 显示 viewport 位置。
- `line step`：按钮点击或 `Up / Down` 键步进。
- `page step`：track 点击或 `+ / -` 键分页。
- `dragging`：thumb 可拖拽到任意位置。
- `compact`：隐藏 label/helper/preview，仅保留比例 pill 与精简轨道。
- `read-only`：保留显示，忽略 touch / key。
- 键盘态：
  - `Tab` 在 `thumb / increase / decrease` 三个部件之间循环。
  - `Up / Down` 执行 line step。
  - `Home / End` 跳到顶部 / 底部。
  - `+ / -` 执行 page step。
  - `Enter / Space` 激活当前 focused part。
  - `Esc` 将 focus 收回到 thumb。

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 初始化标准主卡与底部双预览。
2. 输出稳定首帧。
3. 执行 `Down`，展示 line step。
4. 输出 line step 结果帧。
5. 执行 `+`，展示 page step。
6. 输出 page step 结果帧。
7. 执行 `End`，展示 jump to bottom。
8. 输出底部状态帧。
9. 点击 guide，切换到下一组主卡预设。
10. 输出切换后的主卡帧。
11. 点击 `Compact` 标签，切换 compact 预设并输出收尾帧。

## 8. 编译、runtime、截图验收标准
- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/scroll_bar PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/scroll_bar --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - preview 必须清楚表达 viewport 大小与位置。
  - 按钮 / track / thumb 必须层级清晰，不能与 slider 混淆。
  - compact / read-only 对照必须明显区分。

## 9. 已知限制与下一轮迭代计划
- 当前实现固定为纵向滚动条，不覆盖横向方向。
- 当前不处理 hover / auto-hide / overlay scrollbar 动效。
- 当前不直接驱动容器内容，只表达标准滚动条语义。
- 若后续沉淀到框架层，可补横向支持、容器绑定接口与更细的 accessibility 语义。

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `slider`：这里的 thumb 尺寸由 viewport 决定，目标是定位内容窗口，不是选择抽象数值。
- 相比 `scroll`：这里是独立标准控件，不负责 child layout、惯性或内容实际滚动。
- 相比 `progress_bar`：这里有按钮、track page、thumb drag 与 focus part 语义。
- 相比 `number_box` / `stepper`：这里强调连续内容浏览，不是离散数值编辑。

## 11. 参考设计系统与开源母本
- `Fluent 2`：提供低噪音标准滚动条视觉方向。
- `WPF UI`：提供 Windows 风格 `ScrollBar` 的按钮 / track / thumb 结构参考。
- `WinUI ScrollBar`：补充 viewport-sized thumb 与 line/page step 语义。

## 12. 对应组件名与保留核心状态
- 对应组件名：`ScrollBar`
- 本次保留的核心状态：
  - `content_length / viewport_length / offset`
  - `decrease / track / thumb / increase`
  - `line_step / page_step`
  - `compact` 对照态
  - `read-only` 对照态

## 13. 相比参考原型删掉的效果或装饰
- 不做系统级 auto-hide scrollbar。
- 不做 hover-only 扩宽、阴影浮层和 tooltip。
- 不做容器级惯性滚动与 overscroll 回弹。
- 不做多轴滚动或 overlay 浮动样式。

## 14. EGUI 适配时的简化点与约束
- 固定在 `240 x 320` 页面内优化，优先保证 preview + rail 可读。
- compact 态收缩为比例摘要与精简轨道，不继续显示完整 preview。
- 通过 `get_part_region()` 暴露按钮 / track / thumb 命中区域，便于单测与录制。
- 通过 `handle_navigation_key()` 收口标准键盘语义，避免把滚动条退化成普通 value slider。
