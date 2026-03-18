# shortcut_recorder 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名称：`Keyboard shortcut field / accelerator editor`
- 本次保留状态：`standard`、`listening`、`preset apply`、`compact`、`read only`
- 删除效果：系统级快捷键冲突检测、全局注册、Alt/Win 修饰键、长列表搜索建议、真实桌面焦点环动画
- EGUI 适配说明：保留快捷键录入字段、监听态、预设列表与只读摘要；在 `240 x 320` 画布下优先保证 token 可读性、状态对比和底部双预览完整呈现

## 1. 为什么需要这个控件？

`shortcut_recorder` 用于表达标准快捷键录入语义，适合命令面板热键、全局搜索入口、操作加速键和工作区定制设置页。它强调“进入监听态后捕获组合键”，而不是自由文本录入或离散选项切换。

## 2. 为什么现有控件不够用？
- `textinput` 关注字符输入和光标编辑，不表达快捷键监听态。
- `token_input` 强调多值 token 编辑，不表达 `Ctrl / Shift + Key` 的组合录入。
- `command_palette` 是命令搜索与执行面板，不负责持久化快捷键录入。
- `toggle_button`、`drop_down_button` 等命令控件也不覆盖快捷键捕获和 preset 应用语义。

## 3. 目标场景与示例概览
- 主卡展示标准快捷键录入控件，覆盖默认绑定、监听态、预设切换与清空绑定。
- 底部左侧 `Compact` 预览展示紧凑态录入摘要。
- 底部右侧 `Read only` 预览展示锁定态摘要。
- 录制动作依次展示默认态、监听态、捕获新组合键、切换 preset、清空绑定与 compact 快照。

目录：`example/HelloCustomWidgets/input/shortcut_recorder/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 304`
- 页面结构：标题 -> 引导文案 -> `Standard` 标签 -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read only` 双预览
- 主卡尺寸：`194 x 138`
- 底部双预览容器：`218 x 92`
- 单列预览尺寸：`106 x 82`
- 视觉规则：浅灰页面底板 + 低噪音白色字段卡；监听态使用暖色 focus band，preset 行保留柔和高亮和细边框，不引入强装饰插图

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 304 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Shortcut Recorder` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | `Tap rows or press Enter` | 引导文案 |
| `recorder_primary` | `egui_view_shortcut_recorder_t` | 194 x 138 | `Ctrl + K` | 标准快捷键录入控件 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Ready: Ctrl + K` | 状态摘要 |
| `recorder_compact` | `egui_view_shortcut_recorder_t` | 106 x 82 | `Ctrl + Shift + P` | 紧凑预览 |
| `recorder_locked` | `egui_view_shortcut_recorder_t` | 106 x 82 | `Ctrl + S` | 只读预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主卡 | Compact | Read only |
| --- | --- | --- | --- |
| 默认 | `Ctrl + K` | `Ctrl + Shift + P` | `Ctrl + S` |
| 监听态 | `Listening` 胶囊高亮 | 保持 | 保持 |
| 捕获新绑定 | `Ctrl + Shift + P` | 保持 | 保持 |
| preset 切换 | `Ctrl + 1` / `Ctrl + Shift + F` | 保持 | 保持 |
| clear | `Unassigned` | 保持 | 保持 |
| 紧凑切换 | 保持 | `Ctrl + 1` | 保持 |
| 只读弱化 | 不适用 | 不适用 | 整体降噪并锁定输入 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 应用默认状态并请求首帧截图
2. 切到主卡监听态并请求截图
3. 提交 `Ctrl + Shift + P` 并请求截图
4. 应用主卡 preset `Ctrl + 1` 并请求截图
5. 清空主卡绑定并请求截图
6. 切到 compact 第二组摘要并请求截图
7. 保持 read-only 只读态作为收尾帧

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/shortcut_recorder PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/shortcut_recorder --timeout 10 --keep-screenshots
```

验收重点：
- 主卡字段、preset 行、clear 动作和状态胶囊必须完整可见，不能裁切。
- `Ctrl` / `Shift` / `Key` token 的左右留白必须均衡，不能贴边。
- `Listening` 态要和 `Ready` / `Locked` 明显区分。
- `Compact` 与 `Read only` 需要一眼可区分，同时保持快捷键摘要可读。
- 录制截图中必须看到监听态、已捕获绑定和清空态的变化。

## 9. 已知限制与下一轮迭代计划
- 当前只覆盖 `Ctrl` / `Shift` 两种修饰键，不实现 `Alt` / `Win`。
- 当前不做系统级冲突检测和保留键校验。
- 当前 preset 为固定数组，不接真实用户配置源。
- 当前键盘导航优先覆盖录入、preset 和 clear 闭环，不扩展完整桌面级 focus ring 动画。

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `textinput`：这里强调快捷键捕获，不做自由文本光标编辑。
- 相比 `token_input`：这里是单一组合键语义，不做多值 token 管理。
- 相比 `command_palette`：这里是设置面板式绑定录入，不做搜索即执行。
- 相比 `toggle_button` / `drop_down_button`：这里不是命令触发，而是输入与保存组合键。

## 11. 参考设计系统与开源母本
- 参考设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级补充参考：`ModernWpf`

## 12. 对应组件名称，以及本次保留的核心状态
- 对应组件名称：`Keyboard shortcut field / accelerator editor`
- 本次保留：`standard`、`listening`、`preset apply`、`compact`、`read only`

## 13. 相比参考原型删掉了哪些效果或装饰？
- 不做系统级冲突提示、命令搜索联动和上下文帮助弹层
- 不做 `Alt` / `Win` / 多段 chord 组合录入
- 不做桌面系统风格 focus ring、hover animation 和全局快捷键注册
- 不做真实表单校验链路与持久化配置写入

## 14. EGUI 适配时的简化点与约束
- 使用固定 preset 数组保证示例稳定和截图可复现
- 使用轻量 token pill 和状态胶囊表达快捷键录入，不引入额外图像资源
- 底部固定 `Compact / Read only` 双列，方便直接与主卡对照
- 当前优先完成示例级 `shortcut_recorder`；是否沉入框架层后续再评估
