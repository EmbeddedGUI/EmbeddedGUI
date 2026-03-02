# 进度控件

## 概述

进度控件用于展示任务完成度或加载状态。ProgressBar 是水平进度条，CircularProgressBar 是环形进度条，ActivityRing 是多环活动圆环(类似 Apple Watch 健身环)，Spinner 是旋转加载指示器。

## ProgressBar

水平进度条控件，显示 0-100 的进度值。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=progress_bar)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_progress_bar_init(self)` | 初始化 ProgressBar |
| `egui_view_progress_bar_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_progress_bar_set_process(self, process)` | 设置进度值(0-100) |
| `egui_view_progress_bar_set_on_progress_listener(self, listener)` | 设置进度变化回调 |

### 参数宏

```c
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(name, x, y, w, h, process);
```

### 回调原型

```c
typedef void (*egui_view_on_progress_changed_listener_t)(egui_view_t *self, uint8_t progress);
```

### 代码示例

```c
static egui_view_progress_bar_t progress;

EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(pb_params, 0, 0, 180, 20, 75);

void init_ui(void)
{
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&progress), &pb_params);
    egui_view_set_margin_all(EGUI_VIEW_OF(&progress), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&progress));
}

// 运行时更新进度
void update_progress(uint8_t val)
{
    egui_view_progress_bar_set_process(EGUI_VIEW_OF(&progress), val);
}
```

---

## CircularProgressBar

环形进度条控件，以圆弧形式显示 0-100 的进度值，支持自定义线宽和颜色。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=circular_progress_bar)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_circular_progress_bar_init(self)` | 初始化 CircularProgressBar |
| `egui_view_circular_progress_bar_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_circular_progress_bar_set_process(self, process)` | 设置进度值(0-100) |
| `egui_view_circular_progress_bar_set_stroke_width(self, stroke_width)` | 设置圆弧线宽 |
| `egui_view_circular_progress_bar_set_progress_color(self, color)` | 设置进度颜色 |
| `egui_view_circular_progress_bar_set_bk_color(self, color)` | 设置背景轨道颜色 |
| `egui_view_circular_progress_bar_set_on_progress_listener(self, listener)` | 设置进度变化回调 |

### 参数宏

```c
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(name, x, y, w, h, process);
```

### 代码示例

```c
static egui_view_circular_progress_bar_t cpb;

EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(cpb_params, 0, 0, 96, 96, 75);

static void on_progress(egui_view_t *self, uint8_t progress)
{
    EGUI_LOG_INF("Circular Progress: %d\n", progress);
}

void init_ui(void)
{
    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&cpb), &cpb_params);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&cpb), 6);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&cpb), EGUI_THEME_TRACK_BG);
    egui_view_circular_progress_bar_set_progress_color(EGUI_VIEW_OF(&cpb), EGUI_THEME_PRIMARY);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&cpb), on_progress);
    egui_view_set_margin_all(EGUI_VIEW_OF(&cpb), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&cpb));
}
```

---

## ActivityRing

多环活动圆环控件，最多支持 3 个同心环，每个环独立设置进度值和颜色，类似 Apple Watch 健身环效果。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=activity_ring)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_activity_ring_init(self)` | 初始化 ActivityRing |
| `egui_view_activity_ring_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_activity_ring_set_value(self, ring_index, value)` | 设置指定环的进度值(0-100) |
| `egui_view_activity_ring_get_value(self, ring_index)` | 获取指定环的进度值 |
| `egui_view_activity_ring_set_ring_count(self, count)` | 设置环数量(最大 3) |
| `egui_view_activity_ring_set_ring_color(self, ring_index, color)` | 设置指定环的前景色 |
| `egui_view_activity_ring_set_ring_bg_color(self, ring_index, color)` | 设置指定环的背景色 |
| `egui_view_activity_ring_set_stroke_width(self, stroke_width)` | 设置环线宽 |
| `egui_view_activity_ring_set_ring_gap(self, ring_gap)` | 设置环间距 |
| `egui_view_activity_ring_set_start_angle(self, start_angle)` | 设置起始角度 |
| `egui_view_activity_ring_set_show_round_cap(self, show)` | 启用/禁用圆角端点 |

### 参数宏

```c
EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_activity_ring_t ring;

EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(ring_params, 0, 0, 116, 116);

void init_ui(void)
{
    egui_view_activity_ring_init_with_params(EGUI_VIEW_OF(&ring), &ring_params);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&ring), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&ring), 3);

    // 设置 3 个环的进度值
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring), 0, 75);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring), 1, 50);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring), 2, 30);

    // 设置各环颜色
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring), 0,
        EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring), 1,
        EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring), 2,
        EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));

    // 设置各环背景色
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring), 0,
        EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring), 1,
        EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring), 2,
        EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));

    egui_core_add_user_root_view(EGUI_VIEW_OF(&ring));
}
```

### 配置选项

| 宏 | 说明 |
|----|------|
| `EGUI_VIEW_ACTIVITY_RING_MAX_RINGS` | 最大环数量，默认 3 |

---

## Spinner

旋转加载指示器控件，通过内置定时器驱动弧线旋转动画，用于表示加载中状态。

### 效果展示

[在线演示](https://embeddedgui.github.io/EmbeddedGUI/basic.html?app=spinner)

### API

| 函数 | 说明 |
|------|------|
| `egui_view_spinner_init(self)` | 初始化 Spinner |
| `egui_view_spinner_init_with_params(self, params)` | 使用参数初始化 |
| `egui_view_spinner_start(self)` | 启动旋转动画 |
| `egui_view_spinner_stop(self)` | 停止旋转动画 |
| `egui_view_spinner_set_color(self, color)` | 设置旋转弧线颜色 |

### 参数宏

```c
EGUI_VIEW_SPINNER_PARAMS_INIT(name, x, y, w, h);
```

### 代码示例

```c
static egui_view_spinner_t spinner;

EGUI_VIEW_SPINNER_PARAMS_INIT(spinner_params, 0, 0, 62, 62);

void init_ui(void)
{
    egui_view_spinner_init_with_params(EGUI_VIEW_OF(&spinner), &spinner_params);
    egui_view_spinner_start(EGUI_VIEW_OF(&spinner));
    egui_view_set_margin_all(EGUI_VIEW_OF(&spinner), 6);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&spinner));
}

// 加载完成后停止
void on_load_complete(void)
{
    egui_view_spinner_stop(EGUI_VIEW_OF(&spinner));
}
```
