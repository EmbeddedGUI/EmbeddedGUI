# Timer 驱动的自定义动画

## 概述

EmbeddedGUI 的 Animation 系统适合标准的属性动画（透明度、位移、缩放等），但有些场景需要更灵活的控制，例如：

- 数值递增动画（KPI 数字从 0 跳到目标值）
- 图表数据逐帧生长
- 自定义轨迹运动
- 多个不相关属性的协调变化

这些场景可以使用 Timer API 实现自定义动画逻辑。

## Timer API

Timer 是 EmbeddedGUI 的核心定时器机制，位于 `src/core/egui_timer.h`。

### 结构体

```c
struct egui_timer
{
    struct egui_timer *next;
    uint32_t expiry_time;
    egui_timer_callback_func callback;
    uint32_t period;
    void *user_data;
};

typedef void (*egui_timer_callback_func)(egui_timer_t *);
```

### 核心 API

| API | 说明 |
|-----|------|
| `egui_timer_init_timer(handle, user_data, callback)` | 初始化定时器 |
| `egui_timer_start_timer(handle, ms, period)` | 启动定时器，ms=首次延迟，period=周期间隔 |
| `egui_timer_stop_timer(handle)` | 停止定时器 |
| `egui_timer_check_timer_start(handle)` | 检查定时器是否已启动 |
| `egui_timer_get_current_time()` | 获取当前系统时间（毫秒） |

### 参数说明

- `ms`：首次触发的延迟时间（毫秒）
- `period`：后续每次触发的间隔（毫秒）。设为 0 表示单次触发
- `user_data`：用户自定义数据指针，可在回调中通过 `handle->user_data` 访问

## 用 Timer 实现自定义动画

### 基本模式

```c
static egui_timer_t my_timer;
static int frame = 0;
#define TOTAL_FRAMES 20
#define FRAME_INTERVAL 40  // 40ms = 25fps

static void animation_callback(egui_timer_t *timer)
{
    (void)timer;
    frame++;
    if (frame > TOTAL_FRAMES)
    {
        egui_timer_stop_timer(&my_timer);
        return;
    }

    // 计算进度 0~100
    int progress = (frame * 100) / TOTAL_FRAMES;

    // 在这里更新 UI 状态
    update_ui(progress);
}

void start_custom_animation(void)
{
    frame = 0;
    egui_timer_init_timer(&my_timer, NULL, animation_callback);
    egui_timer_start_timer(&my_timer, FRAME_INTERVAL, FRAME_INTERVAL);
}
```

### 示例：数值递增动画

```c
static egui_timer_t counter_timer;
static int counter_frame = 0;
static char value_buf[16];
#define COUNTER_FRAMES 30
#define COUNTER_INTERVAL 33  // ~30fps
#define TARGET_VALUE 256

static void counter_callback(egui_timer_t *timer)
{
    (void)timer;
    counter_frame++;
    if (counter_frame > COUNTER_FRAMES)
    {
        egui_timer_stop_timer(&counter_timer);
        return;
    }

    int progress = (counter_frame * 100) / COUNTER_FRAMES;
    int value = (TARGET_VALUE * progress) / 100;

    sprintf(value_buf, "%d", value);
    egui_view_label_set_text(EGUI_VIEW_OF(&my_label), value_buf);
}
```

## Timer + 定点数运算实现平滑缓动

Timer 回调中可以手动实现缓动函数，使动画更自然。以下是减速缓动的实现：

```c
static void eased_callback(egui_timer_t *timer)
{
    (void)timer;
    frame++;
    if (frame > TOTAL_FRAMES)
    {
        egui_timer_stop_timer(&my_timer);
        return;
    }

    int progress = (frame * 100) / TOTAL_FRAMES;

    // 减速缓动: t' = 1 - (1-t)^2
    int t_inv = 100 - progress;
    int eased = 100 - (t_inv * t_inv) / 100;

    // 用 eased 值（0~100）驱动 UI 更新
    int value = (target * eased) / 100;
    update_display(value);
}
```

也可以使用项目的定点数工具（`egui_fixmath.h`）进行更精确的计算：

```c
// 定点数乘法
egui_float_t result = EGUI_FLOAT_MULT(a, b);

// 定点数除法
egui_float_t result = EGUI_FLOAT_DIV(a, b);

// 浮点字面量转定点数
egui_float_t val = EGUI_FLOAT_VALUE(0.5f);
```

## 与 Animation 系统的区别

| 特性 | Animation 系统 | Timer 自定义动画 |
|------|---------------|-----------------|
| 适用场景 | 单一属性变化（alpha/translate/scale） | 复杂多属性联动、自定义逻辑 |
| 插值器 | 内置 9 种，自动应用 | 需手动实现缓动公式 |
| 目标绑定 | 自动绑定到 View | 手动操作任意对象 |
| 回调 | start/repeat/end | 每帧回调，完全自由 |
| 重复模式 | RESTART/REVERSE 自动管理 | 手动管理状态 |
| 代码量 | 少，声明式 | 多，命令式 |
| 内存开销 | 动画对象 40~60 字节 | Timer 对象约 20 字节 |

## 选型建议

- 标准属性动画（淡入、滑动、缩放）：优先使用 Animation 系统，代码简洁且经过充分测试
- 数值动画（计数器、进度数字）：使用 Timer，因为 Animation 系统不直接支持数值插值
- 图表数据动画（折线图生长、饼图展开）：使用 Timer，需要逐帧更新数据数组
- 复合动效（多个不相关 View 的协调动画）：Timer 更灵活，一个回调中可以同时更新多个 View
- 需要精确帧控制的场景：Timer 提供确定性的帧间隔

两者也可以混合使用：用 Animation 处理 View 的入场滑动，同时用 Timer 驱动数据的渐变填充。
