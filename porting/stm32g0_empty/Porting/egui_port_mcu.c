/**
 * @file egui_port_mcu.c
 * @brief STM32G0 empty port template using new HAL driver architecture
 *
 * This is a template showing how to use the new HAL driver architecture.
 * Replace the TODO sections with your actual hardware implementations.
 *
 * To use the new HAL drivers:
 * 1. Implement Bus IO operations (SPI/I2C/GPIO) for your hardware
 * 2. Use egui_lcd_st7789 or other LCD drivers from driver/lcd/
 * 3. Use egui_touch_ft6336 or other touch drivers from driver/touch/
 * 4. Connect them via the bridge layer
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_driver_bridge.h"
#include "port_main.h"

/* Uncomment these includes when you implement the concrete panel / touch drivers */
// #include "egui_lcd_st7789.h"
// #include "egui_touch_ft6336.h"

/* ============================================================
 * Bus IO Operations - Implement these for your hardware
 * ============================================================ */

#if 0 /* Example SPI operations - uncomment and implement */
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

static void lcd_gpio_set_rst(uint8_t level)
{
    /* TODO: Set LCD RST pin */
}

static void lcd_gpio_set_dc(uint8_t level)
{
    /* TODO: Set LCD DC pin */
}

static const egui_bus_gpio_ops_t lcd_gpio_ops = {
    .init = NULL,
    .deinit = NULL,
    .set_rst = lcd_gpio_set_rst,
    .set_dc = lcd_gpio_set_dc,
    .set_cs = NULL,
    .set_backlight = NULL,
    .get_int = NULL,
};
#endif

/* ============================================================
 * Display HAL driver (stub implementation)
 * ============================================================ */

static egui_hal_lcd_driver_t s_mcu_lcd_driver;

static int mcu_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static void mcu_lcd_deinit(egui_hal_lcd_driver_t *self)
{
    EGUI_UNUSED(self);
}

static void mcu_lcd_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
}

static void mcu_lcd_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    /* TODO: Replace this stub with your real LCD transfer path. */
    EGUI_UNUSED(self);
    EGUI_UNUSED(data);
    EGUI_UNUSED(len);
}

static void mcu_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "STM32G0_EMPTY_LCD";
    storage->bus_type = EGUI_BUS_TYPE_SPI;
    storage->init = mcu_lcd_init;
    storage->deinit = mcu_lcd_deinit;
    storage->set_window = mcu_lcd_set_window;
    storage->write_pixels = mcu_lcd_write_pixels;
    storage->wait_dma_complete = NULL;
    storage->set_rotation = NULL;
    storage->set_brightness = NULL;
    storage->set_power = NULL;
    storage->set_invert = NULL;
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
    };

    mcu_lcd_setup(&s_mcu_lcd_driver);
    s_mcu_lcd_driver.init(&s_mcu_lcd_driver, &lcd_config);
    egui_display_driver_register(egui_display_driver_from_lcd(&s_mcu_lcd_driver));

    /* When you wire real hardware, replace the stub setup above with a concrete LCD driver.
     *
     * static egui_hal_lcd_driver_t lcd_driver;
     * static egui_hal_touch_driver_t touch_driver;
     *
     * // Initialize LCD driver
     * egui_lcd_st7789_init(&lcd_driver, &lcd_spi_ops, &lcd_gpio_ops);
     * egui_hal_lcd_config_t lcd_config = {
     *     .width = EGUI_CONFIG_SCEEN_WIDTH,
     *     .height = EGUI_CONFIG_SCEEN_HEIGHT,
     *     .color_depth = EGUI_CONFIG_COLOR_DEPTH,
     *     .color_swap = EGUI_CONFIG_COLOR_16_SWAP,
     *     .x_offset = 0,
     *     .y_offset = 0,
     *     .invert_color = 0,
     *     .mirror_x = 0,
     *     .mirror_y = 0,
     * };
     * lcd_driver.init(&lcd_driver, &lcd_config);
     *
     * // Create display driver from LCD driver via bridge
     * egui_display_driver_t *display = egui_display_driver_from_lcd(&lcd_driver);
     * egui_display_driver_register(display);
     *
     * // Initialize Touch driver (optional)
     * egui_touch_ft6336_init(&touch_driver, &touch_i2c_ops, &touch_gpio_ops);
     * egui_hal_touch_config_t touch_config = {
     *     .width = EGUI_CONFIG_SCEEN_WIDTH,
     *     .height = EGUI_CONFIG_SCEEN_HEIGHT,
     *     .swap_xy = 0,
     *     .mirror_x = 0,
     *     .mirror_y = 0,
     * };
     * touch_driver.init(&touch_driver, &touch_config);
     *
     * // Register touch driver with bridge - this also registers with Core
     * // Touch polling is handled automatically by egui_polling_work()
     * egui_touch_driver_bridge_register(&touch_driver);
     */

    egui_platform_register(&mcu_platform);
}
