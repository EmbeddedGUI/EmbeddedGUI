# 自定义平台移植

本文档提供从零开始将 EmbeddedGUI 移植到新硬件平台的完整步骤。

## 移植步骤清单

### 第一步：创建移植目录

复制空模板作为起点：

```bash
cp -r porting/stm32g0_empty porting/your_platform
```

目录结构：

```
porting/your_platform/
├── Porting/
│   ├── egui_port_mcu.c      # Display + Platform Driver 注册
│   └── port_main.c          # 主循环入口
├── app_egui_config.h        # 配置覆盖（放在 example 目录下）
└── build.mk                 # 构建模块定义
```

### 第二步：配置屏幕参数

在你的示例目录下创建 `app_egui_config.h`：

```c
#ifndef _APP_EGUI_CONFIG_H_
#define _APP_EGUI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

// 屏幕尺寸
#define EGUI_CONFIG_SCEEN_WIDTH  240
#define EGUI_CONFIG_SCEEN_HEIGHT 320

// 色深
#define EGUI_CONFIG_COLOR_DEPTH  16    // RGB565

// PFB 大小（必须是屏幕尺寸的整数约数）
#define EGUI_CONFIG_PFB_WIDTH    30    // 240 / 8
#define EGUI_CONFIG_PFB_HEIGHT   40    // 320 / 8

// RGB565 字节交换（SPI 8-bit 接口常用）
#define EGUI_CONFIG_COLOR_16_SWAP 0

#ifdef __cplusplus
}
#endif

#endif
```

### 第三步：实现 Display Driver

在 `egui_port_mcu.c` 中实现显示驱动：

```c
#include "egui.h"

// --- Display Driver ---

static void my_display_init(void)
{
    // 初始化 LCD 硬件（SPI、GPIO、LCD 控制器）
    lcd_hw_init();
}

static void my_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h,
                                  const egui_color_int_t *data)
{
    lcd_set_window(x, y, x + w - 1, y + h - 1);
    lcd_write_data((const uint8_t *)data, w * h * sizeof(egui_color_int_t));
}

static void my_display_flush(void)
{
    // 如果 LCD 需要手动触发刷新，在此处理
    // 大多数 SPI LCD 不需要，留空即可
}

static const egui_display_driver_ops_t my_display_ops = {
    .init               = my_display_init,
    .draw_area          = my_display_draw_area,
    .wait_draw_complete = NULL,    // draw_area 是同步阻塞的，无需等待
    .flush              = my_display_flush,
    .set_brightness     = NULL,
    .set_power          = NULL,
    .set_rotation       = NULL,
    .fill_rect          = NULL,
    .blit               = NULL,
    .blend              = NULL,
    .wait_vsync         = NULL,
};

static egui_display_driver_t my_display = {
    .ops             = &my_display_ops,
    .physical_width  = EGUI_CONFIG_SCEEN_WIDTH,
    .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
    .rotation        = EGUI_DISPLAY_ROTATION_0,
    .brightness      = 255,
    .power_on        = 1,
};
```

### 第四步：实现 Platform Driver

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
    while (1) { }
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
    HAL_Delay(ms);
}

static void my_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
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
    .malloc                 = NULL,
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

### 第五步：注册驱动和主循环

```c
void egui_port_init(void)
{
    egui_display_driver_register(&my_display);
    egui_platform_register(&my_platform);
}

static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];

void port_main(void)
{
    egui_port_init();

    egui_init_config_t init_config = {
        .pfb        = egui_pfb,
        .pfb_backup = NULL,
    };
    egui_init(&init_config);
    uicode_create_ui();
    egui_screen_on();

    while (1)
    {
        egui_polling_work();
    }
}
```

### 第六步：配置构建系统

`build.mk`：

```makefile
EGUI_CODE_SRC += porting/your_platform/Porting/egui_port_mcu.c
EGUI_CODE_SRC += porting/your_platform/Porting/port_main.c
EGUI_CODE_INCLUDE += -Iporting/your_platform
```

## 调试检查点

移植过程中，建议按以下顺序逐步验证，每一步确认正确后再进入下一步。

### 检查点 1：纯色填充

验证 `draw_area` 基本工作。在 `uicode_create_ui` 中不创建任何控件，框架会用背景色（黑色）清屏。

预期结果：屏幕显示纯黑色。

如果屏幕花屏或无显示：
- 检查 SPI 时序和 GPIO 配置
- 检查 LCD 初始化命令序列
- 检查 `EGUI_CONFIG_COLOR_16_SWAP` 是否需要设为 1
- 用示波器或逻辑分析仪检查 SPI 信号

### 检查点 2：矩形绘制

创建一个简单的彩色视图：

```c
static egui_view_t test_view;

void uicode_create_ui(void)
{
    egui_view_init(EGUI_VIEW_OF(&test_view));
    egui_view_set_position(EGUI_VIEW_OF(&test_view), 50, 50);
    egui_view_set_size(EGUI_VIEW_OF(&test_view), 100, 100);
    egui_view_set_bg_color(EGUI_VIEW_OF(&test_view), EGUI_COLOR_RED, EGUI_ALPHA_100);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_view));
}
```

预期结果：屏幕上显示一个红色矩形。

如果矩形位置或大小不对：
- 检查 `draw_area` 中的坐标映射
- 检查 `EGUI_CONFIG_SCEEN_WIDTH/HEIGHT` 是否与实际屏幕匹配
- 检查 LCD 的 CASET/RASET 命令参数

### 检查点 3：文本渲染

添加一个 Label 控件：

```c
static egui_view_label_t label;
EGUI_VIEW_LABEL_PARAMS_INIT(label_params, 10, 10, 200, 30,
    "Hello EGUI!", NULL, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

void uicode_create_ui(void)
{
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label), &label_params);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label));
}
```

预期结果：屏幕上显示白色文本 "Hello EGUI!"。

如果文本不显示或乱码：
- 确认字体资源已正确生成和链接
- 检查 `app_egui_resource_generate.h` 中的字体声明

### 检查点 4：触摸输入

如果有触摸屏，添加一个按钮测试触摸响应：

```c
static egui_view_button_t button;

void on_button_click(egui_view_t *self)
{
    EGUI_LOG_INF("Button clicked!\n");
}

void uicode_create_ui(void)
{
    egui_view_button_init(EGUI_VIEW_OF(&button));
    egui_view_set_position(EGUI_VIEW_OF(&button), 50, 50);
    egui_view_set_size(EGUI_VIEW_OF(&button), 120, 40);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button), on_button_click);
    egui_core_add_user_root_view(EGUI_VIEW_OF(&button));
}
```

预期结果：点击按钮区域时，串口输出 "Button clicked!"。

如果触摸无响应：
- 检查触摸 IC 的 I2C 通信
- 检查触摸坐标是否与屏幕坐标系一致
- 确认 `EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH` 为 1

## 常见问题排查

### 屏幕无显示

1. 检查 LCD 供电和背光
2. 检查 SPI 连线（MOSI、SCK、CS、DC、RST）
3. 用逻辑分析仪确认 SPI 有数据输出
4. 检查 LCD 初始化命令序列是否正确

### 屏幕花屏

1. `EGUI_CONFIG_COLOR_16_SWAP`：SPI 8-bit 模式下通常需要设为 1
2. 检查 `draw_area` 中的坐标计算
3. 确认 PFB 宽高是屏幕宽高的整数约数

### 画面撕裂

1. 启用帧同步（TE 信号）
2. 或使用双缓冲 + DMA 异步传输

### 帧率低

1. 增大 PFB 尺寸减少 `draw_area` 调用次数
2. 提高 SPI 时钟频率
3. 启用 DMA 异步传输
4. 启用双缓冲让 CPU 和 DMA 并行

### 动画不流畅

1. 检查 `get_tick_ms` 返回值是否单调递增
2. 确认 SysTick 中断正常工作
3. 检查是否有其他中断长时间占用 CPU

### RAM 不足

1. 减小 PFB 尺寸
2. 使用 Page union 模式共享控件内存
3. 关闭不需要的功能模块
4. 将大资源放到外部存储

### 编译错误

1. 确认 `app_egui_config.h` 在 include 路径中
2. 确认 `build.mk` 正确包含了源文件和头文件路径
3. 检查工具链版本兼容性（需要 C99 支持）
