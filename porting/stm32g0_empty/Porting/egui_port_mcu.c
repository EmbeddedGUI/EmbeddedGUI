/**
 * @file egui_port_mcu.c
 * @brief STM32G0 empty port template using new HAL driver architecture
 *
 * This is a template showing how to use the new HAL driver architecture
 * with the unified Panel IO interface.
 * Replace the TODO sections with your actual hardware implementations.
 *
 * To use the new HAL drivers:
 * 1. Implement Bus IO operations (SPI/I2C) for your hardware
 * 2. Create Panel IO handles (egui_panel_io_spi_t / egui_panel_io_i2c_t)
 * 3. Use egui_lcd_st7789 or other LCD drivers from driver/lcd/
 * 4. Use egui_touch_ft6336 or other touch drivers from driver/touch/
 * 5. Register display with Core's egui_display_driver_t
 * 6. Register touch with egui_hal_touch_register() (one-line helper)
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_lcd.h"
#include "egui_touch.h"
#include "port_main.h"

/* Uncomment these includes when you implement the concrete panel / touch drivers */
// #include "egui_lcd_st7789.h"
// #include "egui_touch_ft6336.h"
// #include "egui_panel_io_spi.h"
// #include "egui_panel_io_i2c.h"

/* ============================================================
 * Bus IO Operations - Implement these for your hardware
 * ============================================================ */

#if 0  /* Example SPI operations - uncomment and implement */
static void lcd_spi_init(void)
{
    /* TODO: Initialize SPI peripheral */
}

static int lcd_spi_write(const uint8_t *data, uint32_t len)
{
    /* TODO: Send data via SPI, return 0 on success */
    return 0;
}

static void lcd_spi_wait_complete(void)
{
    /* TODO: Wait for DMA transfer to complete */
}

static const egui_bus_spi_ops_t lcd_spi_ops = {
    .init = lcd_spi_init,
    .deinit = NULL,
    .write = lcd_spi_write,
    .wait_complete = lcd_spi_wait_complete,  /* Set to NULL for sync mode */
    .read = NULL,
};

static void lcd_set_rst(uint8_t level)
{
    /* TODO: Set LCD RST pin */
}

static void lcd_set_dc(uint8_t level)
{
    /* TODO: Set LCD DC pin */
}

static void lcd_set_brightness(uint8_t level)
{
    /* TODO: Control backlight */
}
#endif

/* ============================================================
 * Display HAL driver (stub implementation)
 * ============================================================ */

static egui_hal_lcd_driver_t s_mcu_lcd_driver;

static egui_display_driver_ops_t port_display_ops = {0};

static egui_display_driver_t port_display_driver = {
    .ops = &port_display_ops,
    .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
    .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
    .rotation = EGUI_DISPLAY_ROTATION_0,
    .brightness = 255,
    .power_on = 1,
};

static int mcu_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static int mcu_lcd_reset(egui_hal_lcd_driver_t *self)
{
    (void)self;
    return 0; /* TODO: Implement hardware reset */
}

static void mcu_lcd_del(egui_hal_lcd_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

static void mcu_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
{
    /* TODO: Replace this stub with your real LCD transfer path. */
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);
    EGUI_UNUSED(len);
}

static void mcu_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "STM32G0_EMPTY_LCD";
    storage->reset = mcu_lcd_reset;
    storage->init = mcu_lcd_init;
    storage->del = mcu_lcd_del;
    storage->draw_area = mcu_lcd_draw_area;
    storage->mirror = NULL;
    storage->swap_xy = NULL;
    storage->set_power = NULL;
    storage->set_invert = NULL;
    storage->io = NULL;
    storage->set_rst = NULL;
}

/* ============================================================
 * Platform driver
 * ============================================================ */

static void mcu_vlog(const char *format, va_list args)
{
    /* TODO: Implement logging (e.g., UART printf) */
    EGUI_UNUSED(format);
    EGUI_UNUSED(args);
}

static void mcu_assert_handler(const char *file, int line)
{
    EGUI_UNUSED(file);
    EGUI_UNUSED(line);
    while (1)
        ;
}

static void mcu_vsprintf(char *str, const char *format, va_list args)
{
    EGUI_UNUSED(str);
    EGUI_UNUSED(format);
    EGUI_UNUSED(args);
}

static uint32_t mcu_get_tick_ms(void)
{
    /* TODO: Return millisecond tick (e.g., HAL_GetTick()) */
    return 0;
}

static void mcu_delay(uint32_t ms)
{
    /* TODO: Implement delay (e.g., HAL_Delay(ms)) */
    EGUI_UNUSED(ms);
}

static void mcu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t mcu_interrupt_disable(void)
{
    /* TODO: Disable interrupts */
    return 0;
}

static void mcu_interrupt_enable(egui_base_t level)
{
    /* TODO: Enable interrupts */
    EGUI_UNUSED(level);
}

static const egui_platform_ops_t mcu_platform_ops = {
    .malloc = NULL,
    .free = NULL,
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
    egui_hal_lcd_config_t lcd_config = {
        .width = EGUI_CONFIG_SCEEN_WIDTH,
        .height = EGUI_CONFIG_SCEEN_HEIGHT,
        .color_depth = EGUI_CONFIG_COLOR_DEPTH,
        .color_swap = EGUI_CONFIG_COLOR_16_SWAP,
        .x_offset = 0,
        .y_offset = 0,
        .invert_color = 0,
        .mirror_x = 0,
        .mirror_y = 0,
        .custom_init = NULL,
    };

    mcu_lcd_setup(&s_mcu_lcd_driver);
    egui_hal_lcd_register(&port_display_driver, &s_mcu_lcd_driver, &lcd_config);

    /* When you wire real hardware, replace the stub setup above with a concrete LCD driver.
     *
     * static egui_hal_lcd_driver_t lcd_driver;
     * static egui_hal_touch_driver_t touch_driver;
     * static egui_panel_io_spi_t lcd_io;
     * static egui_panel_io_i2c_t touch_io;
     *
     * // Create Panel IO handles
     * egui_panel_io_spi_init(&lcd_io, &lcd_spi_ops, lcd_set_dc, NULL);
     * egui_panel_io_i2c_init(&touch_io, &touch_i2c_ops, 0x70);
     *
     * // Initialize LCD driver with Panel IO
     * egui_lcd_st7789_init(&lcd_driver, &lcd_io.base, lcd_set_rst);
     *
     * // Register LCD with Core (handles reset, init, display driver creation):
     * egui_hal_lcd_register(&port_display_driver, &lcd_driver, &lcd_config);
     *
     * // Initialize Touch driver with Panel IO (optional)
     * egui_touch_ft6336_init(&touch_driver, &touch_io.base,
     *                         touch_set_rst, NULL, touch_get_int);
     *
     * // Register touch with Core (handles reset, init, touch driver creation):
     * egui_hal_touch_config_t touch_config = { ... };
     * egui_hal_touch_register(&touch_driver, &touch_config);
     */

    egui_platform_register(&mcu_platform);
}
