#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "egui.h"
#include "port_main.h"

/**
 * STM32G0 empty port: minimal stub driver registration.
 * Replace the function bodies with actual hardware calls for your platform.
 */

// ============================================================================
// Display driver
// ============================================================================

static void mcu_display_init(void)
{
    // TODO: Initialize your LCD hardware here
}

static void mcu_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    // TODO: Send pixel data to LCD
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);
}

static void mcu_display_flush(void)
{
    // TODO: Flush display if needed
}

static const egui_display_driver_ops_t mcu_display_ops = {
        .init = mcu_display_init,
        .draw_area = mcu_display_draw_area,
        .wait_draw_complete = NULL,
        .flush = mcu_display_flush,
        .set_brightness = NULL,
        .set_power = NULL,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t mcu_display_driver = {
        .ops = &mcu_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Platform driver
// ============================================================================

static void mcu_vlog(const char *format, va_list args)
{
    // TODO: Implement logging (e.g., UART printf)
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
    // TODO: Return millisecond tick (e.g., HAL_GetTick())
    return 0;
}

static void mcu_delay(uint32_t ms)
{
    // TODO: Implement delay (e.g., HAL_Delay(ms))
    EGUI_UNUSED(ms);
}

static void mcu_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t mcu_interrupt_disable(void)
{
    // TODO: Disable interrupts
    return 0;
}

static void mcu_interrupt_enable(egui_base_t level)
{
    // TODO: Enable interrupts
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

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_display_driver_register(&mcu_display_driver);
    egui_platform_register(&mcu_platform);
}
