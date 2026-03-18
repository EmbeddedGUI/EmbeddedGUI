# parallax_view 设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 官方语义参考：`WinUI 3 ParallaxView`
- 对应组件名：`ParallaxView`
- 本次保留状态：标准态、offset stepped、hero shift、active row、compact、read-only
- 删除效果：真实图片背景、惯性滚动、系统级动画、复杂材质阴影、跨容器联动
- EGUI 适配说明：保留“前景滚动驱动背景慢速位移”的核心语义，用纯绘制 layer 和 offset 映射表达景深，不引入真实滚动容器依赖

## 1. 为什么需要这个控件
`parallax_view` 用来表达“前景内容滚动时，背景 hero 区域以更慢速度位移”的标准景深语义。它适合 onboarding、内容导览、分段长页面、dashboard hero 和媒体分层封面等场景，重点不是列表本身，而是 scroll offset 与背景层位移的关联。

## 2. 为什么现有控件不够用
- `split_view`、`master_detail` 强调双栏结构，不表达单面板内的滚动景深
- `flip_view`、`coverflow_strip` 强调离散分页与轮播，不表达连续 offset
- `layer_stack` 有静态层叠效果，但缺少由 offset 驱动的背景位移
- `scroll_bar` 只表达滚动位置，不承担 hero depth 的视觉语义

## 3. 目标场景与示例概览
- 主卡展示标准 `parallax_view`：hero 区背景层随 offset 慢速上移，前景 row 列表保持可读
- 左下 `Compact view` 预览展示更紧凑的 parallax 摘要态，只保留核心层次
- 右下 `Read-only` 预览展示锁定态，对照不可交互时的低噪音视觉
- 支持 touch 点击 row 跳转到该 row 的 `anchor_offset`
- 支持 `Up / Down / Home / End / Plus / Minus`

目录：
- `example/HelloCustomWidgets/layout/parallax_view/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 304`
- 页面结构：标题 -> guide -> `Standard view` -> 主卡 -> 状态文案 -> 分隔线 -> `Compact view / Read-only`
- 主卡尺寸：`194 x 136`
- 底部双预览容器：`218 x 90`
- 单个预览尺寸：`106 x 82`
- 视觉规则：
  - 使用浅色 Fluent 风格 card，不回退到重装饰 showcase 语法
  - 顶部 hero 区保留三层背景条，随 offset 产生慢速位移
  - 前景 rows 以卡片列表方式压在 hero 下方，当前 row 用 tone bar 强调
  - footer pill 固定回显当前 active row

## 5. 控件清单
| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | 224 x 304 | enabled | 页面根容器 |
| `title_label` | `egui_view_label_t` | 224 x 18 | `Parallax View` | 页面标题 |
| `guide_label` | `egui_view_label_t` | 224 x 12 | 可点击 | 触发主卡状态轮换 |
| `parallax_primary` | `egui_view_parallax_view_t` | 194 x 136 | `offset=0` | 标准 parallax 主卡 |
| `status_label` | `egui_view_label_t` | 224 x 12 | `Active Hero Banner` | 当前焦点状态回显 |
| `parallax_compact` | `egui_view_parallax_view_t` | 106 x 82 | compact | 紧凑预览 |
| `parallax_locked` | `egui_view_parallax_view_t` | 106 x 82 | compact + locked | 只读对照 |

## 6. 状态覆盖矩阵
- 标准态：`Hero Banner`
- 切换 1：`Pinned Deck`
- 切换 2：`Quiet Layer`
- 切换 3：`System Cards`
- compact 预览：`Depth Strip` -> `Quiet Stack`
- read-only：固定在 `Review Shelf`
- 输入态：
  - `Up / Down` 按 line step 调整 offset
  - `Plus / Minus` 按 page step 调整 offset
  - `Home / End` 跳到首尾
  - touch row 直接跳到对应 anchor

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 初始化后抓取主卡首态和底部双预览基线快照
2. 程序化切到 `Pinned Deck`
3. 程序化切到 `Quiet Layer`
4. 程序化切换 compact 预览到 `Quiet Stack`
5. 程序化切到主卡尾态 `System Cards` 并收尾等待

## 8. 编译、runtime、截图验收标准
- 构建命令：
  - `make all APP=HelloCustomWidgets APP_SUB=layout/parallax_view PORT=pc`
- Runtime 命令：
  - `python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub layout/parallax_view --timeout 10 --keep-screenshots`
- 单元测试：
  - `make all APP=HelloUnitTest PORT=pc_test`
  - `output\main.exe`
- 验收重点：
  - hero 区与 rows 不可重叠到难以识别
  - 背景层位移必须能从录制截图中看出明显变化
  - footer pill、progress pill、短标签需要人工复核居中与留白
  - compact/read-only 必须一眼可区分

## 9. 已知限制与下一轮迭代计划
- 当前使用固定 row 数据，不接真实滚动容器
- 当前以 stepped offset 演示 parallax，不做连续惯性动画
- 当前背景层是抽象条带，不引入真实图片资源
- 如后续沉淀到框架层，可再补真实 scroll source 绑定

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `split_view` / `master_detail`：这里是单卡内的深度滚动，不是双栏
- 相比 `flip_view` / `coverflow_strip`：这里是连续 offset 语义，不是离散翻页
- 相比 `layer_stack`：这里的层位移由 offset 驱动，而不是静态堆叠
- 相比 `scroll_bar`：这里重点是 hero depth 反馈，不是标准滚动条输入部件

## 11. 参考设计系统与开源母本
- `Fluent 2`
- `WinUI 3 ParallaxView`

## 12. 对应组件名与保留核心状态
- 对应组件名：`ParallaxView`
- 本次保留：
  - offset stepped
  - hero shift
  - active row
  - compact
  - read-only

## 13. 相比参考原型删除的效果或装饰
- 不做真实图片 / 视频背景
- 不做桌面级滚动动画与材质阴影
- 不做多源滚动绑定或跨容器 parallax
- 不做系统级 focus ring 与 hover reveal

## 14. EGUI 适配时的简化点与约束
- 以固定 `row + anchor_offset` 数组驱动，保证示例可重复验证
- 通过背景条带位移模拟 depth，不引入额外资源依赖
- 在 `240 x 320` 页面内优先保证三态对照和文本可读性
- 当前先沉淀为 `HelloCustomWidgets` custom widget，不下沉到框架核心层
