# auto_suggest_box 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`AutoSuggestBox`
- 本次保留状态：标准态、`compact`、`read-only`
- 删除效果：搜索图标、输入联想高亮、复杂 hover、桌面端阴影、Acrylic、异步建议流
- EGUI 适配说明：仓库核心层已经有 `autocomplete -> combobox` 基础实现，本轮不重造文本输入式搜索框，而是先把 Reference Track 页面、建议列表选择语义，以及 `combobox` 的 focus / keyboard 收口补齐

## 1. 为什么需要这个控件

`auto_suggest_box` 适合做轻量命令查找、成员搜索、模板选择和最近项匹配。它比普通下拉框更强调“建议结果”，也比完整文本输入更轻，适合小屏和固定候选集。

## 2. 为什么现有控件不够用

- `combobox` / `autocomplete` 现有示例更偏基础功能验证，缺少标准 Reference Track 页面
- 现有基础实现没有完整 focus ring 和键盘展开 / 选择闭环
- `textinput` 更偏自由输入，不适合直接展示固定建议集
- `menu_flyout` 更偏命令面板，不是输入语义

## 3. 目标场景与示例概览

- 主示例展示标准 `AutoSuggestBox`，用于建议集切换
- 点击 guide 文案后切换主 suggestions snapshot
- 底部左侧 `Compact` 展示窄宽度建议框
- 底部右侧 `Read Only` 展示禁用对照态
- 主控件支持触摸展开与选择
- 主控件支持 `Up / Down / Home / End / Enter / Space / Escape` 键盘操作
- 首帧主动请求焦点，便于验证 focus ring

组件目录：

- `example/HelloCustomWidgets/input/auto_suggest_box/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 280`
- 页面结构：标题 -> guide -> `Standard` -> 主建议框 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主控件尺寸：`208 x 34`
- 底部双预览行：`216 x 72`
- `Compact` 预览：`104 x 28`
- `Read Only` 预览：`104 x 28`
- 视觉约束：
  - 保持浅色 page panel、白色输入面和轻边框
  - 展开列表只保留单层白底与低饱和高亮，不增加 showcase 装饰
  - focus 反馈使用细 ring，不做 glow
  - `compact` 只收紧高度和 item 行高，不改变语义
  - `read-only` 保留当前建议文本，但禁用交互

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 280 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `AutoSuggest Box` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主 snapshot |
| `control_primary` | `egui_view_autocomplete_t` | 208 x 34 | 标准 suggestions | 主建议框 |
| `status_label` | `egui_view_label_t` | 224 x 12 | 回显当前选择 | 当前选中态文案 |
| `control_compact` | `egui_view_autocomplete_t` | 104 x 28 | 紧凑 suggestions | 紧凑预览 |
| `control_read_only` | `egui_view_autocomplete_t` | 104 x 28 | 禁用对照态 | 只读预览 |

## 6. 状态覆盖矩阵

- 标准态：支持展开和选择建议项
- snapshot 切换态：guide 点击切换主 suggestions 数据
- compact 态：缩小尺寸与 item 高度
- read-only 态：保留当前文本，不响应输入
- 键盘态：
  - `Down / Up`：展开后切换建议项；折叠时先展开
  - `Home / End`：切到首项 / 末项
  - `Enter / Space`：展开或确认当前项
  - `Escape`：收起建议列表
- 焦点态：获得焦点时显示 focus ring

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧恢复主 suggestions 与 compact 默认状态，并请求主控件焦点
- 点击主控件展开 suggestions，并在展开后单独请求截图
- 点击建议项，验证主状态回显，并在收起后单独请求截图
- 点击 guide，切换主 snapshot，并在切换后单独请求截图
- 再次展开主控件并选择新建议项，分别覆盖展开态与选择结果态
- 点击 `Compact` 标题切换 compact snapshot，并在切换后单独请求截图
- 展开 compact 控件并选择建议项，分别覆盖展开态与选择结果态
- 末尾再次请求主控件焦点，并截取最终 focus ring 关键帧

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/auto_suggest_box PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/auto_suggest_box --timeout 10 --keep-screenshots`
- 截图验收标准：
  - 主建议框、底部双预览完整可见
  - 展开列表边框、文字、当前高亮项清晰可辨
  - guide 切换、主 suggestions 切换、compact suggestions 切换都要可见
  - focus ring 不压住文字，不超出建议框区域

## 9. 已知限制与下一轮迭代计划

- 当前版本是固定 suggestions 数组，不做动态过滤
- 当前不支持输入中高亮匹配字串
- 当前不做多段 icon / category item
- 如后续沉入框架层，可继续补 placeholder、leading icon 和滚动长列表

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `combobox`：这轮重点是 Reference Track 页面、标准建议语义和 focus / keyboard 收口
- 相比 `textinput`：这里不强调自由文本编辑，而是建议项选择
- 相比 `menu_flyout`：这里是输入语义，不是命令面板
- 相比 `number_box` / `segmented_control`：这里是可展开建议列表，不是步进输入或互斥胶囊切换

## 11. 参考设计系统与开源母本

- Fluent 2：提供标准 AutoSuggestBox 的轻量输入语义和 focus 方向
- WPF UI：提供 Windows Fluent 风格的 AutoSuggestBox 母本
- ModernWpf：补充浅色边框和简化展开列表处理方式

## 12. 对应组件名与保留核心状态

- 对应组件名：`AutoSuggestBox`
- 本次保留核心状态：
  - 标准态
  - compact 对照态
  - read-only 对照态
  - 焦点态
  - snapshot 切换态

## 13. 相比参考原型删除的效果或装饰

- 去掉输入态过滤高亮和异步结果刷新
- 去掉搜索图标、阴影和系统级动画
- 去掉复杂 hover、拖拽和多列 suggestion 模板
- 去掉带 icon / shortcut 的富建议项

## 14. EGUI 适配时的简化点与约束

- 直接复用核心 `autocomplete/combobox` 结构，避免新增重复基础控件
- 通过示例目录下的样式包装统一 Reference Track 颜色与尺寸
- 先把 focus ring 和键盘操作回收到 `combobox`，后续如果需要再扩展 placeholder / 过滤
