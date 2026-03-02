# 基础控件

## 概述

基础控件是 EmbeddedGUI 中最常用的 UI 元素，包括所有控件的基类 View，以及文本标签、图片、按钮、分割线等。这些控件构成了大多数嵌入式 UI 界面的基础骨架。

## View (基类)

所有控件的基类，提供位置、大小、可见性、点击事件等通用能力。

### 效果展示

View 作为基类不单独展示，其能力通过子控件体现。

### API

| 函数 | 说明 |
|------|------|
| `egui_view_init(self)` | 初始化 View |
| `egui_view_set_position(self, x, y)` | 设置位置 |
| `egui_view_set_size(self, width, height)` | 设置大小 |
| `egui_view_set_visible(self, is_visible)` | 设置可见性 |
| `egui_view_get_visible(self)` | 获取可见性 |
| `egui_view_set_gone(self, is_gone)` | 设置是否隐藏(不占布局) |
| `egui_view_set_enable(self, is_enable)` | 设置是否启用 |
| `egui_view_set_clickable(self, is_clickable)` | 设置是否可点击 |
| `egui_view_set_alpha(self, alpha)` | 设置透明度 |
| `egui_view_set_background(self, background)` | 设置背景 |
| `egui_view_set_on_click_listener(self, listener)` | 设置点击回调 |
| `egui_view_set_on_touch_listener(self, listener)` | 设置触摸回调 |
| `egui_view_set_padding(self, left, right, top, bottom)` | 设置内边距 |
| `egui_view_set_padding_all(self, padding)` | 设置统一内边距 |
| `egui_view_set_margin(self, left, right, top, bottom)` | 设置外边距 |
| `egui_view_set_margin_all(self, margin)` | 设置统一外边距 |
| `egui_view_invalidate(self)` | 标记需要重绘 |
| `egui_view_request_layout(self)` | 请求重新布局 |
| `egui_view_set_shadow(self, shadow)` | 设置阴影效果 |
| `egui_view_set_layer(self, layer)` | 设置图层(需开启 EGUI_CONFIG_FUNCTION_SUPPORT_LAYER) |
| `egui_view_set_focusable(self, is_focusable)` | 设置是否可聚焦(需开启 EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS) |
| `egui_view_request_focus(self)` | 请求焦点 |

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 启用触摸支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 启用按键支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` | 启用焦点支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_LAYER` | 启用图层支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW` | 启用阴影支持 |
| `EGUI_CONFIG_DEBUG_CLASS_NAME` | 启用调试类名 |

---

## Label

单行/多行文本标签，支持字体、颜色、对齐方式设置。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=label)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_label_init(self)` | 初始化 Label |
| `egui_view_label_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_label_set_text(self, text)` | 设置文本内容 |
| `egui_view_label_set_font(self, font)` | 设置字体 |
| `egui_view_label_set_font_with_std_height(self, font)` | 设置字体并自动调整高度 |
| `egui_view_label_set_font_color(self, color, alpha)` | 设置字体颜色和透明度 |
| `egui_view_label_set_align_type(self, align_type)` | 设置对齐方式 |
| `egui_view_label_set_line_space(self, line_space)` | 设置行间距 |
| `egui_view_label_get_str_size(self, string, width, height)` | 获取字符串渲染尺寸 |

### 参数宏

```c
// 完整参数初始化
EGUI_VIEW_LABEL_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);

// 简化参数初始化(使用默认字体和白色)
EGUI_VIEW_LABEL_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

### 代码示例

```c
static egui_view_label_t label;

EGUI_VIEW_LABEL_PARAMS_INIT(label_params, 0, 0, 200, 30,
    "Label S", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

void init_ui(void)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label), &label_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label));
}
```

---

## Image

图片显示控件，支持普通模式和缩放模式，可设置图片着色。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=image)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_image_init(self)` | 初始化 Image |
| `egui_view_image_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_image_set_image(self, image)` | 设置图片资源 |
| `egui_view_image_set_image_type(self, image_type)` | 设置显示模式 |
| `egui_view_image_set_image_color(self, color, alpha)` | 设置图片着色 |

### 参数宏

```c
EGUI_VIEW_IMAGE_PARAMS_INIT(name, x, y, w, h, image);
```

### 图片类型常量

| 常量 | 说明 |
|------|------|
| `EGUI_VIEW_IMAGE_TYPE_NORMAL` | 原始尺寸显示 |
| `EGUI_VIEW_IMAGE_TYPE_RESIZE` | 缩放至控件大小 |

### 代码示例

```c
static egui_view_image_t image;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

EGUI_VIEW_IMAGE_PARAMS_INIT(image_params, 0, 0, 96, 96,
    (egui_image_t *)&egui_res_image_star_rgb565_8);

void init_ui(void)
{
    egui_view_image_init_with_params(EGUI_VIEW_OF(&image), &image_params);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&image), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&image));
}
```

---

## Button

文本按钮，继承自 Label，增加了按下态视觉反馈。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=button)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_button_init(self)` | 初始化 Button |
| `egui_view_button_init_with_params(self, params)` | 使用参数初始化 |

Button 继承 Label 的全部 API，可直接使用 `egui_view_label_set_text()`、`egui_view_label_set_font()` 等函数。点击事件通过基类 `egui_view_set_on_click_listener()` 设置。

### 参数宏

```c
// 复用 Label 参数宏
EGUI_VIEW_BUTTON_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);
EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

### 代码示例

```c
static egui_view_button_t button;

EGUI_VIEW_BUTTON_PARAMS_INIT(button_params, 0, 0, 160, 38,
    NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

static void on_click(egui_view_t *self)
{
    EGUI_LOG_INF("Button clicked\n");
}

void init_ui(void)
{
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button), &button_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button), "Click Me");
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button), on_click);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&button));
}
```

---

## ImageButton

图片按钮，继承自 Image，增加了按下态视觉反馈。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=image_button)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_image_button_init(self)` | 初始化 ImageButton |
| `egui_view_image_button_init_with_params(self, params)` | 使用参数初始化 |

ImageButton 继承 Image 的全部 API，可直接使用 `egui_view_image_set_image()`、`egui_view_image_set_image_type()` 等函数。

### 参数宏

```c
// 复用 Image 参数宏
EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(name, x, y, w, h, image);
```

### 代码示例

```c
static egui_view_image_button_t imgbtn;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

EGUI_VIEW_IMAGE_BUTTON_PARAMS_INIT(imgbtn_params, 0, 0, 82, 82,
    (egui_image_t *)&egui_res_image_star_rgb565_8);

static void on_click(egui_view_t *self)
{
    EGUI_LOG_INF("Image Button Clicked\n");
}

void init_ui(void)
{
    egui_view_image_button_init_with_params(EGUI_VIEW_OF(&imgbtn), &imgbtn_params);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&imgbtn), EGUI_VIEW_IMAGE_TYPE_RESIZE);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&imgbtn), on_click);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&imgbtn));
}
```

---

## Divider

分割线控件，用于在列表或布局中分隔内容区域。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=divider)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_divider_init(self)` | 初始化 Divider |
| `egui_view_divider_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_divider_set_color(self, color)` | 设置分割线颜色 |

### 参数宏

```c
EGUI_VIEW_DIVIDER_PARAMS_INIT(name, x, y, w, h, color);
```

### 代码示例

```c
static egui_view_divider_t divider;

EGUI_VIEW_DIVIDER_PARAMS_INIT(divider_params, 0, 0, 168, 3, EGUI_THEME_PRIMARY);

void init_ui(void)
{
    egui_view_divider_init_with_params(EGUI_VIEW_OF(&divider), &divider_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&divider), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&divider));
}
```

---

## Line

折线/多段线控件，通过一组点坐标绘制连续线段，支持线宽和圆角端点。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=line)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_line_init(self)` | 初始化 Line |
| `egui_view_line_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_line_set_points(self, points, count)` | 设置点数组 |
| `egui_view_line_set_line_width(self, width)` | 设置线宽 |
| `egui_view_line_set_line_color(self, color)` | 设置线条颜色 |
| `egui_view_line_set_use_round_cap(self, enable)` | 启用圆角端点 |

### 参数宏

```c
EGUI_VIEW_LINE_PARAMS_INIT(name, x, y, w, h, line_width, line_color);
```

### 代码示例

```c
static egui_view_line_t line;

static const egui_view_line_point_t zigzag_points[] = {
    {0, 40}, {30, 0}, {60, 40}, {90, 0}, {120, 40},
};

EGUI_VIEW_LINE_PARAMS_INIT(line_params, 0, 0, 130, 50, 2, EGUI_THEME_PRIMARY);

void init_ui(void)
{
    egui_view_line_init_with_params(EGUI_VIEW_OF(&line), &line_params);
    egui_view_line_set_points(EGUI_VIEW_OF(&line), zigzag_points, 5);
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&line), 1);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&line));
}
```
