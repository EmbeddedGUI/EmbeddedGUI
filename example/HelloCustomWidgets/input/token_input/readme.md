# token_input 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源母本：web token/tag input 组件语义
- 对应组件名：`TokenInput`
- 本次保留状态：`standard`、`compact`、`read only`、`token overflow summary`、`remove affordance`、`blank-area focus`
- 删减效果：不做桌面级阴影弹层、不做复杂 IME 候选联动、不做拖拽重排、不做动画化 placeholder
- EGUI 适配说明：优先保留多 token 内联编辑、回车/逗号提交、退格回删、remove 图标删除、compact / read-only 对照和 overflow 摘要，避免回到 showcase / HMI 风格

## 1. 为什么需要这个控件

`token_input` 用来表达“一个字段里编辑多个离散值”的标准表单语义，比如收件人、标签、设备分组、筛选条件和关键字集合。它比单值 `textinput` 更接近真实业务里的多项录入场景，也比普通 `chips` 更强调“编辑器”而不是“结果展示”。

## 2. 为什么现有控件不够用

- `textinput` 只能编辑单段文本，不具备 token 提交、回退删除和多值排布语义
- `chips` 更偏展示 / 点击，不承担输入焦点、占位和提交闭环
- `auto_suggest_box` 强调建议选择，不覆盖“多个已提交值 + 当前输入”的混合状态
- `combobox` / `radio_button` 是单选或受限选择，不适合自由追加多个 token

所以需要一个以标准表单风格呈现的 `token_input`，承载多值编辑、回退和紧凑态对照。

## 3. 目标场景与示例概览

- 主卡展示标准 `token_input`，包含标签标题、token 行、占位输入位和状态说明
- 左下 `Compact` 预览展示压缩版 token editor，并在 token 过多时折叠为 `+N`
- 右下 `Read only` 预览展示只读 token 集合，并保留同样的 overflow summary 语义
- 主卡支持新增 token、聚焦尾部输入位、点击空白区聚焦输入位、删除最后一个 token
- 主卡里的可编辑 token 支持 remove 图标删除
- 主卡支持 `Left / Right / Home / End / Backspace / Enter / Space / Tab`

目标目录：

- `example/HelloCustomWidgets/input/token_input/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：控制在 `224 x 292`
- 页面结构：标题 -> guide -> `Standard` -> 主 token 卡 -> 状态行 -> 分隔线 -> `Compact / Read only`
- 主卡建议尺寸：`196 x 92`
- 底部双预览容器：`216 x 70`
- `Compact` 预览：`106 x 48`
- `Read only` 预览：`106 x 48`
- 视觉规则：
  - 采用浅灰 page panel + 白色输入卡，强调标准表单而不是展示型大卡片
  - token 使用低饱和 accent 边框和轻胶囊背景，避免彩色 badge 堆叠
  - 输入占位、remove affordance 与 overflow summary 只做轻量焦点强化，不做悬浮夸张动效
  - read-only 通过统一降噪 palette 弱化，而不是增加额外装饰层

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Token Input` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主场景 |
| `editor_primary` | `egui_view_token_input_t` | 196 x 92 | `Recipients` | 标准 token 编辑器 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Recipients 3` | 当前编辑状态说明 |
| `editor_compact` | `egui_view_token_input_t` | 106 x 48 | `Compact` | 紧凑态对照与 overflow summary 预览 |
| `editor_locked` | `egui_view_token_input_t` | 106 x 48 | read only | 只读态对照与 overflow summary 预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主编辑器 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | 3 个 token + 尾部输入位 | 2 个 token | 固定 token 集合 |
| `Enter` / `Comma` | 提交当前输入为新 token | 不录制 | 不适用 |
| `Backspace` | 空输入位时回删最后一个 token | 不录制 | 不适用 |
| 左右导航 | 在 token 与输入位之间移动焦点 | 不录制 | 不适用 |
| 空白区点击 | 主卡空白区回落到输入位 | 不录制 | 不适用 |
| remove 图标 | 可编辑 token 右侧显示 close 图标并支持删除 | 不录制 | 不显示 |
| 溢出态 | 超过可视容量后把尾部 token 折叠成 `+N` 摘要 | 固定摘要 | 固定摘要 |
| 只读态 | 不响应编辑，只展示结果 | - | 弱化并禁止编辑 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 等待首帧并抓取默认场景
2. 点击主编辑器尾部输入位，验证焦点进入
3. 通过模拟键盘输入 `N / E / T / Enter` 追加一个 token
4. 抓取新增 token 后的主卡截图
5. 点击最后一个 token 的 remove 图标，验证 touch 删除链路
6. 抓取删除后的主卡截图
7. 执行 `Home / End`，验证主卡焦点移动
8. 点击 guide 切换主场景，展示第二组标准 token
9. 点击 `Compact` 标签切换到底部 overflow 预览
10. 抓取 compact / read-only 同时出现 `+N` 摘要的最终截图

录制同时依赖：

- 真实 token 区域命中，避免伪造状态
- 控件内部键盘分发路径，而不是只改数据
- 状态行同步反映 token 数量与当前焦点语义

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/token_input PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/token_input --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

验收重点：

- 主卡、compact、read-only 三个编辑器都必须完整可见
- token、占位输入位、状态行与标签文案之间留白平衡
- token 胶囊的文字要真实居中，不能出现左右 padding 明显失衡
- `+N` 摘要胶囊在 compact / read-only 下都要可见，且隐藏 token 不再暴露 part / remove region
- remove 图标命中区要稳定，touch 删除后焦点不能丢失到隐藏 part
- 新增 token、remove 删除、回删 token、焦点移动这几段链路都要能从截图或单测读出来
- `HelloUnitTest` 需要覆盖 token 提交、回删、remove 图标、overflow summary、只读 / disabled guard 和焦点边界

## 9. 已知限制与下一轮迭代计划

- 当前优先做单行或单卡多行包裹，不做复杂虚拟化
- 当前不做真实输入法候选联动，只保留基础 ASCII 输入路径
- 当前不做拖拽重排 token，先把标准编辑器语义收口
- 当前 overflow 仍采用“最后一个可见 token 替换成 `+N`”的轻量策略，不做多摘要层或展开面板
- 如果后续沉入框架层，再评估与通用 `textinput` / `chips` / `autocomplete` 的复用边界

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `textinput`：这里是多值 token 编辑器，不是单值文本框
- 相比 `chips`：这里有输入位与编辑语义，不是纯展示 chip 列表
- 相比 `auto_suggest_box`：这里强调“已提交 token + 当前输入”的双态共存
- 相比 `number_box` / `segmented_control`：这里不表达数值步进或单选切换

## 11. 参考设计系统与开源母本

- 参考设计系统：`Fluent 2`
- 开源母本：web token/tag input 组件语义

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`TokenInput`
- 本次保留状态：
  - `standard`
  - `compact`
  - `read only`
  - `token overflow summary`
  - `remove affordance`
  - `blank-area focus`
  - `keyboard backspace`

## 13. 相比参考原型删掉了哪些效果或装饰

- 不做复杂建议下拉层和搜索高亮
- 不做拖拽排序、拖拽插入指示器和桌面级 hover 动效
- 不做多层阴影、Acrylic 或高动态渐变
- 不做复杂 IME/剪贴板系统交互

## 14. EGUI 适配时的简化点与约束

- 优先保留 token 提交、回删、左右导航和只读弱化这些核心表单语义
- 首版聚焦 `240 x 320` 下稳定排版，不追求超长 token 的复杂折叠策略
- 尽量复用现有 `chips` / `textinput` / `linearlayout` 的视觉和交互能力，而不是造一套完全孤立的新体系
- 已布局后优先保证焦点、pressed 状态与可见 token 同步，不让隐藏 input / token 残留为当前 part
- 主页示例保持干净、低噪音、标准表单风格


## 15. 最近收口行为

- token 文本会在 `set_tokens()` 与 `add_token()` 入口统一做首尾空白修剪，忽略 `NULL`、空字符串和全空白 token。
- compact overflow 把 input 挤出可视区后，会保留已有 draft，但隐藏 input 不再接收 printable key / commit key，也不会暴露隐藏 part / remove region。
- hidden draft 在布局放宽、input 重新可见后会恢复到 input 焦点；如果中途切到 `read only` 再切回 editable，也会继续回到 input，保证 `Backspace` 优先编辑 draft。
- `compact -> standard` 往返切换遵循同样的 hidden draft 恢复语义；只要 draft 仍存在且用户没有显式改焦点，input 重新可见后会继续回到 input。
- hidden draft 待恢复期间即使先调用 `add_token()` 追加 token，也会保留 draft 与待恢复意图；但如果随后用户显式切到某个 token，自动回到 input 的意图仍会被这次显式选择覆盖。
- 如果 hidden draft 待恢复期间用户显式切到某个 token（键盘导航或 API 设焦点），这个显式选择会覆盖自动恢复 input 的意图；后续布局放宽时保持 token 焦点，不强制跳回 input。
- draft 被提交或清空后，`current_part` 会重新归一化到当前可见的 input / token，避免隐藏 part 残留在 API 可观察状态里。
