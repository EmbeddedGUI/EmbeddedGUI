# swipe_control

## 1. 为什么需要这个控件
`swipe_control` 用来表达“列表行内容默认保持整洁，只有在滑动或键盘 reveal 后才暴露上下文操作”的标准交互语义，适合消息收件箱、任务队列、审核列表这类以单行内容为主、但又需要快速操作的场景。

## 2. 为什么现有控件不够用
- `settings_panel` 和 `data_list_panel` 更偏静态信息行，不承担 reveal actions 的交互语义。
- `split_button` / `toggle_split_button` 强调点击触发的复合按钮，而不是整行内容卡的滑动暴露操作。
- `flip_view` 强调整卡翻页浏览，不是单行列表项上的局部 action reveal。

## 3. 目标场景与示例概览
- 标准态：主行卡默认只显示内容，右键盘或右滑 reveal 起始操作，左键盘或左滑 reveal 末端操作。
- 状态栏：同步反馈当前 track、当前 reveal 侧和当前行标题。
- compact 预览：保留同一控件语义，但压缩为更短标题和更轻量 rails。
- read-only 预览：保留视觉结构，但完全冻结 reveal 交互，验证禁用边界。

## 4. 视觉与布局规格
- 根容器尺寸：`224 x 300`
- 主控件尺寸：`196 x 118`
- 紧凑预览尺寸：`104 x 64`
- 顶部结构：标题、guide、section label
- 主行要求：内容卡、eyebrow pill、状态 pill、accent rail、footer 文案都要完整可见
- action rail 要求：`start_action` / `end_action` 文案真实居中，左右留白平衡

## 5. 控件清单

| 变量名 | 类型 | 尺寸 | 初始状态 | 用途 |
| --- | --- | --- | --- | --- |
| `swipe_control_primary` | `egui_view_swipe_control_t` | `196 x 118` | `Inbox / surface` | 标准主视图 |
| `swipe_control_compact` | `egui_view_swipe_control_t` | `104 x 64` | `Compact / surface` | 紧凑态对照 |
| `swipe_control_locked` | `egui_view_swipe_control_t` | `104 x 64` | `Read only / surface` | 只读态对照 |
| `primary_tracks` | `swipe_control_track_t[3]` | - | `Inbox` | 主视图切换的行数据轨 |
| `compact_tracks` | `swipe_control_track_t[2]` | - | `Compact` | 紧凑态切换的数据轨 |

## 6. 状态覆盖矩阵

| Track | Snapshot | 关键状态 | 语义 |
| --- | --- | --- | --- |
| `Inbox` | `Invoice follow-up` | 默认 surface | 标准行卡保持关闭态 |
| `Inbox` | `Pin` | start reveal | 起始侧操作暴露 |
| `Inbox` | `Delete` | end reveal | 末端侧操作暴露 |
| `Planner` | `Planner sync` | guide 切换 | 不同数据轨复用同一结构 |
| `Compact` | `Queue` | 小尺寸对照 | 压缩文案后仍保留 reveal 语义 |
| `Read only` | `Locked` | 禁用边界 | 视觉保留、交互冻结 |

## 7. `egui_port_get_recording_action()` 录制动作设计
- 首帧等待并截图，确认默认 `Inbox` surface 稳定。
- 发送 `Right` 键，验证 start action reveal。
- 发送 `Left` 键，验证 end action reveal。
- 点击 guide 标签，切换到 `Planner` track，验证标题、helper、状态栏同步变化。
- 点击 compact 标签，切换紧凑预览数据轨，验证底部对照变化。

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=input/swipe_control PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/swipe_control --timeout 10 --keep-screenshots
```

验收重点：
- 不能黑屏、白屏、全空白
- 主行卡、action rail、状态栏、compact / read-only 对照都必须完整可见
- 主卡标题、action rail 文案、顶部短词和底部短词都要检查真实居中与留白
- reveal 后要能从截图中直接看出 start / end 两侧动作变化

## 9. 已知限制与下一轮迭代计划
- 当前版本用纯绘制内容卡模拟 Fluent 2 `SwipeControl` 的 reveal row，不引入真实列表容器或惯性动画。
- 当前没有连续像素级滑动动画，只保留离散 reveal state 与键盘 / 触摸闭环。
- 后续如需更接近真实收件箱，可继续增加多行容器、批量操作和拖动阈值动画。

## 10. 与现有控件的重叠分析与差异化边界
- 区别于 `settings_panel`：这里强调 reveal actions，而不是静态设置行尾部控件。
- 区别于 `data_list_panel`：这里的核心是单行 action reveal，而不是数据行浏览本身。
- 区别于 `split_button` / `toggle_split_button`：这里的交互入口是整行内容卡，不是按钮本体。
- 区别于 `flip_view`：这里不做整卡翻页，只做列表行的局部 action expose。

## 11. 参考设计系统与开源母本
- Fluent 2 / WinUI `SwipeControl`
- WPF / Windows inbox-style swipe row 语义

## 12. 对应组件名，以及本次保留的核心状态
- 组件名：`swipe_control`
- 保留状态：`surface`、`start_action`、`end_action`、`compact`、`read_only`
- 保留交互：`Left/Right/Home/End/Tab/Enter/Space/Plus/Minus/Escape`、触摸滑动与 surface tap close

## 13. 相比参考原型删减了哪些效果或装饰
- 删除了连续 easing 动画、阴影渐变过渡和多 action stack。
- 删除了真实列表滚动容器，只保留单行 hero row 语义。
- 删除了图标资源依赖，使用短文本 action rail 表达操作。

## 14. EGUI 适配时的简化点与约束
- 不引入图标资源，使用 `label + hint` 的 action rail 表达操作。
- 通过 `reveal_state + current_part` 统一键盘与触摸状态机。
- compact / read-only 直接在同一控件内切换，避免拆分多套绘制逻辑。
