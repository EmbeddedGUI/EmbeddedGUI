# password_box 自定义控件设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`PasswordBox`
- 本次保留状态：`standard`、`compact`、`read only`、`reveal toggle`、`keyboard cursor`
- 删减效果：Acrylic、系统级 reveal 动画、caps lock 提示、复杂校验态、桌面 hover 细节
- EGUI 适配说明：保留标准密码遮罩字段、右侧 reveal 按钮和 compact / read-only 对照；在 `240 x 320` 下优先保证字段留白、图标居中、光标与遮罩文本可读

## 1. 为什么需要这个控件

`password_box` 用于表达标准密码输入语义，比如 Wi-Fi 密码、部署密钥、设备管理员口令等。它比普通 `textinput` 更接近用户熟悉的安全输入模式，也能作为后续通用表单控件沉入框架层。

## 2. 为什么现有控件不够用

- `textinput` 只有通用文本输入，没有密码遮罩和 reveal 入口
- `token_input` 面向多值编辑，不适合单条秘密字段
- `auto_suggest_box` 偏建议输入，不适合安全信息录入
- 当前主线里缺少一版接近 `Fluent 2 / WPF UI PasswordBox` 的标准密码框

## 3. 目标场景与示例概览

- 主区域展示标准 `password_box`，包含 label、helper、遮罩文本和 reveal 按钮
- 左下 `Compact` 预览展示紧凑密码字段
- 右下 `Read only` 预览展示只读遮罩字段
- 主卡支持触摸聚焦、键盘编辑、Tab 切换到 reveal 按钮、Space / Enter 切换明文显示
- guide 标签切换标准 snapshot；compact 标签切换紧凑 snapshot

目录：

- `example/HelloCustomWidgets/input/password_box/`

## 4. 视觉与布局规格

- 画布：`240 x 320`
- 根布局：`224 x 278`
- 页面结构：标题 -> guide -> `Standard` -> 主密码框 -> 状态文案 -> 分隔线 -> `Compact / Read only`
- 主密码框：`196 x 70`
- 底部双预览：`216 x 64`
- `Compact`：`106 x 44`
- `Read only`：`106 x 44`
- 视觉规则：
  - 采用浅灰 page panel + 白色表单卡，不回到 showcase / HMI 风格
  - reveal 图标保持低噪音，不使用夸张 hover
  - read-only 通过统一降噪 palette 弱化，而不是额外堆叠装饰件

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 278 | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Password Box` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主 snapshot |
| `box_primary` | `egui_view_password_box_t` | 196 x 70 | `studio-24` | 标准密码框 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Wi-Fi 9 chars hidden` | 当前状态摘要 |
| `box_compact` | `egui_view_password_box_t` | 106 x 44 | `7429` | 紧凑密码框 |
| `box_locked` | `egui_view_password_box_t` | 106 x 44 | `fleet-admin` | 只读遮罩预览 |

## 6. 状态覆盖矩阵

| 状态 / 区域 | 主密码框 | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | 遮罩显示 | 遮罩显示 | 遮罩显示 |
| 触摸聚焦 | 光标显示 | 可切换 | 不响应 |
| reveal 点击 | 明文 / 遮罩切换 | 明文 / 遮罩切换 | 不显示 reveal |
| `Backspace / Delete` | 编辑内容 | 可编辑 | 不适用 |
| `Tab` | field / reveal 切换 | 可切换 | 不适用 |
| `Left / Right / Home / End` | 光标移动 | 可移动 | 不适用 |
| guide / compact 切换 | 切换 snapshot | 切换 snapshot | 固定 |

## 7. `egui_port_get_recording_action()` 录制动作设计

1. 抓取默认遮罩态
2. 点击主密码框聚焦
3. 发送 `Backspace` 与 `2`，验证键盘编辑
4. 抓取编辑后遮罩态
5. 点击 reveal 按钮，抓取明文态
6. `Tab` 到 reveal，按 `Space` 切回遮罩态
7. 点击 guide，切换主 snapshot
8. 点击 compact 标签，切换紧凑 snapshot
9. 抓取最终对照态

## 8. 编译、runtime、截图验收标准

```bash
make all APP=HelloCustomWidgets APP_SUB=input/password_box PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/password_box --timeout 10 --keep-screenshots
make all APP=HelloUnitTest PORT=pc_test
output\main.exe
```

验收重点：

- label、helper、字段文本、reveal 图标都必须完整可见
- 主卡 reveal 按钮与右侧留白必须平衡，不能出现图标偏心
- 遮罩文本与光标不能贴边
- compact 和 read-only 必须保持标准输入控件语义，而不是新造卡片装饰

## 9. 已知限制与下一轮迭代计划

- 当前用 `*` 作为遮罩字符，没有做真实 bullet glyph
- 没有实现 caps lock / strength / validation 提示
- 后续如果沉入框架层，可补 submit / error / disabled 态

## 10. 与现有控件的重叠分析与差异化边界

- 与 `textinput` 的差异：核心在密码遮罩与 reveal 按钮，而不是通用文本编辑
- 与 `token_input` 的差异：核心在单值秘密字段，而不是多 token 管理
- 与 `auto_suggest_box` 的差异：核心在安全输入，不涉及建议列表或下拉面板

## 11. 参考设计系统与开源母本

- 设计系统：`Fluent 2`
- 开源母本：`WPF UI`
- 次级参考：`ModernWpf`

## 12. 对应组件名，以及本次保留的核心状态

- 对应组件名：`PasswordBox`
- 保留状态：`masked`、`revealed`、`compact`、`read only`、`focused`

## 13. 相比参考原型删掉了哪些效果或装饰

- 去掉 Acrylic、阴影动画和桌面 hover
- 去掉系统密码管理入口
- 去掉复杂焦点过渡与 validation adorners

## 14. EGUI 适配时的简化点与约束

- 优先保证 `240 x 320` 下的视觉居中与留白
- reveal 只保留单个右侧图标入口
- 用简化键盘事件闭环代替桌面完整输入法行为
