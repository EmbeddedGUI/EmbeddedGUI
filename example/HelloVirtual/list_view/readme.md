# List View

## 这个示例解决什么问题
`HelloVirtual/list_view` 展示 `egui_view_list_view` 的 richer 用法，但仍然保持 `data_model + holder_ops` 这条主线。

它重点演示四件事：

- `get_view_type + create_holder`：一个列表里混用两种真实行视图
- `Switch / Combobox / ToggleButton / Button / ProgressBar`：holder 里直接放真实控件
- `notify_item_inserted / removed / moved / resized / changed`：业务数据改动后如何通知控件
- `save_holder_state / restore_holder_state`：holder 本地状态如何跨回收保留

## 推荐先看哪里
1. `list_view_demo_data_model`
2. `list_view_demo_holder_ops`
3. `list_view_demo_action_*()`
4. `list_view_demo_save_holder_state()` / `list_view_demo_restore_holder_state()`

## 这个示例里的设计边界
- 业务状态放在 `data_model`：是否启用、模式、进度、是否展开
- holder 本地状态放在 `holder`：这里用 `preview_hits` 演示
- 列表复用、回收、可见窗口、state cache 管理都交给控件内部

## 关键文件
- `src/widget/egui_view_list_view.h`
- `src/widget/egui_view_list_view.c`
- `example/HelloVirtual/list_view/test.c`

## 运行方式
构建：
```bash
make all APP=HelloVirtual APP_SUB=list_view PORT=pc
```

运行时检查：
```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub list_view --keep-screenshots
```

渲染工作流：
```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets list_view --skip-unit-tests
```

## 截图里要确认什么
- 首屏能看到两种不同风格的真实行视图
- 行内 `Switch / Combobox / ToggleButton / Button` 都可交互
- 顶部工具栏操作后，列表能正确插入、删除、移动、局部刷新
- 把一行滚出屏幕后再跳回，holder 本地 `preview` 计数仍然存在
