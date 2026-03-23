# Virtual Grid 示例

## 先看哪个例子

如果你是第一次接 `virtual_grid`，建议先看 `example/HelloVirtual/virtual_grid_basic/`。

`virtual_grid_basic` 只保留：

- 一个 header
- 一个 action bar
- 一个纵向 grid
- 一组最小的 patch / cols / reset 流程

看懂最小 API 接法之后，再回来看当前这个复杂示例，会更容易理解 row slot、列切换、自适应高度和 richer grid 语义。

## 这个示例展示什么

这个示例用 `egui_view_virtual_grid_t` 演示“二维卡片流 / 宫格容器”的典型业务场景：

- 320 个 item 的长数据集，但只虚拟化当前可见 row
- 2 / 3 / 4 列切换，item 宽度变化后高度会重新测量
- item 点击选中后可确认命中的 `stable_id / index`
- `Add / Del / Patch / Jump` 模拟内容增删改和定位
- 脉冲动画结合 `keepalive + item state cache`

适合的场景：

- 商品宫格
- 卡片流页面
- 文件或相册缩略图墙
- 仪表板卡片面板

不太适合的场景：

- 有明显组头和组内关系：优先 `virtual_section_list`
- 有父子层级：优先 `virtual_tree`
- 主轴明确是横向 rail：优先 `virtual_strip`

## 运行方式

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_grid PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_grid --keep-screenshots
```

## 推荐接入方式

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

static const egui_view_virtual_grid_data_source_t card_grid_ds = {
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

static const egui_view_virtual_grid_setup_t card_grid_setup = {
        .params = &card_grid_params,
        .data_source = &card_grid_ds,
        .data_source_context = &grid_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_grid_init_with_setup(EGUI_VIEW_OF(&card_grid), &card_grid_setup);
```

如果后续要整体切换列数、数据源或缓存限额，可以继续使用：

```c
egui_view_virtual_grid_apply_setup(EGUI_VIEW_OF(&card_grid), &card_grid_setup);
```

## 关键设计点

### 1. 底层是“按 row 虚拟化”

`virtual_grid` 不是直接把底层改成二维，而是：

- 先把 item 按列数分组为 row
- 只虚拟化可见 row
- row 内再复用多个 cell child

这样可以继承底层现有能力：

- 可变高度测量
- 锚点滚动稳定
- `keepalive`
- `state cache`

### 2. item 高度可以依赖当前宽度

`measure_item_height()` 会收到当前 cell 的 `width_hint`，因此可以按列数切换不同布局。如果 item 高度变了，要主动通知：

```c
egui_view_virtual_grid_notify_item_resized(EGUI_VIEW_OF(&card_grid), index);
```

或者：

```c
egui_view_virtual_grid_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&card_grid), stable_id);
```

### 3. 点击后可以直接反查命中的 item

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

### 4. row slot 和 cell helper

`virtual_grid` 的 slot 语义是 row slot，不是 item slot。做录制动作、自动化点击或遍历当前可见卡片时，建议直接遍历 row slot 和 cell：

```c
static uint8_t match_card_after_index(egui_view_t *self, const egui_view_virtual_grid_slot_t *slot, const egui_view_virtual_grid_entry_t *entry,
                                      egui_view_t *item_view, void *context)
{
    uint32_t min_index = *(uint32_t *)context;

    EGUI_UNUSED(self);
    EGUI_UNUSED(slot);
    EGUI_UNUSED(item_view);
    return entry != NULL && entry->index >= min_index;
}

uint32_t min_index = 6;
egui_view_t *target_view =
        egui_view_virtual_grid_find_first_visible_item_view(EGUI_VIEW_OF(&card_grid), match_card_after_index, &min_index, NULL);
if (target_view != NULL)
{
    simulate_click(target_view);
}
```

如果当前选中卡片、焦点卡片或动画卡片需要“尽量保持可见”，优先用：

```c
egui_view_virtual_grid_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&card_grid), stable_id, inset);
```

这样只有目标离开当前可见安全带时才会滚动，不需要业务层重复手写 `get_item_y() + scroll_to_*()` 判断。

## 常用 helper

- `egui_view_virtual_grid_get_item_count()`
- `egui_view_virtual_grid_get_row_count()`
- `egui_view_virtual_grid_find_index_by_stable_id()`
- `egui_view_virtual_grid_resolve_item_by_stable_id()`
- `egui_view_virtual_grid_resolve_item_by_view()`
- `egui_view_virtual_grid_find_view_by_stable_id()`
- `egui_view_virtual_grid_find_first_visible_item_view()`
- `egui_view_virtual_grid_get_item_x_by_stable_id()`
- `egui_view_virtual_grid_get_item_y_by_stable_id()`
- `egui_view_virtual_grid_get_item_width_by_stable_id()`
- `egui_view_virtual_grid_get_item_height_by_stable_id()`
- `egui_view_virtual_grid_scroll_to_item_by_stable_id()`
- `egui_view_virtual_grid_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_grid_notify_item_changed_by_stable_id()`
- `egui_view_virtual_grid_notify_item_resized_by_stable_id()`
- `egui_view_virtual_grid_visit_visible_items()`
- `egui_view_virtual_grid_get_slot_count()`
- `egui_view_virtual_grid_get_slot()`
- `egui_view_virtual_grid_find_slot_by_stable_id()`
- `egui_view_virtual_grid_get_slot_item_count()`
- `egui_view_virtual_grid_get_slot_entry()`
- `egui_view_virtual_grid_get_slot_item_view()`

## 相关文件

- `example/HelloVirtual/virtual_grid/test.c`
- `src/widget/egui_view_virtual_grid.h`
- `src/widget/egui_view_virtual_grid.c`
- `src/widget/egui_view_virtual_viewport.h`
