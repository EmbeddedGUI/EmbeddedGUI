/**
 * @file egui_port_mcu.c
 * @brief STM32G0 port using new HAL driver architecture with Panel IO
 *
 * Uses egui_lcd_st7789 and egui_touch_ft6336 drivers, registered directly
 * with Core's egui_display_driver_t and egui_touch_driver_t interfaces.
 * LCD communicates via Panel IO SPI, Touch via Panel IO I2C.
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_touch.h"
#include "port_main.h"

/* New HAL drivers */
#include "egui_hal_stm32g0.h"
#include "egui_lcd_st7789.h"
#include "egui_touch_ft6336.h"
#include "egui_panel_io_spi.h"
#include "egui_panel_io_i2c.h"

/* ============================================================
 * Driver instances (static allocation)
 * ============================================================ */

static egui_hal_lcd_driver_t s_lcd_driver;
static egui_hal_touch_driver_t s_touch_driver;

/* Panel IO handles */
static egui_panel_io_spi_t s_lcd_io;
static egui_panel_io_i2c_t s_touch_io;

/* FT6336 I2C address (0x38 << 1 = 0x70) */
#define FT6336_I2C_ADDR 0x70

/* ============================================================
 * Platform driver
 * ============================================================ */

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

static uint32_t mcu_get_tick_ms(void)
{
    return HAL_GetTick();
}

static void mcu_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
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
#else
static void mcu_memset_fast(void *s, int c, int n)
{
    memset(s, c, n);
}
#endif
#endif /* EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP */

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
        .assert_handler = mcu_assert_handler,
        .delay = mcu_delay,
        .get_tick_ms = mcu_get_tick_ms,
#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
        .memset_fast = mcu_memset_fast,
        .memcpy_fast = NULL,
#endif
        .interrupt_disable = mcu_interrupt_disable,
        .interrupt_enable = mcu_interrupt_enable,
        .load_external_resource = NULL,
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
        .watchdog_feed = NULL,
};

static egui_platform_t mcu_platform = {
        .ops = &mcu_platform_ops,
};

/* ============================================================
 * Board-specific display callbacks (patched after registration)
 * ============================================================ */

static void port_display_set_brightness(uint8_t level)
{
    egui_hal_stm32g0_set_backlight_level(level);
}

static void port_display_set_rotation(egui_display_rotation_t rotation)
{
    egui_hal_lcd_driver_t *lcd = egui_hal_lcd_get();
    if (!lcd)
    {
        return;
    }
    switch (rotation)
    {
    case EGUI_DISPLAY_ROTATION_0:
        if (lcd->mirror)
            lcd->mirror(lcd, 0, 0);
        if (lcd->swap_xy)
            lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_90:
        if (lcd->mirror)
            lcd->mirror(lcd, 1, 0);
        if (lcd->swap_xy)
            lcd->swap_xy(lcd, 1);
        break;
    case EGUI_DISPLAY_ROTATION_180:
        if (lcd->mirror)
            lcd->mirror(lcd, 1, 1);
        if (lcd->swap_xy)
            lcd->swap_xy(lcd, 0);
        break;
    case EGUI_DISPLAY_ROTATION_270:
        if (lcd->mirror)
            lcd->mirror(lcd, 0, 1);
        if (lcd->swap_xy)
            lcd->swap_xy(lcd, 1);
        break;
    }
}

static const egui_display_driver_ops_t mcu_display_ops = {
        .set_brightness = port_display_set_brightness,
        .set_rotation = port_display_set_rotation,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t mcu_display_driver = {
        .ops = &mcu_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_270,
        .brightness = 255,
        .power_on = 1,
};

/* ============================================================
 * Port initialization
 * ============================================================ */

void egui_port_init(void)
{
    /* Register platform */
    egui_platform_register(&mcu_platform);

    /* Create LCD SPI IO handle */
    egui_panel_io_spi_init(&s_lcd_io, egui_hal_stm32g0_get_lcd_spi_ops(), egui_hal_stm32g0_lcd_set_dc, NULL); /* CS is hardware-controlled */

    /* Initialize LCD driver with Panel IO handle */
    egui_lcd_st7789_init(&s_lcd_driver, &s_lcd_io.base, egui_hal_stm32g0_lcd_set_rst);

    /* Register LCD with Core (handles reset, init, display driver creation) */
    egui_hal_lcd_config_t lcd_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .x_offset = 0,
            .y_offset = 0,
            .invert_color = 1,
            .custom_init = NULL,
    };
    egui_hal_lcd_register(&mcu_display_driver, &s_lcd_driver, &lcd_config);

    /* Create Touch I2C IO handle */
    egui_panel_io_i2c_init(&s_touch_io, egui_hal_stm32g0_get_touch_i2c_ops(), FT6336_I2C_ADDR);

    /* Initialize Touch driver with Panel IO handle */
    egui_touch_ft6336_init(&s_touch_driver, &s_touch_io.base, egui_hal_stm32g0_touch_set_rst, NULL, /* set_int - not needed for FT6336 */
                           egui_hal_stm32g0_touch_get_int);

    /* Register touch with Core (handles reset, init, touch driver creation) */
    egui_hal_touch_config_t touch_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };
    egui_hal_touch_register(&s_touch_driver, &touch_config);
}
