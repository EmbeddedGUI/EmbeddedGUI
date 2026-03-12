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
    void (*init)(void);
    void (*draw_area)(int16_t x, int16_t y, int16_t w, int16_t h,
                      const egui_color_int_t *data);
    void (*flush)(void);

    // --- 可选 ---
    void (*wait_draw_complete)(void);  // 等待 draw_area 完成（NULL = draw_area 是同步阻塞的）
    void (*set_brightness)(uint8_t level);
    void (*set_power)(uint8_t on);
    void (*set_rotation)(egui_display_rotation_t rotation);
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h,
                      egui_color_int_t color);
    void (*blit)(int16_t dx, int16_t dy, int16_t w, int16_t h,
                 const egui_color_int_t *src);
    void (*blend)(int16_t dx, int16_t dy, int16_t w, int16_t h,
                  const egui_color_int_t *fg, egui_alpha_t alpha);
    void (*wait_vsync)(void);
} egui_display_driver_ops_t;
```

三个必须实现的函数：

| 函数 | 说明 |
|------|------|
| `init` | 初始化 LCD 硬件（SPI、GPIO、LCD 控制器等） |
| `draw_area` | 将 PFB 渲染好的像素块发送到屏幕指定区域 |
| `flush` | 帧刷新完成通知（某些 LCD 需要手动触发刷新） |

`draw_area` 参数说明：
- `x, y`：目标区域左上角坐标
- `w, h`：区域宽高（像素）
- `data`：`w * h` 个像素的连续数组，格式由 `EGUI_CONFIG_COLOR_DEPTH` 决定

### Platform Driver

```c
typedef struct egui_platform_ops {
    // --- 基础服务 ---
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

    // --- 可选 ---
    void (*load_external_resource)(void *dest, uint32_t res_id,
                                   uint32_t start_offset, uint32_t size);
    void *(*mutex_create)(void);
    void (*mutex_lock)(void *mutex);
    void (*mutex_unlock)(void *mutex);
    void (*mutex_destroy)(void *mutex);
    void (*timer_start)(uint32_t expiry_time_ms);
    void (*timer_stop)(void);
    void (*memcpy_fast)(void *dst, const void *src, int n);
    void (*watchdog_feed)(void);
} egui_platform_ops_t;
```

关键函数优先级：

| 函数 | 重要性 | 说明 |
|------|--------|------|
| `get_tick_ms` | 必须 | 返回单调递增的毫秒时间戳 |
| `pfb_clear` | 必须 | 每帧渲染前清零 PFB 缓冲区 |
| `vlog` | 建议 | 调试日志输出 |
| `assert_handler` | 建议 | 断言失败处理 |
| `interrupt_disable/enable` | 建议 | 保护共享数据 |
| `malloc/free` | 可选 | 动态内存分配 |
| `mutex_*` | 可选 | RTOS 线程安全 |

## 可选接口

### 异步 DMA 传输

如果 `draw_area` 启动 DMA 异步传输，需实现 `wait_draw_complete`，配合双缓冲使用：

```c
// app_egui_config.h
#define EGUI_CONFIG_PFB_DOUBLE_BUFFER 1
```

框架在 DMA 传输期间切换到备用缓冲区继续渲染，CPU 和 DMA 并行工作。

### 电源管理

实现 `set_brightness` 和 `set_power`，配合 `egui_screen_on()` / `egui_screen_off()` 使用。

### 硬件旋转

实现 `set_rotation`，支持 0/90/180/270 度旋转。如果不实现，可启用软件旋转：

```c
#define EGUI_CONFIG_SOFTWARE_ROTATION 1
```

### 2D 硬件加速

如果 MCU 有 DMA2D 或 GPU，实现 `fill_rect`、`blit`、`blend` 可加速渲染。

### 外部资源加载

实现 `load_external_resource`，从 SPI Flash 或 SD 卡加载资源。

### RTOS 互斥锁

在 RTOS 环境下，实现 `mutex_*` 系列函数保护 GUI 数据结构的线程安全。

## 主循环模板

### 裸机环境

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

void main(void)
{
    // 1. 硬件初始化
    system_hw_init();

    // 2. 注册驱动
    egui_port_init();

    // 3. 初始化 GUI 框架
    egui_init_config_t init_config = {
        .pfb        = egui_pfb,
        .pfb_backup = NULL,
    };
    egui_init(&init_config);

    // 4. 创建 UI
    uicode_create_ui();

    // 5. 开屏
    egui_screen_on();

    // 6. 主循环
    while (1)
    {
        egui_polling_work();
    }
}
```

调用顺序：`egui_port_init()` -> `egui_init()` -> `uicode_create_ui()` -> `egui_screen_on()` -> 循环 `egui_polling_work()`

### RTOS 环境

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

### 驱动注册函数

```c
void egui_port_init(void)
{
    egui_display_driver_register(&my_display);
    egui_platform_register(&my_platform);
    // 可选
    egui_touch_driver_register(&my_touch);
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
| `EGUI_CONFIG_COLOR_16_SWAP` | 0 | RGB565 字节交换 |
| `EGUI_CONFIG_PFB_WIDTH` | 屏宽/8 | PFB 块宽度 |
| `EGUI_CONFIG_PFB_HEIGHT` | 屏高/8 | PFB 块高度 |
| `EGUI_CONFIG_MAX_FPS` | 60 | 最大帧率 |

### 功能开关

| 宏 | 默认值 | 说明 |
|---|--------|------|
| `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` | 1 | 触摸支持 |
| `EGUI_CONFIG_FUNCTION_SUPPORT_KEY` | 0 | 按键支持 |
| `EGUI_CONFIG_PFB_DOUBLE_BUFFER` | 0 | PFB 双缓冲 |
| `EGUI_CONFIG_SOFTWARE_ROTATION` | 0 | 软件旋转 |
| `EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE` | 0 | 外部资源 |

### PFB 大小选择

PFB 宽高必须是屏幕宽高的整数约数。

| 场景 | 建议 PFB 大小 | RAM 占用（RGB565） |
|------|--------------|--------------------|
| 极低 RAM (<4KB) | 屏宽/16 x 屏高/16 | ~600B |
| 低 RAM (<8KB) | 屏宽/8 x 屏高/8 | ~2.4KB |
| 充足 RAM | 屏宽 x 屏高/4 | ~19.2KB |
| 全屏缓冲 | 屏宽 x 屏高 | ~76.8KB |

## 移植检查清单

- [ ] `egui_port_init()` 中注册了 Display Driver 和 Platform Driver
- [ ] `draw_area()` 能正确将像素数据发送到屏幕
- [ ] `get_tick_ms()` 返回单调递增的毫秒时间戳
- [ ] `pfb_clear()` 能正确清零缓冲区
- [ ] `app_egui_config.h` 中配置了正确的屏幕尺寸和 PFB 大小
- [ ] PFB 宽高是屏幕宽高的整数约数
- [ ] 主循环中按正确顺序调用初始化函数
- [ ] （如有触摸）Touch Driver 返回正确的坐标
- [ ] （如用双缓冲）`pfb_backup` 指向了备用缓冲区
- [ ] 编译通过且运行时屏幕有正确渲染输出
