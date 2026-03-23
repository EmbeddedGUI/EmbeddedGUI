# Virtual Tree Basic

## 这个示例是做什么的

`HelloVirtual/virtual_tree_basic` 是 `virtual_tree` 的最小入门示例。

它只保留：

- 一个简单的 action bar
- 一个纵向 `virtual_tree`
- 三层最小树结构：`root`、`group`、`task`

顶部不再显示 `Header`，让用户直接观察树的展开、折叠和虚拟化。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_tree_init_with_setup()` 接好 `params + data_source + context`
2. 把树状态放在 view 外部的节点数组里
3. 在点击回调里用 `egui_view_virtual_tree_resolve_node_by_view()` 反查命中的节点
4. branch 折叠/展开后调用 `notify_data_changed()`
5. task patch 后按情况调用 `notify_node_changed_by_stable_id()` 或 `notify_node_resized_by_stable_id()`

## 为什么它适合入门

相比 `HelloVirtual/virtual_tree`，这个 basic case 去掉了：

- 更复杂的动画和状态缓存
- 更重的场景包装
- 更大的数据规模
- 顶部摘要 Header

保留下来的，是最小但仍然完整的 tree 闭环：

- root / group / task 三层层级
- branch 点击折叠/展开
- task 点击命中
- patch 一个 task
- jump 自动展开祖先并定位目标节点
- reset 恢复初始展开状态和顶部位置

## 建议阅读顺序

1. 先看 `tree_basic_reset_model()`
2. 再看 `tree_basic_data_source`
3. 再看 `tree_basic_node_click_cb()`
4. 最后看 `tree_basic_patch_selected()` 和 `tree_basic_jump_to_next()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_tree_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_tree_basic --keep-screenshots
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_tree_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏先能读出层级，不像普通列表
- 点击 group 后，可见节点总数会变化
- 点击 task 后，命中和选中态会变化
- `Patch` 后 task 高度可以切换
- `Jump` 后目标 task 会进入可视区
- `Reset` 后展开状态和滚动位置会恢复
