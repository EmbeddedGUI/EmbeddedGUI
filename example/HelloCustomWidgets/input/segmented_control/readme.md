# segmented_control 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`SegmentedControl`
- 本次保留状态：标准态、`compact`、`read-only`
- 删除效果：阴影、Acrylic、复杂 hover、桌面端拖拽重排、图标化分段装饰
- EGUI 适配说明：仓库核心层已经有基础 `segmented_control`，本轮在 `HelloCustomWidgets` 中补的是 Reference Track 样式包装、页面编排、键盘焦点和标准示例，不再重复造一套同名核心控件

## 1. 为什么需要这个控件

`segmented_control` 适合在同一组互斥选项之间快速切换，比如视图模式、时间范围、过滤级别、密度选择等。它比普通按钮组更紧凑，也比 tab 更轻，适合页内局部状态切换。

## 2. 为什么现有控件不够用

- `tab_bar` 更偏整页切换，不适合轻量页内过滤
- `button_matrix` 更偏离散按钮集合，没有标准 segmented 语义
- `radio_button` 强调表单选择，不适合水平胶囊式切换
- `src/widget/egui_view_segmented_control` 已有基础交互，但缺少这轮需要的 Reference Track 页面、焦点演示和统一样式包装

## 3. 目标场景与示例概览

- 主示例展示标准 segmented control，用于页内视图切换
- guide 标签点击后切换主示例 snapshot，验证不同分段组与不同数量
- 底部左侧 `Compact` 展示窄宽度下的紧凑预览
- 底部右侧 `Read Only` 展示不可交互对照态
- 主控件支持触摸切换
- 主控件支持 `Left / Right / Up / Down / Home / End / Tab` 键盘切换
- 首帧会主动请求焦点，便于验证 focus ring

组件目录：

- `example/HelloCustomWidgets/input/segmented_control/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 280`
- 页面结构：标题 -> guide -> `Standard` -> 主 segmented control -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主控件尺寸：`196 x 38`
- 底部双预览行：`216 x 72`
- `Compact` 预览：`104 x 30`
- `Read Only` 预览：`104 x 30`
- 视觉约束：
  - 保持浅色 page panel、白色控件底板和轻边框
  - 选中项只使用低噪音实色选中胶囊，不引入 showcase 装饰
  - 焦点反馈用细 focus ring，不做系统级 glow
  - `compact` 只缩小尺寸与间距，不改语义
  - `read-only` 保留当前选中态，但禁用交互

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 280 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Segmented Control` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主 snapshot |
| `primary_label` | `egui_view_label_t` | 224 x 11 | `Standard` | 主控件标签 |
| `control_primary` | `egui_view_segmented_control_t` | 196 x 38 | `Overview / Team / Usage / Access` | 标准态主控件 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Workspace team filter active` | 当前选中态回显 |
| `control_compact` | `egui_view_segmented_control_t` | 104 x 30 | `Day / Week` | 紧凑预览 |
| `control_read_only` | `egui_view_segmented_control_t` | 104 x 30 | `Off / Auto / Lock` | 只读对照态 |

## 6. 状态覆盖矩阵

- 标准态：支持主 segmented control 真实点击切换
- snapshot 切换态：guide 点击后切换主控件分组与默认选中项
- compact 态：保留分段切换语义，但收紧间距和圆角
- read-only 态：保留选中项但不响应输入
- 键盘态：
  - `Left / Up` 选择前一项
  - `Right / Down` 选择后一项
  - `Home` 跳到首项
  - `End` 跳到末项
  - `Tab` 在当前控件内部循环切换
- 焦点态：获得焦点时显示细 focus ring

## 7. `egui_port_get_recording_action()` 录制动作设计

- 首帧恢复主控件与 compact 默认状态，并请求主控件焦点
- 点击主控件中的后续分段，验证真实切换反馈
- 点击 guide 标签切换主 snapshot
- 点击新 snapshot 下的目标分段，验证不同数量和文案
- 点击 `Compact` 标签切换 compact snapshot
- 点击 compact 分段，补齐小尺寸状态变化
- 末尾再次请求主控件焦点并截取最终关键帧

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=input/segmented_control PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub input/segmented_control --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 截图验收标准：
  - 主 segmented control 与底部双预览完整可见
  - 分段文字、选中底板和外边框居中且留白均衡
  - guide 切换、主控件切换、compact 切换三类变化都必须在关键帧中可见
  - 焦点 ring 不能压住文字，也不能超出控件裁切边界

## 9. 已知限制与下一轮迭代计划

- 当前仍以固定字符串数组驱动，不包含动态数据模型
- 当前没有图标段、徽标段或溢出折叠
- 当前只验证单行水平布局，不做多行断行
- 如后续沉入框架层，可继续评估更通用的样式 token 和只读态 API

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `tab_bar`：这里是局部互斥切换，不负责整页导航
- 相比 `button_matrix`：这里强调标准 segmented 连续分组语义
- 相比 `radio_button`：这里是水平胶囊切换，不是表单单选列
- 相比已有 `src/widget/egui_view_segmented_control`：这轮主要补齐 Reference Track 页面、焦点验证和样式落地

## 11. 参考设计系统与开源母本

- Fluent 2：提供低噪音分段切换和 focus ring 方向
- WPF UI：提供 Windows Fluent 风格 segmented 控件母本
- ModernWpf：补充紧凑尺寸和浅色边框处理方式

## 12. 对应组件名与保留核心状态

- 对应组件名：`SegmentedControl`
- 本次保留核心状态：
  - 标准态
  - compact 对照态
  - read-only 对照态
  - 焦点态
  - snapshot 切换态

## 13. 相比参考原型删除的效果或装饰

- 去掉桌面端 hover reveal、阴影和 Acrylic
- 去掉图标段、拖拽排序和复杂自适应动画
- 去掉多层装饰描边与系统级过渡
- 去掉额外 badge、计数器和图形化段头

## 14. EGUI 适配时的简化点与约束

- 直接复用核心层已有 segmented 控件，避免与 `src/widget` 同名实现冲突
- 通过示例目录下的样式包装文件补齐 Reference Track 视觉约束
- 焦点 ring 和键盘切换放回核心控件，使后续示例可复用
- 页面保持 `240 x 320` 下的清爽布局，不引入额外卡片装饰
