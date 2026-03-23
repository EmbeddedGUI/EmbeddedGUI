# Virtual Section List 示例

## 这个示例解决什么问题

`egui_view_virtual_section_list_t` 适合“分组 header + 组内 row”混排的场景。

这个示例把 section list 做成了 4 种分组板：

- `Inbox`
- `Ops`
- `Chat`
- `Audit`

它重点演示：

- section header 和 item 使用不同的创建 / 绑定回调
- header 可折叠 / 展开
- item 只在进入可见区域时创建
- 总 row 数超过 1000 时仍然只维护有限 slot
- 点击 row 后可以确认命中的 `section / item / stable_id`
- 组内 pulse 动画通过 `keepalive + state cache` 保住
- 组头用 banner 语义，组内 row 用不同宽度和信息密度表达 `compact / detail / alert`

## 推荐阅读顺序

如果是第一次接 `virtual_section_list`，建议先看 `example/HelloVirtual/virtual_section_list_basic/`。
那个入门 case 只保留 header 折叠、item 选中、Patch、Jump、Reset 这五个最小闭环，更适合先理解 section data source 和 entry 回查。

读完入门示例再看当前这个 `virtual_section_list`，就会更容易理解：

- insert / remove / move 这类结构变更
- `keepalive + state cache` 如何保住 pulse 动画
- 更多 row variant 和更大的 grouped data scale

## 什么时候用 `virtual_section_list`

适合这些业务：

- 分组消息列表
- 工单按状态分组
- 聊天会话按房间分组
- 审计记录按批次分组
- 设置页里的一级组 + 二级条目

如果你的业务天然有“组头”和“组内行”，就应该直接用 `virtual_section_list`，而不是自己在 `virtual_list` 上再拼 header 行。

## 为什么它不应该长得像普通 list

- `virtual_section_list` 应该先让人看到组头，再看到组内条目。
- 组头和组内 row 应该承担不同语义：组头负责分组、折叠和摘要，组内 row 负责局部状态和点击命中。
- 如果 header 和 row 看起来几乎一样，业务上的“分组”就会退化成只是插了几行标题文本。

也就是说，这个容器的最小业务结构不是单个 row，而是 `section header + grouped row`。

## 不太适合这些场景

- 只是普通单层列表：优先 `virtual_list`
- 有递归父子层级和展开树枝：优先 `virtual_tree`
- 一页里是多个异构大模块：优先 `virtual_page`
- 主轴是横向轨道：优先 `virtual_strip`

## 运行方式

```bash
make -j1 all APP=HelloVirtual APP_SUB=virtual_section_list PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_section_list --keep-screenshots
```

## 接入方式

```c
static egui_view_virtual_section_list_t section_list;
static app_context_t section_ctx;

static const egui_view_virtual_section_list_params_t section_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_entry_height = 60,
};

static const egui_view_virtual_section_list_data_source_t section_source = {
        .get_section_count = app_get_section_count,
        .get_section_stable_id = app_get_section_stable_id,
        .find_section_index_by_stable_id = app_find_section_index,
        .get_item_count = app_get_item_count,
        .get_item_stable_id = app_get_item_stable_id,
        .find_item_position_by_stable_id = app_find_item_position,
        .measure_section_header_height = app_measure_header_height,
        .measure_item_height = app_measure_item_height,
        .create_section_header_view = app_create_header_view,
        .create_item_view = app_create_item_view,
        .bind_section_header_view = app_bind_header_view,
        .bind_item_view = app_bind_item_view,
        .unbind_item_view = app_unbind_item_view,
        .should_keep_item_alive = app_should_keep_item_alive,
        .save_item_state = app_save_item_state,
        .restore_item_state = app_restore_item_state,
};

static const egui_view_virtual_section_list_setup_t section_setup = {
        .params = &section_params,
        .data_source = &section_source,
        .data_source_context = &section_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_item_state_t),
};

egui_view_virtual_section_list_init_with_setup(EGUI_VIEW_OF(&section_list), &section_setup);
```

## 结构变化怎么通知

以下变化会影响拍平后的 entry 数量，统一使用：

```c
egui_view_virtual_section_list_notify_data_changed(EGUI_VIEW_OF(&section_list));
```

例如：

- header 折叠 / 展开
- 某个 section 的 item 数量变化
- section 顺序变化

如果只是某一个 header 或 item 的内容 / 高度变化，则使用更细粒度接口：

```c
egui_view_virtual_section_list_notify_section_header_changed(EGUI_VIEW_OF(&section_list), section_index);
egui_view_virtual_section_list_notify_section_header_resized(EGUI_VIEW_OF(&section_list), section_index);
egui_view_virtual_section_list_notify_item_changed(EGUI_VIEW_OF(&section_list), section_index, item_index);
egui_view_virtual_section_list_notify_item_resized(EGUI_VIEW_OF(&section_list), section_index, item_index);
```

## 点击命中

```c
static void section_click_cb(egui_view_t *self)
{
    egui_view_virtual_section_list_entry_t entry;

    if (!egui_view_virtual_section_list_resolve_entry_by_view(EGUI_VIEW_OF(&section_list), self, &entry))
    {
        return;
    }

    if (entry.is_section_header)
    {
        toggle_section(entry.section_index);
        return;
    }

    focus_item(entry.section_index, entry.item_index, entry.stable_id);
}
```

## 常用 helper

- `egui_view_virtual_section_list_resolve_entry_by_view()`
- `egui_view_virtual_section_list_find_view_by_stable_id()`
- `egui_view_virtual_section_list_find_first_visible_entry_view()`
- `egui_view_virtual_section_list_get_entry_y_by_stable_id()`
- `egui_view_virtual_section_list_get_entry_height_by_stable_id()`
- `egui_view_virtual_section_list_ensure_entry_visible_by_stable_id()`
- `egui_view_virtual_section_list_visit_visible_entries()`

## 设计提醒

- 可以用缩进、宽度节奏、色块层级去表达“组内关系”，不要让 header 和 row 共用同一套满宽视觉。
- header 更像 section banner，row 更像 lane 内部单元，用户要先读出“分组”，再读出“条目”。
- 折叠/展开影响的是可见结构映射，优先用 `notify_data_changed()`，不要只刷新某一个 row。
- `compact / detail / alert` 最好同时在高度、信息密度和状态表达上拉开差异，而不是只换背景色。

## 相关文件

- `example/HelloVirtual/virtual_section_list/test.c`
- `src/widget/egui_view_virtual_section_list.h`
- `src/widget/egui_view_virtual_section_list.c`
- `src/widget/egui_view_virtual_viewport.h`
