# Virtual Viewport 高层容器总览

完整使用指南见：
- [docs/virtual_container_family_guide.md](../../../docs/virtual_container_family_guide.md)

`virtual_viewport` 不是单个“长列表控件”，而是一套建立在同一套虚拟化核心之上的容器家族。

它们共享这些能力：

- 子 view 只在需要渲染时创建和绑定
- 可支持 1000+ 数据项
- 支持 `stable_id` 定位、局部刷新、插入、删除、移动
- 支持可变尺寸
- 支持 `keepalive + state cache`

当前仓库已经提供 6 个高层封装：

- `virtual_list`
- `virtual_page`
- `virtual_strip`
- `virtual_grid`
- `virtual_section_list`
- `virtual_tree`

## 先按业务语义选，不要先按“长得像不像 list”选

虚拟化容器首先是数据结构容器，其次才是视觉容器。

同样都是“滚动中按需创建子 view”，但它们承载的业务单元不同：

- `virtual_list` 的单元是 `row`
- `virtual_page` 的单元是 `section / module`
- `virtual_strip` 的单元是横向 `item card`
- `virtual_grid` 的单元是二维 `tile / card`
- `virtual_section_list` 的单元是 `header + grouped row`
- `virtual_tree` 的单元是 `node`

如果业务语义选错了，后面通常会出现这些问题：

- 自己补很多本不该在业务层维护的缩进、分组、折叠逻辑
- 点击命中、定位、局部刷新接口不顺手
- 为了兼容场景差异，不得不在一个容器里塞很多“伪类型 row”
- 示例最后都长得像 list，只是换了配色

## 什么时候用哪一种

| 容器 | 主轴 / 结构 | 业务单元 | 典型视觉语义 | 更适合的场景 |
| --- | --- | --- | --- | --- |
| `virtual_list` | 单轴纵向 | row | 连续信息流 | feed、chat、timeline、log |
| `virtual_page` | 单轴纵向 | section / module | dashboard block、表单区块、摘要页块 | 仪表盘、设置页大分区、详情页模块 |
| `virtual_strip` | 单轴横向 | rail item | poster rail、queue chip、cue strip | 推荐带、图库、播放队列、时间轴条带 |
| `virtual_grid` | 二维网格 | tile / card | 宫格卡片、缩略图墙 | 商品宫格、相册墙、卡片面板 |
| `virtual_section_list` | 纵向分组 | header + row | 分组板、组头 + 组内明细 | 消息分组、工单分组、设置分组 |
| `virtual_tree` | 纵向层级 | node | branch / leaf、拓扑树 | 文件树、设备拓扑、组织树 |

## 一个简单的选择顺序

可以先按下面顺序判断：

1. 有明确父子层级、需要折叠展开：`virtual_tree`
2. 有组头和组内条目：`virtual_section_list`
3. 主轴是横向，item 宽度可能变化：`virtual_strip`
4. 一屏里本质是二维卡片流：`virtual_grid`
5. 一页由多个大模块拼接：`virtual_page`
6. 其余普通连续行内容：`virtual_list`

### `virtual_list`

基线纵向列表，适合：

- feed
- chat
- timeline
- log / task list

当前仓库里没有单独拆一个 `virtual_list/` 目录。更接近的参考是：

- `example/HelloBasic/virtual_viewport/`

这个例程现在故意直接使用底层 `virtual_viewport` adapter，来展示“row 语义也可以不经过高层封装”，便于对照底层和高层 API 的差异。

### `virtual_page`

很多大区块拼成的一页，适合：

- dashboard
- 设置页大分区
- 长详情页的大 section

示例：

- `example/HelloBasic/virtual_page/`

### `virtual_strip`

横向 item 带，适合：

- gallery
- hero rail
- playlist / queue lane
- cue / beat strip

示例：

- `example/HelloBasic/virtual_strip/`

### `virtual_grid`

二维卡片面板，适合：

- 商品宫格
- 文件缩略图墙
- 卡片仪表盘

示例：

- `example/HelloBasic/virtual_grid/`

### `virtual_section_list`

分组 header + 组内 row，适合：

- 分组消息列表
- 工单分组
- 会话分组
- 设置页一级组 + 二级条目

示例：

- `example/HelloBasic/virtual_section_list/`

### `virtual_tree`

树形结构，适合：

- 文件树
- 设备拓扑
- 组织结构
- 可折叠任务树

示例：

- `example/HelloBasic/virtual_tree/`

## 为什么这些示例不应该都长得像 list

如果视觉上看不出差异，通常不是虚拟化能力不够，而是上层场景语义没有被表达出来：

- `virtual_list` 应该像连续 row
- `virtual_page` 应该像一页里的异构模块块
- `virtual_strip` 应该强调横向节奏和宽度变化
- `virtual_grid` 应该强调列数、tile 密度和二维排布
- `virtual_section_list` 应该先看到组头，再看到组内 row
- `virtual_tree` 应该先看到层级和连接关系，再看到叶子内容

也就是说，设计时要先决定“这个容器里的最小业务单元是什么”，再决定它的样式。

## 推荐的初始化方式

所有高层容器现在都统一提供：

- `*_setup_t`
- `*_init_with_setup()`
- `*_apply_setup()`

推荐把下面这些内容一次性放进 `setup`：

- `params`
- `data_source`
- `data_source_context`
- `state_cache_max_entries`
- `state_cache_max_bytes`

典型写法：

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
        .measure_item_height = app_measure_item_height,
        .create_item_view = app_create_item_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_alive = app_should_keep_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
};

static const egui_view_virtual_list_setup_t list_setup = {
        .params = &list_params,
        .data_source = &list_source,
        .data_source_context = &list_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_row_state_t),
};

egui_view_virtual_list_init_with_setup(EGUI_VIEW_OF(&list_view), &list_setup);
```

## 什么时候应该直接用 `virtual_viewport`

如果你的业务单元既不是 row，也不是 page / grid / section / tree，而只是想复用“按需创建 + 可变尺寸 + stable_id + 状态缓存”这套底层机制，可以直接使用 `virtual_viewport`。

更适合直接用底层 viewport 的情况：

- 单元语义完全自定义，现有高层容器名字会误导业务建模
- 想自己决定 `view_type / measure / bind / unbind`，但不想再手搓可见区计算
- 希望一个容器里混合多种异构块，但又不想先硬塞成 list/page 语义

现在底层 viewport 也补齐了和高层容器一致的初始化入口：

- `egui_view_virtual_viewport_setup_t`
- `egui_view_virtual_viewport_init_with_setup()`
- `egui_view_virtual_viewport_apply_setup()`

典型写法：

```c
static egui_view_virtual_viewport_t viewport;
static app_context_t viewport_ctx;

static const egui_view_virtual_viewport_params_t viewport_params = {
        .region = {{8, 72}, {224, 240}},
        .orientation = EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_VERTICAL,
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_item_extent = 72,
};

static const egui_view_virtual_viewport_adapter_t viewport_adapter = {
        .get_count = app_get_count,
        .get_stable_id = app_get_stable_id,
        .find_index_by_stable_id = app_find_index,
        .get_view_type = app_get_view_type,
        .measure_main_size = app_measure_item_extent,
        .create_view = app_create_view,
        .bind_view = app_bind_view,
        .unbind_view = app_unbind_view,
        .should_keep_alive = app_should_keep_alive,
        .save_state = app_save_state,
        .restore_state = app_restore_state,
};

static const egui_view_virtual_viewport_setup_t viewport_setup = {
        .params = &viewport_params,
        .adapter = &viewport_adapter,
        .adapter_context = &viewport_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_item_state_t),
};

egui_view_virtual_viewport_init_with_setup(EGUI_VIEW_OF(&viewport), &viewport_setup);
```

## 直接使用 viewport 时最常用的 helper

对于高层容器，很多 `stable_id / click / 可见槽位` helper 已经包装好了。现在底层 viewport 也提供了一套通用 helper，便于做“自定义语义容器”：

- `egui_view_virtual_viewport_find_slot_index_by_stable_id()`
- `egui_view_virtual_viewport_find_slot_by_stable_id()`
- `egui_view_virtual_viewport_find_view_by_stable_id()`
- `egui_view_virtual_viewport_get_slot_entry()`
- `egui_view_virtual_viewport_resolve_item_by_stable_id()`
- `egui_view_virtual_viewport_resolve_item_by_view()`
- `egui_view_virtual_viewport_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_viewport_visit_visible_items()`
- `egui_view_virtual_viewport_find_first_visible_item_view()`

点击命中时的典型写法：

```c
static void app_item_click_cb(egui_view_t *self)
{
    egui_view_virtual_viewport_entry_t entry;

    if (!egui_view_virtual_viewport_resolve_item_by_view(EGUI_VIEW_OF(&viewport), self, &entry))
    {
        return;
    }

    app_select_item(entry.index, entry.stable_id);
}
```

这样业务层就不需要自己维护“当前这个 view 对应哪一个数据项”的映射表。

## 关键规则

### 1. `stable_id` 必须稳定

不要把“当前索引”当成长期 `stable_id`。同一个业务对象在插入、删除、移动之后，只要对象没变，`stable_id` 就不应该变。

### 2. 尺寸变化后要显式通知

如果数据变化会影响高度或宽度，业务层必须调用对应的 `notify_*_resized()` 接口。

### 3. `keepalive` 和 `state cache` 分工不同

- `keepalive` 解决“短时间内必须保留同一个 view 实例”
- `state cache` 解决“view 可以回收，但离屏后还要恢复动画或临时态”

### 4. 保持目标可见优先使用 `ensure_*`

如果只是希望当前选中项 / 焦点项 / 动画项保持在屏幕内，优先调用 `ensure_*_visible_by_stable_id()`，不要自己在业务层重复写滚动判断。

### 5. 底层 viewport 的刷新语义和高层容器完全一致

直接使用 `virtual_viewport` 时，刷新规则可以按这张表记：

| 数据变化 | 该调用什么 |
| --- | --- |
| 整体数据集重排、过滤、切场景 | `notify_data_changed()` |
| 单项内容变化，但尺寸不变 | `notify_item_changed()` / `notify_item_changed_by_stable_id()` |
| 单项尺寸变化 | `notify_item_resized()` / `notify_item_resized_by_stable_id()` |
| 插入 | `notify_item_inserted()` |
| 删除 | `notify_item_removed()` |
| 移动 | `notify_item_moved()` |

判断原则很简单：只要主轴尺寸可能变，就走 `resized`；只要结构顺序变，就走 `insert / remove / moved / data_changed`。

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_tree.h`

## 状态缓存自检 helper

现在 raw `virtual_viewport` 和所有高层容器都同时提供：

- `set_state_cache_limits(...)`
- `get_state_cache_entry_limit(...)`
- `get_state_cache_byte_limit(...)`

这两个 getter 主要用于运行时自检：

- 确认 `init_with_setup()` / `apply_setup()` 已经把缓存上限正确写入
- 做场景切换或数据源热切换后，快速确认 wrapper 和底层 viewport 的配置仍然一致
- 写单元测试时，不需要再绕回 `egui_view_virtual_viewport_get_state_cache_*()` 去检查高层容器

## `HelloBasic/virtual_viewport` 例程现在展示什么

这个例程现在故意不再长得像普通 `list`。

- 顶层直接使用 raw `virtual_viewport` adapter，而不是再包一层 `virtual_list`
- 场景拆成 `Canvas / Chat / Board`
- 每个场景都复用同一套“按需创建 + stable_id + 状态缓存 + 局部刷新”能力
- 但每个场景的最小业务单元不同，所以 `bind_view()` 里使用了不同模板

当前例程里的三组模板语义：

- `Canvas`：`hero / note / chip`，强调自由堆叠、不同宽度、不同横向位置
- `Chat`：`inbound / outbound / system`，强调气泡、左右对话和系统横幅
- `Board`：`queue / run / block / done`，强调工单板块、中心告警和右侧完成卡

### 为什么 adapter 的 `get_view_type()` 只保留粗粒度类型

这个例程里，adapter 层的 `get_view_type()` 故意只返回少量粗粒度池化类型，而不是把每个业务模板都做成独立 `view_type`。

原因是：

- `view_type` 过细时，切场景后旧类型可能占满 view 池
- 在没有额外 `destroy/recreate` 策略时，新的细粒度类型会拿不到可复用 view
- 结果就是场景切换后出现空白 viewport

所以这里拆成两层：

- adapter 层：只区分少量可复用的池化模板类型，保证 scene switch 时 slot 能复用
- bind 层：再根据 `scene + item state + variant` 决定真正的视觉模板和布局细节

这是直接使用 raw `virtual_viewport` 时一个很重要的实践点：
`view_type` 负责“池里怎么复用”，不一定等于业务里“有多少种卡片”。
