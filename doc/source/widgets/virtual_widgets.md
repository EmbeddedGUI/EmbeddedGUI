# 虚拟容器与大数据集控件

这组文档面向需要在 EmbeddedGUI 中承载大数据集、可变尺寸内容和复杂交互状态的场景。  
它讲的不是传统 `List`，而是建立在 `virtual_viewport` 之上的一整套虚拟容器家族：

- `virtual_viewport`
- `virtual_list`
- `virtual_page`
- `virtual_strip`
- `virtual_grid`
- `virtual_section_list`
- `virtual_tree`
- `virtual_stage`

如果你的界面只有十几个固定子控件，或者传统 `List` 已经足够，就不需要引入这套体系。  
如果你的场景满足下面任意一条，建议优先阅读本章：

- 数据量可能达到几百到上千条
- item 需要按可见区动态创建
- item 尺寸会变化
- 数据会频繁插入、删除、移动、patch
- 需要围绕 `stable_id` 做精确定位和局部刷新
- item 有动画态、编辑态、展开态、选中态，需要避免因回收而丢状态

## 阅读顺序

```{toctree}
:maxdepth: 1

virtual_design
virtual_api
```

## 你会在这组文档里看到什么

[设计理念与选型](virtual_design.md)

- 为什么这套控件不是单纯的 ListView
- `stable_id`、slot 复用、`keepalive`、`state cache` 的设计目的
- 各个 virtual 容器的适用边界
- 带截图的语义差异对比

[API 与接入模板](virtual_api.md)

- `params`、`data_source`、`setup` 应该怎么写
- 各容器最小接入骨架
- 常用 helper 和通知接口怎么选
- 动画、点击、滚动、定位和验证流程

## 快速选型表

| 容器 | 最小业务单元 | 典型场景 |
| --- | --- | --- |
| `virtual_viewport` | 完全自定义 | 画布块、聊天气泡、看板卡、自定义混排 |
| `virtual_list` | row | feed、日志、普通消息列表 |
| `virtual_page` | section / module | 仪表盘、设置页大区块、详情页模块 |
| `virtual_strip` | rail item | 海报带、时间轴条带、播放队列 |
| `virtual_grid` | tile / card | 商品宫格、缩略图墙、tile 面板 |
| `virtual_section_list` | section header + item | 设置分组、工单分组、消息分组 |
| `virtual_tree` | node | 文件树、拓扑树、组织树、任务树 |
| `virtual_stage` | fixed node / absolute block | 固定画布、绝对定位节点页、仪表盘总览、设备看板 |

## 与传统 `List` 和高层 `ListView / GridView` 的关系

传统 [列表控件](list_widgets.md) 解决的是“小规模、固定结构的滚动条目”。  
`ListView / GridView` 解决的是“数据量很大，但业务层只想维护 `data_model + holder_ops`，并直接在 holder 里放真实控件”。  
virtual 家族解决的是“更底层、更通用的大数据集、按需创建、状态恢复、精确通知”问题。

因此在文档和工程实践里，可以这样区分：

- 条目数量很小，且没有复杂局部状态：优先传统 `List`
- 条目数量很大，但只是常见纵向 row 或二维 tile，且想直接写 `ViewHolder + DataModel`：优先 [列表与网格控件](list_widgets.md) 里的 `ListView / GridView`
- 需要 `section / tree / page / stage` 语义，或者要直接控制 raw adapter / slot / state cache 细节：优先 virtual 家族

## 相关示例

- `example/HelloVirtual/virtual_viewport_basic/`
- `example/HelloVirtual/virtual_viewport/`
- `example/HelloVirtual/virtual_page_basic/`
- `example/HelloVirtual/virtual_page/`
- `example/HelloVirtual/virtual_strip_basic/`
- `example/HelloVirtual/virtual_strip/`
- `example/HelloVirtual/virtual_grid_basic/`
- `example/HelloVirtual/virtual_grid/`
- `example/HelloVirtual/virtual_section_list_basic/`
- `example/HelloVirtual/virtual_section_list/`
- `example/HelloVirtual/virtual_tree_basic/`
- `example/HelloVirtual/virtual_tree/`
- `example/HelloVirtual/virtual_stage_basic/`
- `example/HelloVirtual/virtual_stage_showcase/`
- `example/HelloVirtual/virtual_stage/`

## `virtual_stage` 入口说明

如果你想先理解 raw `virtual_viewport` 的最小接法，建议先看 `example/HelloVirtual/virtual_viewport_basic/`，它去掉了顶部摘要 Header，只保留一个简单 action bar、一个纵向 viewport 和两种基础行类型，聚焦 `init_with_setup`、外置 item 状态、点击/拖动回查、单项刷新和 `ensure_item_visible_by_stable_id()`。

`virtual_stage` 不属于“滚动数据集容器”的那条线，它更适合固定页面里有很多绝对定位节点、但只有少量节点需要临时 materialize 成真实控件的场景。

推荐阅读顺序：

- 先看 `example/HelloVirtual/virtual_stage_basic/`，这是最小可用接法，聚焦 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`、`EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_RESOLVE_ID_BY_VIEW(...)`、`EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)`、外置状态、pin/reset。
- 再看 `example/HelloVirtual/virtual_stage_showcase/`，这是 showcase 风格的对比 case，面板和说明走 render-only，8 个代表性控件保持真实 widget，适合和 `HelloShowcase` 对照看。
- 最后看 `example/HelloVirtual/virtual_stage/`，这是复杂 cockpit 场景，覆盖更多节点类型、keepalive 和状态恢复。
- 设计与 API 细节见 `doc/source/architecture/virtual_stage.md`。

补充两条容易踩坑的语义：

- `EGUI_VIEW_VIRTUAL_STAGE_PIN(...)` 不是“先记个愿望，之后再补节点”，它只接受当前已经存在且可 materialize 的节点；render-only、hidden 或不存在的节点都会返回失败。
- 如果节点原来没有 live slot，但你把它切成 `KEEPALIVE`、或者把一个已 pin 节点从 hidden/零尺寸恢复为可见，记得继续发 `notify_node_changed()` / `notify_node_bounds_changed()`，让 `virtual_stage` 重新把它拉回 slot。

## 相关头文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_tree.h`
- `src/widget/egui_view_virtual_stage.h`

## `virtual_page` 入门示例

如果用户刚开始接 `virtual_page`，建议先看 `example/HelloVirtual/virtual_page_basic/`。

这个例程刻意把内容收缩到最小闭环：

- 一个简单的 action bar
- 一个纵向 `virtual_page`
- 三种 section 语义：hero / metric / checklist

它重点覆盖：

- `init_with_setup`
- 外置 section 状态
- 点击回查 section
- `notify_section_changed_by_stable_id()`
- `notify_section_resized_by_stable_id()`
- `scroll_to_section_by_stable_id()` / `ensure_section_visible_by_stable_id()`

读完 `virtual_page_basic` 后，再看 `example/HelloVirtual/virtual_page/`，更容易理解复杂场景里的 keepalive、state cache 和更丰富的模块布局。

## `virtual_strip` 入门示例

如果用户刚开始接 `virtual_strip`，建议先看 `example/HelloVirtual/virtual_strip_basic/`。
这个例程刻意把内容收缩到最小闭环：

- 一个简单的 action bar
- 一个横向 `virtual_strip`
- 一种基础卡片视图，带三种宽度变化

它重点覆盖：

- `init_with_setup`
- 外置 item 状态
- `resolve_item_by_view()` 点击回查
- `notify_item_changed_by_stable_id()` / `notify_item_resized_by_stable_id()`
- `scroll_to_stable_id()` / `ensure_item_visible_by_stable_id()`

读完 `virtual_strip_basic` 后，再看 `example/HelloVirtual/virtual_strip/`，更容易理解复杂场景里的 scene 切换、keepalive、state cache 和 richer 轨道语义。

## `virtual_grid` 入门示例

如果用户刚开始接 `virtual_grid`，建议先看 `example/HelloVirtual/virtual_grid_basic/`。
这个例程刻意把内容收缩到最小闭环：

- 一个简单的 action bar
- 一个纵向 `virtual_grid`
- 一种基础 tile 视图，配几种高度变化

它重点覆盖：

- `init_with_setup`
- 外置 item 状态
- `resolve_item_by_view()` 点击回查
- `notify_item_changed_by_stable_id()` / `notify_item_resized_by_stable_id()`
- `set_column_count()` / `ensure_item_visible_by_stable_id()`

读完 `virtual_grid_basic` 后，再看 `example/HelloVirtual/virtual_grid/`，更容易理解复杂场景里的 row slot、列切换、自适应高度和 richer grid 语义。

## `virtual_section_list` 入门示例

如果用户刚开始接 `virtual_section_list`，建议先看 `example/HelloVirtual/virtual_section_list_basic/`。
这个例程刻意把内容收缩到最小闭环：

- 一个简单的 action bar
- 一个纵向 `virtual_section_list`
- 明确区分的 `section header + grouped row`

它重点覆盖：

- `init_with_setup`
- 外置 section / item 状态
- `resolve_entry_by_view()` 回查 header 或 item
- header 折叠 / 展开后的 `notify_data_changed()`
- item patch 后的 `notify_item_changed_by_stable_id()` / `notify_item_resized_by_stable_id()`
- `scroll_to_item_by_stable_id()` / `ensure_entry_visible_by_stable_id()`

读完 `virtual_section_list_basic` 后，再看 `example/HelloVirtual/virtual_section_list/`，更容易理解复杂场景里的 insert / remove、state cache、keepalive 和 pulse 动画。

## `virtual_tree` 入门示例

如果用户刚开始接 `virtual_tree`，建议先看 `example/HelloVirtual/virtual_tree_basic/`。

这个例程刻意把内容收缩到最小闭环：

- 一个简单的 action bar
- 一个纵向 `virtual_tree`
- 三层最小树结构：root / group / task

它重点覆盖：

- `init_with_setup`
- 外置树节点状态
- 点击回查 node
- branch 折叠 / 展开后的 `notify_data_changed()`
- task patch 后的 `notify_node_changed_by_stable_id()` / `notify_node_resized_by_stable_id()`
- `scroll_to_node_by_stable_id()` / `ensure_node_visible_by_stable_id()`

读完 `virtual_tree_basic` 后，再看 `example/HelloVirtual/virtual_tree/`，更容易理解复杂场景里的 keepalive、state cache、pulse 动画和更大的可见节点流。
