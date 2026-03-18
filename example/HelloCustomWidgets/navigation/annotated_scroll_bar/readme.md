# annotated_scroll_bar 自定义控件设计说明

## 参考来源
- 参考设计系统：`Fluent 2`
- 参考平台控件：`WinUI AnnotatedScrollBar`
- 次级补充参考：`scroll_bar`、`chapter_strip`、`pips_pager`
- 对应组件名：`AnnotatedScrollBar`
- 本次保留状态：`standard`、`compact`、`read only`
- 删除效果：hover tooltip、真实滚轮内容联动、复杂动画、桌面级 pointer affordance
- EGUI 适配说明：保留连续 offset、section marker、固定指示线和注释气泡；不做桌面端 hover，改为常显注释 + 键盘 / 触摸驱动

## 1. 为什么需要这个控件
`annotated_scroll_bar` 用来表达“长列表里按语义分段快速跳转”的导航轨道，适合照片年份浏览、发布说明按版本定位、日志流按阶段跳转这类场景。

## 2. 为什么现有控件不够用
- `scroll_bar` 只表达 viewport 位置，不理解 section marker 和 annotation
- `status_timeline` 偏展示型时间线，不是连续滚动导航
- `pips_pager` 是离散分页，不是长内容的连续 offset rail
- `chapter_strip` 是章节切换条，不是带固定指示线的纵向导航条

## 3. 目标场景与示例概览
- 主区域展示标准 `annotated_scroll_bar`，覆盖 `Gallery rail`、`Release rail`、`Incident rail` 三套 snapshot
- 主区域突出固定指示线、section marker、左侧注释卡和右侧轨道
- 左下 `Compact` 预览展示低噪音摘要态
- 右下 `Read only` 预览展示冻结态
- 状态栏回显当前 section label 与 offset

目录：
- `example/HelloCustomWidgets/navigation/annotated_scroll_bar/`

## 4. 视觉与布局规格
- 画布：`240 x 320`
- 根布局：`224 x 308`
- 主控件：`196 x 156`
- 底部双预览：`216 x 86`
- `Compact` / `Read only` 预览：`104 x 68`
- 视觉规则：
  - 保持浅灰 page panel + 白底轻边框卡片
  - 指示线固定高度，不表达 viewport 尺寸，区别于 `scroll_bar`
  - section marker 贴轨道分布，annotation 气泡放在左侧摘要区
  - `Compact` 只保留当前标签与 rail，不保留长说明

## 5. 控件清单
| 变量名 | 类型 | 尺寸 (W x H) | 初始状态 | 用途 |
| --- | --- | ---: | --- | --- |
| `root_layout` | `egui_view_linearlayout_t` | `224 x 304` | enabled | 页面根布局 |
| `title_label` | `egui_view_label_t` | `224 x 18` | `Annotated Scroll Bar` | 页面标题 |
| `guide_label` | `egui_view_label_t` | `224 x 12` | clickable | 切换主 snapshot |
| `annotated_scroll_bar_primary` | `egui_view_annotated_scroll_bar_t` | `196 x 152` | `Gallery rail` | 主控件 |
| `status_label` | `egui_view_label_t` | `224 x 12` | 当前 section 状态 | 底部状态文案 |
| `annotated_scroll_bar_compact` | `egui_view_annotated_scroll_bar_t` | `104 x 64` | compact | 紧凑预览 |
| `annotated_scroll_bar_locked` | `egui_view_annotated_scroll_bar_t` | `104 x 64` | read only | 只读预览 |

## 6. 状态覆盖矩阵
| 状态 / 区域 | 主 `annotated_scroll_bar` | Compact | Read only |
| --- | --- | --- | --- |
| 默认态 | `Gallery / 2021` | `Compact / Focus` | `Locked / Ship` |
| 键盘步进 | `Up / Down / +/- / Home / End` 改变 offset | 不响应 | 不响应 |
| marker 选择 | 点击 marker 直接跳转 section | 不演示 | 不响应 |
| marker 拖拽切换 | marker 按下后拖动可切换成 rail drag | 不演示 | 不响应 |
| rail 拖动 | 沿 rail 直接拖到目标 offset | 不演示 | 不响应 |
| 切换 snapshot | guide 切换主数据集 | label 单独切换 | 固定冻结 |

## 7. `egui_port_get_recording_action()` 录制动作设计
1. 首帧固定 `Gallery / 2021`
2. `Down` 小步推进，验证固定指示线和 `snapshot | section | offset` 状态更新
3. `+` 大步推进，验证跨 section 跳转
4. `End` 跳到底部 section
5. 点击 guide 切到 `Release rail`
6. 点击 `Compact` label 切换紧凑预览

## 8. 编译、runtime、截图验收标准
```bash
make all APP=HelloCustomWidgets APP_SUB=navigation/annotated_scroll_bar PORT=pc
python scripts/code_runtime_check.py --app HelloCustomWidgets --app-sub navigation/annotated_scroll_bar --timeout 10 --keep-screenshots
```

验收重点：
- 主卡片里摘要区、标签列和轨道三层都必须完整可见
- section label、注释气泡和状态栏不能互相挤压
- 指示线高度固定，不能回退成比例 thumb
- 状态栏三段文案需要保持可扫读，分隔符不能导致换行或拥挤
- `Compact` 和 `Read only` 必须和主态有明显层级差异

## 9. 已知限制与下一轮计划
- 当前版本不做 hover tooltip，改为常显 annotation bubble
- marker label 仍使用轻量避让，不做复杂碰撞排版
- 当前只支持单列纵向 annotated rail，不做双侧标签
- 本轮已完成摘要区留白、read-only 降噪、compact 比例、marker 拖拽切换和状态栏分段回显收口

## 10. 与现有控件的重叠分析与差异化边界
- 相比 `scroll_bar`：这里强调语义 marker 和注释，不强调 viewport 比例
- 相比 `status_timeline`：这里是可交互导航轨道，不是静态阶段展示
- 相比 `pips_pager`：这里是连续长列表定位，不是离散分页
- 相比 `chapter_strip`：这里是纵向 rail + 固定指示线，不是横向章节条

## 11. 参考设计系统与开源母本
- 设计系统：`Fluent 2`
- 平台控件：`WinUI AnnotatedScrollBar`
- 本仓库参考：`scroll_bar`、`chapter_strip`、`pips_pager`

## 12. 对应组件名，以及本次保留的核心状态
- 对应组件名：`AnnotatedScrollBar`
- 本次保留：
  - `standard`
  - `compact`
  - `read only`
  - `marker jump`
  - `rail drag`
  - `keyboard step`

## 13. 相比参考原型删掉了哪些效果或装饰
- 不做 pointer hover 才显示 tooltip
- 不做真实系统滚动容器联动
- 不做复杂 fade / reveal 动画
- 不做多列 annotations 和动态标签折叠动画

## 14. EGUI 适配时的简化点与约束
- 用固定高度 indicator line 替代桌面端更复杂的 reveal affordance
- 用左侧摘要卡承载 annotation bubble，避免悬浮层实现成本
- `Compact` 预览只保留当前标签，降低噪音
- 当前先作为 `HelloCustomWidgets` 示例控件推进，不下沉到 `src/widget/`
