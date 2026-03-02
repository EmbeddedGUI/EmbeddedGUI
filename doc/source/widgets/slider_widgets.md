# 滑动控件

## 概述

滑动控件允许用户通过拖拽手势在一个范围内选择数值。Slider 是水平直线滑块，ArcSlider 是弧形滑块，Roller 是滚轮选择器(从预定义列表中选择)，NumberPicker 是数字选择器(通过上下按钮调整数值)。

## Slider

水平滑块控件，用户通过拖拽滑块在 0-100 范围内选择数值。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=slider)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_slider_init(self)` | 初始化 Slider |
| `egui_view_slider_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_slider_set_value(self, value)` | 设置当前值(0-100) |
| `egui_view_slider_get_value(self)` | 获取当前值 |
| `egui_view_slider_set_on_value_changed_listener(self, listener)` | 设置值变化回调 |

### 参数宏

```c
EGUI_VIEW_SLIDER_PARAMS_INIT(name, x, y, w, h, value);
```

### 回调原型

```c
typedef void (*egui_view_on_value_changed_listener_t)(egui_view_t *self, uint8_t value);
```

### 代码示例

```c
static egui_view_slider_t slider;

EGUI_VIEW_SLIDER_PARAMS_INIT(slider_params, 0, 0, 165, 30, 60);

static void on_value_changed(egui_view_t *self, uint8_t value)
{
    EGUI_LOG_INF("Slider value: %d\n", value);
}

void init_ui(void)
{
    egui_view_slider_init_with_params(EGUI_VIEW_OF(&slider), &slider_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider), on_value_changed);
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&slider));
}
```

---

## ArcSlider

弧形滑块控件，用户通过沿弧线拖拽在 0-100 范围内选择数值，适合圆形表盘风格的 UI。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=arc_slider)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_arc_slider_init(self)` | 初始化 ArcSlider |
| `egui_view_arc_slider_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_arc_slider_set_value(self, value)` | 设置当前值(0-100) |
| `egui_view_arc_slider_get_value(self)` | 获取当前值 |
| `egui_view_arc_slider_set_on_value_changed_listener(self, listener)` | 设置值变化回调 |

### 参数宏

```c
EGUI_VIEW_ARC_SLIDER_PARAMS_INIT(name, x, y, w, h, value);
```

### 回调原型

```c
typedef void (*egui_view_on_arc_value_changed_listener_t)(egui_view_t *self, uint8_t value);
```

### 代码示例

```c
static egui_view_arc_slider_t arc;

EGUI_VIEW_ARC_SLIDER_PARAMS_INIT(arc_params, 0, 0, 92, 92, 60);

static void on_value_changed(egui_view_t *self, uint8_t value)
{
    EGUI_LOG_INF("Arc slider value: %d\n", value);
}

void init_ui(void)
{
    egui_view_arc_slider_init_with_params(EGUI_VIEW_OF(&arc), &arc_params);
    egui_view_arc_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&arc), on_value_changed);
    egui_view_set_margin_all(EGUI_VIEW_OF(&arc), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&arc));
}
```

---

## Roller

滚轮选择器控件，用户通过上下拖拽从预定义的字符串列表中选择一项，带高亮选中行效果。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=roller)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_roller_init(self)` | 初始化 Roller |
| `egui_view_roller_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_roller_set_items(self, items, count)` | 设置选项列表 |
| `egui_view_roller_set_current_index(self, index)` | 设置当前选中索引 |
| `egui_view_roller_get_current_index(self)` | 获取当前选中索引 |
| `egui_view_roller_set_on_selected_listener(self, listener)` | 设置选中变化回调 |

### 参数宏

```c
EGUI_VIEW_ROLLER_PARAMS_INIT(name, x, y, w, h, items, item_count, current_index);
```

### 回调原型

```c
typedef void (*egui_view_on_roller_selected_listener_t)(egui_view_t *self, uint8_t index);
```

### 代码示例

```c
static egui_view_roller_t roller;
static const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};

EGUI_VIEW_ROLLER_PARAMS_INIT(roller_params, 0, 0, 140, 70, weekdays, 5, 2);

static void on_selected(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("Roller selected: %s (index=%d)\n", weekdays[index], index);
}

void init_ui(void)
{
    egui_view_roller_init_with_params(EGUI_VIEW_OF(&roller), &roller_params);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller), on_selected);
    egui_view_set_margin_all(EGUI_VIEW_OF(&roller), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&roller));
}
```

---

## NumberPicker

数字选择器控件，通过上下按钮在指定范围内按步长调整数值。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=number_picker)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_number_picker_init(self)` | 初始化 NumberPicker |
| `egui_view_number_picker_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_number_picker_set_value(self, value)` | 设置当前值 |
| `egui_view_number_picker_get_value(self)` | 获取当前值 |
| `egui_view_number_picker_set_range(self, min, max)` | 设置取值范围 |
| `egui_view_number_picker_set_step(self, step)` | 设置步长 |
| `egui_view_number_picker_set_on_value_changed_listener(self, listener)` | 设置值变化回调 |

### 参数宏

```c
EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(name, x, y, w, h, value, min, max);
```

### 回调原型

```c
typedef void (*egui_view_on_number_changed_listener_t)(egui_view_t *self, int16_t value);
```

### 代码示例

```c
static egui_view_number_picker_t picker;

EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(picker_params, 0, 0, 60, 100, 50, 0, 100);

static void on_value_changed(egui_view_t *self, int16_t value)
{
    EGUI_LOG_INF("NumberPicker value: %d\n", value);
}

void init_ui(void)
{
    egui_view_number_picker_init_with_params(EGUI_VIEW_OF(&picker), &picker_params);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker), 5);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker), on_value_changed);
    egui_view_set_margin_all(EGUI_VIEW_OF(&picker), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&picker));
}
```
