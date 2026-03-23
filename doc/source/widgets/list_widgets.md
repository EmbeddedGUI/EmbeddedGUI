# 列表与网格控件

## 概述

这一页覆盖两类能力：

- 传统小规模控件：`List`、`Table`、`ButtonMatrix`
- 面向大数据集、但接口更接近 Android `ListView / GridView + ViewHolder + DataModel` 的高层控件：`ListView`、`GridView`

如果你的场景是“很多条目，但每个条目仍然由真实控件组成”，建议先从这里的 `ListView` / `GridView` 开始，而不是直接从 raw `virtual_list` / `virtual_grid` 起步。  
只有在你需要 `section / tree / page / stage`、完全自定义 adapter，或者要直接面对 raw virtual 容器语义时，再继续阅读 [虚拟容器与大数据集控件](virtual_widgets.md)。

## 快速选型表

| 控件 | 适合场景 | 关键特点 |
|------|----------|----------|
| `List` | 十几条以内的简单文本列表 | 每个条目是按钮，API 最轻 |
| `ListView` | 大量纵向条目，条目里直接放真实控件 | 业务层主要关心 `data_model + holder_ops` |
| `GridView` | 大量 tile/card，列数和高度会变化 | 业务层主要关心 `data_model + holder_ops` |
| `Table` | 小规模行列表格数据 | 直接按单元格写内容 |
| `ButtonMatrix` | 计算器、快捷键盘、宫格按钮面板 | 在一个 view 里绘制多个按钮 |

---

## ListView

`ListView` 是建立在 `virtual_list` 之上的高层包装，目标不是替代 virtual 家族，而是把常见“竖向大列表 + 真实控件行视图”的接法收敛成 Android 风格的 `data_model + holder_ops`。

### 什么时候优先用它

- 数据量较大，需要按可见区创建和回收行视图
- 每一行都想直接放真实控件，例如 `Switch`、`Combobox`、`Button`、`ProgressBar`
- 业务层更想关心 `ViewHolder` 和 `DataModel`，不想直接写 raw adapter

### 最小心智模型

- `data_model` 负责回答“有多少条数据、stable id 是什么、view_type 是什么、高度是多少”
- `holder_ops` 负责回答“这一行真实控件怎么创建、怎么绑定、怎么保存局部状态”
- 结构变化后由业务层调用 `notify_*`，复用、keepalive、状态缓存和可见窗口管理交给控件内部

### 关键 API

| 函数 | 说明 |
|------|------|
| `egui_view_list_view_init_with_setup(self, setup)` | 一次接好 `params + data_model + holder_ops + context` |
| `egui_view_list_view_set_data_model(self, model, ops, context)` | 运行时替换数据模型和 holder 逻辑 |
| `egui_view_list_view_resolve_item_by_view(self, item_view, entry)` | 在真实控件回调里反查当前 item |
| `egui_view_list_view_notify_item_changed_by_stable_id(self, stable_id)` | 单项内容变化但高度不变 |
| `egui_view_list_view_notify_item_resized_by_stable_id(self, stable_id)` | 单项高度变化 |
| `egui_view_list_view_notify_item_inserted/removed/moved(...)` | 结构变化通知 |
| `egui_view_list_view_set_keepalive_limit(self, max_keepalive_slots)` | 设置 keepalive 上限 |
| `egui_view_list_view_set_state_cache_limits(self, max_entries, max_bytes)` | 设置 holder 状态缓存上限 |

### 最小接入骨架

```c
static egui_view_list_view_t list_view;
static demo_context_t demo_ctx;

EGUI_VIEW_LIST_VIEW_PARAMS_INIT(demo_params, 0, 0, 240, 320);

static const egui_view_list_view_data_model_t demo_data_model = {
    .get_count = demo_get_count,
    .get_stable_id = demo_get_stable_id,
    .find_index_by_stable_id = demo_find_index_by_stable_id,
    .get_view_type = demo_get_view_type,
    .measure_item_height = demo_measure_item_height,
    .default_view_type = 0,
};

static const egui_view_list_view_holder_ops_t demo_holder_ops = {
    .create_holder = demo_create_holder,
    .destroy_holder = demo_destroy_holder,
    .bind_holder = demo_bind_holder,
    .unbind_holder = demo_unbind_holder,
    .save_holder_state = demo_save_holder_state,
    .restore_holder_state = demo_restore_holder_state,
};

void init_ui(void)
{
    const egui_view_list_view_setup_t setup = {
        .params = &demo_params,
        .data_model = &demo_data_model,
        .holder_ops = &demo_holder_ops,
        .data_model_context = &demo_ctx,
        .state_cache_max_entries = 8,
        .state_cache_max_bytes = 64,
    };

    egui_view_list_view_init_with_setup(EGUI_VIEW_OF(&list_view), &setup);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&list_view));
}
```

### 推荐示例

- 最小例程：`example/HelloVirtual/list_view_basic/`
- richer 场景：`example/HelloVirtual/list_view/`
- 头文件：`src/widget/egui_view_list_view.h`

---

## GridView

`GridView` 是建立在 `virtual_grid` 之上的高层包装，适合“很多 tile/card，但业务层只想维护 `ViewHolder + DataModel`”的场景。

### 什么时候优先用它

- 内容天然是 tile/card，而不是普通纵向 row
- 列数会切换，或者 tile 高度要跟随宽度变化
- tile 里直接放真实控件，例如 `ToggleButton`、`Switch`、`Button`、`ProgressBar`

### 最小心智模型

- `data_model` 负责回答“总数、stable id、view_type 和每个 tile 在当前宽度下的高度”
- `holder_ops` 负责创建和绑定真实 tile 视图
- `set_column_count()`、`notify_*`、状态缓存和复用规则都直接复用 `virtual_grid` 的成熟能力

### 关键 API

| 函数 | 说明 |
|------|------|
| `egui_view_grid_view_init_with_setup(self, setup)` | 一次接好 `params + data_model + holder_ops + context` |
| `egui_view_grid_view_set_data_model(self, model, ops, context)` | 运行时替换数据模型和 holder 逻辑 |
| `egui_view_grid_view_set_column_count(self, column_count)` | 动态切换列数 |
| `egui_view_grid_view_resolve_item_by_view(self, item_view, entry)` | 在真实控件回调里反查当前 tile |
| `egui_view_grid_view_notify_item_changed_by_stable_id(self, stable_id)` | 单项内容变化但高度不变 |
| `egui_view_grid_view_notify_item_resized_by_stable_id(self, stable_id)` | 单项高度变化 |
| `egui_view_grid_view_notify_item_inserted/removed/moved(...)` | 结构变化通知 |
| `egui_view_grid_view_set_keepalive_limit(self, max_keepalive_slots)` | 设置 keepalive 上限 |
| `egui_view_grid_view_set_state_cache_limits(self, max_entries, max_bytes)` | 设置 holder 状态缓存上限 |

### 最小接入骨架

```c
static egui_view_grid_view_t grid_view;
static demo_context_t demo_ctx;

EGUI_VIEW_GRID_VIEW_PARAMS_INIT(demo_params, 0, 0, 240, 320);

static const egui_view_grid_view_data_model_t demo_data_model = {
    .get_count = demo_get_count,
    .get_stable_id = demo_get_stable_id,
    .find_index_by_stable_id = demo_find_index_by_stable_id,
    .get_view_type = demo_get_view_type,
    .measure_item_height = demo_measure_item_height,
    .default_view_type = 0,
};

static const egui_view_grid_view_holder_ops_t demo_holder_ops = {
    .create_holder = demo_create_holder,
    .destroy_holder = demo_destroy_holder,
    .bind_holder = demo_bind_holder,
    .unbind_holder = demo_unbind_holder,
    .save_holder_state = demo_save_holder_state,
    .restore_holder_state = demo_restore_holder_state,
};

void init_ui(void)
{
    const egui_view_grid_view_setup_t setup = {
        .params = &demo_params,
        .data_model = &demo_data_model,
        .holder_ops = &demo_holder_ops,
        .data_model_context = &demo_ctx,
        .state_cache_max_entries = 8,
        .state_cache_max_bytes = 64,
    };

    egui_view_grid_view_init_with_setup(EGUI_VIEW_OF(&grid_view), &setup);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid_view));
}
```

### 推荐示例

- 最小例程：`example/HelloVirtual/grid_view_basic/`
- richer 场景：`example/HelloVirtual/grid_view/`
- 头文件：`src/widget/egui_view_grid_view.h`

---

## List

可滚动的文本列表控件，继承自 Scroll。每个列表项是一个 Button，支持点击回调。最多支持 16 个列表项。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=list)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_list_init(self)` | 初始化 List |
| `egui_view_list_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_list_add_item(self, text)` | 添加列表项，返回索引(-1 表示已满) |
| `egui_view_list_clear(self)` | 清空所有列表项 |
| `egui_view_list_set_item_height(self, height)` | 设置列表项高度 |
| `egui_view_list_set_on_item_click(self, callback)` | 设置列表项点击回调 |

### 参数宏

```c
EGUI_VIEW_LIST_PARAMS_INIT(name, x, y, w, h, item_h);
```

### 常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_LIST_MAX_ITEMS` | 最大列表项数量(默认 16) |

### 代码示例

```c
static egui_view_list_t list;

EGUI_VIEW_LIST_PARAMS_INIT(list_params, 0, 0, 200, 150, 30);

static void on_item_click(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("List item clicked: %d\n", index);
}

void init_ui(void)
{
    egui_view_list_init_with_params(
        EGUI_VIEW_OF(&list), &list_params);

    egui_view_list_add_item(EGUI_VIEW_OF(&list), "Settings");
    egui_view_list_add_item(EGUI_VIEW_OF(&list), "Display");
    egui_view_list_add_item(EGUI_VIEW_OF(&list), "Sound");
    egui_view_list_add_item(EGUI_VIEW_OF(&list), "Network");
    egui_view_list_add_item(EGUI_VIEW_OF(&list), "About");

    egui_view_list_set_on_item_click(
        EGUI_VIEW_OF(&list), on_item_click);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&list));
}
```

---

## Table

表格控件，支持表头行、网格线、自定义行高和颜色。最多支持 16 行 8 列。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=table)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_table_init(self)` | 初始化 Table |
| `egui_view_table_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_table_set_cell(self, row, col, text)` | 设置单元格文本 |
| `egui_view_table_set_size(self, rows, cols)` | 设置行列数 |
| `egui_view_table_set_header_rows(self, count)` | 设置表头行数 |
| `egui_view_table_set_row_height(self, height)` | 设置行高 |
| `egui_view_table_set_show_grid(self, show)` | 显示/隐藏网格线 |
| `egui_view_table_set_header_bg_color(self, color)` | 设置表头背景色 |
| `egui_view_table_set_grid_color(self, color)` | 设置网格线颜色 |

### 参数宏

```c
EGUI_VIEW_TABLE_PARAMS_INIT(name, x, y, w, h, row_count, col_count);
```

### 常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_TABLE_MAX_ROWS` | 最大行数(默认 16) |
| `EGUI_VIEW_TABLE_MAX_COLS` | 最大列数(默认 8) |

### 代码示例

```c
static egui_view_table_t table;

EGUI_VIEW_TABLE_PARAMS_INIT(table_params, 0, 0, 200, 120, 4, 3);

void init_ui(void)
{
    egui_view_table_init_with_params(
        EGUI_VIEW_OF(&table), &table_params);

    egui_view_table_set_header_rows(EGUI_VIEW_OF(&table), 1);
    egui_view_table_set_row_height(EGUI_VIEW_OF(&table), 28);
    egui_view_table_set_show_grid(EGUI_VIEW_OF(&table), 1);

    // 表头
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 0, "Name");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 1, "Value");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 0, 2, "Unit");

    // 数据行
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 0, "Temp");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 1, "25");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 1, 2, "C");

    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 0, "Humi");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 1, "60");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 2, 2, "%");

    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 0, "Press");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 1, "1013");
    egui_view_table_set_cell(EGUI_VIEW_OF(&table), 3, 2, "hPa");

    egui_core_add_user_root_view(EGUI_VIEW_OF(&table));
}
```

---

## ButtonMatrix

按钮矩阵控件，在一个视图内绘制多个按钮，适合计算器键盘、快捷操作面板等场景。相比使用多个独立 Button，ButtonMatrix 内存占用更低。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=button_matrix)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_button_matrix_init(self)` | 初始化 ButtonMatrix |
| `egui_view_button_matrix_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_button_matrix_set_labels(self, labels, count, cols)` | 设置按钮标签、数量和列数 |
| `egui_view_button_matrix_set_on_click(self, callback)` | 设置按钮点击回调 |
| `egui_view_button_matrix_set_btn_color(self, color)` | 设置按钮背景色 |
| `egui_view_button_matrix_set_btn_pressed_color(self, color)` | 设置按钮按下色 |
| `egui_view_button_matrix_set_text_color(self, color)` | 设置文本颜色 |
| `egui_view_button_matrix_set_border_color(self, color)` | 设置边框颜色 |
| `egui_view_button_matrix_set_gap(self, gap)` | 设置按钮间距 |
| `egui_view_button_matrix_set_corner_radius(self, radius)` | 设置按钮圆角 |
| `egui_view_button_matrix_set_font(self, font)` | 设置字体 |

### 参数宏

```c
EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(name, x, y, w, h, cols, gap);
```

### 常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_BUTTON_MATRIX_MAX_BUTTONS` | 最大按钮数量(默认 16) |

### 代码示例

```c
static egui_view_button_matrix_t btnmatrix;

static const char *calc_labels[] = {
    "7", "8", "9", "/",
    "4", "5", "6", "*",
    "1", "2", "3", "-",
    "0", ".", "=", "+",
};

EGUI_VIEW_BUTTON_MATRIX_PARAMS_INIT(
    bm_params, 0, 0, 200, 200, 4, 2);

static void on_btn_click(egui_view_t *self, uint8_t btn_index)
{
    EGUI_LOG_INF("Button: %s\n", calc_labels[btn_index]);
}

void init_ui(void)
{
    egui_view_button_matrix_init_with_params(
        EGUI_VIEW_OF(&btnmatrix), &bm_params);
    egui_view_button_matrix_set_labels(
        EGUI_VIEW_OF(&btnmatrix), calc_labels, 16, 4);
    egui_view_button_matrix_set_on_click(
        EGUI_VIEW_OF(&btnmatrix), on_btn_click);
    egui_view_button_matrix_set_corner_radius(
        EGUI_VIEW_OF(&btnmatrix), 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&btnmatrix));
}
```
