# master_detail 设计说明

## 参考来源

- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`ModernWpf`
- 对应组件名：`MasterDetail` / `MasterDetails`
- 保留状态：标准态、`compact`、`read-only`
- 删除效果：Acrylic、阴影、复杂 hover、系统级转场动画
- EGUI 适配说明：保留左侧 master 列表驱动右侧 detail 面板的核心语义，压缩到 `240 x 320` 页面内；compact 预览改为更轻量的 detail 摘要，避免小尺寸面板信息过载

## 1. 为什么需要这个控件

`master_detail` 用于在同一块页面区域里同时呈现“列表选择”和“详情阅读”两层信息。它适合文件库、成员列表、审核队列、内容分组等需要先选中条目，再在同屏查看对应摘要的场景。

## 2. 为什么现有控件不够用

- `nav_panel` 解决的是导航入口，不强调右侧详情阅读
- `settings_panel` 强调设置项行和尾部值控件，不是 master-detail 双栏结构
- `card_panel` 更像单卡片信息摘要，没有左侧列表驱动
- `list`、`table` 只能列出条目，缺少同屏 detail 面板语义

## 3. 目标场景与示例概览

- 主示例展示 `Files / Review / Members / Archive` 四个条目
- `Standard` 区域展示完整 master-detail 双栏
- `Compact` 展示缩窄后的辅助预览，保留列表切换和右侧简化详情
- `Read Only` 展示不可交互的静态对照态
- 支持触摸点击切换 master 行
- 支持 `Up / Down / Left / Right / Tab / Home / End` 键盘切换
- guide 点击会推动主卡切换，`Compact` 标题点击会推动 compact 预览切换

组件目录：

- `example/HelloCustomWidgets/layout/master_detail/`

## 4. 视觉与布局规格

- 页面尺寸：`240 x 320`
- 根布局：`224 x 292`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`194 x 96`
- 底部双预览行：`222 x 88`
- `Compact` 预览：`108 x 72`
- `Read Only` 预览：`108 x 72`
- 视觉约束：
  - 使用浅色 page panel，保持低噪音 Reference Track 风格
  - master 行左侧保留 glyph，突出列表身份
  - detail 面板保留 eyebrow / title / meta / body / footer 的标准结构
  - compact 模式下右侧 detail 改为“标题 + meta + 一行摘要 + footer”简化结构

## 5. 控件清单

| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 292 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Master Detail` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 引导切换主卡状态 |
| `panel_primary` | `egui_view_master_detail_t` | 194 x 96 | `Files` | 标准 master-detail 主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Files library: Open` | 当前选择状态回显 |
| `panel_compact` | `egui_view_master_detail_t` | 108 x 72 | `Files` | 紧凑态预览 |
| `panel_readonly` | `egui_view_master_detail_t` | 108 x 72 | `Archive` | 只读态对照 |

## 6. 状态覆盖矩阵

- 标准态：切换 row 后右侧 detail 更新
- compact 态：保留左侧 row 列表和右侧简化 detail
- read-only 态：保留结构，不响应触摸与键盘切换
- 按压态：点击 row 时显示 pressed 反馈
- 键盘态：
  - `Up / Left` 上移
  - `Down / Right` 下移
  - `Home` 跳到首项
  - `End` 跳到末项
  - `Tab` 循环切换

## 7. `egui_port_get_recording_action()` 录制动作设计

- 点击主卡第 2、3 个 master 行，验证标准态切换
- 点击 guide，验证页面级引导切换
- 点击 `Compact` 标题，验证 compact 辅助切换
- 点击 compact 第 2 行，验证小尺寸可交互
- 点击 read-only 面板，验证只读态不发生状态改变

## 8. 编译、runtime、截图验收标准

- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=layout/master_detail PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/master_detail --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\\main.exe`
- 截图验收标准：
  - 主卡左右双栏完整可见
  - compact 与 read-only 双预览不黑屏、不挤压、不出现明显截断
  - 标题、短词 footer、状态文案和列表 glyph 需人工复核居中与留白

## 9. 已知限制与下一轮迭代计划

- 当前版本仍以静态 snapshot 数据驱动为主，不包含真实滚动数据源
- detail 面板正文暂为单行摘要，不做长文本换行
- 如未来沉淀到框架层，可继续补充更通用的数据绑定与焦点管理

## 10. 与现有控件的重叠分析与差异化边界

- 相比 `nav_panel`：本控件重点是列表驱动的详情阅读，不承担页面导航容器职责
- 相比 `settings_panel`：本控件没有设置项 value cell、switch、chevron 语义
- 相比 `card_panel`：本控件不是单卡摘要，而是双栏联动结构
- 相比 `list` / `table`：本控件强调“当前选中项 + detail pane”的同步关系

## 11. 参考设计系统与开源母本

- Fluent 2：提供低噪音页面内双栏信息组织方向
- WPF UI：提供 `MasterDetail` 组件语义与页面示例结构
- ModernWpf：作为 Windows Fluent 风格补充参考

## 12. 对应组件名与保留核心状态

- 对应组件名：`MasterDetail`
- 保留核心状态：
  - 标准态
  - compact 对照态
  - read-only 对照态
  - 触摸按压态
  - 键盘切换态

## 13. 相比参考原型删除的效果或装饰

- 去掉桌面端大面积阴影
- 去掉 Acrylic / 毛玻璃背景
- 去掉复杂 hover、press、reveal 光效
- 去掉系统页级转场，只保留局部状态反馈

## 14. EGUI 适配时的简化点与约束

- 使用固定尺寸和手工布局，保证 `240 x 320` 下稳定呈现
- master 项数量限制为 `4`
- compact 模式下不再完整复制标准 detail 结构，而是摘要化显示
- 颜色与圆角使用轻量混色，避免 HMI / showcase 风格的重装饰
