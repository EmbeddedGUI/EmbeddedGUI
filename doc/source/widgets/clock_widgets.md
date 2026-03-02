# 时钟控件

## 概述

时钟控件提供了多种时间和日期显示方式，适用于仪表盘、运动手表、智能家居等嵌入式场景。包括模拟时钟、数字时钟、秒表和迷你日历四个控件。

## AnalogClock

模拟时钟控件，绘制圆形表盘、时针、分针和可选的秒针，支持刻度显示。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=analog_clock)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_analog_clock_init(self)` | 初始化模拟时钟 |
| `egui_view_analog_clock_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_analog_clock_set_time(self, h, m, s)` | 设置时间(时/分/秒) |
| `egui_view_analog_clock_show_second(self, show)` | 设置是否显示秒针 |
| `egui_view_analog_clock_show_ticks(self, show)` | 设置是否显示刻度 |

### 参数宏

```c
EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(name, x, y, w, h, hour, min, sec);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `hour` | `uint8_t` | 小时(0-11) |
| `minute` | `uint8_t` | 分钟(0-59) |
| `second` | `uint8_t` | 秒(0-59) |
| `show_second` | `uint8_t` | 是否绘制秒针 |
| `show_ticks` | `uint8_t` | 是否绘制刻度 |
| `dial_color` | `egui_color_t` | 表盘颜色 |
| `hour_color` | `egui_color_t` | 时针颜色 |
| `minute_color` | `egui_color_t` | 分针颜色 |
| `second_color` | `egui_color_t` | 秒针颜色 |
| `tick_color` | `egui_color_t` | 刻度颜色 |
| `hour_hand_width` | `egui_dim_t` | 时针宽度 |
| `minute_hand_width` | `egui_dim_t` | 分针宽度 |
| `second_hand_width` | `egui_dim_t` | 秒针宽度 |

### 代码示例

```c
static egui_view_analog_clock_t clock;

EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(clock_params, 10, 10, 100, 100, 10, 30, 0);

void init_ui(void)
{
    egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&clock), &clock_params);
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&clock), 1);
    egui_view_analog_clock_show_ticks(EGUI_VIEW_OF(&clock), 1);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&clock));
}

// 定时更新时间
void update_time(uint8_t h, uint8_t m, uint8_t s)
{
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&clock), h, m, s);
}
```

---

## DigitalClock

数字时钟控件，继承自 Label，以文本形式显示时间，支持 12/24 小时制和冒号闪烁效果。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=digital_clock)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_digital_clock_init(self)` | 初始化数字时钟 |
| `egui_view_digital_clock_init_with_params(self, params)` | 使用参数初始化(复用 Label 参数) |
| `egui_view_digital_clock_set_time(self, hour, minute, second)` | 设置时间 |
| `egui_view_digital_clock_set_format(self, format_24h)` | 设置时间格式(0=12h, 1=24h) |
| `egui_view_digital_clock_set_colon_blink(self, enable)` | 启用冒号闪烁 |
| `egui_view_digital_clock_set_colon_visible(self, visible)` | 设置冒号可见性 |
| `egui_view_digital_clock_set_show_second(self, show)` | 设置是否显示秒 |

### 参数宏

```c
// 复用 Label 参数宏
EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);
EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

DigitalClock 继承 Label，可使用 `egui_view_label_set_font()`、`egui_view_label_set_font_color()` 等函数设置字体样式。

### 代码示例

```c
static egui_view_digital_clock_t dclock;

EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT_SIMPLE(dclock_params, 10, 10, 120, 30, "00:00");

void init_ui(void)
{
    egui_view_digital_clock_init_with_params(EGUI_VIEW_OF(&dclock),
        &dclock_params);
    egui_view_digital_clock_set_format(EGUI_VIEW_OF(&dclock), 1);
    egui_view_digital_clock_set_show_second(EGUI_VIEW_OF(&dclock), 1);
    egui_view_digital_clock_set_colon_blink(EGUI_VIEW_OF(&dclock), 1);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&dclock), 14, 30, 0);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&dclock));
}
```

---

## Stopwatch

秒表控件，继承自 Label，支持计时、暂停、停止和毫秒显示。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=stopwatch)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_stopwatch_init(self)` | 初始化秒表 |
| `egui_view_stopwatch_init_with_params(self, params)` | 使用参数初始化(复用 Label 参数) |
| `egui_view_stopwatch_set_elapsed(self, elapsed_ms)` | 设置已用时间(毫秒) |
| `egui_view_stopwatch_get_elapsed(self)` | 获取已用时间(毫秒) |
| `egui_view_stopwatch_set_state(self, state)` | 设置状态 |
| `egui_view_stopwatch_get_state(self)` | 获取当前状态 |
| `egui_view_stopwatch_set_show_ms(self, show)` | 设置是否显示毫秒 |

### 状态常量

| 常量 | 值 | 说明 |
|------|----|------|
| `EGUI_VIEW_STOPWATCH_STATE_STOPPED` | 0 | 已停止 |
| `EGUI_VIEW_STOPWATCH_STATE_RUNNING` | 1 | 运行中 |
| `EGUI_VIEW_STOPWATCH_STATE_PAUSED` | 2 | 已暂停 |

### 参数宏

```c
// 复用 Label 参数宏
EGUI_VIEW_STOPWATCH_PARAMS_INIT(name, x, y, w, h, text, font, color, alpha);
EGUI_VIEW_STOPWATCH_PARAMS_INIT_SIMPLE(name, x, y, w, h, text);
```

### 代码示例

```c
static egui_view_stopwatch_t sw;

EGUI_VIEW_STOPWATCH_PARAMS_INIT_SIMPLE(sw_params, 10, 10, 140, 30, "00:00.000");

void init_ui(void)
{
    egui_view_stopwatch_init_with_params(EGUI_VIEW_OF(&sw), &sw_params);
    egui_view_stopwatch_set_show_ms(EGUI_VIEW_OF(&sw), 1);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&sw));
}

void start_stopwatch(void)
{
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&sw),
        EGUI_VIEW_STOPWATCH_STATE_RUNNING);
}

void pause_stopwatch(void)
{
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&sw),
        EGUI_VIEW_STOPWATCH_STATE_PAUSED);
}

void reset_stopwatch(void)
{
    egui_view_stopwatch_set_state(EGUI_VIEW_OF(&sw),
        EGUI_VIEW_STOPWATCH_STATE_STOPPED);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&sw), 0);
}
```

---

## MiniCalendar

迷你日历控件，显示月历网格，支持日期选择、今日高亮和周起始日设置。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=mini_calendar)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_mini_calendar_init(self)` | 初始化日历 |
| `egui_view_mini_calendar_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_mini_calendar_set_date(self, year, month, day)` | 设置日期(选中日) |
| `egui_view_mini_calendar_set_today(self, day)` | 设置今日高亮 |
| `egui_view_mini_calendar_set_first_day_of_week(self, day)` | 设置周起始日(0=周日, 1=周一) |
| `egui_view_mini_calendar_set_on_date_selected_listener(self, listener)` | 设置日期选择回调 |

### 参数宏

```c
EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(name, x, y, w, h, year, month, day);
```

### 结构体字段

| 字段 | 类型 | 说明 |
|------|------|------|
| `year` | `uint16_t` | 年份 |
| `month` | `uint8_t` | 月份(1-12) |
| `day` | `uint8_t` | 选中日(1-31) |
| `today_day` | `uint8_t` | 今日高亮 |
| `first_day_of_week` | `uint8_t` | 周起始日 |
| `header_color` | `egui_color_t` | 表头颜色 |
| `text_color` | `egui_color_t` | 文本颜色 |
| `today_color` | `egui_color_t` | 今日标记颜色 |
| `selected_color` | `egui_color_t` | 选中日颜色 |
| `weekend_color` | `egui_color_t` | 周末颜色 |

### 代码示例

```c
static egui_view_mini_calendar_t calendar;

EGUI_VIEW_MINI_CALENDAR_PARAMS_INIT(cal_params, 10, 10, 200, 180, 2026, 2, 27);

static void on_date_selected(egui_view_t *self, uint8_t day)
{
    EGUI_LOG_INF("Selected day: %d\n", day);
}

void init_ui(void)
{
    egui_view_mini_calendar_init_with_params(EGUI_VIEW_OF(&calendar), &cal_params);
    egui_view_mini_calendar_set_today(EGUI_VIEW_OF(&calendar), 27);
    egui_view_mini_calendar_set_first_day_of_week(EGUI_VIEW_OF(&calendar), 1);
    egui_view_mini_calendar_set_on_date_selected_listener(
        EGUI_VIEW_OF(&calendar), on_date_selected);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&calendar));
}
```
