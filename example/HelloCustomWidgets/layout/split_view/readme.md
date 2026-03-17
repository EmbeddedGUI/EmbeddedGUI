# split_view 设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考开源库：`WPF UI`
- 补充参考：`ModernWpf`
- 对应组件名：`SplitView`
- 保留状态：标准态、compact pane、read-only 对照
- 删除效果：Acrylic、阴影扩散、Reveal/Hover 光效、系统级转场动画、真实图标资源
- EGUI 适配说明：保留“左侧 pane + 右侧 content + 可折叠 pane”语义，压缩到 `240 x 320` 页面内；compact 版本改为更轻的窄侧栏，read-only 版本仅保留结构和静态视觉反馈

## 1. 为什么需要这个控件
`split_view` 用来表达“左侧可折叠导航/列表面板，右侧显示当前内容”的标准双栏结构。它适合文件浏览、设置分类、内容导航、工作区面板等场景，重点不是复杂拖拽，而是稳定的 pane 展开/收起语义。

## 2. 为什么现有控件不够用
- `master_detail` 强调 master 列表驱动 detail 阅读，但没有独立的 pane toggle 语义
- `nav_panel` 更偏导航容器，不强调同屏 content 区域的阅读结构
- 旧 `split_resizer` 偏 showcase / dashboard 风格，不是 Fluent 风格的标准 pane 控件
- `list`、`table` 只能列出项目，缺少“可折叠侧栏 + 内容区”的组合表达

## 3. 目标场景与示例概览
- 主区域展示标准 `split_view`：左侧 pane 默认展开，右侧内容区跟随选中项变化
- 左下 `Compact` 预览展示窄侧栏语义，默认折叠，只保留 glyph rail
- 右下 `Read Only` 预览展示禁交互对照，结构存在但不响应 touch / key
- 支持 touch 点击 row 切换项目
- 支持 touch 点击 pane toggle 收起/展开 pane
- 支持 `Up / Down / Home / End / Tab / Left / Right / Enter / Space`

目录：
- `example/HelloCustomWidgets/layout/split_view/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 296`
- 页面结构：标题 -> guide -> `Standard` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact / Read Only`
- 主卡尺寸：`194 x 104`
- 底部双预览容器：`222 x 90`
- `Compact` 预览：`108 x 74`
- `Read Only` 预览：`108 x 74`
- 视觉规则：
  - 使用浅色 page panel + 低噪音边框，不回退到 HMI / 工业面板语言
  - pane 顶部保留 toggle，展开态显示标题，折叠态只保留图标列
  - content 区域保留 title / meta / body / footer 的轻量信息层次
  - compact 版本压缩正文，只保留必要层次

## 5. 控件清单
| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 296 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Split View` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 切换主卡 pane 展开态 |
| `panel_primary` | `egui_view_split_view_t` | 194 x 104 | pane open | 标准 split view |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Overview board: Pane open` | 当前状态反馈 |
| `panel_compact` | `egui_view_split_view_t` | 108 x 74 | pane compact | compact 预览 |
| `panel_read_only` | `egui_view_split_view_t` | 108 x 74 | read-only | 只读对照 |

## 6. 状态覆盖矩阵
- 标准态：pane 展开，row 选中后右侧 content 更新
- compact pane：pane 折叠，仅显示 glyph rail，可再展开
- read-only：不响应 touch / key，但保留结构和低饱和反馈
- 按压态：row / toggle 点击时显示 pressed 反馈
- 键盘态：
  - `Up / Down` 切换项目
  - `Left / Right` 收起/展开 pane
  - `Home / End` 跳转首尾
  - `Tab` 循环切换项目
  - `Enter / Space` 切换 pane

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 初始化后立即请求首帧快照，避免首帧未完成布局时出现空白截图
2. 程序化切到主卡 `pane open` 基线态，抓取标准展开结构
3. 程序化切到主卡 `pane compact`，抓取折叠 rail 语义
4. 在主卡折叠态切换到第二、第三项，再展开 pane，覆盖选中项切换后的 detail 变化
5. 将 compact 预览切到展开态并切换条目，覆盖小尺寸 pane/content 对照
6. 再把 compact 预览收回 rail，保留 compact pane 收口状态
7. read-only 预览保持只读对照，不在录制中修改其交互结果

## 8. 编译、runtime、截图验收标准
- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=layout/split_view PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/split_view --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - 主卡与两个预览都完整可见，不黑屏、不截断
  - pane 收起/展开前后结构清晰，不能糊成普通卡片
  - toggle、短标题、footer pill 需要人工复核居中与留白
  - read-only 预览点击后不应出现选中变化

## 9. 已知限制与下一轮迭代计划
- 当前仍是静态 snapshot 数据，不接真实滚动列表或动态数据源
- 没有做真实图标资源和动画，仅保留结构语义
- content 区域正文仍按单行摘要处理，不做长文本换行
- 如后续沉淀到框架层，可继续补充更通用的数据绑定与焦点管理

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `master_detail`：本控件强调 pane toggle 与 compact pane 语义
- 相比 `nav_panel`：本控件不是整页导航容器，而是同屏双栏内容结构
- 相比旧 `split_resizer`：本控件不再强调 resizer/showcase，而是 Fluent 风格的 pane/content 布局
- 相比 `list` / `table`：本控件自带右侧内容区，不是纯列表控件

## 11. 参考设计系统与开源母本
- `Fluent 2`：提供标准 SplitView 的视觉方向与 pane 语义
- `WPF UI`：提供可折叠 pane 的参考组件命名与页面内组织方式
- `ModernWpf`：补充 Windows Fluent 风格的 SplitView 结构语义

## 12. 对应组件名与保留核心状态
- 对应组件名：`SplitView`
- 本次保留的核心状态：
  - 标准展开态
  - compact pane 折叠/展开态
  - read-only 对照态
  - row 选中态
  - 键盘 / touch 切换态

## 13. 相比参考原型删除的效果或装饰
- 不做桌面级阴影与半透明材质
- 不做系统 hover / reveal / transition 动效
- 不做真实 menu overlay 或外层导航系统整合
- 不做拖拽式 resizer，只保留 toggle 式 pane 语义

## 14. EGUI 适配时的简化点与约束
- 固定在 `240 x 320` 下调优，优先保证小屏可读性
- pane 项数量限制为 `5`
- compact 版本以窄 rail 表达，不复制完整桌面交互细节
- 颜色与圆角维持低噪音浅色 Fluent 方向，避免回到旧 showcase 语法
