# 容器控件

## 概述

容器控件用于组织和管理子控件的布局。EmbeddedGUI 提供了四种容器：Group 是最基础的容器，Card 在 Group 基础上增加了圆角和边框装饰，LinearLayout 支持水平/垂直线性排列，GridLayout 支持网格排列。所有容器都继承自 `egui_view_group_t`。

---

## Group

最基础的容器控件，可容纳任意数量的子控件。Group 本身不做布局计算，子控件使用绝对坐标定位。

### 效果展示

Group 作为基础容器不单独展示，其能力通过子类容器体现。

### API

| 函数 | 说明 |
|------|------|
| `egui_view_group_init(self)` | 初始化 Group |
| `egui_view_group_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_group_add_child(self, child)` | 添加子控件 |
| `egui_view_group_remove_child(self, child)` | 移除子控件 |
| `egui_view_group_clear_childs(self)` | 清空所有子控件 |
| `egui_view_group_get_child_count(self)` | 获取子控件数量 |
| `egui_view_group_get_first_child(self)` | 获取第一个子控件 |
| `egui_view_group_set_disallow_process_touch_event(self, disallow)` | 禁止处理触摸事件 |
| `egui_view_group_request_disallow_intercept_touch_event(self, disallow)` | 禁止拦截触摸事件 |
| `egui_view_group_layout_childs(self, is_horizontal, is_auto_w, is_auto_h, align)` | 手动布局子控件 |
| `egui_view_group_calculate_all_child_width(self, width)` | 计算所有子控件总宽度 |
| `egui_view_group_calculate_all_child_height(self, height)` | 计算所有子控件总高度 |
| `egui_view_group_bring_child_to_front(self, child)` | 将子控件移到最前(需开启 LAYER) |
| `egui_view_group_send_child_to_back(self, child)` | 将子控件移到最后(需开启 LAYER) |

### 参数宏

```c
EGUI_VIEW_GROUP_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_group_t group;
static egui_view_label_t label;

EGUI_VIEW_GROUP_PARAMS_INIT(group_params, 0, 0, 200, 100);

void init_ui(void)
{
    egui_view_group_init_with_params(EGUI_VIEW_OF(&group), &group_params);

    egui_view_label_init(EGUI_VIEW_OF(&label));
    egui_view_set_position(EGUI_VIEW_OF(&label), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&label), 180, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&label), "Hello Group");

    egui_view_group_add_child(EGUI_VIEW_OF(&group), EGUI_VIEW_OF(&label));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&group));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER` | 启用图层排序(bring_to_front / send_to_back) |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 启用按键事件分发 |

---

## Card

带圆角、边框和背景色的装饰容器，继承自 Group。适合用于卡片式 UI 布局。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=card)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_card_init(self)` | 初始化 Card |
| `egui_view_card_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_card_set_corner_radius(self, radius)` | 设置圆角半径 |
| `egui_view_card_set_border(self, width, color)` | 设置边框宽度和颜色 |
| `egui_view_card_set_bg_color(self, color, alpha)` | 设置背景颜色和透明度 |
| `egui_view_card_add_child(self, child)` | 添加子控件 |
| `egui_view_card_layout_childs(self, is_horizontal, align_type)` | 布局子控件 |

### 参数宏

```c
EGUI_VIEW_CARD_PARAMS_INIT(name, x, y, w, h, radius);
```

### 代码示例

```c
static egui_view_card_t card;
static egui_view_label_t title;

EGUI_VIEW_CARD_PARAMS_INIT(card_params, 10, 10, 200, 80, 8);

void init_ui(void)
{
    egui_view_card_init_with_params(EGUI_VIEW_OF(&card), &card_params);
    egui_view_card_set_bg_color(EGUI_VIEW_OF(&card),
        EGUI_COLOR_MAKE(0x33, 0x33, 0x33), EGUI_ALPHA_100);
    egui_view_card_set_border(EGUI_VIEW_OF(&card), 1,
        EGUI_COLOR_MAKE(0x66, 0x66, 0x66));

    egui_view_label_init(EGUI_VIEW_OF(&title));
    egui_view_set_position(EGUI_VIEW_OF(&title), 10, 10);
    egui_view_set_size(EGUI_VIEW_OF(&title), 180, 30);
    egui_view_label_set_text(EGUI_VIEW_OF(&title), "Card Title");

    egui_view_card_add_child(EGUI_VIEW_OF(&card), EGUI_VIEW_OF(&title));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&card));
}
```

---

## LinearLayout

线性布局容器，支持水平和垂直两种排列方向，可设置对齐方式和自动尺寸。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=linearlayout)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_linearlayout_init(self)` | 初始化 LinearLayout |
| `egui_view_linearlayout_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_linearlayout_set_orientation(self, is_horizontal)` | 设置排列方向(0=垂直, 1=水平) |
| `egui_view_linearlayout_set_align_type(self, align_type)` | 设置对齐方式 |
| `egui_view_linearlayout_set_auto_width(self, is_auto)` | 启用自动宽度 |
| `egui_view_linearlayout_set_auto_height(self, is_auto)` | 启用自动高度 |
| `egui_view_linearlayout_is_orientation_horizontal(self)` | 查询是否水平排列 |
| `egui_view_linearlayout_is_align_type(self)` | 查询对齐方式 |
| `egui_view_linearlayout_is_auto_width(self)` | 查询是否自动宽度 |
| `egui_view_linearlayout_is_auto_height(self)` | 查询是否自动高度 |

### 参数宏

```c
// 垂直布局
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(name, x, y, w, h, align);

// 水平布局
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT_H(name, x, y, w, h, align);
```

### 对齐方式常量

| 常量 | 说明 |
|------|------|
| `EGUI_ALIGN_LEFT` | 左对齐 |
| `EGUI_ALIGN_HCENTER` | 水平居中 |
| `EGUI_ALIGN_RIGHT` | 右对齐 |
| `EGUI_ALIGN_TOP` | 顶部对齐 |
| `EGUI_ALIGN_VCENTER` | 垂直居中 |
| `EGUI_ALIGN_BOTTOM` | 底部对齐 |
| `EGUI_ALIGN_CENTER` | 水平+垂直居中 |

### 代码示例

```c
static egui_view_linearlayout_t layout;
static egui_view_button_t btn1, btn2, btn3;

EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_params, 0, 0, 200, 150,
    EGUI_ALIGN_HCENTER);

void init_ui(void)
{
    egui_view_linearlayout_init_with_params(
        EGUI_VIEW_OF(&layout), &layout_params);

    // 初始化三个按钮并添加到垂直布局
    egui_view_button_init(EGUI_VIEW_OF(&btn1));
    egui_view_set_size(EGUI_VIEW_OF(&btn1), 160, 36);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn1), "Button 1");
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn1), 4);

    egui_view_button_init(EGUI_VIEW_OF(&btn2));
    egui_view_set_size(EGUI_VIEW_OF(&btn2), 160, 36);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn2), "Button 2");
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn2), 4);

    egui_view_button_init(EGUI_VIEW_OF(&btn3));
    egui_view_set_size(EGUI_VIEW_OF(&btn3), 160, 36);
    egui_view_label_set_text(EGUI_VIEW_OF(&btn3), "Button 3");
    egui_view_set_margin_all(EGUI_VIEW_OF(&btn3), 4);

    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&btn1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&btn2));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&btn3));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout));
}
```

---

## GridLayout

网格布局容器，按指定列数自动排列子控件为网格形式。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=gridlayout)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_gridlayout_init(self)` | 初始化 GridLayout |
| `egui_view_gridlayout_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_gridlayout_set_col_count(self, col_count)` | 设置列数 |
| `egui_view_gridlayout_set_align_type(self, align_type)` | 设置对齐方式 |

### 参数宏

```c
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(name, x, y, w, h, cols, align);
```

### 代码示例

```c
static egui_view_gridlayout_t grid;
static egui_view_button_t btns[6];

EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 200, 150, 3,
    EGUI_ALIGN_HCENTER);

void init_ui(void)
{
    egui_view_gridlayout_init_with_params(
        EGUI_VIEW_OF(&grid), &grid_params);

    const char *labels[] = {"A", "B", "C", "D", "E", "F"};
    for (int i = 0; i < 6; i++)
    {
        egui_view_button_init(EGUI_VIEW_OF(&btns[i]));
        egui_view_set_size(EGUI_VIEW_OF(&btns[i]), 56, 36);
        egui_view_label_set_text(EGUI_VIEW_OF(&btns[i]), labels[i]);
        egui_view_set_margin_all(EGUI_VIEW_OF(&btns[i]), 4);
        egui_view_group_add_child(
            EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&btns[i]));
    }

    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));
}
```
