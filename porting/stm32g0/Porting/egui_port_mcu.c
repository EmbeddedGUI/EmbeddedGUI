/**
 * @file egui_port_mcu.c
 * @brief STM32G0 port using new HAL driver architecture
 *
 * Uses egui_lcd_st7789 and egui_touch_ft6336 drivers through the bridge layer.
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "port_main.h"

/* New HAL drivers */
#include "egui_hal_stm32g0.h"
#include "egui_lcd_st7789.h"
#include "egui_touch_ft6336.h"
#include "egui_driver_bridge.h"

/* ============================================================
 * Driver instances (static allocation)
 * ============================================================ */

static egui_hal_lcd_driver_t s_lcd_driver;
static egui_hal_touch_driver_t s_touch_driver;

/* ============================================================
 * Platform driver
 * ============================================================ */

static void mcu_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void mcu_assert_handler(const char *file, int line)
{
#if EGUI_CONFIG_DEBUG_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
    char s_buf[0x200];
    memset(s_buf, 0, sizeof(s_buf));
    sprintf(s_buf, "vvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\nAssert@ file = %s, line = %d\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", file, line);
    printf("%s", s_buf);
#endif
    while (1)
        ;
}

static void *mcu_malloc(int size)
{
    return malloc(size);
}

static void mcu_free(void *ptr)
{
    free(ptr);
}

static void mcu_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

#if APP_EGUI_CONFIG_USE_DMA_TO_RESET_PFB_BUFFER
extern DMA_HandleTypeDef hdma_memtomem_dma2_channel5;
const egui_color_int_t fixed_0_buffer[EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT] = {0};

static void mcu_pfb_clear(void *s, int n)
{
    HAL_DMA_Start(&hdma_memtomem_dma2_channel5, (uint32_t)fixed_0_buffer, (uint32_t)s, n >> 2);
    HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_channel5, HAL_DMA_FULL_TRANSFER, 1000);
}
#else
static void mcu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}
#endif

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

static const egui_platform_ops_t mcu_platform_ops = {
    .malloc = mcu_malloc,
    .free = mcu_free,
    .vlog = mcu_vlog,
    .assert_handler = mcu_assert_handler,
    .vsprintf = mcu_vsprintf,
    .delay = mcu_delay,
    .get_tick_ms = mcu_get_tick_ms,
    .pfb_clear = mcu_pfb_clear,
    .interrupt_disable = mcu_interrupt_disable,
    .interrupt_enable = mcu_interrupt_enable,
    .load_external_resource = NULL,
    .mutex_create = NULL,
    .mutex_lock = NULL,
    .mutex_unlock = NULL,
    .mutex_destroy = NULL,
    .timer_start = NULL,
    .timer_stop = NULL,
    .memcpy_fast = NULL,
    .watchdog_feed = NULL,
};

static egui_platform_t mcu_platform = {
    .ops = &mcu_platform_ops,
};

/* ============================================================
 * Port initialization
 * ============================================================ */

void egui_port_init(void)
{
    /* Initialize LCD driver using new HAL */
    egui_lcd_st7789_init(&s_lcd_driver,
                         egui_hal_stm32g0_get_lcd_spi_ops(),
                         egui_hal_stm32g0_get_lcd_gpio_ops());

    /* Set backlight control callback */
    s_lcd_driver.set_brightness = egui_hal_stm32g0_set_backlight;

    /* Configure and initialize LCD */
    egui_hal_lcd_config_t lcd_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .x_offset = 0,
        .y_offset = 0,
        .invert_color = 1,
    };
    s_lcd_driver.init(&s_lcd_driver, &lcd_config);

    /* Create display driver from LCD driver via bridge */
    egui_display_driver_t *display = egui_display_driver_from_lcd(&s_lcd_driver);

    /* Initialize Touch driver using new HAL */
    egui_touch_ft6336_init(&s_touch_driver,
                           egui_hal_stm32g0_get_touch_i2c_ops(),
                           egui_hal_stm32g0_get_touch_gpio_ops());

    /* Configure and initialize Touch */
    egui_hal_touch_config_t touch_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .swap_xy = 0,
        .mirror_x = 0,
        .mirror_y = 0,
    };
    s_touch_driver.init(&s_touch_driver, &touch_config);

    /* Register touch driver with bridge - this also registers with Core */
    egui_touch_driver_bridge_register(&s_touch_driver);

    /* Register drivers with Core */
    egui_display_driver_register(display);
    egui_platform_register(&mcu_platform);
}
