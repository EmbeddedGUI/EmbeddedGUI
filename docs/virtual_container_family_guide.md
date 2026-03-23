# Virtual 虚拟容器家族使用指南

本文面向准备在 EmbeddedGUI 中接入 `virtual_viewport` 及其高层封装容器的开发者，重点回答四类问题：

1. 为什么要做这套虚拟容器，而不是继续用普通 `list`、`scroll` 或者一次性创建全部子控件。
2. 这套架构的设计理念是什么，尤其是“只在需要渲染时创建 view”与“动画、输入态、选中态不丢失”之间如何平衡。
3. 六类高层容器与底层 `virtual_viewport` 的场景边界在哪里，应该怎么选型。
4. API 应该怎么用，数据源要提供什么，数据变化后应该发哪种通知，点击、定位、状态缓存和 keepalive 又该怎么接。

这份文档不是替代各示例目录里的 `readme.md`，而是把它们整理为一套完整的方法论。建议阅读顺序是：

1. 先看本文的“设计理念”和“选型原则”。
2. 再看你实际要用的容器章节。
3. 最后打开对应示例目录和头文件，对照落地。

相关示例入口：

- [raw viewport 入门示例](../example/HelloVirtual/virtual_viewport_basic/readme.md)
- [raw viewport 总览](../example/HelloVirtual/virtual_viewport/readme.md)
- [virtual_page 入门示例](../example/HelloVirtual/virtual_page_basic/readme.md)
- [virtual_page 示例](../example/HelloVirtual/virtual_page/readme.md)
- [virtual_strip 入门示例](../example/HelloVirtual/virtual_strip_basic/readme.md)
- [virtual_strip 示例](../example/HelloVirtual/virtual_strip/readme.md)
- [virtual_grid 入门示例](../example/HelloVirtual/virtual_grid_basic/readme.md)
- [virtual_grid 示例](../example/HelloVirtual/virtual_grid/readme.md)
- [virtual_section_list 入门示例](../example/HelloVirtual/virtual_section_list_basic/readme.md)
- [virtual_section_list 示例](../example/HelloVirtual/virtual_section_list/readme.md)
- [virtual_tree 入门示例](../example/HelloVirtual/virtual_tree_basic/readme.md)
- [virtual_tree 示例](../example/HelloVirtual/virtual_tree/readme.md)

补充说明：如果你的页面不是“滚动数据集”，而是“固定画布 + 大量绝对定位节点 + 少量活跃控件”，请直接看：

- [virtual_stage_basic 入门示例](../example/HelloVirtual/virtual_stage_basic/readme.md)
- [virtual_stage_showcase 对比示例](../example/HelloVirtual/virtual_stage_showcase/readme.md)
- [virtual_stage 复杂示例](../example/HelloVirtual/virtual_stage/readme.md)
- [virtual_stage 架构说明](../doc/source/architecture/virtual_stage.md)

实践上再记住两点：

- 固定节点数组且回调就在当前文件时，优先直接用一步式 bridge 宏：常规交互场景用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_INTERACTIVE_BRIDGE_INIT_WITH_LIMIT(...)`，整屏且要状态恢复的 rich 场景用 `EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_SCREEN_STATEFUL_BRIDGE_INIT_WITH_LIMIT(...)`；这样 `ops + bridge` 一次收口，用户第一次接入只要关心回调函数和节点数组。
- 如果 `ops` 需要跨文件导出、复用，或者你已经把 `params` / `node_source` 单独拆出来了，再回退到 `EGUI_VIEW_VIRTUAL_STAGE_ARRAY_OPS_*_INIT(...) + EGUI_VIEW_VIRTUAL_STAGE_NODE_ARRAY_BRIDGE_INIT*(...)` 这套分层宏。
- 如果手上已经是 `egui_view_virtual_stage_t *`，继续优先用 bridge 头里的 typed convenience 宏，如 `EGUI_VIEW_VIRTUAL_STAGE_INIT_ARRAY_BRIDGE(...)`、`EGUI_VIEW_VIRTUAL_STAGE_ADD_ROOT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_SET_BACKGROUND(...)`、`EGUI_VIEW_VIRTUAL_STAGE_REQUEST_LAYOUT(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODE(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_BOUNDS_IDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_NOTIFY_NODES_BOUNDS(...)`、`EGUI_VIEW_VIRTUAL_STAGE_TOGGLE_PIN(...)`，把 `EGUI_VIEW_OF(...)` 这层也尽量省掉。少量 `stable_id` 的一次性联动优先直接用 `NOTIFY_IDS(...)` / `NOTIFY_BOUNDS_IDS(...)`，需要复用数组时再用 `NOTIFY_NODES(...)` / `NOTIFY_NODES_BOUNDS(...)`。
- `EGUI_VIEW_VIRTUAL_STAGE_PIN(...)` 只适合当前已经存在且可 materialize 的节点；render-only、hidden 或不存在的节点不会被 pin 成功。
- 如果一个节点原来没有 live slot，但你把它改成了 `KEEPALIVE`，或者让一个已 pin 节点从 hidden/零尺寸恢复可见，更新 desc 后要补发 `notify_node_changed()` / `notify_node_bounds_changed()`。

---

## 1. 为什么需要这套虚拟容器

在嵌入式 GUI 里，很多业务界面都会碰到下面这些现实问题：

- 数据量可能是几百、上千，甚至更多，但屏幕一次只能看到十几个业务单元。
- 每个单元并不一定等高或等宽，内容一变，尺寸就会变。
- 数据会动态增删改移，不能假设索引稳定不变。
- 某些 item 有动画、选中态、输入态、展开态、拖拽态、临时编辑态，不能因为滚出屏幕就把状态直接丢掉。
- 内存很紧，不能把 1000 个子控件全部常驻在 view 树里。
- 又因为是嵌入式场景，很多项目并不希望频繁 `malloc/free`，更希望使用固定对象池、静态数组或者可控的复用策略。

普通 `scroll + group + 手工管理子控件` 的方案，做到几十个条目还可以，到了几百个之后通常会遇到三个典型问题：

- view 树太大，布局和重绘成本明显上升。
- 业务层自己维护“当前可见区有哪些 item、哪些要回收、哪些要复用”，代码很快失控。
- 因为缺少统一的 `stable_id`、状态缓存、定位 helper 和精确通知机制，后续插入、删除、局部刷新、动画恢复都容易出错。

这套 virtual 容器家族要解决的，本质上不是“做一个更长的列表控件”，而是提供一套**面向大数据集、按需创建、支持稳定定位和状态保留的容器架构**。  
它可以承载的最小业务单元未必是传统 row，也可以是：

- 一页里的异构 section
- 横向轨道中的 item
- 宫格中的 tile
- 分组列表里的组头与组内条目
- 树形结构中的节点
- 甚至是完全自定义的画布块、聊天气泡、看板卡片

这也是为什么仓库里最后没有只停留在一个“listview”，而是拆成了 `virtual_page`、`virtual_strip`、`virtual_grid`、`virtual_section_list`、`virtual_tree`，并保留底层 `virtual_viewport` 作为通用虚拟化核心。

---

## 2. 一张图看整体架构

可以把整套设计理解成三层：

```text
+--------------------------------------------------------------+
|                        Business Data                         |
|  item / section / node / grouped entry / custom block       |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|             Semantic Wrapper Or Raw Viewport Layer           |
|  virtual_list / page / strip / grid / section_list / tree   |
|  or direct virtual_viewport adapter                         |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                 Virtualization Core: Viewport                |
|  stable_id | measure | slot reuse | keepalive | state cache |
|  scroll    | locate  | visible visit | precise notify       |
+-------------------------------+------------------------------+
                                |
                                v
+--------------------------------------------------------------+
|                     Actual View Instances                    |
|  create_view / bind_view / unbind_view / destroy_view       |
|  these can come from static pools, not necessarily heap     |
+--------------------------------------------------------------+
```

这张图有两个非常关键的含义：

第一，**虚拟化不等于堆分配**。  
框架需要的是“逻辑上的创建、绑定、解绑、销毁接口”，并不要求你一定从堆里 `malloc` 一个新 view。你完全可以像当前示例一样，使用静态对象池或固定数组，按 `view_type` 从池中取对象，把它绑定到当前可见的业务单元，再在回收时解绑并放回池里。  
换句话说，virtual 容器解决的是 view 生命周期和可见性管理，不是强迫你采用某一种内存分配方式。

第二，**高层容器的价值主要是业务语义和易用性**。  
底层 `virtual_viewport` 已经能做按需创建、尺寸测量、slot 复用、状态缓存和定位。高层容器做的事情是把一类常见业务模型包成更顺手的 API，让你不必每次都自己写 adapter 映射。  
比如：

- `virtual_page` 把“section”的概念直接暴露出来。
- `virtual_strip` 把主轴切成横向，并把尺寸语义改成 item width。
- `virtual_grid` 把二维 tile 语义建立在 row slot 之上。
- `virtual_section_list` 帮你管理 section header + row 的扁平化映射。
- `virtual_tree` 帮你把树节点展开为可见节点流。

---

## 3. 设计理念

### 3.1 先决定“最小业务单元”是什么，再决定长什么样

这套架构最重要的设计原则之一是：**容器的第一职责是承载业务单元，而不是追求视觉上像不像 list**。

很多界面最后“看起来都像 list”，往往不是虚拟化能力不够，而是建模阶段就把所有东西都拍平成了 row。  
一旦模型选错，后面会出现这些问题：

- 业务层要自己补层级、补组头、补横向轨道、补二维排布。
- 命中测试和定位 helper 的语义不顺手。
- 插入、删除、移动和尺寸更新接口开始变得别扭。
- 示例视觉也很难体现容器之间的差异。

所以更合理的顺序应该是：

1. 先问自己：屏幕里的最小业务单元是什么。
2. 再问：这个单元是否天然有横向、分组、层级、二维或异构 section 语义。
3. 最后才决定视觉风格长什么样。

例如：

- 如果业务最小单元是“仪表盘模块”，那就是 `virtual_page`。
- 如果最小单元是“海报轨道里的卡片”，那就是 `virtual_strip`。
- 如果最小单元是“二维墙上的 tile”，那就是 `virtual_grid`。
- 如果最小单元是“树节点”，那就是 `virtual_tree`。

### 3.2 `stable_id` 是第一公民，索引不是

索引适合描述“当前顺序中的第几个”，不适合描述“这个业务对象本身是谁”。  
一旦列表发生插入、删除、移动，如果你把索引当作身份标识，就会遇到：

- 选中项错乱
- 动画状态套错对象
- `ensure_visible` 定位到错误对象
- 局部刷新命中了错误 item

因此这套 virtual 容器要求你尽量提供稳定的 `stable_id`，并围绕 `stable_id` 暴露了一整套 helper：

- `find_index_by_stable_id()`
- `resolve_*_by_stable_id()`
- `find_view_by_stable_id()`
- `scroll_to_*_by_stable_id()`
- `ensure_*_visible_by_stable_id()`
- `notify_*_changed_by_stable_id()`
- `notify_*_resized_by_stable_id()`

只要你的业务对象没变，`stable_id` 就不应该因为重新排序而变化。

### 3.3 只创建可见 view，不等于状态一定会丢

用户最容易担心的一点是：  
“如果 item 只有在需要渲染时才创建，那有动画、输入态、展开态、选中态时，回收再创建会不会把状态弄丢？”

答案是：**会不会丢，取决于你有没有把状态放到正确层级，而不是取决于是否做虚拟化。**

这套架构提供了两种配套机制：

- `keepalive`
- `state cache`

二者解决的问题不一样。

#### `keepalive` 解决的是“尽量保住同一个 view 实例”

当某个 item 的 view 实例本身很重要时，应该考虑 keepalive，例如：

- 某个 item 正在播放一段局部动画，而且动画对象直接挂在 view 上。
- 某个条目正处在复杂编辑态，子控件内部暂时有一批运行时状态。
- 某个卡片刚刚被选中，需要在短时间内继续保持同一个对象实例。

这时，数据源可以通过 `should_keep_alive(...)` 告诉框架：  
“这个 stable_id 对应的 view 先别回收到普通池里。”

#### `state cache` 解决的是“view 可以回收，但状态要恢复”

如果某个状态并不要求同一个 view 实例一直存活，只要求“滚回来时能恢复”，就应该使用状态缓存。典型场景包括：

- 展开/折叠状态
- 选中状态
- 某个 pulse 动画的阶段或起始 tick
- 临时输入内容
- 某个局部编辑器的滚动位置

这时可以使用：

- `set_state_cache_limits(...)`
- `write_*_state(...)`
- `read_*_state(...)`
- `write_*_state_for_view(...)`
- `read_*_state_for_view(...)`
- `clear_*_state_cache()`
- `remove_*_state_by_stable_id()`

新的 wrapper 现在也都提供了 getter，方便运行时自检：

- `get_state_cache_entry_limit(...)`
- `get_state_cache_byte_limit(...)`

### 3.4 视图类型 `view_type` 用来池化复用，不一定等于业务模板总数

这是直接使用 raw `virtual_viewport` 时一个很容易踩坑的点。  
`view_type` 的职责是告诉框架：“哪些 view 之间可以互相复用同一个对象池”。它并不一定要精确映射成所有业务视觉模板。

如果你把 `view_type` 切得太细，可能带来两个问题：

- 池被切碎，某些类型几乎没有复用机会。
- 场景切换之后，新场景想要的类型在旧池里拿不到，导致复用效率下降，甚至在你自己的对象池实现里引出空池问题。

更好的做法通常是：

- 在 adapter 层保持较粗粒度的 `view_type`，只区分真正影响结构的类型。
- 在 `bind_view()` 层再根据场景、状态和变体决定最终视觉细节。

仓库里的 `HelloVirtual/virtual_viewport` 示例就是按这个思路写的：  
底层池化类型不会细到每一个业务模板，真正的差异化布局是在 bind 阶段完成的。

### 3.5 精确通知比“全部重刷”更重要

框架提供了完整的数据变化通知体系，目的是让你在数据变化后用最小成本修正虚拟化窗口和测量缓存。  
常见策略如下：

- 整体换场景、整体过滤、整体重排：`notify_data_changed()`
- 单项内容变了，但尺寸不变：`notify_*_changed(...)`
- 单项尺寸变了：`notify_*_resized(...)`
- 插入：`notify_*_inserted(...)`
- 删除：`notify_*_removed(...)`
- 移动：`notify_*_moved(...)`
- 对 `virtual_stage` 来说，如果整体变更发生在一次 capture 尚未结束时，旧 view 会先收到 `cancel` 再被回收或复用，避免池化复用后残留 pressed 一类瞬时交互状态。

如果尺寸可能变化却只发了 `changed`，就容易出现：

- 文本压到别的区域
- 下一项位置不对
- 滚动锚点抖动
- 你之前见过的“长时间滚动后触发 region assert”这类边界问题更容易被放大

所以一个简单原则是：

- **内容变但主轴尺寸不变，用 `changed`**
- **主轴尺寸变，用 `resized`**
- **结构顺序变，用 `insert/remove/moved/data_changed`**

---

## 4. 容器选型总表

| 容器 | 最小业务单元 | 主轴/结构 | 典型场景 | 不太适合 |
| --- | --- | --- | --- | --- |
| `virtual_viewport` | 完全自定义 | 自定义 | 画布块、聊天气泡、看板卡、自定义混排 | 已经能明确归类为 list/page/grid/tree 的场景 |
| `virtual_list` | row | 纵向单轴 | feed、日志、任务流、普通消息列表 | 强业务分组、二维排布、递归树 |
| `virtual_page` | section/module | 纵向单轴 | 仪表盘、设置页大区块、详情页异构模块 | 纯 row 列表、强组头、递归树 |
| `virtual_strip` | rail item | 横向单轴 | 海报带、播放队列、时间轴条带、图库轨道 | 主轴不是横向、二维 tile 墙 |
| `virtual_grid` | tile/card | 二维网格 | 商品宫格、相册墙、卡片面板、缩略图墙 | 强组头语义、递归树 |
| `virtual_section_list` | section header + item | 纵向分组 | 设置分组、消息分组、工单分组 | 递归父子层级、纯异构 page |
| `virtual_tree` | node | 纵向层级 | 文件树、设备拓扑、组织树、任务树 | 只有组头没有递归层级 |

下面给出每类容器当前示例的视觉效果，方便从“语义”而不是“名字”来选。

### 4.1 Raw `virtual_viewport`：完全自定义语义

`virtual_viewport` 最适合“已有业务单元，但现成 wrapper 名字都不贴切”的场景。  
当前示例故意做成了三种完全不同的表现：画布块、聊天气泡、看板卡片。

**Canvas 场景**

![Raw Viewport Canvas](../runtime_check_output/HelloVirtual_virtual_viewport/default/frame_0000.png)

**Chat 场景**

![Raw Viewport Chat](../runtime_check_output/HelloVirtual_virtual_viewport/default/frame_0012.png)

**Board 场景**

![Raw Viewport Board](../runtime_check_output/HelloVirtual_virtual_viewport/default/frame_0016.png)

### 4.2 `virtual_page`：一页里的异构模块

`virtual_page` 不是长 row，而是一页里按 section 拼出的多个模块。  
它更像 dashboard 或设置页的“大块内容”。

如果只想先理解最小接入模式，建议先看 `virtual_page_basic`；它去掉了顶部摘要 Header，只保留一个简单 action bar 和三类 section，更适合快速建立 API 心智模型。

![Virtual Page](../runtime_check_output/HelloVirtual_virtual_page/default/frame_0004.png)

### 4.3 `virtual_strip`：横向轨道

`virtual_strip` 的重点不是“横着滚”，而是“主轴天然是横向，item 宽度本身就可能不同”。  
它适合 poster rail、queue rail、timeline strip。
如果你只想先建立最小 API 心智模型，建议先看 `virtual_strip_basic`；它去掉了顶部摘要 Header，只保留一个简单 action bar 和单一 strip，更容易对照 `init_with_setup`、点击回查、单项刷新和 jump helper。

![Virtual Strip](../runtime_check_output/HelloVirtual_virtual_strip/default/frame_0004.png)

### 4.4 `virtual_grid`：二维 tile 墙

`virtual_grid` 的重点不是“把 list 缩成多列”，而是按 tile 语义设计二维密度、列数切换和宽度自适应布局。
如果你只想先看最小接法，建议先读 `virtual_grid_basic`；它去掉了顶部摘要 Header，只保留一个简单 action bar 和单一 grid，更适合快速理解 `set_column_count()`、item height 变化通知和基础定位 helper。

![Virtual Grid](../runtime_check_output/HelloVirtual_virtual_grid/default/frame_0010.png)

### 4.5 `virtual_section_list`：先有组头，再有组内条目

`virtual_section_list` 适合“天然存在 section header + 组内明细”的业务，而不是把组头硬塞进普通 list。
如果只想先理解最小接入模式，建议先看 `virtual_section_list_basic`；它去掉了顶部摘要 Header，只保留一个简单 action bar 和一个最小 grouped list，更适合快速建立 section data source、entry 回查和结构刷新通知的心智模型。

![Virtual Section List](../runtime_check_output/HelloVirtual_virtual_section_list/default/frame_0000.png)

### 4.6 `virtual_tree`：层级与连接关系优先

`virtual_tree` 的核心是父子层级、可见节点流以及展开/折叠语义。  
视觉上应该先让人看到 branch/leaf 关系，再看具体内容。

如果只想先理解最小接入模式，建议先看 `virtual_tree_basic`；它去掉了顶部摘要 Header，只保留一个简单 action bar 和 `root / group / task` 三层树，更适合快速建立 tree data source、结构刷新和点击回查的心智模型。

![Virtual Tree](../runtime_check_output/HelloVirtual_virtual_tree/default/frame_0004.png)

---

## 5. 所有 virtual 容器共享的使用套路

虽然每个 wrapper 的命名不同，但使用节奏几乎一致，可以概括成五步。

### 5.1 第一步：定义参数 `params`

`params` 负责描述容器自身，而不是数据内容。常见字段包括：

- `region`：容器位置与大小
- `overscan_before` / `overscan_after`：可见区前后的超扫量
- `max_keepalive_slots`：keepalive 上限
- `estimated_*`：预估尺寸

这类字段的作用是：

- 帮助虚拟化窗口在初始阶段快速建立布局
- 控制滑动时前后多保留多少可见外区域
- 控制“最多愿意保住多少暂时不在屏幕里的 view”

### 5.2 第二步：定义数据源 `data_source` 或 adapter

高层容器用 `*_data_source_t`，底层 viewport 用 `adapter`。  
它们本质都要回答下面几个问题：

- 一共有多少业务单元。
- 某个索引对应哪个 `stable_id`。
- 如果已知 `stable_id`，能不能反查索引。
- 当前 item 应该用哪一种 view 类型。
- 当前 item 尺寸是多少。
- 如何创建 view。
- 如何把 view 绑定到某个业务单元。
- 解绑时如何保存状态。
- 重建时如何恢复状态。
- 是否需要 keepalive。

### 5.3 第三步：定义 `setup`

从实际使用体验看，推荐优先使用 `setup` 初始化，而不是零散地逐个 setter。

原因很简单：

- `setup` 可以把 params、data source、context、state cache 限额一次性收拢。
- 后续如果场景切换、数据源切换、缓存策略切换，可以直接 `apply_setup()`。
- 单元测试更容易做初始化一致性校验。

典型模式如下：

```c
static egui_view_virtual_grid_t card_grid;
static app_context_t grid_ctx;

static const egui_view_virtual_grid_params_t card_grid_params = {
        .region = {{8, 72}, {224, 240}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 6,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = 76,
};

static const egui_view_virtual_grid_setup_t card_grid_setup = {
        .params = &card_grid_params,
        .data_source = &card_grid_ds,
        .data_source_context = &grid_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&card_grid), &card_grid_setup);
```

### 5.4 第四步：在 `bind` 里只做“当前可见项”的视觉绑定

`bind` 阶段最适合做这些事情：

- 设置文本
- 设置颜色
- 设置图标
- 根据数据决定显示/隐藏某些子控件
- 调整当前这一个 item 的局部布局
- 恢复与这个 `stable_id` 绑定的临时状态

不建议在 `bind` 里做这些事情：

- 对全量数据集做遍历
- 顺手改动别的 item 的数据
- 把 `index` 当成长期身份
- 把“下一项会不会存在”这种结构逻辑塞进当前 view 内部

### 5.5 第五步：在数据变化时发送精确通知

这是整套体系能否稳定运行的关键。  
通知发对了，slot 复用、布局修正、滚动锚点和状态缓存都能正常工作。通知发错了，最常见的问题就是错位、重叠、文字出框、定位失准和边界断言。

一个通用对照表如下：

| 数据变化 | 推荐通知 |
| --- | --- |
| 整体重排、整体过滤、整体换场景 | `notify_data_changed()` |
| 单项内容变化，尺寸不变 | `notify_*_changed()` |
| 单项尺寸变化 | `notify_*_resized()` |
| 插入一批项 | `notify_*_inserted()` |
| 删除一批项 | `notify_*_removed()` |
| 移动一项 | `notify_*_moved()` |

---

## 6. API 命名规律与通用能力

为了降低学习成本，这些控件的命名规律基本是统一的。  
只要你学会其中一个，迁移到另一个通常只需要替换“领域名词”。

### 6.1 初始化与配置

几乎所有容器都提供：

- `*_apply_params()`
- `*_init_with_params()`
- `*_apply_setup()`
- `*_init_with_setup()`

如果已经创建完控件，后续想整体换一套配置，可以继续调用 `apply_setup()`。

### 6.2 数据源查询

高层容器通常提供：

- `get_data_source()`
- `get_data_source_context()`

raw viewport 提供：

- `get_adapter()`
- `get_adapter_context()`

### 6.3 滚动与定位

常见 helper：

- `scroll_by(...)`
- `scroll_to_*()`
- `scroll_to_*_by_stable_id()`
- `get_scroll_x()` / `get_scroll_y()` / `get_logical_offset()`
- `find_index_by_stable_id()`
- `resolve_*_by_stable_id()`
- `resolve_*_by_view()`
- `find_view_by_stable_id()`
- `ensure_*_visible_by_stable_id()`

这类 API 的核心价值是：  
让业务层尽量围绕 `stable_id` 做交互，而不是自己去维护“当前这个 view 对应哪个数据对象”的映射表。

例如点击当前 item 时，典型写法是：

```c
static void card_click_cb(egui_view_t *self)
{
    egui_view_virtual_grid_entry_t entry;

    if (!egui_view_virtual_grid_resolve_item_by_view(EGUI_VIEW_OF(&card_grid), self, &entry))
    {
        return;
    }

    select_item(entry.index, entry.stable_id);
}
```

### 6.4 可见项遍历与命中搜索

常见 helper：

- `visit_visible_*()`
- `find_first_visible_*_view()`
- `get_slot_count()`
- `get_slot()`
- `find_slot_by_stable_id()`

它们适合：

- 自动化点击
- 录制动作验证
- 自定义可见项统计
- 找到第一个满足条件的可见项

### 6.5 状态缓存

现在 raw viewport 以及高层 wrapper 都统一提供：

- `set_state_cache_limits(...)`
- `get_state_cache_entry_limit(...)`
- `get_state_cache_byte_limit(...)`
- `clear_*_state_cache()`
- `remove_*_state_by_stable_id()`
- `write_*_state(...)`
- `read_*_state(...)`
- `write_*_state_for_view(...)`
- `read_*_state_for_view(...)`

这组 API 的推荐用法是：

- 初始化时通过 `setup.state_cache_max_entries` 与 `setup.state_cache_max_bytes` 统一下发上限。
- 在调试或单测里通过 getter 验证配置是否正确写入。
- 在 `unbind` 前保存状态，在 `bind`/`restore` 时恢复状态。

### 6.6 keepalive

所有容器都支持 keepalive，只是暴露形式不同。  
业务层重点关注的不是某个单独 helper，而是数据源回调里的：

- `should_keep_alive(...)`

和参数里的：

- `max_keepalive_slots`

经验上，keepalive 不要开太大。  
一个常用策略是只保住少量“高价值 item”：

- 当前选中项
- 正在播放动画的项
- 正在输入的项
- 刚被点击、即将继续交互的项

---

## 7. 各容器如何使用

下面按容器分别说明。为了避免重复，代码只放最典型的接入骨架，完整可运行示例请直接看对应目录。

### 7.1 `virtual_list`

头文件：

- [egui_view_virtual_list.h](../src/widget/egui_view_virtual_list.h)

适合：

- feed
- log
- 普通消息列表
- 任务流

当前仓库没有单独拆一个 `HelloVirtual/virtual_list/` 目录，原因是 raw `virtual_viewport` 示例已经覆盖了最基础的按需 row 绑定逻辑。  
如果你的业务单元就是标准 row，而且不需要额外的分组、层级、二维或横向语义，那么可以优先用 `virtual_list`。

最小骨架：

```c
static egui_view_virtual_list_t list_view;
static app_context_t list_ctx;

static const egui_view_virtual_list_params_t list_params = {
        .region = {{8, 72}, {224, 240}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_item_height = 72,
};

static const egui_view_virtual_list_data_source_t list_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_view_type = app_get_view_type,
        .measure_item_height = app_measure_item_height,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_view_type = 0,
};

static const egui_view_virtual_list_setup_t list_setup = {
        .params = &list_params,
        .data_source = &list_source,
        .data_source_context = &list_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_item_state_t),
};

egui_view_virtual_list_init_with_setup(EGUI_VIEW_OF(&list_view), &list_setup);
```

最常用 helper：

- `egui_view_virtual_list_resolve_item_by_view()`
- `egui_view_virtual_list_find_view_by_stable_id()`
- `egui_view_virtual_list_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_list_notify_item_changed_by_stable_id()`
- `egui_view_virtual_list_notify_item_resized_by_stable_id()`

### 7.2 `virtual_page`

头文件与示例：

- [egui_view_virtual_page.h](../src/widget/egui_view_virtual_page.h)
- [virtual_page/test.c](../example/HelloVirtual/virtual_page/test.c)

适合：

- 一页中的异构 dashboard 模块
- 设置页的大分区
- 详情页上的摘要块、趋势块、操作块

关键认识：

- 它的最小单元是 `section`
- 不是 row
- 一个 section 自己内部完全可以再有复杂子结构

最小骨架：

```c
static egui_view_virtual_page_t page_view;
static app_context_t page_ctx;

static const egui_view_virtual_page_params_t page_params = {
        .region = {{8, 72}, {224, 240}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_section_height = 84,
};

static const egui_view_virtual_page_data_source_t page_source = {
        .get_count = app_get_section_count,
        .get_stable_id = app_get_section_stable_id,
        .find_index_by_stable_id = app_find_section_index,
        .get_section_type = app_get_section_type,
        .measure_section_height = app_measure_section_height,
        .create_section_view = app_create_section_view,
        .bind_section_view = app_bind_section_view,
        .unbind_section_view = app_unbind_section_view,
        .should_keep_alive = app_should_keep_alive,
        .save_section_state = app_save_section_state,
        .restore_section_state = app_restore_section_state,
        .default_section_type = 0,
};

static const egui_view_virtual_page_setup_t page_setup = {
        .params = &page_params,
        .data_source = &page_source,
        .data_source_context = &page_ctx,
        .state_cache_max_entries = 48,
        .state_cache_max_bytes = 48 * sizeof(my_section_state_t),
};

egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&page_view), &page_setup);
```

最常用 helper：

- `egui_view_virtual_page_resolve_section_by_view()`
- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_ensure_section_visible_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`

### 7.3 `virtual_strip`

头文件与示例：

- [egui_view_virtual_strip.h](../src/widget/egui_view_virtual_strip.h)
- [virtual_strip_basic/test.c](../example/HelloVirtual/virtual_strip_basic/test.c)
- [virtual_strip/test.c](../example/HelloVirtual/virtual_strip/test.c)

适合：

- 海报带
- 推荐带
- 播放队列
- 时间轴条带

关键认识：

- 主轴是横向
- 测量函数关注的是 `item width`
- 点击与定位语义仍然围绕 `stable_id`

最小骨架：

```c
static egui_view_virtual_strip_t strip_view;
static app_context_t strip_ctx;

static const egui_view_virtual_strip_params_t strip_params = {
        .region = {{8, 72}, {224, 144}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_item_width = 72,
};

static const egui_view_virtual_strip_data_source_t strip_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_item_view_type = app_get_item_view_type,
        .measure_item_width = app_measure_item_width,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_strip_resolve_item_by_view()`
- `egui_view_virtual_strip_get_item_x_by_stable_id()`
- `egui_view_virtual_strip_get_item_width_by_stable_id()`
- `egui_view_virtual_strip_scroll_to_stable_id()`
- `egui_view_virtual_strip_ensure_item_visible_by_stable_id()`

### 7.4 `virtual_grid`

头文件与示例：

- [egui_view_virtual_grid.h](../src/widget/egui_view_virtual_grid.h)
- [virtual_grid_basic/test.c](../example/HelloVirtual/virtual_grid_basic/test.c)
- [virtual_grid/test.c](../example/HelloVirtual/virtual_grid/test.c)

适合：

- 商品宫格
- 文件或相册缩略图墙
- 仪表板 tile 面板

关键认识：

- `virtual_grid` 的 slot 是 **row slot**，不是 item slot
- 每个 row slot 里再管理多个 cell
- item 高度可以依赖当前列宽重新测量

最小骨架：

```c
static egui_view_virtual_grid_t grid_view;
static app_context_t grid_ctx;

static const egui_view_virtual_grid_params_t grid_params = {
        .region = {{8, 72}, {224, 240}},
        .column_count = 2,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 6,
        .column_spacing = 6,
        .row_spacing = 6,
        .estimated_item_height = 76,
};

static const egui_view_virtual_grid_data_source_t grid_source = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_item_view_type = app_get_item_view_type,
        .measure_item_height = app_measure_item_height,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_grid_resolve_item_by_view()`
- `egui_view_virtual_grid_get_item_x_by_stable_id()`
- `egui_view_virtual_grid_get_item_y_by_stable_id()`
- `egui_view_virtual_grid_get_slot_item_count()`
- `egui_view_virtual_grid_get_slot_item_view()`
- `egui_view_virtual_grid_ensure_item_visible_by_stable_id()`

如果 item 高度因为列宽变化而变化，必须记得发：

```c
egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&grid_view), stable_id);
```

### 7.5 `virtual_section_list`

头文件与示例：

- [egui_view_virtual_section_list.h](../src/widget/egui_view_virtual_section_list.h)
- [virtual_section_list_basic/test.c](../example/HelloVirtual/virtual_section_list_basic/test.c)
- [virtual_section_list/test.c](../example/HelloVirtual/virtual_section_list/test.c)

适合：

- 设置分组
- 工单分组
- 会话分组
- “组头 + 组内条目”的任何场景

关键认识：

- section header 和 item 都是一级公民
- 可以分别测量、分别创建、分别绑定、分别缓存状态
- 提供 section 级和 item 级两组通知接口

最小骨架：

```c
static egui_view_virtual_section_list_t section_list;
static app_context_t section_ctx;

static const egui_view_virtual_section_list_data_source_t section_source = {
        .get_section_count = app_get_section_count,
        .get_section_stable_id = app_get_section_stable_id,
        .find_section_index_by_stable_id = app_find_section_index,
        .get_item_count = app_get_item_count,
        .get_item_stable_id = app_get_item_stable_id,
        .find_item_position_by_stable_id = app_find_item_position,
        .get_section_header_view_type = app_get_header_view_type,
        .get_item_view_type = app_get_item_view_type,
        .measure_section_header_height = app_measure_header_height,
        .measure_item_height = app_measure_item_height,
        .create_section_header_view = app_create_header_view,
        .create_item_view = app_create_item_view,
        .bind_section_header_view = app_bind_header_view,
        .bind_item_view = app_bind_item_view,
        .unbind_section_header_view = app_unbind_header_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_section_header_alive = app_should_keep_header_alive,
        .should_keep_item_alive = app_should_keep_item_alive,
        .save_section_header_state = app_save_header_state,
        .save_item_state = app_save_item_state,
        .restore_section_header_state = app_restore_header_state,
        .restore_item_state = app_restore_item_state,
        .default_section_header_view_type = 0,
        .default_item_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_section_list_resolve_entry_by_view()`
- `egui_view_virtual_section_list_find_item_position_by_stable_id()`
- `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`
- `egui_view_virtual_section_list_notify_section_header_changed_by_stable_id()`
- `egui_view_virtual_section_list_notify_item_resized_by_stable_id()`

### 7.6 `virtual_tree`

头文件与示例：

- [egui_view_virtual_tree.h](../src/widget/egui_view_virtual_tree.h)
- [virtual_tree/test.c](../example/HelloVirtual/virtual_tree/test.c)

适合：

- 文件树
- 设备拓扑
- 组织树
- 任务树

关键认识：

- 数据源需要回答根节点、子节点和展开态
- `entry` 会告诉你当前节点的 `depth`、`parent_stable_id`、`child_count`、`is_expanded`
- 树内部会生成“当前可见节点流”，很多 helper 是围绕 visible node 工作

最小骨架：

```c
static egui_view_virtual_tree_t tree_view;
static app_context_t tree_ctx;

static const egui_view_virtual_tree_data_source_t tree_source = {
        .get_root_count = app_get_root_count,
        .get_root_stable_id = app_get_root_stable_id,
        .get_child_count = app_get_child_count,
        .get_child_stable_id = app_get_child_stable_id,
        .is_node_expanded = app_is_node_expanded,
        .get_node_view_type = app_get_node_view_type,
        .measure_node_height = app_measure_node_height,
        .create_node_view = app_create_node_view,
        .bind_node_view = app_bind_node_view,
        .unbind_node_view = app_unbind_node_view,
        .should_keep_alive = app_should_keep_alive,
        .save_node_state = app_save_node_state,
        .restore_node_state = app_restore_node_state,
        .default_view_type = 0,
};
```

最常用 helper：

- `egui_view_virtual_tree_resolve_node_by_view()`
- `egui_view_virtual_tree_find_visible_index_by_stable_id()`
- `egui_view_virtual_tree_scroll_to_node_by_stable_id()`
- `egui_view_virtual_tree_ensure_node_visible_by_stable_id()`
- `egui_view_virtual_tree_notify_node_resized_by_stable_id()`

---

## 8. 动画、输入态和临时状态如何设计，才不会因为回收而丢失

这是大数据虚拟化场景里最值得单独讲清楚的部分。

### 8.1 不要把所有状态都压在 view 对象上

如果某个状态本质上属于业务对象，就不应该只存在于 view 里。  
例如：

- 是否展开
- 是否选中
- 当前进度值
- 最近一次点击时间
- 动画开始 tick
- 某个临时 badge 是否已读

更稳妥的做法是把它们分层：

| 状态类型 | 推荐存放层级 |
| --- | --- |
| 长期业务状态 | 业务数据模型 |
| 需要跨回收恢复的临时 UI 状态 | state cache |
| 必须保留同一对象实例的短期状态 | keepalive 对应的 view 实例 |

### 8.2 什么时候优先用 keepalive

适合 keepalive 的情况：

- 某个 view 内部已经起了动画对象，短时间内不希望重建
- 某个 item 正在编辑，内部组合控件状态较多
- 某个项马上还会继续交互，希望避免抖动

不适合 keepalive 的情况：

- 数据集很大，但你想给大量 item 都保持实例
- 状态本来就只有几个字节，却想靠 keepalive 硬保留整个 view
- 只是想恢复选中或展开，没必要保住对象本身

### 8.3 什么时候优先用 state cache

适合 state cache 的情况：

- 状态很小
- 状态可以序列化成结构体
- view 回收后允许重新绑定，只要能恢复视觉/交互状态即可

推荐模式：

```c
typedef struct my_item_state
{
    uint8_t expanded;
    uint8_t selected;
    uint16_t pulse_phase;
    uint32_t anim_start_tick;
} my_item_state_t;
```

在保存时：

```c
static void app_save_item_state(void *ctx, egui_view_t *view, uint32_t stable_id)
{
    my_item_state_t state;

    EGUI_UNUSED(ctx);
    EGUI_UNUSED(view);

    state.expanded = app_get_expanded(stable_id);
    state.selected = app_get_selected(stable_id);
    state.pulse_phase = app_get_pulse_phase(stable_id);
    state.anim_start_tick = app_get_anim_start_tick(stable_id);

    (void)egui_view_virtual_grid_write_item_state(EGUI_VIEW_OF(&grid_view), stable_id, &state, sizeof(state));
}
```

在恢复时：

```c
static void app_restore_item_state(void *ctx, egui_view_t *view, uint32_t stable_id)
{
    my_item_state_t state;

    EGUI_UNUSED(ctx);
    EGUI_UNUSED(view);

    if (egui_view_virtual_grid_read_item_state(EGUI_VIEW_OF(&grid_view), stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    app_apply_expanded(stable_id, state.expanded);
    app_apply_selected(stable_id, state.selected);
    app_apply_pulse_phase(stable_id, state.pulse_phase);
    app_apply_anim_start_tick(stable_id, state.anim_start_tick);
}
```

### 8.4 对动画最稳妥的做法：把动画驱动数据化

很多人会在每个 item view 上直接维护一套动画内部变量，然后担心回收后丢失。  
更稳妥的做法通常是：

- 把动画定义成“某个业务状态 + 起始时间”的函数
- view 只负责在 bind 时根据当前时间计算显示结果
- 真正长期存在的是 `anim_start_tick`、`phase`、`progress` 这类数据

这样即使 view 回收再创建：

- 你也能根据 `stable_id` 找回对应状态
- 动画视觉可以自然延续
- 不需要把大量 view 都 keepalive

### 8.5 一个实用的组合策略

在实际项目里，下面这种策略通常最平衡：

- 业务长期状态放模型
- 小型临时状态放 state cache
- 只给少数高价值 item 开 keepalive
- 动画尽量数据驱动，不依赖 view 私有对象永远存活

可以把它记成一句话：  
**长期靠模型，短期靠缓存，极少数必须保实例的才用 keepalive。**

---

## 9. 原始 `virtual_viewport` 何时直接用，何时应该继续封装新容器

如果你的场景已经可以明确归入 page、strip、grid、section_list、tree 之一，那优先使用现成 wrapper。  
因为 wrapper 的价值就在于：

- API 名字顺手
- 业务语义清晰
- helper 语义和数据结构更贴近场景

但如果你遇到下面这些情况，直接用 raw viewport 更合适：

- 业务单元既不是 row，也不是 section、tile、node
- 一个容器里混合了数种异构块，但又不想硬塞成 `virtual_page`
- 你想自己定义 `view_type`、测量逻辑和绑定策略
- 你准备把这类场景进一步抽象成新的高层容器

如果后续要继续封装新的虚拟容器，推荐遵循这四步：

1. 先定义“最小业务单元”是什么。
2. 决定是直接一单元一 slot，还是先扁平化、先分行、先展开成可见流。
3. 为这个业务语义设计专用 `*_data_source_t` 和 `*_entry_t`。
4. 把最常用的定位、点击命中、可见项遍历和状态缓存 helper 包装成语义化 API。

也就是说，新容器的目标不是复制一份 viewport，而是让业务层不必知道底层 viewport 的内部细节。

---

## 10. 常见坑与排查建议

### 10.1 把索引当 `stable_id`

这是最常见的问题源头。  
一旦插入或移动，定位、选中、动画恢复都会错。

### 10.2 忘记发送 `resized`

只发 `changed` 不发 `resized`，很容易出现：

- 文本出框
- item 重叠
- 滚动锚点不稳

### 10.3 `view_type` 切太细

会导致池化效率下降，切场景后复用不稳定。

### 10.4 keepalive 开太大

keepalive 不是越大越好。  
开得太大，本质上是在偷偷把虚拟化收益吃回去。

### 10.5 在 `bind` 里做全量逻辑

`bind` 应该只处理当前这个可见项。  
不要借机修改整批数据，更不要在 `bind` 里遍历全量数据集。

### 10.6 用视觉判断容器，而不是用业务语义判断

“看起来像 list”不代表应该用 `virtual_list`。  
例如聊天气泡、看板 lane、组头 + row、树节点，本质语义都不同。

---

## 11. 推荐验证流程

虚拟容器的质量不能只靠编译通过判断，必须做运行时和视觉验证。推荐流程如下：

1. 编译目标示例
2. 运行 `code_runtime_check.py`
3. 查看关键截图
4. 观察滚动、点击、选中、插入、删除、patch、jump 之后的渲染
5. 运行 `HelloUnitTest`

常用命令：

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_grid PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_grid --keep-screenshots

make -j1 all APP=HelloUnitTest PORT=pc_test
output\\main.exe
```

如果你正在做 UI 调整，真正应该看的不是“程序没崩”，而是：

- 文本有没有压边或越界
- 顶部半截 item 在滚动裁剪时是否仍然稳定
- 点击后选中态是否清晰
- 变更通知后是否产生跳动或重叠
- 不同容器的视觉语义是否真的区分开了

---

## 12. API 速查表

下面这张表适合在接入时快速回忆“该去哪个头文件找什么”。

| 容器 | 头文件 | 初始化核心 | 定位核心 | 状态核心 |
| --- | --- | --- | --- | --- |
| `virtual_viewport` | [egui_view_virtual_viewport.h](../src/widget/egui_view_virtual_viewport.h) | `init_with_setup` | `resolve_item_by_view` / `ensure_item_visible_by_stable_id` | `set_state_cache_limits` / `write_state` |
| `virtual_list` | [egui_view_virtual_list.h](../src/widget/egui_view_virtual_list.h) | `init_with_setup` | `resolve_item_by_view` / `ensure_item_visible_by_stable_id` | `write_item_state` / `get_state_cache_entry_limit` |
| `virtual_page` | [egui_view_virtual_page.h](../src/widget/egui_view_virtual_page.h) | `init_with_setup` | `resolve_section_by_view` / `ensure_section_visible_by_stable_id` | `write_section_state` / `get_state_cache_entry_limit` |
| `virtual_strip` | [egui_view_virtual_strip.h](../src/widget/egui_view_virtual_strip.h) | `init_with_setup` | `resolve_item_by_view` / `ensure_item_visible_by_stable_id` | `write_item_state` / `get_state_cache_entry_limit` |
| `virtual_grid` | [egui_view_virtual_grid.h](../src/widget/egui_view_virtual_grid.h) | `init_with_setup` | `resolve_item_by_view` / `ensure_item_visible_by_stable_id` | `write_item_state` / `get_state_cache_entry_limit` |
| `virtual_section_list` | [egui_view_virtual_section_list.h](../src/widget/egui_view_virtual_section_list.h) | `init_with_setup` | `resolve_entry_by_view` / `ensure_entry_visible_by_stable_id` | `write_entry_state` / `get_state_cache_entry_limit` |
| `virtual_tree` | [egui_view_virtual_tree.h](../src/widget/egui_view_virtual_tree.h) | `init_with_setup` | `resolve_node_by_view` / `ensure_node_visible_by_stable_id` | `write_node_state` / `get_state_cache_entry_limit` |

---

## 13. 落地建议

如果你现在正准备在自己的业务里落这套控件，建议按下面的顺序来：

1. 先判断你的最小业务单元是什么，选对容器，不要急着抄某个示例外观。
2. 先把 `stable_id` 体系梳理清楚，再写数据源。
3. 先只实现创建、绑定、定位和基础通知，让滚动与点击稳定。
4. 再补 `state cache`，最后只给少数高价值项加 `keepalive`。
5. 所有 UI 调整都必须配合运行截图和实际滚动/点击检查。

如果场景已经超出当前六类高层容器的语义边界，就直接上 raw `virtual_viewport`；等你把这个场景跑顺之后，再决定是否值得往上再包一层新的语义化容器。

从架构上说，这套 virtual 家族真正想提供的不是“又一个 listview”，而是一套可以同时兼顾**大数据量、可变尺寸、稳定定位、状态保留、对象池复用和业务语义表达**的通用底座。  
只要你把 `stable_id`、精确通知、keepalive 和状态缓存这四件事用对了，它不仅能做 list，也能做 page、tree、section board、grid wall，甚至你自己的自定义 page/container。
