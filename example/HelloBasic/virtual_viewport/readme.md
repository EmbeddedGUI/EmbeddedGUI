# Virtual Viewport / Virtual List / Virtual Page

## 这个示例在展示什么

这不是一个单一的大列表例程，而是一页多场景演示，用同一套虚拟化底座覆盖三类常见业务：

- `Feed`
  - 1000+ 条动态卡片
  - 不同 item 高度
  - 点击选中、局部 patch、插入、移动
  - 脉冲动画
- `Chat`
  - 左右气泡和系统消息混排
  - unread / typing 等状态
  - 跳转到指定消息
- `Ops`
  - 任务队列 / 工单流
  - 不同状态、不同尺寸、不同视觉样式
  - 删除、刷新、滚动后继续复用

顶部 `Feed / Chat / Ops` 用来切场景，`Add / Del / Move / Patch / Jump` 用来模拟真实数据变更。

## 这套能力解决什么问题

- 子 view 只在需要渲染时创建和绑定
- 活跃槽位数量被限制在 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS`
- 支持 1000+ item
- 支持 item 尺寸变化、插入、删除、移动
- 支持点击并确认到底是哪个 item 被点击
- 支持 `stable_id` 定位、滚动和刷新
- 支持动画/临时状态在回收后恢复，而不只依赖 keepalive

## 三层 API 怎么选

### 1. 普通纵向列表优先用 `virtual_list`

`virtual_list` 面向“行列表”，适合 feed、chat、timeline、task list。

最少只需要实现：

- `get_count`
- `create_item_view`
- `bind_item_view`

常用可选能力：

- `get_stable_id`
- `find_index_by_stable_id`
- `measure_item_height`
- `unbind_item_view`
- `should_keep_alive`
- `save_item_state`
- `restore_item_state`

常用便捷接口：

- `egui_view_virtual_list_find_index_by_stable_id()`
- `egui_view_virtual_list_scroll_to_stable_id()`
- `egui_view_virtual_list_notify_item_changed_by_stable_id()`
- `egui_view_virtual_list_notify_item_resized_by_stable_id()`

状态袋接口：

- `egui_view_virtual_list_set_state_cache_limits()`
- `egui_view_virtual_list_write_item_state()`
- `egui_view_virtual_list_read_item_state()`
- `egui_view_virtual_list_write_item_state_for_view()`
- `egui_view_virtual_list_read_item_state_for_view()`
- `egui_view_virtual_list_remove_item_state_by_stable_id()`
- `egui_view_virtual_list_clear_item_state_cache()`

默认行为：

- 不实现 `get_view_type` 时，默认 `view_type = 0`
- 不实现 `measure_item_height` 时，默认返回 `estimated_item_height`
- 不实现 `find_index_by_stable_id` 但实现了 `get_stable_id` 时，框架会自动线性查找
- 两者都不实现时，默认 `stable_id = index + 1`

```c
static egui_view_virtual_list_t my_list;

static const egui_view_virtual_list_data_source_t my_data_source = {
        .get_count = my_get_count,
        .get_stable_id = my_get_stable_id,
        .measure_item_height = my_measure_item_height,
        .create_item_view = my_create_item_view,
        .bind_item_view = my_bind_item_view,
        .save_item_state = my_save_item_state,
        .restore_item_state = my_restore_item_state,
        .should_keep_alive = my_should_keep_alive,
};

EGUI_VIEW_VIRTUAL_LIST_PARAMS_INIT(my_list_params, 8, 40, 224, 272);

egui_view_virtual_list_init_with_params(EGUI_VIEW_OF(&my_list), &my_list_params);
egui_view_virtual_list_set_data_source(EGUI_VIEW_OF(&my_list), &my_data_source, &my_context);
egui_view_virtual_list_set_estimated_item_height(EGUI_VIEW_OF(&my_list), 72);
egui_view_virtual_list_set_keepalive_limit(EGUI_VIEW_OF(&my_list), 4);
egui_view_virtual_list_set_state_cache_limits(EGUI_VIEW_OF(&my_list), 96, 96 * sizeof(my_item_state_t));
egui_core_add_user_root_view(EGUI_VIEW_OF(&my_list));
```

### 2. 长页面 / section 容器优先用 `virtual_page`

如果你的业务更像“一个页面里有很多 section”，直接用 `virtual_page` 更顺手。

适合场景：

- 首页聚合页
- dashboard
- 设置页
- 表单页
- 详情页里的大 section

对应的 `stable_id` 便捷接口：

- `egui_view_virtual_page_find_section_index_by_stable_id()`
- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_notify_section_changed_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`

对应的状态袋接口：

- `egui_view_virtual_page_set_state_cache_limits()`
- `egui_view_virtual_page_write_section_state()`
- `egui_view_virtual_page_read_section_state()`
- `egui_view_virtual_page_write_section_state_for_view()`
- `egui_view_virtual_page_read_section_state_for_view()`
- `egui_view_virtual_page_remove_section_state_by_stable_id()`
- `egui_view_virtual_page_clear_section_state_cache()`

```c
static egui_view_virtual_page_t dashboard_page;

static const egui_view_virtual_page_data_source_t dashboard_sections = {
        .get_count = dashboard_get_section_count,
        .get_stable_id = dashboard_get_section_id,
        .measure_section_height = dashboard_measure_section_height,
        .create_section_view = dashboard_create_section_view,
        .bind_section_view = dashboard_bind_section_view,
        .save_section_state = dashboard_save_section_state,
        .restore_section_state = dashboard_restore_section_state,
};

EGUI_VIEW_VIRTUAL_PAGE_PARAMS_INIT(dashboard_page_params, 8, 8, 224, 304);

egui_view_virtual_page_init_with_params(EGUI_VIEW_OF(&dashboard_page), &dashboard_page_params);
egui_view_virtual_page_set_data_source(EGUI_VIEW_OF(&dashboard_page), &dashboard_sections, &dashboard_ctx);
egui_view_virtual_page_set_estimated_section_height(EGUI_VIEW_OF(&dashboard_page), 96);
egui_view_virtual_page_set_state_cache_limits(EGUI_VIEW_OF(&dashboard_page), 48, 48 * sizeof(dashboard_section_state_t));
egui_core_add_user_root_view(EGUI_VIEW_OF(&dashboard_page));
```

### 3. 只有在更底层场景下再直接用 `virtual_viewport`

适合：

- 需要横向主轴
- 需要直接操作 adapter
- 需要完全自定义生命周期桥接

## 一个关键原则：只创建“槽位 view”

不要给每个数据项都真实创建一个 view。

正确做法是：

- 最多只创建 `EGUI_VIEW_VIRTUAL_VIEWPORT_MAX_SLOTS` 个槽位 view
- 滚动时复用这些 view
- 在 `bind_item_view` / `bind_section_view` 中重新绑定当前数据

## 高度变化后要显式通知

如果 item 高度会变化，业务状态更新后要显式通知虚拟容器重新计算锚点和布局。

列表场景：

```c
egui_view_virtual_list_notify_item_resized(EGUI_VIEW_OF(&my_list), index);
egui_view_virtual_list_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&my_list), stable_id);
```

页面场景：

```c
egui_view_virtual_page_notify_section_resized(EGUI_VIEW_OF(&dashboard_page), index);
egui_view_virtual_page_notify_section_resized_by_stable_id(EGUI_VIEW_OF(&dashboard_page), stable_id);
```

## keepalive 和状态袋怎么配合

这是这套架构里最关键的部分。

### keepalive 适合什么

`should_keep_alive` 适合“这个 item 暂时绝对不能销毁”的情况：

- 正在拖拽
- 正在编辑
- 焦点还在 item 内
- 动画短时间内必须保持同一个 view 实例

优点：

- 最简单
- 不需要序列化状态
- 回来时就是同一个 view 实例

缺点：

- 占槽位
- 不能无限保留

### 状态袋适合什么

状态袋适合“实例可以回收，但希望状态能回来”的情况：

- 动画进度
- 展开/折叠进度
- 临时高亮
- 局部交互态
- 自定义缓存字段

使用方式：

1. 给 item 提供稳定的 `stable_id`
2. 配置状态袋上限
3. 在 `save_item_state` / `restore_item_state` 中读写 blob

```c
typedef struct my_item_state
{
    uint16_t anim_elapsed_ms;
    uint8_t expanded;
    uint8_t alpha;
} my_item_state_t;

static void my_save_item_state(void *ctx, egui_view_t *item_view, uint32_t stable_id)
{
    my_item_state_t state;

    state.anim_elapsed_ms = calc_elapsed_ms(item_view);
    state.expanded = calc_expanded(item_view);
    state.alpha = item_view->alpha;

    (void)egui_view_virtual_list_write_item_state_for_view(item_view, stable_id, &state, sizeof(state));
}

static void my_restore_item_state(void *ctx, egui_view_t *item_view, uint32_t stable_id)
{
    my_item_state_t state;

    if (egui_view_virtual_list_read_item_state_for_view(item_view, stable_id, &state, sizeof(state)) != sizeof(state))
    {
        return;
    }

    restore_anim_elapsed(item_view, state.anim_elapsed_ms);
    restore_expanded(item_view, state.expanded);
    egui_view_set_alpha(item_view, state.alpha);
}
```

### 什么时候两个都要用

建议策略：

- 短时间强一致：用 keepalive
- 长时间离屏恢复：用状态袋

本示例里：

- 选中 item 会 keepalive
- `Feed` 里的 live / warn 行会 keepalive
- `Chat` 里的 typing 行会 keepalive
- `Ops` 里的 running / blocked 行会 keepalive
- 同时，行内 `pulse` 动画还通过状态袋保存进度，避免回收后总是从头开始

## 这个 demo 里覆盖了哪些能力

- 大数量 item 虚拟化
- 不同高度和不同样式 item
- 点击选中和选中态刷新
- `stable_id` 定位与跳转
- 插入 / 删除 / 移动 / patch
- keepalive
- 动画状态恢复

## 相关文件

- `src/widget/egui_view_virtual_viewport.h`
- `src/widget/egui_view_virtual_viewport.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_list.c`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_page.c`
- `example/HelloBasic/virtual_viewport/test.c`
