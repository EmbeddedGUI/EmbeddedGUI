# Grid View

## 这个示例解决什么问题
`HelloVirtual/grid_view` 展示 `egui_view_grid_view` 的 richer 用法，重点是把“业务网格”写法控制在 `data_model + holder_ops` 里。

它重点演示：

- 两种 tile `view_type`
- 真实控件直接放进 tile：`ToggleButton / Switch / Button / ProgressBar`
- `measure_item_height(width_hint)` 配合列数切换做自适应高度
- `notify_item_inserted / removed / changed / resized`
- `save_holder_state / restore_holder_state` 保存 tile 本地状态

## 推荐先看哪里
1. `grid_view_demo_data_model`
2. `grid_view_demo_holder_ops`
3. `grid_view_demo_apply_column_count()`
4. `grid_view_demo_save_holder_state()` / `grid_view_demo_restore_holder_state()`

## 关键文件
- `src/widget/egui_view_grid_view.h`
- `src/widget/egui_view_grid_view.c`
- `example/HelloVirtual/grid_view/test.c`

## 运行方式
构建：
```bash
make all APP=HelloVirtual APP_SUB=grid_view PORT=pc
```

运行时检查：
```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub grid_view --keep-screenshots
```

渲染工作流：
```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets grid_view --skip-unit-tests
```

## 截图里要确认什么
- 首屏能同时看到不同 tile 类型
- 切换 `2 Col / 3 Col / 4 Col` 后，tile 高度会跟随宽度变化
- 行内真实控件可以正常更新数据模型
- 滚出屏幕后再跳回，tile 本地 `preview` 状态仍能恢复
