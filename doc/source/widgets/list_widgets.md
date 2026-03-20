# 列表控件

## 概述

列表控件用于展示结构化的数据集合。EmbeddedGUI 提供了三种列表类控件：List 是可滚动的文本列表，Table 用于展示行列数据表格，ButtonMatrix 用于创建按钮网格(如计算器键盘)。

> 如果你的场景需要承载数百到上千条数据，并且要求“只为可见区创建 view、支持 stable_id、支持状态缓存和精确插删改移通知”，应优先阅读 [虚拟容器与大数据集控件](virtual_widgets.md)，而不是继续使用这里的传统 `List`。

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
