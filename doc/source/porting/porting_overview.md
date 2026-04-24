# 移植概述

EmbeddedGUI 采用驱动注册机制，将硬件相关代码与核心框架解耦。移植到新平台时，只需实现并注册少量驱动接口即可。

## 架构概览

```
+---------------------------------------------+
|           EGUI Core (src/core/)              |
|                                             |
|   egui_core    egui_input    egui_canvas    |
|       |            |                        |
|  +----+----+  +----+----+                  |
|  | PFB Mgr |  | Key Evt |                  |
|  +----+----+  +----+----+                  |
+-------+------------+------------------------+
        |            |
  +-----+-----+  +--+------------+
  | Display   |  | Touch Driver  |
  | Driver    |  | (optional)    |
  | (required)|  +--------------+
  +-----+-----+
  +-----+-----+
  | Platform  |
  | Driver    |
  | (required)|
  +-----------+
```

| 驱动 | 必须 | 说明 |
|------|------|------|
| Display Driver | 是 | 屏幕硬件：绘制、刷新、旋转、亮度、电源 |
| Platform Driver | 是 | 平台服务：内存、日志、定时器、中断 |
| Touch Driver | 否 | 触摸硬件：读取触摸坐标 |

## 必须实现的接口

### Display Driver

```c
typedef struct egui_display_driver_ops {
    // --- 必须实现 ---
    void (*init)(egui_core_t *core);
    void (*draw_area)(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h,
                      const egui_color_int_t *data);
    void (*flush)(egui_core_t *core);

    // --- 可选 ---
    void (*wait_draw_complete)(egui_core_t *core);  // 等待 draw_area 完成（NULL = draw_area 是同步阻塞的）
    void (*set_brightness)(egui_core_t *core, uint8_t level);
    void (*set_power)(egui_core_t *core, uint8_t on);
    void (*set_rotation)(egui_core_t *core, egui_display_rotation_t rotation);
    void (*fill_rect)(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h,
                      egui_color_int_t color);
    void (*blit)(egui_core_t *core, int16_t dx, int16_t dy, int16_t w, int16_t h,
                 const egui_color_int_t *src);
    void (*blend)(egui_core_t *core, int16_t dx, int16_t dy, int16_t w, int16_t h,
                  const egui_color_int_t *fg, egui_alpha_t alpha);
    void (*wait_vsync)(egui_core_t *core);
} egui_display_driver_ops_t;
```

三个必须实现的函数：

| 函数 | 说明 |
|------|------|
| `init` | 初始化当前 `core` 对应的 LCD 硬件（SPI、GPIO、LCD 控制器等） |
| `draw_area` | 将 PFB 渲染好的像素块发送到屏幕指定区域 |
| `flush` | 当前 `core` 一帧刷新完成通知（某些 LCD 需要手动触发刷新） |

`draw_area` 参数说明：
- `core`：目标屏幕所属的 `egui_core_t`
- `x, y`：目标区域左上角坐标
- `w, h`：区域宽高（像素）
- `data`：`w * h` 个像素的连续数组，格式由 `EGUI_CONFIG_COLOR_DEPTH` 决定

### Platform Driver

```c
typedef struct egui_platform_ops {
    // --- 基础服务 ---
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    void *(*malloc)(int size);
    void (*free)(void *ptr);
#endif
#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
    void (*vlog)(const char *fmt, va_list args);
    void (*vsprintf)(char *str, const char *fmt, va_list args);
#endif
    void (*assert_handler)(const char *file, int line);
    void (*delay)(uint32_t ms);
    uint32_t (*get_tick_ms)(void);
#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
    void (*memset_fast)(void *s, int c, int n);
    void (*memcpy_fast)(void *dst, const void *src, int n);
#endif
    egui_base_t (*interrupt_disable)(void);
    void (*interrupt_enable)(egui_base_t level);

    // --- 可选 ---
    void (*load_external_resource)(void *dest, uint32_t res_id,
                                   uint32_t start_offset, uint32_t size);
    void *(*mutex_create)(void);
    void (*mutex_lock)(void *mutex);
    void (*mutex_unlock)(void *mutex);
    void (*mutex_destroy)(void *mutex);
    void (*timer_start)(uint32_t expiry_time_ms);
    void (*timer_stop)(void);
    void (*watchdog_feed)(void);
} egui_platform_ops_t;
```

`EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC`、`EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF`、`EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP`
默认都为 `0`。当宏关闭时，对应字段在编译期不会出现在 `egui_platform_ops_t` 中，框架会直接调用标准库实现；
只有在需要平台定制堆、日志重定向或 DMA/优化版 `memset`/`memcpy` 时，才需要开启对应宏并注册回调。

关键函数优先级：

| 函数 | 重要性 | 说明 |
|------|--------|------|
| `get_tick_ms` | 必须 | 返回单调递增的毫秒时间戳 |
| `assert_handler` | 建议 | 断言失败时输出定位信息并停机 |
| `vlog` | 建议 | 启用 `EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF=1` 时提供调试日志输出 |
| `interrupt_disable/enable` | 建议 | 保护共享数据 |
| `memset_fast/memcpy_fast` | 可选 | 启用 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP=1` 时，可接 DMA 或平台优化例程 |
| `malloc/free` | 可选 | 启用 `EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC=1` 时，提供定制动态内存分配 |
| `mutex_*` | 可选 | RTOS 线程安全基础能力 |

## 可选接口

### 异步 DMA 传输

如果 `draw_area` 启动 DMA 异步传输，需实现 `wait_draw_complete`，配合双缓冲使用：

```c
// app_egui_config.h
#define EGUI_CONFIG_PFB_BUFFER_COUNT 2
```

框架在 DMA 传输期间切换到备用缓冲区继续渲染，CPU 和 DMA 并行工作。

### 电源管理

实现 `set_brightness` 和 `set_power`，配合 `egui_screen_on(core)` / `egui_screen_off(core)` 使用。

### 硬件旋转

实现 `set_rotation`，支持 0/90/180/270 度旋转。如果不实现，可启用软件旋转：

```c
#define EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE 1
```

如果是多屏应用，或者不同 core 需要不同旋转策略，优先通过 `egui_display_setup_t.render_config->software_rotation` 在运行时逐屏配置；`EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE` 只负责开启裁剪后的软件旋转支持，并作为默认 runtime 值。

### 2D 硬件加速

如果 MCU 有 DMA2D 或 GPU，实现 `fill_rect`、`blit`、`blend` 可加速渲染。

### 外部资源加载

实现 `load_external_resource`，从 SPI Flash 或 SD 卡加载资源。

### RTOS 互斥锁

在 RTOS 环境下，实现 `mutex_*` 系列函数保护 GUI 数据结构的线程安全。

但要注意，`mutex_*` 只是“能做同步”的基础能力，不等于可以在任意线程里直接修改任意 `core` 的 UI。若平台要跟进多屏或 per-core 线程模型，建议同步遵守下面的边界：

- 每个 display/core 只由所属 GUI 线程或所属轮询上下文直接调用 `egui_polling_work(core)`
- UI 树、动画、定时器、dirty region 只在所属线程内直接修改
- 外部线程改 UI 时，优先通过任务队列、回调投递或等价的 `post_core_task` 入口切回目标 core
- 退出流程先停线程，再释放 display driver、window / panel 和 platform 资源，避免把 shutdown 竞态留到最后

## 主循环模板

### 裸机环境

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

void main(void)
{
    // 1. 硬件初始化
    system_hw_init();

    // 2. 准备平台单例
    egui_port_init(&core);

    // 3. 初始化 GUI 框架并创建 UI
    egui_setup_display(&core, &setup);

    // 4. 主循环
    while (1)
    {
        egui_polling_work(&core);
    }
}
```

调用顺序：`egui_port_init(&core)` -> `egui_setup_display(&core, &setup)` -> 循环 `egui_polling_work(&core)`

### RTOS 环境

```c
void gui_task(void *arg)
{
    egui_port_init(&core);
    egui_setup_display(&core, &setup);

    while (1)
    {
        egui_polling_work(&core);
        os_delay(1);  // 让出 CPU
    }
}
```

### 驱动注册函数

```c
void egui_port_init(egui_core_t *core)
{
    egui_platform_register(core, &my_platform);
}

void port_main(void)
{
    egui_core_t core;
    static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
    egui_color_int_t *pfb_bufs[1] = { egui_pfb };
    egui_display_setup_t setup = {
        .screen_width = EGUI_CONFIG_SCEEN_WIDTH,
        .screen_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .pfb_width = EGUI_CONFIG_PFB_WIDTH,
        .pfb_height = EGUI_CONFIG_PFB_HEIGHT,
        .pfb_buffers = pfb_bufs,
        .pfb_buffer_count = 1,
        .display_driver = &my_display,
        .render_config = NULL,
        .touch_register = egui_port_register_touch_driver, // 可选
        .uicode_init = uicode_disp0_init,
        .display_id = 0,
    };

    egui_port_init(&core);
    egui_setup_display(&core, &setup);
}
```

## 配置宏参考

在 `app_egui_config.h` 中覆盖默认配置：

### 屏幕与 PFB

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_SCEEN_WIDTH` | 240 | 屏幕宽度 |
| `EGUI_CONFIG_SCEEN_HEIGHT` | 320 | 屏幕高度 |
| `EGUI_CONFIG_COLOR_DEPTH` | 16 | 色深（16=RGB565, 32=ARGB8888） |
| `EGUI_CONFIG_COLOR_16_SWAP` | 0 | RGB565 字节交换（默认 runtime 值） |
| `EGUI_CONFIG_PFB_WIDTH` | 屏宽/8 | PFB 块宽度 |
| `EGUI_CONFIG_PFB_HEIGHT` | 屏高/8 | PFB 块高度 |
| `EGUI_CONFIG_MAX_FPS` | 60 | 最大帧率 |

### 功能开关

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 1 | 触摸支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 0 | 按键支持 |
| `EGUI_CONFIG_PFB_BUFFER_COUNT` | 2 | PFB 缓冲区数量 |
| `EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE` | 0 | 软件旋转（编译期裁剪开关，同时也是默认 runtime 值） |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | 0 | 外部资源 |

### PFB 大小选择

复杂 port 或多屏场景下，`EGUI_CONFIG_COLOR_16_SWAP` 更适合作为默认值，真正启用与否通过 `egui_display_setup_t.render_config` 按 display/core 覆盖；
软件旋转则直接通过 `egui_display_setup_t.render_config->software_rotation` 控制，`EGUI_CONFIG_FUNCTION_SOFTWARE_ROTATION_ENABLE` 用于编译期裁剪并提供默认 runtime 值。

PFB 宽高建议是屏幕宽高的整数约数。

| 场景 | 建议 PFB 大小 | RAM 占用（RGB565） |
|------|--------------|--------------------|
| 极低 RAM (<4KB) | 屏宽/16 x 屏高/16 | ~600B |
| 低 RAM (<8KB) | 屏宽/8 x 屏高/8 | ~2.4KB |
| 充足 RAM | 屏宽 x 屏高/4 | ~19.2KB |
| 全屏缓冲 | 屏宽 x 屏高 | ~76.8KB |

## 移植检查清单

- [ ] `egui_port_init()` 中已准备好 Display Driver 和 Platform Driver，并能被当前初始化流程正确绑定
- [ ] `draw_area()` 能正确将像素数据发送到屏幕
- [ ] `get_tick_ms()` 返回单调递增的毫秒时间戳
- [ ] 默认配置下标准库 `memset` / `memcpy` 可用；若启用 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP`，`memset_fast()` / `memcpy_fast()` 工作正确
- [ ] `app_egui_config.h` 中配置了正确的屏幕尺寸和 PFB 大小
- [ ] PFB 宽高优先选为屏幕宽高的整数约数
- [ ] 主循环中按正确顺序调用初始化函数
- [ ] （如有触摸）Touch Driver 返回正确的坐标
- [ ] 编译通过且运行时屏幕有正确渲染输出
