# Virtual Strip 示例

## 这个示例展示什么

这个示例用 `egui_view_virtual_strip_t` 演示“横向虚拟化容器 / card rail”的典型业务场景：

- 260 / 180 / 320 个 item 的长横向数据集
- `Gallery / Queue / Timeline` 三种场景共用同一个横向虚拟容器
- item 宽度可变，点击后可确认命中的 `stable_id / index`
- `Add / Del / Patch / Jump` 模拟真实数据增删改和定位
- 横向滚动、局部刷新、离屏回收与复用
- 脉冲动画结合 `keepalive + state cache`

适合的业务场景：

- gallery 或 hero rail
- 音乐或视频播放队列
- 横向 timeline 或 cue strip
- 首页横向推荐带

## 运行方式

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_strip PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_strip --keep-screenshots
```

## 推荐接入方式

```c
static egui_view_virtual_strip_t strip_view;
static app_context_t strip_ctx;

static const egui_view_virtual_strip_params_t strip_params = {
        .region = {{8, 72}, {224, 240}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 6,
        .estimated_item_width = 88,
};

static const egui_view_virtual_strip_data_source_t strip_source = {
        .get_count = strip_get_count,
        .get_stable_id = strip_get_stable_id,
        .find_index_by_stable_id = strip_find_index,
        .measure_item_width = strip_measure_width,
        .create_item_view = strip_create_view,
        .bind_item_view = strip_bind_view,
        .unbind_item_view = strip_unbind_view,
        .should_keep_alive = strip_should_keep_alive,
        .save_item_state = strip_save_item_state,
        .restore_item_state = strip_restore_item_state,
};

static const egui_view_virtual_strip_setup_t strip_setup = {
        .params = &strip_params,
        .data_source = &strip_source,
        .data_source_context = &strip_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_strip_init_with_setup(EGUI_VIEW_OF(&strip_view), &strip_setup);
```

容器初始化完成后，如果要整体切换 `params`、数据源或缓存限额，可以继续使用：

```c
egui_view_virtual_strip_apply_setup(EGUI_VIEW_OF(&strip_view), &strip_setup);
```

## 关键设计点

### 1. `virtual_strip` 是横向 item 虚拟化

它不是把 `virtual_list` 简单改成横向滚动，而是把底层 `virtual_viewport` 的主轴切到 X：

- item 在 X 轴排布
- 数据源提供 `measure_item_width()`
- 容器维护有限数量的 slot view
- 滚动时只重绑当前进入可见区的 item

### 2. 点击后可以直接反查命中的 item

```c
static void strip_click_cb(egui_view_t *self)
{
    egui_view_virtual_strip_entry_t entry;

    if (!egui_view_virtual_strip_resolve_item_by_view(EGUI_VIEW_OF(&strip_view), self, &entry))
    {
        return;
    }

    select_item(entry.index, entry.stable_id);
}
```

### 3. 宽度变化后要显式通知

如果业务修改了 variant、badge、选中态或其它会影响宽度的状态，需要显式通知：

```c
egui_view_virtual_strip_notify_item_resized(EGUI_VIEW_OF(&strip_view), index);
egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
```

如果只是内容刷新但宽度不变，则用：

```c
egui_view_virtual_strip_notify_item_changed(EGUI_VIEW_OF(&strip_view), index);
```

### 4. 按 `stable_id` 定位

横向带的常见需求不是“滚到某个 offset”，而是跳到某个 item。典型写法：

```c
egui_view_virtual_strip_scroll_to_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, inset);
```

如果只是希望“选中项、焦点项、正在播的 item”保持在可见安全带内，不要每次都强制跳转，直接用：

```c
egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, inset);
```

目标已经处于当前可见安全带时不会重复滚动，适合点击选中、键盘移动和动画播放中的跟随定位。

## 常用 helper

- `egui_view_virtual_strip_get_item_count()`
- `egui_view_virtual_strip_find_index_by_stable_id()`
- `egui_view_virtual_strip_resolve_item_by_stable_id()`
- `egui_view_virtual_strip_resolve_item_by_view()`
- `egui_view_virtual_strip_find_view_by_stable_id()`
- `egui_view_virtual_strip_find_first_visible_item_view()`
- `egui_view_virtual_strip_get_item_x_by_stable_id()`
- `egui_view_virtual_strip_get_item_width_by_stable_id()`
- `egui_view_virtual_strip_scroll_to_item()`
- `egui_view_virtual_strip_scroll_to_stable_id()`
- `egui_view_virtual_strip_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_strip_notify_item_changed_by_stable_id()`
- `egui_view_virtual_strip_notify_item_resized_by_stable_id()`
- `egui_view_virtual_strip_visit_visible_items()`
- `egui_view_virtual_strip_get_slot_count()`
- `egui_view_virtual_strip_get_slot()`
- `egui_view_virtual_strip_find_slot_by_stable_id()`
- `egui_view_virtual_strip_get_slot_entry()`

## 和其它封装的区别

- `virtual_list` 适合纵向很多 row
- `virtual_page` 适合纵向很多大 section
- `virtual_strip` 适合横向很多 item
- `virtual_grid` 适合二维卡片面板

如果你的业务天然就是“一条横向带，卡片尺寸还会变”，直接用 `virtual_strip` 会比在 `virtual_viewport` 上再手写一层更顺手。

## 相关文件

- `example/HelloBasic/virtual_strip/test.c`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_strip.c`
- `src/widget/egui_view_virtual_viewport.h`
