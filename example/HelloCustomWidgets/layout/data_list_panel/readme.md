# data_list_panel 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 次级补充参考：`ModernWpf`
- 对应组件名：`ListView` / `ItemsView` / `Details row`
- 本次保留状态：标准态、`compact`、`read-only`
- 删除效果：Acrylic、阴影、复杂 hover、虚拟滚动、桌面级列头拖拽
- EGUI 适配说明：保留标准数据列表行、焦点行、头部摘要和 footer 状态胶囊的核心语义，在 `240 x 320` 页面对信息密度做收紧处理

## 1. 为什么需要这个控件

`data_list_panel` 用于表达标准化的数据行列表，适合任务队列、资产审阅、归档清单、同步记录等需要在同一张卡片里快速浏览多条结构化条目的场景。

## 2. 为什么现有控件不够用

- `list` 只有基础列表能力，不强调参考型数据行结构和焦点行反馈
- `table` 更像网格数据，不适合低噪音列表卡片语义
- `settings_panel` 强调设置行与尾部控件，不是数据记录列表
- `master_detail` 强调双栏联动，而不是单卡片内的数据行浏览

## 3. 目标场景与示例概览

- 主示例展示 `Sync queue`、`Asset review`、`Archive sweep` 三组 snapshot
- 底部左侧 `Compact` 展示更高密度的数据行预览
- 底部右侧 `Read Only` 展示只读弱化态
- 支持触摸切换当前行
- 支持 `Up / Down / Left / Right / Tab / Home / End` 键盘切换
- guide 标签切换主 snapshot，`Compact` 标签切换 compact snapshot
- 录制采用 setter + 真实点击混合路径：主卡行切换走 setter，guide / compact 保留真实点击

组件目录：

- `example/HelloCustomWidgets/layout/data_list_panel/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`194 x 116`
- 底部双预览行：`222 x 94`
- `Compact` 预览：`108 x 80`
- `Read Only` 预览：`108 x 80`
- 视觉约束：
  - 保持浅色 page panel 和轻边框 Reference Track 风格
  - 主卡只保留 eyebrow、title、summary、rows、footer 五层结构
  - 标准态行内使用 `glyph + title + value pill` 的最小数据表达
  - `compact / read-only` 预览隐藏顶部 summary，改为 `glyph + title + right value` 的低噪音行
  - 选中行通过轻色底、左侧细条和状态文案增强，不引入 showcase 装饰

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Data List` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 推动主 snapshot 切换 |
| `panel_primary` | `egui_view_data_list_panel_t` | 194 x 116 | `Sync queue` | 标准数据列表主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Sync queue: Nightly sync Blocked` | 当前焦点状态回显 |
| `panel_compact` | `egui_view_data_list_panel_t` | 108 x 80 | `Recent` | 紧凑预览 |
| `panel_readonly` | `egui_view_data_list_panel_t` | 108 x 80 | `Locked` | 只读静态对照 |

## 6. 状态覆盖矩阵

- 标准态：支持主卡数据行切换
- snapshot 切换态：guide 点击后主卡切换到下一组数据集
- compact 态：隐藏顶部 summary，只保留标题、数据行和 footer 的紧凑表达
- read-only 态：保留结构但不响应输入
- 按压态：点击行时显示 pressed 反馈
- 键盘态：
  - `Up / Left` 上移
  - `Down / Right` 下移
  - `Home` 跳到首项
  - `End` 跳到末项
  - `Tab` 循环切换

## 7. `egui_port_get_recording_action()` 录制动作设计

- 默认态先请求主卡与 compact 首帧
- 用 setter 切换主卡第 3 行，验证焦点行变化
- 点击 guide，验证主 snapshot 切换
- 点击 `Compact` 标签，验证 compact snapshot 切换
- 再用 setter 切到归档 snapshot，补充最终关键帧

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=layout/data_list_panel PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/data_list_panel --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 截图验收标准：
  - 主卡与底部双预览完整可见
  - 行内 `glyph`、标题、右侧数值 / value pill 和 footer summary 需要人工复核居中与留白
  - 主卡切换、guide 切换和 compact 切换三类变化都必须在关键帧中可见

## 9. 已知限制与下一轮迭代计划

- 当前以静态 snapshot 数据驱动为主，不包含真实数据源和滚动
- 当前每组最多 `4` 行，主要用于参考型演示
- 当前 value pill 只承载简短数值，不做复杂列头和排序
- 如后续沉入框架层，可继续评估更通用的数据源接口和滚动列表能力

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `list`：这里强调标准数据行结构和焦点行视觉反馈
- 相比 `table`：这里不是网格表，而是卡片内的数据清单
- 相比 `settings_panel`：这里没有设置项尾部控件语义，重点是数据记录
- 相比 `master_detail`：这里不拆分详情面板，焦点信息通过外部状态文案和 footer 表达

## 11. 参考设计系统与开源母本

- Fluent 2：提供低噪音数据列表与行焦点语义方向
- WPF UI：提供标准 Windows Fluent 风格列表卡片和数据行组织参考
- ModernWpf：作为简化 Windows Fluent 列表表达补充参考

## 12. 对应组件名与保留核心状态

- 对应组件名：`ListView` / `ItemsView`
- 本次保留核心状态：
  - 标准态
  - compact 对照态
  - read-only 对照态
  - 行焦点切换态
  - snapshot 切换态

## 13. 相比参考原型删除的效果或装饰

- 去掉桌面端大面积阴影和 Acrylic
- 去掉复杂 hover、reveal 和滚动条表现
- 去掉列头拖拽、排序箭头和虚拟化大列表行为
- 去掉系统级页面转场，只保留局部数据行反馈

## 14. EGUI 适配时的简化点与约束

- 使用 `snapshot + item` 固定数据结构，保证 `240 x 320` 页面内稳定呈现
- 数据行数量限制为 `4`
- compact 模式下隐藏顶部 summary，并去掉双行 meta 表达，保留更轻量的标题 + value 行
- 当前先完成示例级 `data_list_panel`，后续再评估是否沉入框架层
