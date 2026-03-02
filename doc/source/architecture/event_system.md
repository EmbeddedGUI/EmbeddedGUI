# 事件循环与输入处理

## 概述

EmbeddedGUI 采用轮询式事件循环，由 `egui_polling_work()` 驱动。输入事件（触摸、键盘）通过队列缓冲，在主循环中统一分发到视图树。这种设计避免了中断上下文中的复杂处理，适合嵌入式系统的单线程模型。

## 主循环

### egui_polling_work() 工作流程

```
egui_polling_work()
    |
    +-- egui_timer_polling_work()        // 1. 处理定时器和动画
    |
    +-- egui_input_polling_work()        // 2. 处理触摸输入队列
    |
    +-- egui_input_key_dispatch_work()   // 3. 处理键盘输入队列
```

对应源码（`src/core/egui_core.c`）：

```c
void egui_polling_work(void)
{
    egui_timer_polling_work();

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    if (!egui_input_check_idle())
    {
        egui_input_polling_work();
    }
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    if (!egui_input_check_key_idle())
    {
        egui_input_key_dispatch_work();
    }
#endif
}
```

### 完整帧循环

在 porting 层，典型的主循环如下：

```c
while (1)
{
    egui_polling_work();                  // 处理事件和定时器
    if (egui_check_need_refresh())        // 检查是否有脏区域
    {
        egui_core_draw_view_group_pre_work();  // 计算布局
        egui_polling_refresh_display();        // PFB 分块渲染
    }
}
```

## 触摸事件

### 事件类型

定义在 `src/core/egui_motion_event.h`：

| 事件类型 | 值 | 说明 |
|----------|-----|------|
| `EGUI_MOTION_EVENT_ACTION_DOWN` | 1 | 手指按下 |
| `EGUI_MOTION_EVENT_ACTION_UP` | 2 | 手指抬起 |
| `EGUI_MOTION_EVENT_ACTION_MOVE` | 3 | 手指移动 |
| `EGUI_MOTION_EVENT_ACTION_CANCEL` | 4 | 事件取消 |

启用多点触控后还支持 `ACTION_POINTER_DOWN`、`ACTION_POINTER_UP` 和 `ACTION_SCROLL`。

### 事件结构体

```c
struct egui_motion_event
{
    egui_snode_t node;            // 链表节点
    uint8_t type;                 // 事件类型
    uint32_t timestamp;           // 时间戳 (ms)
    egui_location_t location;     // 屏幕坐标
};
```

### 输入注入

外部（中断或驱动）通过以下 API 向输入队列添加事件：

```c
// 添加触摸事件
int egui_input_add_motion(uint8_t type, egui_dim_t x, egui_dim_t y);

// 多点触控
int egui_input_add_motion_multi(uint8_t type, uint8_t pointer_count,
                                 egui_dim_t x1, egui_dim_t y1,
                                 egui_dim_t x2, egui_dim_t y2);
```

事件存储在基于对象池的单向链表中，连续的 MOVE 事件会自动合并以减少队列压力。

### Velocity Tracker

系统内置速度追踪器 `egui_velocity_tracker_t`，在每次 `add_motion` 时自动记录采样点。抬手时可获取滑动速度，用于惯性滚动等效果：

```c
egui_float_t vx = egui_input_get_velocity_x();  // 像素/毫秒
egui_float_t vy = egui_input_get_velocity_y();
```

追踪器保留最近 200ms 内的采样点（`EGUI_VELOCITY_TRACKER_LONGEST_PAST_TIME`），通过线性回归计算速度。

## 事件分发机制

触摸事件的分发遵循 Android 风格的责任链模式，从根视图向下传递：

```
egui_core_process_input_motion(event)
    |
    v
root_view_group.dispatch_touch_event(event)
    |
    +-- on_intercept_touch_event()    // group 是否拦截？
    |     |-- 返回 1: group 自己处理
    |     |-- 返回 0: 继续向下分发
    |
    +-- 遍历子视图（从后往前，即 Z 轴最高的优先）
    |     +-- 命中测试：event 坐标是否在子视图区域内
    |     +-- child.dispatch_touch_event(event)
    |           |-- on_touch_listener 优先处理
    |           |-- on_touch_event 默认处理
    |
    +-- 无子视图消费 -> group.on_touch_event(event)
```

### 点击事件

当一个 clickable 的视图收到完整的 DOWN -> UP 序列时，触发点击：

```c
int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    if (self->is_clickable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            egui_view_set_pressed(self, true);
            break;
        case EGUI_MOTION_EVENT_ACTION_UP:
            egui_view_set_pressed(self, false);
            egui_view_perform_click(self);  // 触发 on_click_listener
            break;
        }
        return 1;  // 消费事件
    }
    return 0;
}
```

## 键盘事件

启用 `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` 后支持键盘输入。

### 事件类型

| 事件类型 | 说明 |
|----------|------|
| `EGUI_KEY_EVENT_ACTION_DOWN` | 按键按下 |
| `EGUI_KEY_EVENT_ACTION_UP` | 按键释放 |
| `EGUI_KEY_EVENT_ACTION_LONG_PRESS` | 长按 |
| `EGUI_KEY_EVENT_ACTION_REPEAT` | 重复 |

### 键码

系统定义了完整的键码枚举（`enum egui_key_code`），包括：
- 导航键：UP / DOWN / LEFT / RIGHT / TAB
- 动作键：ENTER / ESCAPE / BACKSPACE / DELETE / SPACE
- 数字键：0-9
- 字母键：A-Z
- 特殊字符：PERIOD / COMMA / MINUS 等

### 键盘事件分发

```c
// 注入键盘事件
int egui_input_add_key(uint8_t type, uint8_t key_code,
                        uint8_t is_shift, uint8_t is_ctrl);
```

分发流程：
1. TAB 键触发焦点导航（需启用 `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS`）
2. 有焦点视图时，事件分发到焦点视图
3. 无焦点时，事件分发到根视图组
4. ENTER 键在 clickable 视图上触发点击

## 点击监听器和触摸监听器

### 点击监听器

最常用的交互方式，设置后视图自动变为 clickable：

```c
static void on_my_button_click(egui_view_t *self)
{
    // 处理点击
}

egui_view_set_on_click_listener(EGUI_VIEW_OF(&my_button), on_my_button_click);
```

### 触摸监听器

需要处理原始触摸事件时使用，优先级高于默认的 `on_touch_event`：

```c
static int on_my_view_touch(egui_view_t *self, egui_motion_event_t *event)
{
    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        // 处理按下
        return 1;  // 返回 1 表示消费事件
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        // 处理移动
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        // 处理抬起
        return 1;
    }
    return 0;  // 返回 0 表示不消费，继续默认处理
}

egui_view_set_on_touch_listener(EGUI_VIEW_OF(&my_view), on_my_view_touch);
```

## 输入相关配置

| 配置宏 | 说明 |
|--------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 启用触摸支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 启用键盘支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH` | 启用多点触控 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` | 启用焦点管理 |
| `EGUI_CONFIG_INPUT_MOTION_CACHE_COUNT` | 触摸事件缓冲池大小 |
| `EGUI_CONFIG_INPUT_KEY_CACHE_COUNT` | 键盘事件缓冲池大小 |
