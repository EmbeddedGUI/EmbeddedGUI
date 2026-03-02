# EmbeddedGUI 移植指南

本文档介绍如何将 EmbeddedGUI 移植到新的硬件平台。

## 1. 架构概览

EmbeddedGUI 采用驱动注册机制，将硬件相关代码与核心框架解耦。移植时需要实现并注册以下驱动：

```
┌─────────────────────────────────────────┐
│           EGUI Core (src/core/)          │
│                                         │
│   egui_core    egui_input    egui_canvas │
│       │            │                    │
│  ┌────┴────┐  ┌────┴────┐              │
│  │ PFB Mgr │  │ Key Evt │              │
│  └────┬────┘  └────┬────┘              │
└───────┼────────────┼────────────────────┘
        │            │
  ┌─────┴─────┐  ┌──┴──────────┐
  │ Display   │  │ Touch Driver │
  │ Driver    │  │ (可选)       │
  │ (必须)    │  └──────────────┘
  └─────┬─────┘
  ┌─────┴─────┐
  │ Platform  │
  │ Driver    │
  │ (必须)    │
  └───────────┘
```

| 驱动 | 必须 | 说明 |
|------|------|------|
| Display Driver | 是 | 屏幕硬件：绘制、刷新、旋转、亮度、电源 |
| Platform Driver | 是 | 平台服务：内存、日志、定时器、中断 |
| Touch Driver | 否 | 触摸硬件：读取触摸坐标 |

## 2. 快速开始

移植的最小步骤：

1. 复制 `porting/stm32g0_empty/` 作为模板
2. 实现 Display Driver 的 `init`、`draw_area`、`flush`
3. 实现 Platform Driver 的 `get_tick_ms`、`vlog`、`pfb_clear`
4. 在 `app_egui_config.h` 中配置屏幕参数
5. 在主循环中调用 `egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → `egui_polling_work()`

## 3. Display Driver（显示驱动）

### 3.1 操作表定义

```c
typedef struct egui_display_driver_ops {
    // ---- 必须实现 ----
    void (*init)(void);                    // 初始化 LCD 硬件
    void (*draw_area)(int16_t x, int16_t y, int16_t w, int16_t h,
                      const egui_color_int_t *data);  // 同步绘制像素数据
    void (*flush)(void);                   // 帧刷新完成通知

    // ---- 可选：异步 DMA 传输 ----
    void (*draw_area_async)(...);          // 异步绘制（NULL = 不支持）
    void (*wait_draw_complete)(void);      // 等待异步完成

    // ---- 可选：电源管理 ----
    void (*set_brightness)(uint8_t level); // 亮度 0-255（NULL = 不支持）
    void (*set_power)(uint8_t on);         // 电源开关（NULL = 不支持）

    // ---- 可选：硬件旋转 ----
    void (*set_rotation)(egui_display_rotation_t rotation); // NULL = 走软件旋转

    // ---- 可选：2D 硬件加速 ----
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h,
                      egui_color_int_t color);        // 硬件矩形填充
    void (*blit)(int16_t dx, int16_t dy, int16_t w, int16_t h,
                 const egui_color_int_t *src);         // 硬件块拷贝
    void (*blend)(int16_t dx, int16_t dy, int16_t w, int16_t h,
                  const egui_color_int_t *fg, egui_alpha_t alpha); // 硬件 Alpha 混合
    void (*wait_vsync)(void);              // 等待 VSync/TE 信号
} egui_display_driver_ops_t;
```

### 3.2 驱动实例

```c
typedef struct egui_display_driver {
    const egui_display_driver_ops_t *ops;
    int16_t physical_width;       // 物理宽度（旋转前）
    int16_t physical_height;      // 物理高度（旋转前）
    egui_display_rotation_t rotation;  // 当前旋转角度
    uint8_t brightness;           // 当前亮度
    uint8_t power_on;             // 电源状态
} egui_display_driver_t;
```

### 3.3 最小实现示例

```c
static void my_display_init(void)
{
    // 初始化 SPI、GPIO、LCD 控制器
    lcd_hw_init();
}

static void my_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const egui_color_int_t *data)
{
    // 设置 LCD 窗口并发送像素数据
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    lcd_write_data((const uint8_t *)data, w * h * sizeof(egui_color_int_t));
}

static void my_display_flush(void)
{
    // 如果 LCD 需要手动刷新，在此处理
}

static const egui_display_driver_ops_t my_display_ops = {
    .init               = my_display_init,
    .draw_area          = my_display_draw_area,
    .draw_area_async    = NULL,    // 不支持异步
    .wait_draw_complete = NULL,
    .flush              = my_display_flush,
    .set_brightness     = NULL,    // 不支持亮度调节
    .set_power          = NULL,    // 不支持电源控制
    .set_rotation       = NULL,    // 不支持硬件旋转
    .fill_rect          = NULL,    // 无 2D 加速
    .blit               = NULL,
    .blend              = NULL,
    .wait_vsync         = NULL,
};

static egui_display_driver_t my_display = {
    .ops             = &my_display_ops,
    .physical_width  = 240,        // 你的屏幕物理宽度
    .physical_height = 320,        // 你的屏幕物理高度
    .rotation        = EGUI_DISPLAY_ROTATION_0,
    .brightness      = 255,
    .power_on        = 1,
};
```

### 3.4 draw_area 详解

`draw_area` 是最核心的函数。框架会将 PFB 渲染好的像素块通过此函数发送到屏幕：

- `x, y`：目标区域左上角坐标
- `w, h`：区域宽高（像素）
- `data`：`w * h` 个像素的连续数组，格式由 `EGUI_CONFIG_COLOR_DEPTH` 决定（通常 RGB565）

典型的 SPI LCD 实现流程：
1. 设置列地址范围 (CASET)
2. 设置行地址范围 (RASET)
3. 发送写内存命令 (RAMWR)
4. 通过 SPI 发送像素数据

### 3.5 异步 DMA 传输（可选）

如果你的平台支持 SPI DMA，可以实现 `draw_area_async` 来提升性能：

```c
static void my_draw_area_async(int16_t x, int16_t y, int16_t w, int16_t h,
                                const egui_color_int_t *data)
{
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    spi_dma_start((const uint8_t *)data, w * h * sizeof(egui_color_int_t));
}

static void my_wait_draw_complete(void)
{
    while (!spi_dma_is_complete()) { }
}
```

配合双缓冲使用时，框架会在 DMA 传输期间切换到备用缓冲区继续渲染，实现 CPU 和 DMA 并行工作。需要：
1. 在 `app_egui_config.h` 中定义 `#define EGUI_CONFIG_PFB_DOUBLE_BUFFER 1`
2. 在 `egui_init_config_t` 中设置 `pfb_backup` 指向备用缓冲区

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
static egui_color_int_t pfb_backup[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

egui_init_config_t init_config = {
    .pfb        = egui_pfb,
    .pfb_backup = pfb_backup,   // 启用双缓冲
};
egui_init(&init_config);
```

## 4. Platform Driver（平台驱动）

### 4.1 操作表定义

```c
typedef struct egui_platform_ops {
    // ---- 基础服务（建议全部实现）----
    void *(*malloc)(int size);             // 动态内存分配（NULL = 不支持）
    void (*free)(void *ptr);               // 释放内存
    void (*vlog)(const char *fmt, va_list args);  // 日志输出
    void (*assert_handler)(const char *file, int line); // 断言处理
    void (*vsprintf)(char *str, const char *fmt, va_list args); // 字符串格式化
    void (*delay)(uint32_t ms);            // 毫秒延时
    uint32_t (*get_tick_ms)(void);         // 获取毫秒时间戳（单调递增）
    void (*pfb_clear)(void *s, int n);     // PFB 缓冲区清零
    egui_base_t (*interrupt_disable)(void); // 关中断
    void (*interrupt_enable)(egui_base_t level); // 开中断

    // ---- 可选：外部资源 ----
    void (*load_external_resource)(void *dest, uint32_t res_id,
                                   uint32_t start_offset, uint32_t size);

    // ---- 可选：RTOS 互斥锁 ----
    void *(*mutex_create)(void);           // 创建互斥锁
    void (*mutex_lock)(void *mutex);       // 加锁
    void (*mutex_unlock)(void *mutex);     // 解锁
    void (*mutex_destroy)(void *mutex);    // 销毁互斥锁

    // ---- 可选：定时器回调 ----
    void (*timer_start)(uint32_t expiry_time_ms); // 启动定时唤醒
    void (*timer_stop)(void);              // 取消定时唤醒

    // ---- 可选：性能优化 ----
    void (*memcpy_fast)(void *dst, const void *src, int n); // DMA 加速拷贝
    void (*watchdog_feed)(void);           // 喂看门狗
} egui_platform_ops_t;
```

### 4.2 最小实现示例

```c
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static void my_vlog(const char *format, va_list args)
{
    vprintf(format, args);  // 或通过 UART 输出
}

static void my_assert_handler(const char *file, int line)
{
    printf("ASSERT: %s:%d\n", file, line);
    while (1) { }  // 死循环便于调试
}

static void my_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static uint32_t my_get_tick_ms(void)
{
    return HAL_GetTick();  // 替换为你的平台 API
}

static void my_delay(uint32_t ms)
{
    HAL_Delay(ms);  // 替换为你的平台 API
}

static void my_pfb_clear(void *s, int n)
{
    memset(s, 0, n);  // 可替换为 DMA 清零
}

static egui_base_t my_interrupt_disable(void)
{
    __disable_irq();
    return 0;
}

static void my_interrupt_enable(egui_base_t level)
{
    (void)level;
    __enable_irq();
}

static const egui_platform_ops_t my_platform_ops = {
    .malloc                 = NULL,  // 如不需要动态内存可设 NULL
    .free                   = NULL,
    .vlog                   = my_vlog,
    .assert_handler         = my_assert_handler,
    .vsprintf               = my_vsprintf,
    .delay                  = my_delay,
    .get_tick_ms            = my_get_tick_ms,
    .pfb_clear              = my_pfb_clear,
    .interrupt_disable      = my_interrupt_disable,
    .interrupt_enable       = my_interrupt_enable,
    .load_external_resource = NULL,
    .mutex_create           = NULL,
    .mutex_lock             = NULL,
    .mutex_unlock           = NULL,
    .mutex_destroy          = NULL,
    .timer_start            = NULL,
    .timer_stop             = NULL,
    .memcpy_fast            = NULL,
    .watchdog_feed          = NULL,
};

static egui_platform_t my_platform = {
    .ops = &my_platform_ops,
};
```

### 4.3 关键函数说明

| 函数 | 重要性 | 说明 |
|------|--------|------|
| `get_tick_ms` | 必须 | 返回单调递增的毫秒时间戳，用于动画、定时器、输入超时等 |
| `pfb_clear` | 必须 | 每帧渲染前清零 PFB 缓冲区，可用 DMA 加速 |
| `vlog` | 建议 | 调试日志输出，通过 `EGUI_LOG_DBG/INF/WRN/ERR` 宏调用 |
| `assert_handler` | 建议 | 断言失败时调用，建议死循环或打印堆栈 |
| `interrupt_disable/enable` | 建议 | 保护共享数据（如输入事件队列），裸机可用全局中断开关 |
| `malloc/free` | 可选 | 仅在使用动态内存分配的功能时需要 |
| `mutex_*` | 可选 | RTOS 环境下保护 GUI 数据结构的线程安全 |
| `timer_start/stop` | 可选 | 精确定时唤醒，替代轮询方式降低功耗 |
| `memcpy_fast` | 可选 | DMA 加速内存拷贝，提升大块数据传输性能 |
| `watchdog_feed` | 可选 | 长时间渲染操作中喂狗，防止看门狗复位 |

## 5. Touch Driver（触摸驱动，可选）

如果你的设备有触摸屏，需要实现触摸驱动。框架会自动将原始触摸数据转换为 DOWN/MOVE/UP 事件。

### 5.1 操作表

```c
typedef struct egui_touch_driver_ops {
    void (*init)(void);
    void (*read)(uint8_t *pressed, int16_t *x, int16_t *y);
} egui_touch_driver_ops_t;
```

### 5.2 实现示例

```c
static void my_touch_init(void)
{
    // 初始化 I2C、触摸控制器（如 FT5336、GT911 等）
    touch_ic_init();
}

static void my_touch_read(uint8_t *pressed, int16_t *x, int16_t *y)
{
    // 读取触摸状态
    *pressed = touch_ic_is_pressed();
    if (*pressed)
    {
        *x = touch_ic_get_x();
        *y = touch_ic_get_y();
    }
}

static const egui_touch_driver_ops_t my_touch_ops = {
    .init = my_touch_init,
    .read = my_touch_read,
};

static egui_touch_driver_t my_touch = {
    .ops = &my_touch_ops,
};
```

注意：`read()` 只需报告当前触摸状态，框架会自动进行边沿检测，生成 `EGUI_MOTION_EVENT_ACTION_DOWN`、`EGUI_MOTION_EVENT_ACTION_MOVE`、`EGUI_MOTION_EVENT_ACTION_UP` 事件。

需要在 `app_egui_config.h` 中启用：
```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 1
```

## 6. Key Input（按键输入，可选）

如果你的设备有物理按键，平台层直接调用 `egui_input_add_key()` 传入按键事件即可。所有事件类型（DOWN/UP/LONG_PRESS/REPEAT）由平台自行决定和生成。

### 6.1 接口

```c
// 平台层直接调用此函数传入按键事件
int egui_input_add_key(uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
```

事件类型：
- `EGUI_KEY_EVENT_ACTION_DOWN` — 按键按下
- `EGUI_KEY_EVENT_ACTION_UP` — 按键释放
- `EGUI_KEY_EVENT_ACTION_LONG_PRESS` — 长按（由平台检测）
- `EGUI_KEY_EVENT_ACTION_REPEAT` — 按键重复（由平台检测）

### 6.2 实现示例

```c
// 在平台的按键中断或轮询中直接调用
void my_key_handler(void)
{
    uint8_t cur = gpio_read_key_pin();
    if (cur && !prev_key_state)
    {
        egui_input_add_key(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_ENTER, 0, 0);
    }
    else if (!cur && prev_key_state)
    {
        egui_input_add_key(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_ENTER, 0, 0);
    }
    prev_key_state = cur;
}
```

需要在 `app_egui_config.h` 中启用：
```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
```

## 7. 驱动注册与主循环

### 7.1 Port 初始化函数

将所有驱动注册集中在 `egui_port_init()` 中：

```c
void egui_port_init(void)
{
    // 必须注册
    egui_display_driver_register(&my_display);
    egui_platform_register(&my_platform);

    // 可选注册
    egui_touch_driver_register(&my_touch);   // 有触摸屏时
}
```

### 7.2 主循环

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
// 可选：双缓冲备用缓冲区
// static egui_color_int_t pfb_backup[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

void main(void)
{
    // 1. 硬件初始化（时钟、GPIO、SPI、I2C 等）
    system_hw_init();

    // 2. 注册驱动
    egui_port_init();

    // 3. 初始化 GUI 框架
    egui_init_config_t init_config = {
        .pfb        = egui_pfb,
        .pfb_backup = NULL,       // 双缓冲时改为 pfb_backup
    };
    egui_init(&init_config);

    // 4. 创建 UI
    uicode_create_ui();

    // 5. 开屏（清屏 + 启动渲染）
    egui_screen_on();

    // 6. 主循环
    while (1)
    {
        egui_polling_work();
    }
}
```

调用顺序很重要：`egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → 循环 `egui_polling_work()`

### 7.3 RTOS 环境

在 RTOS 环境下，建议将 GUI 放在独立任务中：

```c
void gui_task(void *arg)
{
    egui_port_init();
    egui_init_config_t init_config = { .pfb = egui_pfb, .pfb_backup = NULL };
    egui_init(&init_config);
    uicode_create_ui();
    egui_screen_on();

    while (1)
    {
        egui_polling_work();
        os_delay(1);  // 让出 CPU
    }
}
```

如果其他任务需要操作 GUI（如更新数据），应通过 mutex 保护：
```c
// 在 platform ops 中实现 mutex_create/lock/unlock
// 其他任务中：
egui_platform_get()->ops->mutex_lock(gui_mutex);
egui_view_label_set_text(label, new_text);
egui_platform_get()->ops->mutex_unlock(gui_mutex);
```

## 8. 配置宏参考

在你的 `app_egui_config.h` 中覆盖默认配置：

### 8.1 屏幕与 PFB

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_SCEEN_WIDTH` | 240 | 屏幕宽度（像素） |
| `EGUI_CONFIG_SCEEN_HEIGHT` | 320 | 屏幕高度（像素） |
| `EGUI_CONFIG_COLOR_DEPTH` | 16 | 色深（16 = RGB565, 32 = ARGB8888） |
| `EGUI_CONFIG_COLOR_16_SWAP` | 0 | RGB565 字节交换（SPI 8-bit 接口常用） |
| `EGUI_CONFIG_PFB_WIDTH` | 屏宽/8 | PFB 块宽度，建议为屏宽的整数约数 |
| `EGUI_CONFIG_PFB_HEIGHT` | 屏高/8 | PFB 块高度，建议为屏高的整数约数 |
| `EGUI_CONFIG_MAX_FPS` | 60 | 最大帧率限制 |

### 8.2 功能开关

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 1 | 启用触摸支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 0 | 启用按键支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS` | 0 | 启用焦点系统（自动启用按键） |
| `EGUI_CONFIG_PFB_DOUBLE_BUFFER` | 0 | 启用 PFB 双缓冲 |
| `EGUI_CONFIG_SOFTWARE_ROTATION` | 0 | 启用软件旋转 |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | 0 | 启用外部资源加载 |

### 8.3 按键参数

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_INPUT_KEY_CACHE_COUNT` | 5 | 按键事件队列大小 |

### 8.4 PFB 大小选择建议

PFB 大小直接影响 RAM 占用和渲染效率：

```
RAM 占用 = PFB_WIDTH × PFB_HEIGHT × COLOR_BYTES
         = 30 × 40 × 2 = 2400 字节 (RGB565)
```

| 场景 | 建议 PFB 大小 | RAM 占用 |
|------|--------------|---------|
| 极低 RAM (<4KB) | 屏宽/16 × 屏高/16 | ~600B |
| 低 RAM (<8KB) | 屏宽/8 × 屏高/8 | ~2.4KB |
| 充足 RAM | 屏宽 × 屏高/4 | ~19.2KB |
| 全屏缓冲 | 屏宽 × 屏高 | ~76.8KB |

PFB 越大，每帧需要的 `draw_area` 调用次数越少，渲染效率越高。但 PFB 宽高必须是屏幕宽高的整数约数。

## 9. 高级功能

### 9.1 2D 硬件加速

如果你的 MCU 有 DMA2D（如 STM32F4/F7/H7）或 GPU，可以实现加速接口：

```c
static void my_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h,
                          egui_color_int_t color)
{
    dma2d_fill(x, y, w, h, color);
}

static void my_blend(int16_t dx, int16_t dy, int16_t w, int16_t h,
                      const egui_color_int_t *fg, egui_alpha_t alpha)
{
    dma2d_blend(dx, dy, w, h, fg, alpha);
}
```

### 9.2 帧同步（防撕裂）

框架支持两种帧同步模式，防止 PFB 块跨越 LCD 刷新周期导致画面撕裂：

**方式一：非阻塞（推荐）— TE 中断驱动**

TE 中断设置标志位，定时器回调检查标志位后再刷新。CPU 零等待。

```c
// TE 中断处理函数
void LCD_TE_IRQHandler(void)
{
    egui_display_notify_vsync();  // 设置 frame_sync_ready 标志
}

// 驱动初始化时启用非阻塞帧同步
static egui_display_driver_t my_display = {
    .ops              = &my_display_ops,
    .physical_width   = 240,
    .physical_height  = 320,
    .rotation         = EGUI_DISPLAY_ROTATION_0,
    .brightness       = 255,
    .power_on         = 1,
    .frame_sync_enabled = 1,   // 启用非阻塞帧同步
    .frame_sync_ready   = 0,
};
```

工作流程：
```
TE 中断 → egui_display_notify_vsync() → 设 flag
Timer → egui_core_refresh_screen()
         ├─ flag=1 → 清 flag, render + flush（对齐 TE）
         └─ flag=0 → 跳过本轮，下次再来（零等待）
```

**方式二：阻塞等待 — 简单轮询**

适用于没有 TE 中断的场景，CPU 会忙等：

```c
static void my_wait_vsync(void)
{
    while (!gpio_read_te_pin()) { }  // 忙等 TE 引脚
}
// 在 ops 中设置 .wait_vsync = my_wait_vsync
// 不设置 frame_sync_enabled（默认 0）
```

**无帧同步**

如果 LCD 没有 TE 信号，`wait_vsync = NULL` 且 `frame_sync_enabled = 0`，框架按定时器节奏直接刷新。

### 9.3 外部资源加载

从 SPI Flash、SD 卡等加载图片/字体资源：

```c
static void my_load_resource(void *dest, uint32_t res_id,
                              uint32_t offset, uint32_t size)
{
    spi_flash_read(RESOURCE_BASE_ADDR + offset, dest, size);
}
```

需要启用 `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1`。

### 9.4 软件旋转

当 LCD 不支持硬件旋转时，框架可以在软件层面旋转 PFB 输出：

1. 启用 `EGUI_CONFIG_SOFTWARE_ROTATION 1`
2. Display Driver 的 `set_rotation` 设为 NULL
3. 运行时调用 `egui_display_set_rotation(EGUI_DISPLAY_ROTATION_90)` 切换方向

注意：90°/270° 旋转需要额外的临时缓冲区（与 PFB 同大小）。

### 9.5 屏幕开关机

框架提供高级 API 管理屏幕电源状态，封装了正确的调用顺序：

**关屏（省电模式）：**
```c
egui_screen_off();
// 内部执行：
// 1. egui_core_suspend()  — 停止渲染 + 停止刷新定时器
// 2. egui_display_set_power(0) — 关闭 LCD 硬件（SLPIN 等）
```

**开屏（唤醒）：**
```c
egui_screen_on();
// 内部执行：
// 1. egui_display_set_power(1) — 唤醒 LCD 硬件（SLPOUT 等）
// 2. egui_core_clear_screen()  — 清屏避免花屏（用黑色填充整屏）
// 3. egui_core_resume()        — 标记全屏脏区 + 重启刷新定时器
//    → 下一帧自动重绘完整 UI
```

**典型使用场景：**
```c
// 按键触发休眠
void on_power_button_pressed(void)
{
    if (egui_core_is_suspended())
    {
        egui_screen_on();
    }
    else
    {
        egui_screen_off();
    }
}
```

**Display Driver 需要实现 `set_power`：**
```c
static void mcu_set_power(uint8_t on)
{
    if (on)
    {
        lcd_send_cmd(LCD_CMD_SLPOUT);   // 退出睡眠
        delay_ms(120);                   // 等待 LCD 稳定
        lcd_send_cmd(LCD_CMD_DISPON);   // 开启显示
        backlight_on();                  // 打开背光
    }
    else
    {
        backlight_off();                 // 先关背光
        lcd_send_cmd(LCD_CMD_DISPOFF);  // 关闭显示
        lcd_send_cmd(LCD_CMD_SLPIN);    // 进入睡眠
    }
}
```

注意事项：
- `egui_init()` 后框架处于挂起状态，不会渲染也不会启动定时器
- 用户在 UI 创建完毕后调用 `egui_screen_on()` 开屏，框架自动清屏 + 启动渲染
- 关屏后所有定时器停止，不消耗 CPU
- 开屏时框架会自动重绘整个 UI，无需手动触发
- 亮度可通过 `egui_display_set_brightness(level)` 单独控制（0-255）

## 10. 文件结构模板

建议的移植目录结构：

```
porting/
└── your_platform/
    ├── Porting/
    │   ├── egui_port_mcu.c      # 驱动注册（Display + Platform + Touch）
    │   └── port_main.c          # 主循环入口
    ├── app_egui_config.h        # 配置覆盖
    └── build.mk                 # 构建模块定义
```

`build.mk` 示例：
```makefile
EGUI_CODE_SRC += porting/your_platform/Porting/egui_port_mcu.c
EGUI_CODE_SRC += porting/your_platform/Porting/port_main.c
EGUI_CODE_INCLUDE += -Iporting/your_platform
```

## 11. 移植检查清单

- [ ] `egui_port_init()` 中注册了 Display Driver 和 Platform Driver
- [ ] `draw_area()` 能正确将像素数据发送到屏幕
- [ ] `get_tick_ms()` 返回单调递增的毫秒时间戳
- [ ] `pfb_clear()` 能正确清零缓冲区
- [ ] `app_egui_config.h` 中配置了正确的屏幕尺寸和 PFB 大小
- [ ] PFB 宽高是屏幕宽高的整数约数
- [ ] 主循环中按正确顺序调用 `egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → `egui_polling_work()`
- [ ] （如有触摸）Touch Driver 的 `read()` 返回正确的坐标
- [ ] （如有按键）平台层通过 `egui_input_add_key()` 正确报告按键事件
- [ ] （如用双缓冲）`egui_init_config_t.pfb_backup` 指向了备用缓冲区
- [ ] 编译通过且运行时屏幕有正确渲染输出
