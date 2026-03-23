# Grid View Basic

## 这个示例解决什么问题

`HelloVirtual/grid_view_basic` 用新的 `egui_view_grid_view` 做一个尽量接近 Android `GridView + ViewHolder + DataModel` 的最小例程。

它刻意只保留最少接口：

- `data_model` 负责数量、stable id 和 item 高度
- `holder_ops` 负责真实控件的创建、销毁、绑定
- 网格内部的复用、滚动和可见窗口管理都交给控件自己处理

示例里的 tile 也是真实控件：

- `ToggleButton`
- `ProgressBar`
- `Label`

## 重点看什么

1. `grid_view_basic_data_model`：网格数据模型最小接口
2. `grid_view_basic_holder_ops`：holder 生命周期
3. `grid_view_basic_create_holder()`：tile 里直接放真实控件
4. `grid_view_basic_toggle_cb()`：真实控件回调里反查 holder / item
5. `grid_view_basic_bind_holder()`：根据数据更新 tile UI

## 关键文件

- `src/widget/egui_view_grid_view.h`
- `src/widget/egui_view_grid_view.c`
- `example/HelloVirtual/grid_view_basic/test.c`

## 运行方式

构建：

```bash
make all APP=HelloVirtual APP_SUB=grid_view_basic PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub grid_view_basic --keep-screenshots
```

渲染工作流：

```bash
python scripts/hello_basic_render_workflow.py --app HelloVirtual --widgets grid_view_basic --skip-unit-tests
```

## 截图里要确认什么

- 首屏能直接看到多张 tile，不是空白 grid
- 点击 `ToggleButton` 后，tile 状态和进度显示会变化
- 纵向滚动后，tile 仍然能稳定复用
- 开关某个 tile 后，变高的卡片不会把布局撑乱

如果你想先理解“怎么在虚拟网格里直接放真实控件”，先看这个示例，再去看更复杂的 `virtual_grid_basic`。
