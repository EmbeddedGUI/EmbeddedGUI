# 输入控件

## 概述

输入控件提供二值状态(开/关、选中/未选中)的交互能力。Switch 是滑动开关，Checkbox 是复选框，RadioButton 是单选按钮(配合 RadioGroup 实现互斥选择)，ToggleButton 是带文本的切换按钮。

## Switch

滑动开关控件，支持开/关两种状态，带平滑过渡动画。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=switch)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_switch_init(self)` | 初始化 Switch |
| `egui_view_switch_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_switch_set_checked(self, is_checked)` | 设置开关状态 |
| `egui_view_switch_set_on_checked_listener(self, listener)` | 设置状态变化回调 |

### 参数宏

```c
EGUI_VIEW_SWITCH_PARAMS_INIT(name, x, y, w, h, is_checked);
```

### 回调原型

```c
typedef void (*egui_view_on_checked_listener_t)(egui_view_t *self, int is_checked);
```

### 代码示例

```c
static egui_view_switch_t sw;

EGUI_VIEW_SWITCH_PARAMS_INIT(sw_params, 0, 0, 96, 38, 0);

static void on_checked(egui_view_t *self, int is_checked)
{
    EGUI_LOG_INF("Switch checked: %d\n", is_checked);
}

void init_ui(void)
{
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&sw), &sw_params);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&sw), on_checked);
    egui_view_set_margin_all(EGUI_VIEW_OF(&sw), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&sw));
}
```

---

## Checkbox

复选框控件，支持选中/未选中状态，可附带文本标签。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=checkbox)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_checkbox_init(self)` | 初始化 Checkbox |
| `egui_view_checkbox_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_checkbox_set_checked(self, is_checked)` | 设置选中状态 |
| `egui_view_checkbox_set_on_checked_listener(self, listener)` | 设置状态变化回调 |
| `egui_view_checkbox_set_text(self, text)` | 设置文本标签 |
| `egui_view_checkbox_set_font(self, font)` | 设置字体 |
| `egui_view_checkbox_set_text_color(self, color)` | 设置文本颜色 |

### 参数宏

```c
// 无文本
EGUI_VIEW_CHECKBOX_PARAMS_INIT(name, x, y, w, h, is_checked);

// 带文本
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(name, x, y, w, h, is_checked, text);
```

### 代码示例

```c
static egui_view_checkbox_t checkbox;

EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(cb_params, 0, 0, 180, 42, 0, "Checkbox M");

static void on_checked(egui_view_t *self, int is_checked)
{
    EGUI_LOG_INF("Checkbox checked: %d\n", is_checked);
}

void init_ui(void)
{
    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&checkbox), &cb_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox), on_checked);
    egui_view_set_margin_all(EGUI_VIEW_OF(&checkbox), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&checkbox));
}
```

---

## RadioButton

单选按钮控件，需配合 RadioGroup 使用以实现同组内互斥选择。可附带文本标签。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=radio_button)

### API

RadioButton API:

| 函数 | 说明 |
|------|------|
| `egui_view_radio_button_init(self)` | 初始化 RadioButton |
| `egui_view_radio_button_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_radio_button_set_checked(self, is_checked)` | 设置选中状态 |
| `egui_view_radio_button_set_text(self, text)` | 设置文本标签 |
| `egui_view_radio_button_set_font(self, font)` | 设置字体 |
| `egui_view_radio_button_set_text_color(self, color)` | 设置文本颜色 |

RadioGroup API:

| 函数 | 说明 |
|------|------|
| `egui_view_radio_group_init(group)` | 初始化 RadioGroup |
| `egui_view_radio_group_add(group, button)` | 将 RadioButton 加入组 |
| `egui_view_radio_group_set_on_changed_listener(group, listener)` | 设置选中变化回调 |

### 参数宏

```c
// 无文本
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT(name, x, y, w, h, is_checked);

// 带文本
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(name, x, y, w, h, is_checked, text);
```

### 回调原型

```c
typedef void (*egui_view_on_radio_changed_listener_t)(egui_view_t *self, int index);
```

### 代码示例

```c
static egui_view_radio_button_t radio_a;
static egui_view_radio_button_t radio_b;
static egui_view_radio_group_t group;

EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(ra_params, 0, 0, 160, 34, 1, "Option A");
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(rb_params, 0, 0, 160, 34, 0, "Option B");

static void on_changed(egui_view_t *self, int index)
{
    EGUI_LOG_INF("Radio selected index: %d\n", index);
}

void init_ui(void)
{
    egui_view_radio_group_init(&group);
    egui_view_radio_group_set_on_changed_listener(&group, on_changed);

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_a), &ra_params);
    egui_view_radio_group_add(&group, EGUI_VIEW_OF(&radio_a));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_b), &rb_params);
    egui_view_radio_group_add(&group, EGUI_VIEW_OF(&radio_b));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&radio_a));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&radio_b));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}
```

---

## ToggleButton

切换按钮控件，带文本标签，点击在开/关状态间切换，支持自定义开/关颜色和圆角。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=toggle_button)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_toggle_button_init(self)` | 初始化 ToggleButton |
| `egui_view_toggle_button_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_toggle_button_set_toggled(self, is_toggled)` | 设置切换状态 |
| `egui_view_toggle_button_is_toggled(self)` | 获取当前切换状态 |
| `egui_view_toggle_button_set_text(self, text)` | 设置文本 |
| `egui_view_toggle_button_set_on_toggled_listener(self, listener)` | 设置切换回调 |

### 参数宏

```c
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(name, x, y, w, h, text, is_toggled);
```

### 回调原型

```c
typedef void (*egui_view_on_toggled_listener_t)(egui_view_t *self, uint8_t is_toggled);
```

### 代码示例

```c
static egui_view_toggle_button_t toggle;

EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_params, 0, 0, 160, 38, "Toggle", 0);

static void on_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_LOG_INF("Toggle toggled: %d\n", is_toggled);
}

void init_ui(void)
{
    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle), &toggle_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle), on_toggled);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&toggle));
}
```
