# EmbeddedGUI 移植指南

本文档基于当前仓库的驱动层实现，说明如何把 EmbeddedGUI 移植到新的硬件平台，或如何为 `pc` / `emscripten` 这类模拟平台接入统一的驱动接口。

> 当前推荐架构：**Bus IO + HAL Driver + Core**。
> Porting 层直接实现 Core 的 `egui_display_driver_ops_t`，并注册 `egui_display_driver_t`。触摸通过 `egui_hal_touch_register()` 一行完成注册。

## 1. 架构概览

EmbeddedGUI 当前采用三层结构：

```
┌──────────────────────────────────────────────┐
│              EGUI Core (src/core/)            │
│  egui_core / egui_input / egui_canvas         │
└───────────────────────┬──────────────────────┘
                        │
        ┌───────────────┴────────────────┐
        │                                │
┌───────▼────────┐               ┌───────▼─────────┐
│ HAL LCD Driver │               │ HAL Touch Driver│
│ driver/lcd/    │               │ driver/touch/   │
└───────┬────────┘               └───────┬─────────┘
        │                                │
   driver/bus/*                     driver/bus/*
   GPIO / SPI / I2C / 8080          GPIO / I2C / SPI
```

职责划分如下：

| 层级 | 必须 | 说明 |
|------|------|------|
| HAL LCD Driver | 是 | LCD 控制器驱动，如 `ST7789`、`ST7735`、`GC9A01` |
| HAL Touch Driver | 否 | 触摸控制器驱动，如 `FT6336`、`STMPE610` |
| Platform Driver | 是 | 提供内存、日志、tick、延时、中断、PFB 清零等服务 |
| Bus / GPIO Ops | 视硬件而定 | SPI / I2C / 8080 / GPIO 的底层实现 |

说明：

- MCU port 优先复用 `driver/lcd/`、`driver/touch/` 下的通用控制器驱动。
- Porting 层实现 `egui_display_driver_ops_t`，在 `draw_area` 中调用 HAL LCD driver 的 `draw_area`。
- Touch 通过 `egui_hal_touch_register(&s_touch_driver)` 一行注册到 Core，不再需要手写 `port_touch_read` 适配函数。
- `pc` / `emscripten` 不复用 `ST7701`、`GC9503` 之类硬件 LCD 驱动，而是使用 `porting/pc/egui_hal_sdl_sim.c` 中的专用 SDL 模拟驱动。
- 不再需要为 PC 模拟层引入额外的像素路径抽象，例如 `EGUI_HAL_LCD_PIXEL_PATH_RAM` 一类宏。

## 2. 快速开始

推荐按下面顺序完成一次新 port：

1. 复制 `porting/stm32g0/` 作为起点并按需裁剪板级驱动，或直接参考 `porting/stm32g0/Porting/egui_port_mcu.c`
2. 实现 Bus / GPIO 操作层：SPI、I2C、8080、RST、DC、BL、INT 等
3. 选择 LCD 驱动和可选 Touch 驱动，优先复用 `driver/lcd/`、`driver/touch/` 下已有实现
4. 在 `egui_port_init()` 中实例化 HAL 驱动，调用 `reset()` → `init()`，实现 `egui_display_driver_ops_t` 并注册到 Core；触摸通过 `egui_hal_touch_register()` 注册
5. 实现 `egui_platform_ops_t`，至少提供 `vlog`、`delay`、`get_tick_ms`、`pfb_clear`
6. 在 `app_egui_config.h` 中配置屏幕尺寸、颜色格式、PFB 大小、功能开关
7. 在主循环中执行 `egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → `egui_polling_work()`

建议先做以下验证：

```bash
make all APP=HelloSimple PORT=pc
make run

make all APP=HelloBasic APP_SUB=button PORT=pc
make run
```

先在 `pc` 验证渲染和点击，再切到 MCU 验证真实硬件，可以更快定位是“驱动问题”还是“业务 UI 问题”。

## 3. Display Driver（显示驱动）

### 3.1 推荐接口：`egui_hal_lcd_driver_t`

LCD 控制器统一实现为 `egui_hal_lcd_driver_t`：

```c
typedef struct egui_hal_lcd_driver egui_hal_lcd_driver_t;

struct egui_hal_lcd_driver {
    const char *name;
    egui_bus_type_t bus_type;

    int (*reset)(egui_hal_lcd_driver_t *self);
    int (*init)(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config);
    void (*del)(egui_hal_lcd_driver_t *self);

    void (*draw_area)(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                      int16_t w, int16_t h, const void *data, uint32_t len);

    void (*set_rotation)(egui_hal_lcd_driver_t *self, uint8_t rotation);
    void (*set_brightness)(egui_hal_lcd_driver_t *self, uint8_t level);
    void (*set_power)(egui_hal_lcd_driver_t *self, uint8_t on);
    void (*set_invert)(egui_hal_lcd_driver_t *self, uint8_t invert);
};
```

最小必须能力：

- `reset()`：硬件复位（RST 引脚低→高）
- `init()`：控制器初始化序列
- `draw_area()`：设置窗口并输出像素数据
- `del()`：释放驱动资源（RST 拉低 + 清零结构体）

生命周期：`factory_init` → `reset()` → `init()` → ... → `set_power(0)` → `del()`

### 3.2 Port 层的典型接法

port 层通常只负责”组装驱动”，而不是重写一份控制器逻辑：

```c
#include “egui.h”
#include “core/egui_display_driver.h”
#include “egui_hal_stm32g0.h”
#include “egui_lcd_st7789.h”

static egui_hal_lcd_driver_t s_lcd_driver;

static void port_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    s_lcd_driver.draw_area(&s_lcd_driver, x, y, w, h, data, w * h * sizeof(egui_color_int_t));
}

static const egui_display_driver_ops_t s_display_ops = {
    .init = NULL,
    .draw_area = port_display_draw_area,
    .wait_draw_complete = NULL,
    .flush = NULL,
    .set_brightness = egui_hal_stm32g0_set_backlight,
    /* ... other ops ... */
};
static egui_display_driver_t s_display_driver = {0};

void egui_port_init(void)
{
    egui_lcd_st7789_init(&s_lcd_driver,
                         egui_hal_stm32g0_get_lcd_spi_ops(),
                         egui_hal_stm32g0_get_lcd_gpio_ops());

    egui_hal_lcd_config_t lcd_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .color_depth = EGUI_CONFIG_COLOR_DEPTH,
        .color_swap = 0,
        .x_offset = 0,
        .y_offset = 0,
        .invert_color = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };

    s_lcd_driver.reset(&s_lcd_driver);
    s_lcd_driver.init(&s_lcd_driver, &lcd_config);

    s_display_driver.ops = &s_display_ops;
    s_display_driver.physical_width = EGUI_CONFIG_SCEEN_WIDTH;
    s_display_driver.physical_height = EGUI_CONFIG_SCEEN_HEIGHT;
    s_display_driver.rotation = EGUI_DISPLAY_ROTATION_0;
    s_display_driver.brightness = 255;
    s_display_driver.power_on = 1;
    egui_display_driver_register(&s_display_driver);
}
```

### 3.3 新增一个 LCD 控制器驱动

如果仓库里还没有你的控制器，建议在 `driver/lcd/` 下新增 `egui_lcd_xxx.c/.h`，并把平台相关依赖都通过 bus/gpio ops 传进来。

最小示例：

```c
static int my_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    lcd_hw_reset();
    lcd_send_init_cmds();
    return 0;
}

static void my_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                             int16_t w, int16_t h, const void *data, uint32_t len)
{
    EGUI_UNUSED(self);
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    lcd_write_data((const uint8_t *)data, len);
}
```

建议：

- 控制器驱动只关注寄存器和时序，不要写死平台 HAL
- SPI / I2C / 8080 / GPIO 一律通过 ops 访问
- 旋转、背光、电源这些能力尽量走回调，不要放在业务层硬编码

### 3.4 DMA 与双缓冲

如果 `draw_area()` 走 DMA：

- `draw_area()` 中启动 DMA 发送
- porting 层的 `wait_draw_complete()` 通过 Panel IO 的 `wait_tx_done()` 等待完成
- 若 `EGUI_CONFIG_PFB_BUFFER_COUNT >= 2`，可以让下一块 PFB 渲染和上一块发送并行

示例：

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

egui_init(egui_pfb);
```

### 3.5 PC / emscripten 的特殊情况

PC 和浏览器端不是真实 LCD 控制器，因此当前使用专用 SDL 模拟 HAL：

- `porting/pc/egui_hal_sdl_sim.c`
- `porting/pc/egui_port_pc.c`
- `porting/emscripten/egui_port_emscripten.c`

特点：

- `SDL_LCD` 直接把像素块写入 SDL 帧缓冲
- `SDL_TOUCH` 直接从 SDL 鼠标 / 触摸事件获取坐标
- porting 层实现 `egui_display_driver_ops_t` 的 `draw_area` 来调用 SDL LCD driver
- 触摸通过 `egui_hal_touch_register()` 注册 SDL touch driver 到 Core
- emscripten 复用同一套 SDL 模拟 HAL，避免维护第二套”假 LCD 驱动”

## 4. Platform Driver（平台驱动）

### 4.1 接口定义

平台驱动仍然通过 `egui_platform_ops_t` 提供基础系统能力：

```c
typedef struct egui_platform_ops {
    void *(*malloc)(int size);
    void (*free)(void *ptr);
    void (*vlog)(const char *fmt, va_list args);
    void (*assert_handler)(const char *file, int line);
    void (*vsprintf)(char *str, const char *fmt, va_list args);
    void (*delay)(uint32_t ms);
    uint32_t (*get_tick_ms)(void);
    void (*pfb_clear)(void *s, int n);
    egui_base_t (*interrupt_disable)(void);
    void (*interrupt_enable)(egui_base_t level);

    void (*load_external_resource)(void *dest, uint32_t res_id,
                                   uint32_t start_offset, uint32_t size);

    void *(*mutex_create)(void);
    void (*mutex_lock)(void *mutex);
    void (*mutex_unlock)(void *mutex);
    void (*mutex_destroy)(void *mutex);

    void (*timer_start)(uint32_t expiry_time_ms);
    void (*timer_stop)(void);
    void (*memcpy_fast)(void *dst, const void *src, uint32_t size);
    void (*watchdog_feed)(void);
} egui_platform_ops_t;
```

### 4.2 最小实现示例

```c
static void mcu_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

static void mcu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t mcu_interrupt_disable(void)
{
    egui_base_t level = __get_PRIMASK();
    __disable_irq();
    return level;
}

static void mcu_interrupt_enable(egui_base_t level)
{
    __set_PRIMASK(level);
}
```

### 4.3 关键点

- `get_tick_ms()` 必须返回单调递增的毫秒时间戳
- `pfb_clear()` 必须正确清空缓冲区；有 DMA 清零能力时可以在这里加速
- `interrupt_disable/enable()` 用于保护输入队列等共享数据
- `delay()` 在 `emscripten` 这类单线程环境中可以为空实现，避免阻塞主循环

## 5. Touch Driver（触摸驱动，可选）

### 5.1 推荐接口：`egui_hal_touch_driver_t`

当前触摸驱动统一实现为 `egui_hal_touch_driver_t`，支持多点触摸：

```c
typedef struct egui_hal_touch_data {
    uint8_t point_count;
    uint8_t gesture;
    egui_hal_touch_point_t points[EGUI_HAL_TOUCH_MAX_POINTS];
} egui_hal_touch_data_t;

struct egui_hal_touch_driver {
    const char *name;
    egui_bus_type_t bus_type;
    uint8_t max_points;

    int (*reset)(egui_hal_touch_driver_t *self);
    int (*init)(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config);
    void (*del)(egui_hal_touch_driver_t *self);
    int (*read)(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data);

    void (*set_rotation)(egui_hal_touch_driver_t *self, uint8_t rotation);
    void (*enter_sleep)(egui_hal_touch_driver_t *self);
    void (*exit_sleep)(egui_hal_touch_driver_t *self);
};
```

当前 Core 最终仍以单点事件为主，`egui_hal_touch_register()` 会自动把 HAL touch driver 的第一触点作为主触点上报给 Core。

### 5.2 Port 层接入方式

使用 `egui_hal_touch_register()` 一行即可完成 HAL touch driver 到 Core 的注册，不再需要手写 `port_touch_read` 适配函数：

```c
#include "egui_touch.h"
#include "egui_touch_ft6336.h"

static egui_hal_touch_driver_t s_touch_driver;

void egui_port_init(void)
{
    egui_touch_ft6336_init(&s_touch_driver,
                           egui_hal_stm32g0_get_touch_i2c_ops(),
                           egui_hal_stm32g0_get_touch_gpio_ops());

    egui_hal_touch_config_t touch_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };

    s_touch_driver.reset(&s_touch_driver);
    s_touch_driver.init(&s_touch_driver, &touch_config);

    egui_hal_touch_register(&s_touch_driver);
}
```

### 5.3 自定义触摸驱动时的最小能力

```c
static int my_touch_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    EGUI_UNUSED(self);
    memset(data, 0, sizeof(*data));

    if (!touch_ic_is_pressed())
    {
        return 0;
    }

    data->point_count = 1;
    data->points[0].x = touch_ic_get_x();
    data->points[0].y = touch_ic_get_y();
    data->points[0].id = 0;
    data->points[0].pressure = 1;
    return 0;
}
```

如果应用启用了触摸，需要在 `app_egui_config.h` 中设置：

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH 1
```

## 6. Key Input（按键输入，可选）

如果设备有物理按键，平台层直接调用 `egui_input_add_key()` 上报事件即可：

```c
int egui_input_add_key(uint8_t type, uint8_t key_code, uint8_t is_shift, uint8_t is_ctrl);
```

常用事件类型：

- `EGUI_KEY_EVENT_ACTION_DOWN`
- `EGUI_KEY_EVENT_ACTION_UP`
- `EGUI_KEY_EVENT_ACTION_LONG_PRESS`
- `EGUI_KEY_EVENT_ACTION_REPEAT`

示例：

```c
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

如果启用按键输入，需要在 `app_egui_config.h` 中设置：

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
```

## 7. 驱动注册与主循环

### 7.1 `egui_port_init()`

当前推荐把 LCD、Touch、Platform 注册都集中在 `egui_port_init()`：

```c
static egui_hal_lcd_driver_t s_lcd_driver;
static egui_hal_touch_driver_t s_touch_driver;
static egui_platform_t s_platform = {
    .ops = &mcu_platform_ops,
};

static void port_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    s_lcd_driver.draw_area(&s_lcd_driver, x, y, w, h, data, w * h * sizeof(egui_color_int_t));
}

static const egui_display_driver_ops_t s_display_ops = {
    .init = NULL,
    .draw_area = port_display_draw_area,
    /* ... */
};
static egui_display_driver_t s_display_driver = {0};

void egui_port_init(void)
{
    egui_lcd_st7789_init(&s_lcd_driver,
                         egui_hal_stm32g0_get_lcd_spi_ops(),
                         egui_hal_stm32g0_get_lcd_gpio_ops());

    egui_hal_lcd_config_t lcd_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .color_depth = EGUI_CONFIG_COLOR_DEPTH,
        .color_swap = 0,
    };
    s_lcd_driver.reset(&s_lcd_driver);
    s_lcd_driver.init(&s_lcd_driver, &lcd_config);

    s_display_driver.ops = &s_display_ops;
    s_display_driver.physical_width = EGUI_CONFIG_SCEEN_WIDTH;
    s_display_driver.physical_height = EGUI_CONFIG_SCEEN_HEIGHT;
    s_display_driver.rotation = EGUI_DISPLAY_ROTATION_0;
    s_display_driver.brightness = 255;
    s_display_driver.power_on = 1;
    egui_display_driver_register(&s_display_driver);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_touch_ft6336_init(&s_touch_driver,
                           egui_hal_stm32g0_get_touch_i2c_ops(),
                           egui_hal_stm32g0_get_touch_gpio_ops());
    s_touch_driver.reset(&s_touch_driver);
    s_touch_driver.init(&s_touch_driver, &(egui_hal_touch_config_t) {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
    });
    egui_hal_touch_register(&s_touch_driver);
#endif

    egui_platform_register(&s_platform);
}
```

### 7.2 主循环

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_BUFFER_COUNT][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

int main(void)
{
    system_hw_init();

    egui_port_init();

    egui_init(egui_pfb);

    uicode_create_ui();
    egui_screen_on();

    while (1)
    {
        egui_polling_work();
    }
}
```

调用顺序非常重要：

`egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → 循环 `egui_polling_work()`

### 7.3 RTOS 环境

RTOS 下建议让 GUI 在单独任务中运行；如果其他任务要操作 UI，需要配合平台层 mutex：

```c
void gui_task(void *arg)
{
    egui_port_init();
    egui_init(egui_pfb);
    uicode_create_ui();
    egui_screen_on();

    while (1)
    {
        egui_polling_work();
        os_delay(1);
    }
}
```

## 8. 配置宏参考

### 8.1 屏幕与 PFB

常用配置：

```c
#define EGUI_CONFIG_SCEEN_WIDTH        240
#define EGUI_CONFIG_SCEEN_HEIGHT       320
#define EGUI_CONFIG_COLOR_DEPTH        16
#define EGUI_CONFIG_COLOR_16_SWAP      0

#define EGUI_CONFIG_PFB_WIDTH          40
#define EGUI_CONFIG_PFB_HEIGHT         40
```

### 8.2 功能开关

```c
#define EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH   1
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY     0
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 0
#define EGUI_CONFIG_SOFTWARE_ROTATION        0
```

### 8.3 按键参数

如果平台启用了按键输入，可按需配置长按、重复触发等相关参数。

### 8.4 PFB 大小建议

建议优先保证：

- `EGUI_CONFIG_PFB_WIDTH` 和 `EGUI_CONFIG_PFB_HEIGHT` 是屏幕尺寸的整数约数
- PFB 不要太小，否则 LCD 窗口切换过于频繁
- PFB 也不要盲目太大，否则会推高 RAM 占用

常见策略：

- 小 RAM MCU：`屏宽 x 8` 或 `屏宽 x 16`
- PC / emscripten：可以适当放大，优先保证调试流畅
- DMA + 双缓冲：PFB 可适度增大以减少切块次数

## 9. 高级功能

### 9.1 2D 硬件加速

如果平台有 DMA2D / PXP / GPU，可在 porting 层的 `egui_display_driver_ops_t` 中实现 `fill_rect`、`blit`、`blend` 等加速回调。

### 9.2 帧同步（防撕裂）

若 LCD 提供 TE / VSYNC：

- 可以在底层实现等待 VSYNC 的机制
- 避免在屏幕扫描过程中切换大面积数据
- 对 RGB / MIPI / 高刷新屏尤其重要

### 9.3 外部资源加载

如果图片 / 字体在 SPI Flash、SD 卡或文件系统中，可实现：

```c
static void my_load_external_resource(void *dest, uint32_t res_id,
                                      uint32_t start_offset, uint32_t size)
{
    spi_flash_read(resource_addr + start_offset, dest, size);
}
```

同时开启：

```c
#define EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE 1
```

### 9.4 软件旋转

如果 LCD 控制器本身不支持旋转，可以：

- 开启 `EGUI_CONFIG_SOFTWARE_ROTATION`
- 让 LCD 驱动的 `set_rotation` 为空
- 由框架在 PFB 输出阶段完成旋转

### 9.5 屏幕开关机

框架推荐使用高级 API：

```c
egui_screen_off();
egui_screen_on();
```

显示驱动若实现了 `set_power()` 和 `set_brightness()`，即可统一接入关屏、开屏、背光控制流程。

## 10. 文件结构模板

### 10.1 MCU port 推荐目录

```text
porting/
└── your_platform/
    ├── Porting/
    │   ├── egui_hal_your_platform.c   # Bus/GPIO ops 封装
    │   ├── egui_hal_your_platform.h
    │   ├── egui_port_mcu.c            # HAL Driver + Core driver registration + Platform 注册
    │   └── port_main.c                # 主循环入口
    ├── app_egui_config.h
    └── build.mk
```

如果有新的 LCD / Touch 控制器驱动，建议放在公共驱动层，而不是塞回 porting：

```text
driver/
├── bus/
├── lcd/
└── touch/
```

### 10.2 PC / emscripten 目录说明

```text
porting/
├── pc/
│   ├── egui_hal_sdl_sim.c
│   ├── egui_port_pc.c
│   ├── sdl_port.c
│   └── main.c
└── emscripten/
    ├── egui_port_emscripten.c
    ├── main_emscripten.c
    └── build.mk
```

这两个 port 共享 SDL 模拟驱动逻辑，不再额外复制一套 LCD / Touch 控制器驱动。

### 10.3 `build.mk` 示例

```makefile
SRC += $(EGUI_PORT_PATH)/Porting

INCLUDE += $(EGUI_PORT_PATH)
INCLUDE += $(EGUI_PORT_PATH)/Porting
```

### 10.4 构建输出约定

当前构建规则约定如下：

- `TARGET` 保持不变，默认仍是 `main`
- 最终产物仍输出到 `output/main` 或 `output/main.exe`
- 为避免切换 `APP` / `PORT` 时对象文件互相污染，目标文件放在 `output/obj/{APP}_{PORT}/`

也就是说：**对象文件按 `APP + PORT` 隔离，最终可执行文件名不跟着变。**

## 11. 移植检查清单

- [ ] `egui_port_init()` 中已实现 `egui_display_driver_ops_t` 并注册 `egui_display_driver_t`，以及注册 Platform Driver
- [ ] 如启用触摸，已通过 `egui_hal_touch_register()` 注册 HAL touch driver
- [ ] LCD 驱动的 `draw_area()` 能正确输出像素
- [ ] `get_tick_ms()` 返回单调递增毫秒时间戳
- [ ] `pfb_clear()` 能正确清空缓冲区
- [ ] `app_egui_config.h` 中的屏幕尺寸、颜色格式、PFB 大小与硬件一致
- [ ] PFB 大小合理，宽高尽量为屏幕尺寸的整数约数
- [ ] 主循环调用顺序正确：`egui_port_init()` → `egui_init()` → `uicode_create_ui()` → `egui_screen_on()` → `egui_polling_work()`
- [ ] `pc` port 下能正常渲染，并能完成鼠标点击 / 触摸输入
- [ ] `emscripten` port 编译通过，页面可正常显示和交互
- [ ] 目标 MCU port 编译通过，屏幕方向、颜色、触摸坐标都正确
- [ ] 切换 driver 后，其他 port 不会因共享代码改动而编译失败或功能异常


