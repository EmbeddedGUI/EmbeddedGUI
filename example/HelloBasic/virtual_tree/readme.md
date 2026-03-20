# Virtual Tree 示例

## 这个示例展示什么

这个示例基于 `egui_view_virtual_tree_t`，演示“树形数据 + 虚拟化复用”的典型场景：

- 6 个 root，全部展开后总节点数超过 1000
- root / group / task 三层结构
- 节点按深度缩进，分支和叶子使用不同样式
- 点击节点后确认命中的可见节点
- 分支节点支持折叠和展开
- `Patch / Jump` 演示节点内容变化、局部刷新和按 `stable_id` 定位
- 脉冲动画结合 `keepalive + state cache`

适合的场景：

- 文件树或目录树
- 组织架构或设备拓扑
- 可折叠任务树
- 分层导航

## 运行方式

```bash
make -j1 all APP=HelloBasic APP_SUB=virtual_tree PORT=pc
python scripts/code_runtime_check.py --app HelloBasic --app-sub virtual_tree --keep-screenshots
```

## 推荐接入方式

```c
static egui_view_virtual_tree_t tree_view;
static app_context_t tree_ctx;

static const egui_view_virtual_tree_params_t tree_params = {
        .region = {{8, 48}, {224, 264}},
        .overscan_before = 1,
        .overscan_after = 1,
        .max_keepalive_slots = 4,
        .estimated_node_height = 60,
};

static const egui_view_virtual_tree_data_source_t tree_source = {
        .get_root_count = tree_get_root_count,
        .get_root_stable_id = tree_get_root_stable_id,
        .get_child_count = tree_get_child_count,
        .get_child_stable_id = tree_get_child_stable_id,
        .is_node_expanded = tree_is_node_expanded,
        .get_node_view_type = tree_get_node_view_type,
        .measure_node_height = tree_measure_node_height,
        .create_node_view = tree_create_node_view,
        .bind_node_view = tree_bind_node_view,
        .unbind_node_view = tree_unbind_node_view,
        .should_keep_alive = tree_should_keep_alive,
        .save_node_state = tree_save_node_state,
        .restore_node_state = tree_restore_node_state,
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

如果容器已经创建，后续要切换数据源或缓存限额，可以继续调用：

```c
egui_view_virtual_tree_apply_setup(EGUI_VIEW_OF(&tree_view), &tree_setup);
```

## 数据源设计要点

### 1. `stable_id` 要覆盖整棵树

`virtual_tree` 的定位、状态缓存和点击回溯都依赖 `stable_id`，要求是：

- 每个节点都要有稳定且全局唯一的 `stable_id`
- 节点在展开、折叠、滚动后都不能变

### 2. 结构变化优先用 `notify_data_changed()`

当前 `virtual_tree` 的重点是先把树语义包到虚拟列表之上。以下变化建议统一调用：

- 分支折叠或展开
- 父子关系变化
- 可见节点数量变化

```c
egui_view_virtual_tree_notify_data_changed(EGUI_VIEW_OF(&tree_view));
```

### 3. 单节点变化用细粒度接口

当只是节点内容或高度变化时，不必整棵树重建：

```c
egui_view_virtual_tree_notify_node_changed_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
egui_view_virtual_tree_notify_node_resized_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id);
```

如果展开、选中或播放状态变化后，希望当前节点保持在可见安全带内，可以继续调用：

```c
egui_view_virtual_tree_ensure_node_visible_by_stable_id(EGUI_VIEW_OF(&tree_view), stable_id, inset);
```

## 点击命中确认

如果节点根 view 自己就接点击事件，可以直接从 view 反查树语义：

```c
static void my_node_click_cb(egui_view_t *self)
{
    egui_view_virtual_tree_entry_t entry;

    if (!egui_view_virtual_tree_resolve_node_by_view(EGUI_VIEW_OF(&tree_view), self, &entry))
    {
        return;
    }

    on_node_clicked(entry.visible_index, entry.stable_id, entry.depth, entry.has_children);
}
```

这对录制动作、自动化测试和业务回调都很方便，不需要额外维护 `view -> node` 映射表。

## 常用 helper

- `egui_view_virtual_tree_get_root_count()`
- `egui_view_virtual_tree_get_visible_node_count()`
- `egui_view_virtual_tree_find_visible_index_by_stable_id()`
- `egui_view_virtual_tree_resolve_node_by_stable_id()`
- `egui_view_virtual_tree_resolve_node_by_view()`
- `egui_view_virtual_tree_scroll_to_node_by_stable_id()`
- `egui_view_virtual_tree_ensure_node_visible_by_stable_id()`
- `egui_view_virtual_tree_visit_visible_nodes()`
- `egui_view_virtual_tree_find_slot_by_stable_id()`
- `egui_view_virtual_tree_find_view_by_stable_id()`
- `egui_view_virtual_tree_find_first_visible_node_view()`
- `egui_view_virtual_tree_get_slot_count()`
- `egui_view_virtual_tree_get_slot()`
- `egui_view_virtual_tree_get_slot_entry()`

## `keepalive` 和状态缓存怎么配合

建议策略：

- 短时间内必须保留同一实例的节点用 `should_keep_alive`
- 允许回收但希望恢复动画或临时态的节点用 `save_node_state / restore_node_state`

## 相关文件

- `example/HelloBasic/virtual_tree/test.c`
- `src/widget/egui_view_virtual_tree.h`
- `src/widget/egui_view_virtual_tree.c`
- `src/widget/egui_view_virtual_list.h`
- `src/widget/egui_view_virtual_viewport.h`
