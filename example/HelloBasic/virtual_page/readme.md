# Virtual Page 示例

## 这个示例展示什么

这个示例用 `egui_view_virtual_page_t` 演示“长页面 / 大 section 容器”的典型业务场景：

- 280 个 section 的长页面
- section 高度和视觉样式可变
- 点击 section 后确认命中的 `index / stable_id`
- `Add / Del / Patch / Jump` 模拟内容增删改和定位
- 脉冲动画结合 `keepalive + state cache`

适合的业务场景：

- dashboard
- 设置页
- 多 section 表单页
- 详情页中的大块内容流

## 运行方式

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_page PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_page --keep-screenshots
```

## 推荐接入方式

现在推荐直接用 `setup` 把初始化信息一次性带齐：

```c
static egui_view_virtual_page_t dashboard_page;
static app_context_t dashboard_ctx;

static const egui_view_virtual_page_params_t dashboard_page_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_section_height = 90,
};

static const egui_view_virtual_page_data_source_t dashboard_sections = {
        .get_count = dashboard_get_count,
        .get_stable_id = dashboard_get_stable_id,
        .find_index_by_stable_id = dashboard_find_index,
        .measure_section_height = dashboard_measure_height,
        .create_section_view = dashboard_create_view,
        .bind_section_view = dashboard_bind_view,
        .unbind_section_view = dashboard_unbind_view,
        .should_keep_alive = dashboard_should_keep_alive,
        .save_section_state = dashboard_save_state,
        .restore_section_state = dashboard_restore_state,
};

static const egui_view_virtual_page_setup_t dashboard_page_setup = {
        .params = &dashboard_page_params,
        .data_source = &dashboard_sections,
        .data_source_context = &dashboard_ctx,
        .state_cache_max_entries = 64,
        .state_cache_max_bytes = 64 * sizeof(my_section_state_t),
};

egui_view_virtual_page_init_with_setup(EGUI_VIEW_OF(&dashboard_page), &dashboard_page_setup);
```

如果容器已经初始化完成，后续要替换 `params`、数据源或缓存限额，可以继续使用：

```c
egui_view_virtual_page_apply_setup(EGUI_VIEW_OF(&dashboard_page), &dashboard_page_setup);
```

## 点击命中和可见 section 遍历

如果 section 根 view 自己就接点击事件，可以直接反查命中的 section：

```c
static void dashboard_click_cb(egui_view_t *self)
{
    egui_view_virtual_page_entry_t entry;

    if (!egui_view_virtual_page_resolve_section_by_view(EGUI_VIEW_OF(&dashboard_page), self, &entry))
    {
        return;
    }

    focus_section(entry.index, entry.stable_id);
    egui_view_virtual_page_notify_section_changed(EGUI_VIEW_OF(&dashboard_page), entry.index);
}
```

做录制动作或自动化点击时，也可以直接扫描当前可见 slot：

```c
static uint8_t match_clickable_section(egui_view_t *self, const egui_view_virtual_page_slot_t *slot, const egui_view_virtual_page_entry_t *entry,
                                       egui_view_t *section_view, void *context)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(entry);
    EGUI_UNUSED(section_view);
    EGUI_UNUSED(context);
    return slot != NULL;
}

egui_view_t *target_view =
        egui_view_virtual_page_find_first_visible_section_view(EGUI_VIEW_OF(&dashboard_page), match_clickable_section, NULL, NULL);
if (target_view != NULL)
{
    simulate_click(target_view);
}
```

## 使用建议

### 1. section 要有稳定 `stable_id`

这样才能安全使用：

- `egui_view_virtual_page_scroll_to_section_by_stable_id()`
- `egui_view_virtual_page_ensure_section_visible_by_stable_id()`
- `egui_view_virtual_page_notify_section_resized_by_stable_id()`
- `egui_view_virtual_page_write_section_state()`
- `egui_view_virtual_page_read_section_state()`

### 2. 高度变化时主动通知

如果点击后 section 会展开、折叠或切样式，业务层要显式通知：

```c
egui_view_virtual_page_notify_section_resized(EGUI_VIEW_OF(&dashboard_page), index);
```

如果只是希望当前 section 保持在可见安全带里，而不是每次都强制跳转到固定位置，优先用：

```c
egui_view_virtual_page_ensure_section_visible_by_stable_id(EGUI_VIEW_OF(&dashboard_page), stable_id, inset);
```

### 3. `keepalive` 和状态缓存分工不同

- `keepalive` 适合短时间内必须保留同一个 view 实例的 section
- `state cache` 适合允许回收，但离屏后还要恢复动画或临时态的 section

### 4. 常用 helper

- `egui_view_virtual_page_get_section_count()`
- `egui_view_virtual_page_resolve_section_by_stable_id()`
- `egui_view_virtual_page_resolve_section_by_view()`
- `egui_view_virtual_page_find_view_by_stable_id()`
- `egui_view_virtual_page_find_first_visible_section_view()`
- `egui_view_virtual_page_get_section_y_by_stable_id()`
- `egui_view_virtual_page_get_section_height_by_stable_id()`
- `egui_view_virtual_page_ensure_section_visible_by_stable_id()`
- `egui_view_virtual_page_visit_visible_sections()`
- `egui_view_virtual_page_get_slot_count()`
- `egui_view_virtual_page_get_slot()`
- `egui_view_virtual_page_get_slot_entry()`

## 和 `virtual_list` 的区别

- `virtual_list` 更适合很多行
- `virtual_page` 更适合很多大 section
- 底层都复用 `virtual_viewport`

当你的业务语义已经是 page、dashboard 或 settings 时，直接用 `virtual_page` 会比在 `virtual_list` 上自己再封一层更顺手。

## 相关文件

- `example/HelloBasic/virtual_page/test.c`
- `src/widget/egui_view_virtual_page.h`
- `src/widget/egui_view_virtual_page.c`
- `src/widget/egui_view_virtual_viewport.h`
