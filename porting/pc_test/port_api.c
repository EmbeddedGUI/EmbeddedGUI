#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"

/**
 * PC test port (headless): display and platform driver registration.
 * Replaces the old port_api.c which directly implemented egui_api_* functions.
 */

// ============================================================================
// Display driver (headless - no-op)
// ============================================================================

static void test_display_init(void)
{
}

static void test_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);
}

static void test_display_flush(void)
{
}

static const egui_display_driver_ops_t test_display_ops = {
        .init = test_display_init,
        .draw_area = test_display_draw_area,
        .wait_draw_complete = NULL,
        .flush = test_display_flush,
        .set_brightness = NULL,
        .set_power = NULL,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t test_display_driver = {
        .ops = &test_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Platform driver
// ============================================================================

static void *test_malloc(int size)
{
    return malloc(size);
}

static void test_free(void *ptr)
{
    free(ptr);
}

static void test_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void test_assert_handler(const char *file, int line)
{
    printf("ASSERT: %s:%d\n", file, line);
}

static void test_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static uint32_t test_get_tick_ms(void)
{
    return 0;
}

static void test_delay(uint32_t ms)
{
    EGUI_UNUSED(ms);
}

static void test_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t test_interrupt_disable(void)
{
    return 0;
}

static void test_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

static const egui_platform_ops_t test_platform_ops = {
        .malloc = test_malloc,
        .free = test_free,
        .vlog = test_vlog,
        .assert_handler = test_assert_handler,
        .vsprintf = test_vsprintf,
        .delay = test_delay,
        .get_tick_ms = test_get_tick_ms,
        .pfb_clear = test_pfb_clear,
        .interrupt_disable = test_interrupt_disable,
        .interrupt_enable = test_interrupt_enable,
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

static egui_platform_t test_platform = {
        .ops = &test_platform_ops,
};

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_display_driver_register(&test_display_driver);
    egui_platform_register(&test_platform);
}
