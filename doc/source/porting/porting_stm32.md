# STM32 移植

本文档介绍 EmbeddedGUI 在 STM32G0 系列 MCU 上的移植实现，当前基于 SPI 接口驱动 ST7789 LCD 屏幕，并通过 FT6336 提供触摸输入。

## 硬件配置

参考平台：`STM32G0B0RE`

| 组件 | 型号/接口 | 说明 |
|------|----------|------|
| MCU | STM32G0B0RE | Cortex-M0+, 64MHz, 128KB Flash, 144KB RAM |
| LCD | ST7789 | 240x320, RGB565, SPI 接口 |
| 触摸 | FT6336 | 电容触摸，I2C 接口 |

## 文件结构

```text
porting/stm32g0/
├── Porting/
│   ├── egui_port_mcu.c    # Platform / LCD / Touch 初始化与注册
│   ├── port_main.c        # 主循环入口
│   └── port_main.h        # 主循环头文件
├── GCC/
│   └── Makefile.base      # GCC 构建规则
├── MDK-ARM/
│   └── proj_stm32g0.uvprojx
├── STM32G0B0RETX_FLASH.ld # 链接脚本
├── STM32G0B0RETX_RAM.ld   # RAM 链接脚本
└── build.mk               # 构建模块定义
```

## 主流程概览

当前 `stm32g0` port 的主流程位于 `port_main.c`，典型顺序如下：

```c
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

void port_main(void)
{
    egui_init(&core, egui_pfb);
    egui_port_init(&core);
    egui_display_driver_register(&core, egui_port_get_display_driver());
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_port_register_touch_driver(&core);
#endif
    uicode_disp0_init(&core);
    egui_screen_on(&core);

    while (1)
    {
        egui_polling_work(&core);
    }
}
```

如果只是单屏场景，上述流程已经足够。若后续需要多屏、异构分辨率或统一化初始化流程，建议改用 `egui_display_setup_t + egui_setup_display()`。

## Display Driver 注册

`egui_port_mcu.c` 中主要完成三件事：

1. 注册 `platform`
2. 初始化 LCD / Touch HAL 驱动
3. 准备并导出 `egui_display_driver_t`

当前 `stm32g0` 已不再手写一层 `draw_area(core, ...)` 转发。像素输出桥接由 `egui_hal_lcd_register()` 自动完成，板级代码主要补充亮度和旋转能力。

```c
static void port_display_set_brightness(egui_core_t *core, uint8_t level)
{
    EGUI_UNUSED(core);
    egui_hal_stm32g0_set_backlight_level(level);
}

static void port_display_set_rotation(egui_core_t *core, egui_display_rotation_t rotation)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_get(core);
    if (lcd == NULL)
    {
        return;
    }

    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        if (lcd->mirror) lcd->mirror(lcd, 0, 0);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_90:
        if (lcd->mirror) lcd->mirror(lcd, 1, 0);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 1);
        break;
    case EGUI_DISPLAY_ROTATION_180:
        if (lcd->mirror) lcd->mirror(lcd, 1, 1);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_270:
        if (lcd->mirror) lcd->mirror(lcd, 0, 1);
        if (lcd->swap_xy) lcd->swap_xy(lcd, 1);
        break;
    }
}
```

这套结构同时支持同步和异步（DMA）两种传输模式。

## SPI 屏幕通信

典型的 SPI LCD 写入流程如下：

1. 设置列地址范围（`CASET`）
2. 设置行地址范围（`RASET`）
3. 发送写内存命令（`RAMWR`）
4. 通过 SPI 持续发送像素数据

ST7789 的驱动初始化由 `egui_lcd_st7789_init()` 完成，底层 SPI IO 通过 `egui_panel_io_spi_t` 适配。

## SysTick 定时器

平台时间基准来自 HAL 的 SysTick：

```c
static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}
```

`HAL_GetTick()` 每 `1ms` 递增一次，用于动画、定时器和输入超时等逻辑。

## DMA 异步传输

### 基本原理

启用 SPI DMA 后，CPU 可以在数据发送期间继续渲染下一个 PFB 分块，从而提高整体吞吐。

### PFB 缓冲区清零加速

当配置 `EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP=1` 时，可以注册平台自定义 `memset_fast`。当前 `stm32g0` 支持在清零 PFB 时改走 DMA：

```c
#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

static void mcu_memset_fast(void *s, int c, int n)
{
    if (c == 0)
    {
        HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
        HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
        return;
    }

    memset(s, c, n);
}
#endif
```

这样可以保证：

- `c == 0` 时使用 DMA 清零，收益最大
- 非零填充值时回退到标准 `memset`，语义不变

### 多缓冲 + DMA

当前多缓冲已经由 `core` 内部的 `egui_pfb_manager_t` 统一管理，不再由应用层手动切换“主 / 备份 PFB 指针”。主流程只需要把编译期声明的 PFB 数组交给 `egui_init()`：

```c
EGUI_CONFIG_PFB_BUFFER_DECLARE(egui_pfb);
static egui_core_t core;

void port_main(void)
{
    egui_init(&core, egui_pfb);
}
```

DMA 发送完成后，需要在中断回调中通知 core 推进 PFB 队列：

```c
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ST7789_SPI_PORT.Instance)
    {
        egui_pfb_notify_flush_complete(&core);
    }
}
```

## 触摸输入

### FT6336 触摸驱动

当前移植不再由应用层手写触摸轮询转 `motion event`，而是通过 HAL touch driver 统一注册到指定 `core`：

```c
static egui_hal_touch_driver_t s_touch_driver;

void egui_port_register_touch_driver(egui_core_t *core)
{
    egui_hal_touch_config_t touch_config = {
        .width = EGUI_CONFIG_SCREEN_WIDTH,
        .height = EGUI_CONFIG_SCREEN_HEIGHT,
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };

    egui_hal_touch_register(core, &s_touch_driver, &touch_config);
}
```

FT6336 的 `INT / RST` 管脚仍由底层 port 提供，但触摸数据上报已经统一封装在 HAL touch driver 内部，应用层无需再自己转换成 motion event。

## 典型资源占用

基于 `STM32G0B0RE + RGB565 + PFB 30x40` 的典型占用大致如下：

| 项目 | 大小 |
|------|------|
| 核心框架代码 | ~5-8KB Flash |
| PFB 缓冲区 | 2,400B RAM（单缓冲） |
| 每个控件实例 | ~50-200B RAM |
| 字体资源（16px, 4-bit） | ~2-10KB Flash（取决于字符数） |
| 图片资源 | 取决于尺寸和格式 |

## 构建方式

```bash
# 使用 GCC 工具链构建
make all APP=HelloSimple PORT=stm32g0

# 使用 Keil 工程
# porting/stm32g0/MDK-ARM/proj_stm32g0.uvprojx
```

如果只是做尺寸分析或纯编译检查，优先使用仓库统一的脚本和空平台方案，不建议直接依赖 STM32 硬件工程。

## 中断优先级建议

| 中断 | 优先级 | 说明 |
|------|--------|------|
| SysTick | 高 | 保证时间基准准确 |
| SPI DMA | 中 | DMA 传输完成回调 |
| GPIO EXTI（触摸） | 低 | 触摸中断通常不要求极低延迟 |

## 小结

当前 `stm32g0` 移植的关键特点是：

1. 使用 `egui_hal_lcd_register()` 自动桥接显示驱动
2. 使用 `egui_hal_touch_register()` 统一绑定触摸驱动
3. 主流程采用 `egui_init(&core, egui_pfb)` + 手动注册 display / touch 的单屏模式
4. 多缓冲和 DMA flush 由 core 内部 PFB 管理器统一协调

如果后续扩展到多屏或更复杂平台，建议以仓库根目录下的 `porting/PORTING_GUIDE.md` 为准，逐步迁移到 `egui_setup_display()` 方案。
