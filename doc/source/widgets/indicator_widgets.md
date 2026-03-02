# 指示控件

## 概述

指示控件用于以图形化方式展示数值、方向和状态信息，适用于仪表盘、传感器监控、工业控制等嵌入式场景。包括仪表盘、刻度尺、指南针、心率和 LED 指示灯五个控件。

## Gauge

弧形仪表盘控件，通过弧形进度条和指针显示 0-100 的数值。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=gauge)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_gauge_init(self)` | 初始化仪表盘 |
| `egui_view_gauge_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_gauge_set_value(self, value)` | 设置数值(0-100) |

### 参数宏

```c
EGUI_VIEW_GAUGE_PARAMS_INIT(name, x, y, w, h, value);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `value` | `uint8_t` | 当前值(0-100) |
| `stroke_width` | `egui_dim_t` | 弧线宽度 |
| `start_angle` | `int16_t` | 起始角度(默认 150, 约 7 点钟方向) |
| `sweep_angle` | `int16_t` | 扫过角度(默认 240) |
| `bk_color` | `egui_color_t` | 背景弧线颜色 |
| `progress_color` | `egui_color_t` | 进度弧线颜色 |
| `needle_color` | `egui_color_t` | 指针颜色 |

### 代码示例

```c
static egui_view_gauge_t gauge;

EGUI_VIEW_GAUGE_PARAMS_INIT(gauge_params, 10, 10, 100, 100, 75);

void init_ui(void)
{
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&gauge), &gauge_params);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&gauge));
}

void update_gauge(uint8_t val)
{
    egui_view_gauge_set_value(EGUI_VIEW_OF(&gauge), val);
}
```

---

## Scale

刻度尺控件，显示带有主/次刻度线的线性标尺，支持水平和垂直方向，可选数值标签和指示器。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=scale)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_scale_init(self)` | 初始化刻度尺 |
| `egui_view_scale_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_scale_set_range(self, min, max)` | 设置量程范围 |
| `egui_view_scale_set_value(self, value)` | 设置当前值 |
| `egui_view_scale_set_ticks(self, major_count, minor_count)` | 设置主/次刻度数量 |
| `egui_view_scale_set_orientation(self, is_horizontal)` | 设置方向(0=垂直, 1=水平) |
| `egui_view_scale_set_tick_color(self, color)` | 设置刻度颜色 |
| `egui_view_scale_set_label_color(self, color)` | 设置标签颜色 |
| `egui_view_scale_set_indicator_color(self, color)` | 设置指示器颜色 |
| `egui_view_scale_show_labels(self, show)` | 显示/隐藏数值标签 |
| `egui_view_scale_show_indicator(self, show)` | 显示/隐藏指示器 |
| `egui_view_scale_set_font(self, font)` | 设置标签字体 |

### 参数宏

```c
EGUI_VIEW_SCALE_PARAMS_INIT(name, x, y, w, h, min, max, major_tick_count);
```

### 代码示例

```c
static egui_view_scale_t scale;

EGUI_VIEW_SCALE_PARAMS_INIT(scale_params, 10, 10, 200, 40, 0, 100, 5);

void init_ui(void)
{
    egui_view_scale_init_with_params(EGUI_VIEW_OF(&scale), &scale_params);
    egui_view_scale_set_ticks(EGUI_VIEW_OF(&scale), 5, 4);
    egui_view_scale_set_orientation(EGUI_VIEW_OF(&scale), 1);
    egui_view_scale_show_labels(EGUI_VIEW_OF(&scale), 1);
    egui_view_scale_show_indicator(EGUI_VIEW_OF(&scale), 1);
    egui_view_scale_set_value(EGUI_VIEW_OF(&scale), 60);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scale));
}
```

---

## Compass

指南针控件，显示圆形表盘和方向指针，支持角度文字显示。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=compass)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_compass_init(self)` | 初始化指南针 |
| `egui_view_compass_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_compass_set_heading(self, heading)` | 设置方位角(0-359) |
| `egui_view_compass_set_show_degree(self, show)` | 设置是否显示角度文字 |

### 参数宏

```c
EGUI_VIEW_COMPASS_PARAMS_INIT(name, x, y, w, h, heading);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `heading` | `int16_t` | 方位角(0-359) |
| `stroke_width` | `egui_dim_t` | 表盘线宽 |
| `dial_color` | `egui_color_t` | 表盘颜色 |
| `north_color` | `egui_color_t` | 北方标记颜色 |
| `needle_color` | `egui_color_t` | 指针颜色 |
| `text_color` | `egui_color_t` | 文字颜色 |
| `show_degree` | `uint8_t` | 是否显示角度 |

### 代码示例

```c
static egui_view_compass_t compass;

EGUI_VIEW_COMPASS_PARAMS_INIT(compass_params, 10, 10, 100, 100, 0);

void init_ui(void)
{
    egui_view_compass_init_with_params(EGUI_VIEW_OF(&compass), &compass_params);
    egui_view_compass_set_show_degree(EGUI_VIEW_OF(&compass), 1);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&compass));
}

void update_heading(int16_t deg)
{
    egui_view_compass_set_heading(EGUI_VIEW_OF(&compass), deg);
}
```

---

## HeartRate

心率显示控件，绘制心形图标和 BPM 数值，支持脉搏动画效果。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=heart_rate)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_heart_rate_init(self)` | 初始化心率控件 |
| `egui_view_heart_rate_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_heart_rate_set_bpm(self, bpm)` | 设置心率值 |
| `egui_view_heart_rate_set_animate(self, enable)` | 启用/禁用脉搏动画 |
| `egui_view_heart_rate_set_heart_color(self, color)` | 设置心形颜色 |
| `egui_view_heart_rate_set_pulse_phase(self, phase)` | 设置脉搏动画相位 |

### 参数宏

```c
EGUI_VIEW_HEART_RATE_PARAMS_INIT(name, x, y, w, h, bpm);
```

### 代码示例

```c
static egui_view_heart_rate_t hr;

EGUI_VIEW_HEART_RATE_PARAMS_INIT(hr_params, 10, 10, 80, 80, 72);

void init_ui(void)
{
    egui_view_heart_rate_init_with_params(EGUI_VIEW_OF(&hr), &hr_params);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&hr), 1);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&hr));
}

void update_bpm(uint8_t bpm)
{
    egui_view_heart_rate_set_bpm(EGUI_VIEW_OF(&hr), bpm);
}
```

---

## Led

LED 指示灯控件，显示圆形灯珠，支持开/关状态切换和闪烁效果。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=led)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_led_init(self)` | 初始化 LED |
| `egui_view_led_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_led_set_on(self)` | 点亮 LED |
| `egui_view_led_set_off(self)` | 熄灭 LED |
| `egui_view_led_toggle(self)` | 切换开关状态 |
| `egui_view_led_set_blink(self, period_ms)` | 启用闪烁(指定周期毫秒) |
| `egui_view_led_stop_blink(self)` | 停止闪烁 |
| `egui_view_led_set_colors(self, on_color, off_color)` | 设置亮/灭颜色 |

### 参数宏

```c
EGUI_VIEW_LED_PARAMS_INIT(name, x, y, w, h, is_on);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `is_on` | `uint8_t` | 当前开关状态 |
| `is_blinking` | `uint8_t` | 是否正在闪烁 |
| `blink_period` | `uint16_t` | 闪烁周期(毫秒) |
| `on_color` | `egui_color_t` | 点亮颜色 |
| `off_color` | `egui_color_t` | 熄灭颜色 |
| `border_color` | `egui_color_t` | 边框颜色 |
| `border_width` | `egui_dim_t` | 边框宽度 |

### 代码示例

```c
static egui_view_led_t led_green;
static egui_view_led_t led_red;

EGUI_VIEW_LED_PARAMS_INIT(led_g_params, 10, 10, 20, 20, 1);
EGUI_VIEW_LED_PARAMS_INIT(led_r_params, 40, 10, 20, 20, 0);

void init_ui(void)
{
    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_green), &led_g_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_green), EGUI_COLOR_GREEN, EGUI_COLOR_DARK_GREEN);

    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_red), &led_r_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_red), EGUI_COLOR_RED, EGUI_COLOR_DARK_RED);
    egui_view_led_set_blink(EGUI_VIEW_OF(&led_red), 500);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&led_green));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&led_red));
}
```
