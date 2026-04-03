# Virtual Grid Basic

## 这个示例是做什么的

`HelloVirtual/virtual_grid_basic` 是 `virtual_grid` 的最小入门示例。

它只保留：

- 一个简单的 action bar
- 一个纵向 `virtual_grid`
- 一种基础 tile 视图，配几种高度变化

顶部不再显示 `Header`，这样用户可以直接把注意力放在 grid 的虚拟化行为上。

## 这个示例重点演示什么

1. 用 `egui_view_virtual_grid_init_with_setup()` 接好 `params + data_source + context`
2. 把 item 状态放在外部数组里
3. 在点击回调里用 `egui_view_virtual_grid_resolve_item_by_view()` 反查 item
4. 单项变化后按情况调用 `notify_item_changed_by_stable_id()` 或 `notify_item_resized_by_stable_id()`
5. 用 `set_column_count()` 和 `ensure_item_visible_by_stable_id()` 做基础列切换和定位

## 为什么它适合入门

相比 `HelloVirtual/virtual_grid`，这个 basic case 去掉了：

- 更复杂的 dashboard 语义
- 更重的 mutation 流程
- keepalive/state cache 的组合展示
- 顶部摘要 Header

留下来的只有最基本的 grid 能力：

- 很多 item
- 少量 live slot
- 点击
- 纵向滚动
- patch
- 列数切换
- reset

## 建议阅读顺序

1. 先看 `grid_basic_init_items()`
2. 再看 `grid_basic_data_source`
3. 再看 `grid_basic_item_click_cb()`
4. 最后看 `grid_basic_patch_selected()` 和 `grid_basic_cycle_columns()`

## 运行方式

```bash
make all APP=HelloVirtual APP_SUB=virtual_grid_basic PORT=pc
python scripts/code_runtime_check.py --app HelloVirtual --app-sub virtual_grid_basic --keep-screenshots
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets virtual_grid_basic --skip-unit-tests
```

## 看图时重点确认

- 首屏能直接看到多张卡片
- 点击卡片后，选中态会变化
- `Patch` 后卡片内容会变化，必要时高度也会变化
- 纵向滚动后，可见窗口会移动
- `Cols` 会在 2/3/4 列之间切换
- `Reset` 会恢复初始数据、列数和顶部位置
