# List View Basic

## 这个示例解决什么问题

`HelloVirtual/list_view_basic` 用新的 `egui_view_list_view` 做一个尽量接近 Android `ListView + ViewHolder + DataModel` 心智模型的最小例程。

它刻意只保留三件事：

- `data_model` 告诉控件一共有多少条数据、每条数据的 stable id 和高度
- `holder_ops` 负责创建真实控件、绑定真实控件
- 外部业务代码只处理数据和真实控件回调，不再手写 slot / recycle / visible window

示例里的每一行都是真实控件，不是画布重建：

- `Switch`
- `Combobox`
- `Label`

## 重点看什么

1. `list_view_basic_data_model`：最小数据模型接口
2. `list_view_basic_holder_ops`：最小 holder 生命周期接口
3. `list_view_basic_create_holder()`：一行里怎么直接放真实控件
4. `list_view_basic_switch_checked_cb()` / `list_view_basic_combo_selected_cb()`：真实控件回调里怎么反查 holder 和 item
5. `list_view_basic_bind_holder()`：数据变化后只更新当前 holder 的显示

## 关键文件

- `src/widget/egui_view_list_view.h`
- `src/widget/egui_view_list_view.c`
- `example/HelloVirtual/list_view_basic/test.c`

## 运行方式

构建：

```bash
make all APP=HelloVirtual APP_SUB=list_view_basic PORT=pc
```

运行时检查：

```bash
python scripts/code_runtime_check.py --app HelloVirtual --app-sub list_view_basic --keep-screenshots
```

渲染工作流：

```bash
python scripts/checks/hello_basic_render_workflow.py --app HelloVirtual --widgets list_view_basic --skip-unit-tests
```

## 截图里要确认什么

- 首屏能直接看到多行真实控件，不是空白列表
- 点击 `Switch` 后，行文案和状态会更新
- 展开 `Combobox` 后，下拉项可见且能完成选择
- 纵向滚动后，列表还能正常复用和显示

如果你想先理解“怎么在虚拟列表里直接放真实控件”，先看这个示例，再去看更复杂的 `virtual_viewport_basic`。
