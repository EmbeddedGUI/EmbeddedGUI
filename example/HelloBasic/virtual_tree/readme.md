# Virtual Tree 示例

## 这个控件适合什么

`egui_view_virtual_tree_t` 用来承载天然具有父子层级的数据，而不是把树硬拍平成普通 list。

这个例程构造了 3 层结构：

- `root`
- `group`
- `task`

全部展开后节点总数超过 1000，但屏幕上仍然只维护有限 slot。

## 这个例程在展示什么

- `root / group / leaf` 使用不同的视觉语义，而不是同一种 row 只是缩进
- branch 节点支持折叠 / 展开
- 节点只在进入可视区时创建和绑定
- 点击后可以反查命中的 `visible_index / stable_id / depth`
- 局部 patch、选中、展开只触发必要刷新
- 热节点 pulse 动画通过 `keepalive + state cache` 保住
- 示例录制覆盖了点击、折叠、jump、滚动、再点击的过程

## 为什么它不该看起来像 list

这个例程故意把树的三个层次拉开：

- `root` 是顶层 hub，接近树冠 / 主干入口
- `group` 是 branch，重点是纵向主干和横向 elbow
- `task` 是 leaf，重点是叶子节点和局部状态，不再是大卡片

如果叶子节点也做成一张张满宽卡片，树很快就会退化成“看起来只是带缩进的 list”。

## 什么时候用 `virtual_tree`

适合：

- 文件树 / 目录树
- 设备拓扑
- 组织结构
- 可折叠任务树
- 分层导航

不太适合：

- 只是 group header + row，没有递归层级：优先 `virtual_section_list`
- 只是单层列表：优先 `virtual_list`
- 只是长页面模块：优先 `virtual_page`

## 运行方式

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_tree PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_tree --keep-screenshots
```

截图输出目录：

```bash
runtime_check_output/HelloBasic_virtual_tree/default/
```

## 接入方式

```c
static egui_view_virtual_tree_t tree_view;
static app_context_t tree_ctx;

static const egui_view_virtual_tree_params_t tree_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_node_height = 58,
};

static const egui_view_virtual_tree_data_source_t tree_source = {
        .get_root_count = app_get_root_count,
        .get_root_stable_id = app_get_root_stable_id,
        .get_child_count = app_get_child_count,
        .get_child_stable_id = app_get_child_stable_id,
        .is_node_expanded = app_is_node_expanded,
        .get_node_view_type = app_get_view_type,
        .measure_node_height = app_measure_node_height,
        .create_node_view = app_create_node_view,
        .bind_node_view = app_bind_node_view,
        .unbind_node_view = app_unbind_node_view,
        .should_keep_alive = app_should_keep_alive,
        .save_node_state = app_save_node_state,
        .restore_node_state = app_restore_node_state,
};

static const egui_view_virtual_tree_setup_t tree_setup = {
        .params = &tree_params,
        .data_source = &tree_source,
        .data_source_context = &tree_ctx,
        .state_cache_max_entries = 96,
        .state_cache_max_bytes = 96 * sizeof(my_node_state_t),
};

egui_view_virtual_tree_init_with_setup(EGUI_VIEW_OF(&tree_view), &tree_setup);
```

## 结构变化怎么通知

树结构整体变化时，直接用：

```c
egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
```

例如：

- 分支折叠 / 展开
- 父子关系变化
- 某个 branch 下的可见节点数量变化

如果只是单个节点内容或高度变化，则用：

```c
egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
```

## 点击命中

```c
static void tree_click_cb(egui_view_t *self)
{
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_node_by_view(EGUI_VIEW_OF(&tree_view), self, &entry))
    {
        return;
    }

    on_node_clicked(entry.visible_index, entry.stable_id, entry.depth, entry.has_children);
}
```

## 常用 helper

- `egui_view_virtual_tree_resolve_node_by_view()`
- `egui_view_virtual_tree_find_view_by_stable_id()`
- `egui_view_virtual_tree_find_first_visible_node_view()`
- `egui_view_virtual_tree_find_visible_index_by_stable_id()`
- `egui_view_virtual_tree_scroll_to_node_by_stable_id()`
- `egui_view_virtual_tree_ensure_node_visible_by_stable_id()`
- `egui_view_virtual_tree_visit_visible_nodes()`

## 设计提醒

- 节点测量逻辑要和层级语义一致，`root / group / leaf` 不必同高
- 树形差异最好通过主干、elbow、宽度和节点重量体现，不只是缩进
- 折叠 / 展开影响的是可见结构，优先使用 `notify_data_changed()`
- 叶子节点不要过度卡片化，否则很容易重新退化成 list

## 相关文件

- `example/HelloBasic/virtual_tree/test.c`
- `src/widget/egui_view_virtual_tree.h`
- `src/widget/egui_view_virtual_tree.c`
- `src/widget/egui_view_virtual_viewport.h`
