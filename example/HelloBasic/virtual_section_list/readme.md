# Virtual Section List 示例

## 这个示例展示什么

这个示例用 `egui_view_virtual_section_list_t` 演示“分组列表 / section header + row 混排”的典型业务场景：

- 24 个 section，默认总量超过 1000 个 row
- section header 和 item 使用不同生命周期回调
- header 点击折叠或展开
- item 点击选中后可确认命中的 section、row 和 `stable_id`
- `Add / Del / Patch / Jump` 模拟真实数据变更
- 行内脉冲动画结合 `keepalive + state cache`

适合的场景：

- 分组消息列表
- 工单或任务分桶
- 时间线按天分组
- 设置页中的一级分组和二级条目

## 运行方式

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_section_list PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_section_list --keep-screenshots
```

## 推荐接入方式

```c
static egui_view_virtual_section_list_t grouped_list;
static app_context_t grouped_ctx;

static const egui_view_virtual_section_list_params_t grouped_list_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_entry_height = 64,
};

static const egui_view_virtual_section_list_data_source_t grouped_source = {
        .get_section_count = grouped_get_section_count,
        .get_section_stable_id = grouped_get_section_stable_id,
        .find_section_index_by_stable_id = grouped_find_section_index,
        .get_item_count = grouped_get_item_count,
        .get_item_stable_id = grouped_get_item_stable_id,
        .find_item_position_by_stable_id = grouped_find_item_position,
        .measure_section_header_height = grouped_measure_header_height,
        .measure_item_height = grouped_measure_item_height,
        .create_section_header_view = grouped_create_header_view,
        .create_item_view = grouped_create_item_view,
        .bind_section_header_view = grouped_bind_header_view,
        .bind_item_view = grouped_bind_item_view,
        .unbind_item_view = grouped_unbind_item_view,
        .should_keep_item_alive = grouped_should_keep_item_alive,
        .save_item_state = grouped_save_item_state,
        .restore_item_state = grouped_restore_item_state,
};

static const egui_view_virtual_section_list_setup_t grouped_list_setup = {
        .params = &grouped_list_params,
        .data_source = &grouped_source,
        .data_source_context = &grouped_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_section_list_init_with_setup(EGUI_VIEW_OF(&grouped_list), &grouped_list_setup);
```

容器初始化后，如果想整体切换 `params`、数据源或缓存限额，可以继续调用：

```c
egui_view_virtual_section_list_apply_setup(EGUI_VIEW_OF(&grouped_list), &grouped_list_setup);
```

## 点击命中和可见项遍历

如果 header 或 item 根 view 自己就接点击事件，可以直接通过 entry helper 反查 section 或 item：

```c
static void grouped_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;

    if (!egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&grouped_list), self, &entry))
    {
        return;
    }

    if (entry.is_section_header)
    {
        toggle_section(entry.section_index);
        egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&grouped_list));
        return;
    }

    select_item(entry.section_index, entry.item_index, entry.stable_id);
    egui_view_virtual_section_list_notify_item_changed(EGUI_VIEW_OF(&grouped_list), entry.section_index, entry.item_index);
}
```

做录制、自动化点击或可见项扫描时，也可以直接遍历当前 slot：

```c
static uint8_t click_first_visible_item(egui_view_t *self, const egui_view_virtual_section_list_slot_t *slot,
                                        const egui_view_virtual_section_list_entry_t *entry, egui_view_t *entry_view, void *context)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);

    if (entry->is_section_header || *(egui_view_t **)context != NULL)
    {
        return 1;
    }

    EGUI_UNUSED(entry_view);
    EGUI_UNUSED(context);
    return entry != NULL && !entry->is_section_header;
}

egui_view_t *target_view =
        egui_view_virtual_section_list_find_first_visible_entry_view(EGUI_VIEW_OF(&grouped_list), click_first_visible_item, NULL, NULL);
if (target_view != NULL)
{
    simulate_click(target_view);
}
```

## 数据源设计要点

### 1. `stable_id` 必须全局唯一

`virtual_section_list` 会把 header 和 item 都拍平成底层列表，所以：

- section header 的 `stable_id`
- item 的 `stable_id`

都必须在整个 section list 内全局唯一，不能只在单个 section 内唯一。

### 2. 结构变化优先用 `notify_data_changed()`

以下变化都会影响拍平后的 entry 数量：

- header 折叠或展开
- section 的 item 数量变化
- section 顺序调整

这种情况建议直接调用：

```c
egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&grouped_list));
```

### 3. 单个 header 或 item 变化用细粒度通知

```c
egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&grouped_list), section_index);
egui_view_virtual_section_list_notify_section_header_resized(EGUI_VIEW_OF(&grouped_list), section_index);
egui_view_virtual_section_list_notify_item_changed(EGUI_VIEW_OF(&grouped_list), section_index, item_index);
egui_view_virtual_section_list_notify_item_resized(EGUI_VIEW_OF(&grouped_list), section_index, item_index);
```

### 4. 常用 helper

- `egui_view_virtual_section_list_get_section_count()`
- `egui_view_virtual_section_list_get_item_count()`
- `egui_view_virtual_section_list_resolve_entry_by_stable_id()`
- `egui_view_virtual_section_list_resolve_entry_by_view()`
- `egui_view_virtual_section_list_find_view_by_stable_id()`
- `egui_view_virtual_section_list_find_first_visible_entry_view()`
- `egui_view_virtual_section_list_get_entry_y_by_stable_id()`
- `egui_view_virtual_section_list_get_entry_height_by_stable_id()`
- `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`
- `egui_view_virtual_section_list_visit_visible_entries()`
- `egui_view_virtual_section_list_get_slot_count()`
- `egui_view_virtual_section_list_get_slot()`
- `egui_view_virtual_section_list_get_slot_entry()`

如果业务里存在“点击 item 后展开更多内容”“键盘导航当前行”“播放中消息保持可见”这类需求，优先用 `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`，
让容器只在目标离开当前可见安全带时才滚动。

## 相关文件

- `example/HelloBasic/virtual_section_list/test.c`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_section_list.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_viewport.h`
