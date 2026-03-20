# Virtual Viewport 高层封装总览

这组能力不是单一的“大列表控件”，而是一套建立在 `virtual_viewport` 之上的虚拟化容器家族。共同目标是：

- 子 view 只在需要渲染时创建和绑定
- 槽位数量受 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS` 限制
- 支持 1000+ 数据项
- 支持 `stable_id` 定位、局部刷新、插入删除移动
- 支持可变尺寸
- 支持 `keepalive + state cache`

当前仓库已经提供 6 个高层封装：

- `virtual_list`
- `virtual_page`
- `virtual_strip`
- `virtual_grid`
- `virtual_section_list`
- `virtual_tree`

## 现在推荐的初始化方式

高层虚拟容器现在统一提供 `*_setup_t`、`*_apply_setup()` 和 `*_init_with_setup()`。推荐把以下内容一次性收进 `setup`：

- `params`
- `data_source`
- `data_source_context`
- `state_cache_max_entries`
- `state_cache_max_bytes`

典型写法如下：

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

如果只是想写得更短，也可以用：

```c
EGUI_VIEW_VIRTUAL_LIST_SETUP_INIT(list_setup, &list_params, &list_source, &list_ctx);
```

如果容器已经初始化完成，后续要切换 `params`、数据源或缓存限额，优先使用：

```c
egui_view_virtual_list_apply_setup(EGUI_VIEW_OF(&list_view), &list_setup);
```

## 什么时候用哪一种

### 1. 普通长列表用 `virtual_list`

适合：

- feed
- chat
- timeline
- task list

常用 helper：

- `egui_view_virtual_list_get_item_count()`
- `egui_view_virtual_list_resolve_item_by_stable_id()`
- `egui_view_virtual_list_resolve_item_by_view()`
- `egui_view_virtual_list_find_view_by_stable_id()`
- `egui_view_virtual_list_find_first_visible_item_view()`
- `egui_view_virtual_list_scroll_to_stable_id()`
- `egui_view_virtual_list_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_list_notify_item_changed_by_stable_id()`
- `egui_view_virtual_list_notify_item_resized_by_stable_id()`
- `egui_view_virtual_list_visit_visible_items()`
- `egui_view_virtual_list_get_slot_count()`
- `egui_view_virtual_list_find_slot_by_stable_id()`
- `egui_view_virtual_list_get_slot_entry()`

示例：

- `example/HelloBasic/virtual_viewport/`

### 2. 大 section 页面用 `virtual_page`

适合：

- dashboard
- 首页聚合页
- 设置页大分区
- 详情页里的大块 section

常用 helper：

- `egui_view_virtual_page_get_section_count()`
- `egui_view_virtual_page_resolve_section_by_stable_id()`
- `egui_view_virtual_page_resolve_section_by_view()`
- `egui_view_virtual_page_find_view_by_stable_id()`
- `egui_view_virtual_page_find_first_visible_section_view()`
- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_ensure_section_visible_by_stable_id()`
- `egui_view_virtual_page_notify_section_changed_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`
- `egui_view_virtual_page_visit_visible_sections()`
- `egui_view_virtual_page_get_slot_count()`
- `egui_view_virtual_page_find_slot_by_stable_id()`
- `egui_view_virtual_page_get_slot_entry()`

示例：

- `example/HelloBasic/virtual_page/`

### 3. 横向卡片带用 `virtual_strip`

适合：

- gallery
- story rail
- playlist lane
- timeline strip

常用 helper：

- `egui_view_virtual_strip_get_item_count()`
- `egui_view_virtual_strip_resolve_item_by_stable_id()`
- `egui_view_virtual_strip_resolve_item_by_view()`
- `egui_view_virtual_strip_find_view_by_stable_id()`
- `egui_view_virtual_strip_find_first_visible_item_view()`
- `egui_view_virtual_strip_scroll_to_stable_id()`
- `egui_view_virtual_strip_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_strip_notify_item_changed_by_stable_id()`
- `egui_view_virtual_strip_notify_item_resized_by_stable_id()`
- `egui_view_virtual_strip_visit_visible_items()`
- `egui_view_virtual_strip_get_slot_count()`
- `egui_view_virtual_strip_find_slot_by_stable_id()`
- `egui_view_virtual_strip_get_slot_entry()`

示例：

- `example/HelloBasic/virtual_strip/`

### 4. 二维卡片流用 `virtual_grid`

适合：

- 商品宫格
- 卡片流页面
- 文件或相册缩略图墙
- 仪表盘卡片面板

`virtual_grid` 的关键点不是把底层直接做成二维，而是：

- 先按列数把 item 分组成 row
- 只虚拟化可见 row
- row 内再复用多个 cell child

常用 helper：

- `egui_view_virtual_grid_get_item_count()`
- `egui_view_virtual_grid_get_row_count()`
- `egui_view_virtual_grid_find_index_by_stable_id()`
- `egui_view_virtual_grid_resolve_item_by_stable_id()`
- `egui_view_virtual_grid_resolve_item_by_view()`
- `egui_view_virtual_grid_find_view_by_stable_id()`
- `egui_view_virtual_grid_find_first_visible_item_view()`
- `egui_view_virtual_grid_scroll_to_item_by_stable_id()`
- `egui_view_virtual_grid_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_grid_notify_item_changed_by_stable_id()`
- `egui_view_virtual_grid_notify_item_resized_by_stable_id()`
- `egui_view_virtual_grid_visit_visible_items()`
- `egui_view_virtual_grid_get_slot_count()`
- `egui_view_virtual_grid_find_slot_by_stable_id()`
- `egui_view_virtual_grid_get_slot_item_count()`
- `egui_view_virtual_grid_get_slot_entry()`
- `egui_view_virtual_grid_get_slot_item_view()`

注意：

- `virtual_grid` 的 slot 语义是 row slot，不是单个 item slot
- `find_slot_by_stable_id()` 返回的是承载该 item 的 row slot

示例：

- `example/HelloBasic/virtual_grid/`

### 5. 分组列表用 `virtual_section_list`

适合：

- 分组消息列表
- 工单按状态分组
- 时间线按天分组
- 设置页里的一级组和二级条目

常用 helper：

- `egui_view_virtual_section_list_get_section_count()`
- `egui_view_virtual_section_list_get_item_count()`
- `egui_view_virtual_section_list_resolve_entry_by_stable_id()`
- `egui_view_virtual_section_list_resolve_entry_by_view()`
- `egui_view_virtual_section_list_find_view_by_stable_id()`
- `egui_view_virtual_section_list_find_first_visible_entry_view()`
- `egui_view_virtual_section_list_scroll_to_section_by_stable_id()`
- `egui_view_virtual_section_list_scroll_to_item_by_stable_id()`
- `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`
- `egui_view_virtual_section_list_notify_section_header_changed()`
- `egui_view_virtual_section_list_notify_item_resized()`
- `egui_view_virtual_section_list_visit_visible_entries()`
- `egui_view_virtual_section_list_get_slot_count()`
- `egui_view_virtual_section_list_find_slot_by_stable_id()`
- `egui_view_virtual_section_list_get_slot_entry()`

示例：

- `example/HelloBasic/virtual_section_list/`

### 6. 树形场景用 `virtual_tree`

适合：

- 文件树
- 目录树
- 组织架构
- 设备拓扑
- 可折叠任务树

常用 helper：

- `egui_view_virtual_tree_get_root_count()`
- `egui_view_virtual_tree_get_visible_node_count()`
- `egui_view_virtual_tree_find_visible_index_by_stable_id()`
- `egui_view_virtual_tree_resolve_node_by_stable_id()`
- `egui_view_virtual_tree_resolve_node_by_view()`
- `egui_view_virtual_tree_find_view_by_stable_id()`
- `egui_view_virtual_tree_find_first_visible_node_view()`
- `egui_view_virtual_tree_scroll_to_node_by_stable_id()`
- `egui_view_virtual_tree_ensure_node_visible_by_stable_id()`
- `egui_view_virtual_tree_notify_node_changed_by_stable_id()`
- `egui_view_virtual_tree_notify_node_resized_by_stable_id()`
- `egui_view_virtual_tree_visit_visible_nodes()`
- `egui_view_virtual_tree_get_slot_count()`
- `egui_view_virtual_tree_find_slot_by_stable_id()`
- `egui_view_virtual_tree_get_slot_entry()`

树结构整体变化时，当前建议统一使用：

```c
egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
```

示例：

- `example/HelloBasic/virtual_tree/`

## 关键规则

### 1. `stable_id` 必须稳定

不要把“当前索引”当成长期 `stable_id`。同一个业务对象在插入、删除、移动之后，只要对象没变，它的 `stable_id` 就不应该变。

### 2. 尺寸变化后要显式通知

如果数据变化会影响尺寸，业务层要主动调用对应的 `notify_*_resized()`。

### 3. `keepalive` 和 `state cache` 分工不同

- `keepalive` 解决“短时间内必须保留同一个 view 实例”
- `state cache` 解决“view 可以回收，但离屏后还要恢复状态”

推荐策略：

- 正在编辑、拖拽、持有焦点中的条目优先 `keepalive`
- 动画进度、展开态、局部高亮优先存入 `state cache`

### 4. 保持目标可见优先用 `ensure` helper

如果业务只是希望“当前选中项 / 焦点项 / 动画项保持在屏幕内”，优先使用对应的 `ensure_*_visible_by_stable_id()`，
不要在业务层重复手写“先判断可见，再决定是否 scroll_to” 的样板逻辑。`ensure` helper 只会在目标离开当前可见安全带时触发滚动。

如果你需要自己做录制动作、自动化点击或更细粒度的可见性筛选，也可以直接复用底层几何 helper：

- `egui_view_virtual_viewport_is_slot_center_visible()`
- `egui_view_virtual_viewport_is_slot_fully_visible()`
- `egui_view_virtual_viewport_is_main_span_center_visible()`
- `egui_view_virtual_viewport_is_main_span_fully_visible()`
- `egui_view_virtual_viewport_is_main_span_fully_visible()`

如果你经常要做“从当前可见项里挑第一个满足条件的 view 去点击 / 聚焦 / 录制”，优先用各容器的 `find_first_visible_*_view()`，
把业务过滤条件写进 matcher；只有确实需要遍历全部可见项时再用 `visit_visible_*()`。

### 5. 不要为每个数据项都真实创建 view

正确做法是：

- 容器只维护有限数量的槽位 view
- 滚动时复用这些 view
- 在 `bind_*_view()` 里重新绑定当前数据

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_tree.h`
