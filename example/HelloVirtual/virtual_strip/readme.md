# Virtual Strip 示例

## 先看哪个例子

如果你是第一次接 `virtual_strip`，建议先看 `example/HelloVirtual/virtual_strip_basic/`。

`virtual_strip_basic` 只保留：

- 一个 header
- 一个 action bar
- 一个横向 strip
- 一组最小的 patch / jump / reset 流程

看懂最小 API 接法之后，再回来看当前这个复杂示例，会更容易理解 scene 切换、keepalive、state cache 和 richer 轨道语义。

## 这个示例解决什么问题

`egui_view_virtual_strip_t` 用来做“横向虚拟化 item 带”，不是把普通列表横过来那么简单。

这个示例放了 3 种典型场景：

- `Gallery`：poster rail / hero rail
- `Queue`：播放队列 / 工单队列
- `Timeline`：beat marker / cue strip

它重点演示：

- item 在 X 轴滚动时按需创建和复用
- item 宽度可以变化
- 不同 scene 共用同一套虚拟化容器
- 点击后可确认命中的 `index / stable_id`
- `Add / Del / Patch / Jump` 会触发真实的数据变化和定位
- pulse 动画通过 `keepalive + state cache` 保住
- `Timeline` 额外用固定 playhead 叠层强调“轨道参考线”语义，而不是普通横向卡片排布

## 什么时候用 `virtual_strip`

适合这些业务：

- 首页横向推荐带
- 图片 / 视频 gallery
- 播放队列 / 候选队列
- 时间轴 cue strip

如果你的主轴是横向，而且每个 item 的宽度可能不同，就应该优先考虑 `virtual_strip`。

## 为什么它不应该长得像“横过来的 list”

- `Gallery` 应该先读成 poster rail / hero rail，强调卡片宽度变化和轨道节奏。
- `Queue` 应该更像一条连续的工作/播放队列，强调顺序、状态和局部 patch。
- `Timeline` 应该先被看成 cue strip / beat marker，而不是一排矮卡片。

也就是说，`virtual_strip` 的最小业务单元是横向轨道里的 item，不是把 `virtual_list` 的 row 旋转 90 度。

## 不太适合这些场景

- 主轴不是横向：优先 `virtual_list` / `virtual_page`
- 有明显组头和组内条目：优先 `virtual_section_list`
- 有递归父子层级：优先 `virtual_tree`
- 本质上是一屏二维卡片流：优先 `virtual_grid`

## 运行方式

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_strip PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_strip --keep-screenshots
```

## 接入方式

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

已经初始化过的容器可以继续用：

```c
egui_view_virtual_strip_apply_setup(EGUI_VIEW_OF(&strip_view), &strip_setup);
```

## 宽度变化和定位

如果 item 的选中态、文案、样式变化会影响宽度，必须通知容器：

```c
egui_view_virtual_strip_notify_item_resized_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id);
```

如果只是希望当前 item 保持在屏幕内，不要自己重复写“先判断再滚动”的逻辑，直接用：

```c
egui_view_virtual_strip_ensure_item_visible_by_stable_id(EGUI_VIEW_OF(&strip_view), stable_id, inset);
```

## 点击命中

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

## 常用 helper

- `egui_view_virtual_strip_resolve_item_by_view()`
- `egui_view_virtual_strip_find_view_by_stable_id()`
- `egui_view_virtual_strip_find_first_visible_item_view()`
- `egui_view_virtual_strip_get_item_x_by_stable_id()`
- `egui_view_virtual_strip_get_item_width_by_stable_id()`
- `egui_view_virtual_strip_scroll_to_stable_id()`
- `egui_view_virtual_strip_ensure_item_visible_by_stable_id()`
- `egui_view_virtual_strip_visit_visible_items()`

## 设计提醒

- 窄 item 不要硬塞固定长标题，应该按可用宽度降级文案或隐藏次要信息。
- 状态表达不一定总要靠 badge；当 accent、pulse、边框已经足够时，badge 可以退化成只在 focus/selected 时出现。
- 如果不同 scene 的底部信息密度差异很大，就应该拆成不同版式，而不是只换颜色继续共用一套“列表行”布局。
- 对 `Timeline` 这类场景，可以增加固定 playhead、节拍线或参考点，让用户先读出“轨道”，再读出 item。

## 相关文件

- `example/HelloVirtual/virtual_strip/test.c`
- `src/widget/egui_view_virtual_strip.h`
- `src/widget/egui_view_virtual_strip.c`
- `src/widget/egui_view_virtual_viewport.h`
