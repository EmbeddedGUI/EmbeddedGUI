# 布局系统

## 概述

EmbeddedGUI 提供三种布局方式：手动布局、LinearLayout 线性布局和 GridLayout 网格布局。手动布局通过直接设置坐标和尺寸实现，适合简单场景；自动布局容器则负责计算子视图的排列位置，适合动态内容。

## 手动布局

最基础的布局方式，直接指定视图相对于父容器的位置和大小：

```c
// 设置位置（相对父视图左上角）
egui_view_set_position(EGUI_VIEW_OF(&my_label), 10, 20);

// 设置大小
egui_view_set_size(EGUI_VIEW_OF(&my_label), 100, 30);
```

坐标系统：
- 原点在父视图内容区域（padding 内侧）的左上角
- X 轴向右为正，Y 轴向下为正
- 单位为像素（`egui_dim_t`，即 `int16_t`）

## Padding 和 Margin

### Padding（内边距）

定义视图内容区域与视图边界之间的间距。对于容器视图，子视图的坐标原点从 padding 内侧开始。

```c
// 分别设置四边
egui_view_set_padding(EGUI_VIEW_OF(&my_group), 5, 5, 10, 10);
//                                              左 右  上  下

// 四边统一设置
egui_view_set_padding_all(EGUI_VIEW_OF(&my_group), 8);
```

### Margin（外边距）

定义视图与相邻视图之间的间距。在自动布局容器中，margin 参与排列计算。

```c
// 分别设置四边
egui_view_set_margin(EGUI_VIEW_OF(&my_label), 4, 4, 2, 2);

// 四边统一设置
egui_view_set_margin_all(EGUI_VIEW_OF(&my_label), 5);
```

### Padding 与 Margin 的关系

```
+------ 视图边界 (region) ------+
|          margin (外)          |
|  +------ padding (内) ------+ |
|  |                          | |
|  |     内容区域 / 子视图     | |
|  |                          | |
|  +--------------------------+ |
+-------------------------------+
```

启用 `EGUI_CONFIG_REDUCE_MARGIN_PADDING_SIZE` 后，margin/padding 类型从 `int16_t` 缩减为 `int8_t`，节省 RAM 但限制范围为 -128~127。

## LinearLayout 线性布局

`egui_view_linearlayout_t` 将子视图沿单一方向（水平或垂直）依次排列。

### 结构体

```c
struct egui_view_linearlayout
{
    egui_view_group_t base;
    uint8_t align_type;                 // 对齐方式
    uint8_t is_auto_width;              // 自动调整宽度
    uint8_t is_auto_height;             // 自动调整高度
    uint8_t is_orientation_horizontal;  // 0=垂直, 1=水平
};
```

### API

| API | 说明 |
|-----|------|
| `egui_view_linearlayout_init(self)` | 初始化（默认垂直方向） |
| `egui_view_linearlayout_set_orientation(self, h)` | 设置方向 (0=垂直, 1=水平) |
| `egui_view_linearlayout_set_align_type(self, type)` | 设置对齐方式 |
| `egui_view_linearlayout_set_auto_width(self, v)` | 启用自动宽度 |
| `egui_view_linearlayout_set_auto_height(self, v)` | 启用自动高度 |

### 对齐方式

对齐常量定义在 `egui_common.h`：

| 常量 | 值 | 说明 |
|------|----|------|
| `EGUI_ALIGN_LEFT` | 0x02 | 水平左对齐 |
| `EGUI_ALIGN_HCENTER` | 0x01 | 水平居中 |
| `EGUI_ALIGN_RIGHT` | 0x04 | 水平右对齐 |
| `EGUI_ALIGN_TOP` | 0x20 | 垂直顶部对齐 |
| `EGUI_ALIGN_VCENTER` | 0x10 | 垂直居中 |
| `EGUI_ALIGN_BOTTOM` | 0x40 | 垂直底部对齐 |
| `EGUI_ALIGN_CENTER` | 0x11 | 水平+垂直居中 |

对齐方式可通过按位或组合，如 `EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP`。

### Auto Width / Auto Height

启用后，容器会根据子视图的总尺寸自动调整自身大小：

- 垂直布局 + `auto_height`：容器高度 = 所有子视图高度之和（含 margin）
- 水平布局 + `auto_width`：容器宽度 = 所有子视图宽度之和（含 margin）

### 参数宏初始化

可通过参数宏在编译期初始化：

```c
// 垂直布局，居中对齐
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_params, 0, 0, 200, 300, EGUI_ALIGN_HCENTER);

// 水平布局
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT_H(layout_params_h, 0, 0, 300, 50, EGUI_ALIGN_VCENTER);

// 使用参数初始化
egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&my_layout), &layout_params);
```

## GridLayout 网格布局

`egui_view_gridlayout_t` 将子视图按网格排列，指定列数后自动换行。

### 结构体

```c
struct egui_view_gridlayout
{
    egui_view_group_t base;
    uint8_t col_count;    // 列数
    uint8_t align_type;   // 对齐方式
};
```

### API

| API | 说明 |
|-----|------|
| `egui_view_gridlayout_init(self)` | 初始化（默认 2 列） |
| `egui_view_gridlayout_set_col_count(self, cols)` | 设置列数 |
| `egui_view_gridlayout_set_align_type(self, type)` | 设置对齐方式 |

### 布局算法

GridLayout 的布局逻辑：
1. 将容器宽度等分为 `col_count` 列，每列宽度 = `container_width / col_count`
2. 子视图按添加顺序依次填入单元格
3. 每个子视图在单元格内水平居中
4. 行高取该行中最高子视图的高度（含 margin）
5. `is_gone` 的子视图不参与布局

### 参数宏初始化

```c
// 3 列网格，居中对齐
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 320, 3, EGUI_ALIGN_HCENTER);

egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&my_grid), &grid_params);
```

## 嵌套布局示例

一个典型的页面布局：顶部标题栏（水平），下方内容区（垂直排列多个卡片）。

```c
static egui_view_linearlayout_t page_layout;     // 页面根布局（垂直）
static egui_view_linearlayout_t title_bar;        // 标题栏（水平）
static egui_view_label_t title_label;
static egui_view_label_t card_1;
static egui_view_label_t card_2;
static egui_view_label_t card_3;

void page_init(void)
{
    // 页面根布局 - 垂直
    egui_view_linearlayout_init(EGUI_VIEW_OF(&page_layout));
    egui_view_set_position(EGUI_VIEW_OF(&page_layout), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&page_layout), 240, 320);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&page_layout), 0);

    // 标题栏 - 水平居中
    egui_view_linearlayout_init(EGUI_VIEW_OF(&title_bar));
    egui_view_set_size(EGUI_VIEW_OF(&title_bar), 240, 40);
    egui_view_linearlayout_set_orientation(EGUI_VIEW_OF(&title_bar), 1);
    egui_view_linearlayout_set_align_type(EGUI_VIEW_OF(&title_bar), EGUI_ALIGN_CENTER);

    // 标题文字
    egui_view_label_init(EGUI_VIEW_OF(&title_label));
    egui_view_set_size(EGUI_VIEW_OF(&title_label), 100, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&title_label), "My App");

    // 卡片
    egui_view_label_init(EGUI_VIEW_OF(&card_1));
    egui_view_set_size(EGUI_VIEW_OF(&card_1), 220, 60);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_1), 5);

    egui_view_label_init(EGUI_VIEW_OF(&card_2));
    egui_view_set_size(EGUI_VIEW_OF(&card_2), 220, 60);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_2), 5);

    egui_view_label_init(EGUI_VIEW_OF(&card_3));
    egui_view_set_size(EGUI_VIEW_OF(&card_3), 220, 60);
    egui_view_set_margin_all(EGUI_VIEW_OF(&card_3), 5);

    // 构建视图树
    egui_view_group_add_child(EGUI_VIEW_OF(&title_bar), EGUI_VIEW_OF(&title_label));
    egui_view_group_add_child(EGUI_VIEW_OF(&page_layout), EGUI_VIEW_OF(&title_bar));
    egui_view_group_add_child(EGUI_VIEW_OF(&page_layout), EGUI_VIEW_OF(&card_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&page_layout), EGUI_VIEW_OF(&card_2));
    egui_view_group_add_child(EGUI_VIEW_OF(&page_layout), EGUI_VIEW_OF(&card_3));
}
```

布局结果：

```
+--- page_layout (240x320, 垂直) ---+
| +--- title_bar (240x40, 水平) ---+ |
| |        [My App]                | |
| +--------------------------------+ |
| +--- card_1 (220x60, margin=5) --+ |
| |                                | |
| +--------------------------------+ |
| +--- card_2 (220x60, margin=5) --+ |
| |                                | |
| +--------------------------------+ |
| +--- card_3 (220x60, margin=5) --+ |
| |                                | |
| +--------------------------------+ |
+------------------------------------+
```
