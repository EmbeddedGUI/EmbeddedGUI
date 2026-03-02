# STM32 移植

本文档介绍 EmbeddedGUI 在 STM32G0 系列 MCU 上的移植实现，使用 SPI 接口驱动 ST7789 LCD 屏幕，FT6336 触摸控制器。

## 硬件配置

参考平台：STM32G0B0RE

| 组件 | 型号/接口 | 说明 |
|------|----------|------|
| MCU | STM32G0B0RE | Cortex-M0+, 64MHz, 128KB Flash, 144KB RAM |
| LCD | ST7789 | 240x320, RGB565, SPI 接口 |
| 触摸 | FT6336 | 电容触摸, I2C 接口 |

## 文件结构

```
porting/stm32g0/
├── Porting/
│   ├── egui_port_mcu.c    # Display/Platform Driver 注册
│   ├── port_main.c        # 主循环入口
│   ├── port_main.h        # 主循环头文件
│   ├── app_lcd.c          # LCD 应用层（触摸轮询、DMA 回调）
│   ├── app_lcd.h          # LCD 应用层头文件
│   ├── lcd_st7789.c       # ST7789 SPI 驱动
│   ├── lcd_st7789.h       # ST7789 驱动头文件
│   ├── tc_ft6336.c        # FT6336 触摸驱动
│   └── tc_ft6336.h        # FT6336 驱动头文件
├── GCC/
│   └── Makefile.base      # GCC 构建规则
├── STM32G0B0RETX_FLASH.ld # 链接脚本
├── build.mk               # 构建模块定义
└── app_egui_config.h      # 配置覆盖（在 example 目录下）
```

## SPI 屏幕驱动

### Display Driver 注册

`egui_port_mcu.c` 中注册 Display Driver：

```c
static void mcu_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h,
                                   const egui_color_int_t *data)
{
    app_lcd_draw_data(x, y, w, h, (void *)data);
}

static void mcu_display_draw_area_async(int16_t x, int16_t y, int16_t w, int16_t h,
                                         const egui_color_int_t *data)
{
    st7789_draw_image_dma_async(x, y, w, h, (const uint16_t *)data);
}

static void mcu_display_wait_draw_complete(void)
{
    st7789_wait_dma_complete();
}
```

同时支持同步和异步（DMA）两种传输模式。

### ST7789 SPI 通信

典型的 SPI LCD 写入流程：

1. 设置列地址范围（CASET 命令）
2. 设置行地址范围（RASET 命令）
3. 发送写内存命令（RAMWR）
4. 通过 SPI 发送像素数据

## SysTick 定时器

Platform Driver 使用 HAL 库的 SysTick 提供毫秒时间戳：

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

`HAL_GetTick()` 基于 SysTick 中断，每 1ms 递增一次，为动画、定时器和输入超时提供时间基准。

## DMA 异步传输

### 基本原理

SPI DMA 传输允许 CPU 在数据传输期间继续执行其他任务（如渲染下一个 PFB 块）。

### PFB 缓冲区清零加速

使用 DMA 进行 PFB 缓冲区清零，替代 `memset`：

```c
#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

static void mcu_pfb_clear(void *s, int n)
{
    HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
    HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
}
#endif
```

### 双缓冲 + DMA

`app_lcd.c` 实现了双缓冲机制，在 DMA 传输当前 PFB 时切换到备用缓冲区：

```c
void app_lcd_draw_data(int16_t x, int16_t y, int16_t width, int16_t height, void *data)
{
#if APP_EGUI_CONFIG_LCD_ENABLE_BACKUP_PFB
    if (data == egui_normal_pfb_buffer)
    {
        egui_core_set_pfb_buffer_ptr(egui_pfb_backup);
    }
    else
    {
        egui_core_set_pfb_buffer_ptr(egui_normal_pfb_buffer);
    }
    st7789_draw_image_dma_cache(x, y, width, height, (uint16_t *)data);
#else
    st7789_draw_image(x, y, width, height, (uint16_t *)data);
#endif
}
```

SPI DMA 传输完成中断回调：

```c
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == ST7789_SPI_PORT.Instance)
    {
        egui_pfb_notify_flush_complete();
    }
}
```

### 多缓冲环形队列

`port_main.c` 支持最多 4 个 PFB 缓冲区的环形队列：

```c
static egui_color_int_t egui_pfb[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
static egui_color_int_t egui_pfb_2[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT];
#endif

void port_main(void)
{
    egui_port_init();
    egui_init(&init_config);
#if EGUI_CONFIG_PFB_BUFFER_COUNT >= 2
    egui_pfb_add_buffer(egui_pfb_2);
#endif
    // ...
}
```

## 触摸输入

### FT6336 触摸驱动

通过 I2C 读取触摸坐标，在 `app_lcd.c` 中轮询处理：

```c
void app_lcd_polling_work(void)
{
    if (is_touch_in_process)
    {
        uint16_t x, y;
        if (app_lcd_get_touch_point(&x, &y))
        {
            if (last_touch_x == APP_LCD_INVALID_POINT)
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_DOWN, x, y);
            else
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_MOVE, x, y);
            last_touch_x = x; last_touch_y = y;
        }
        else
        {
            if (last_touch_x != APP_LCD_INVALID_POINT)
            {
                egui_input_add_motion(EGUI_MOTION_EVENT_ACTION_UP, last_touch_x, last_touch_y);
                last_touch_x = APP_LCD_INVALID_POINT;
            }
            is_touch_in_process = 0;
        }
    }
}
```

触摸中断通过 GPIO EXTI 触发：

```c
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == APP_EGUI_TOUCH_INT_PIN)
        is_touch_in_process = 1;
}
```

## 主循环

```c
void port_main(void)
{
    egui_port_init();
    egui_init(&init_config);
    app_lcd_init();
    uicode_create_ui();
    egui_screen_on();

    while (1)
    {
        egui_polling_work();
        app_lcd_polling_work();  // 触摸轮询
    }
}
```

## 典型资源占用

基于 STM32G0B0RE，RGB565，PFB 30x40 的典型占用：

| 项目 | 大小 |
|------|------|
| 核心框架代码 | ~5-8KB Flash |
| PFB 缓冲区 | 2,400B RAM（单缓冲） |
| 每个控件实例 | ~50-200B RAM |
| 字体资源（16px, 4-bit） | ~2-10KB Flash（取决于字符数） |
| 图片资源 | 取决于尺寸和格式 |

## 构建命令

```bash
# 使用 GCC 工具链构建
make all APP=HelloSimple PORT=stm32g0

# 使用空平台（仅分析大小，不含硬件驱动）
make all APP=HelloSimple PORT=stm32g0_empty
```

## 中断优先级

建议的中断优先级配置：

| 中断 | 优先级 | 说明 |
|------|--------|------|
| SysTick | 最高 | 保证时间戳准确 |
| SPI DMA | 中 | DMA 传输完成回调 |
| GPIO EXTI（触摸） | 低 | 触摸中断不需要实时响应 |
