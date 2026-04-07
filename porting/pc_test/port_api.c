#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_lcd.h"
#include "port_api.h"

/**
 * PC test port (headless): platform registration plus a no-op LCD HAL driver.
 */

static egui_hal_lcd_driver_t s_test_lcd_driver;

static egui_display_driver_ops_t port_display_ops = {0};

static egui_display_driver_t port_display_driver = {
        .ops = &port_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Display HAL driver (headless - no-op)
// ============================================================================

static int test_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static int test_lcd_reset(egui_hal_lcd_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset */
}

static void test_lcd_del(egui_hal_lcd_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

static void test_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(x);
    EGUI_UNUSED(y);
    EGUI_UNUSED(w);
    EGUI_UNUSED(h);
    EGUI_UNUSED(data);
    EGUI_UNUSED(len);
}

static void test_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "PC_TEST_LCD";
    storage->reset = test_lcd_reset;
    storage->init = test_lcd_init;
    storage->del = test_lcd_del;
    storage->draw_area = test_lcd_draw_area;
    storage->mirror = NULL;
    storage->swap_xy = NULL;
    storage->set_power = NULL;
    storage->set_invert = NULL;
    storage->io = NULL;
    storage->set_rst = NULL;
}

// ============================================================================
// Platform driver
// ============================================================================

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
typedef union test_alloc_header
{
    size_t payload_size;
    uintptr_t align_uintptr;
    long double align_long_double;
} test_alloc_header_t;

static egui_port_alloc_stats_t s_alloc_stats;

static void *test_malloc(int size)
{
    test_alloc_header_t *header;
    size_t payload_size;

    if (size <= 0)
    {
        return NULL;
    }

    payload_size = (size_t)size;
    if (payload_size > (size_t)-1 - sizeof(test_alloc_header_t))
    {
        return NULL;
    }

    header = (test_alloc_header_t *)malloc(sizeof(test_alloc_header_t) + payload_size);
    if (header == NULL)
    {
        return NULL;
    }

    header->payload_size = payload_size;

    s_alloc_stats.alloc_count++;
    s_alloc_stats.live_alloc_count++;
    s_alloc_stats.alloc_bytes += payload_size;
    s_alloc_stats.live_bytes += payload_size;
    if (s_alloc_stats.live_bytes > s_alloc_stats.peak_live_bytes)
    {
        s_alloc_stats.peak_live_bytes = s_alloc_stats.live_bytes;
    }

    return (void *)(header + 1);
}

static void test_free(void *ptr)
{
    test_alloc_header_t *header;
    size_t payload_size;

    if (ptr == NULL)
    {
        return;
    }

    header = ((test_alloc_header_t *)ptr) - 1;
    payload_size = header->payload_size;

    s_alloc_stats.free_count++;
    if (s_alloc_stats.live_alloc_count > 0U)
    {
        s_alloc_stats.live_alloc_count--;
    }
    s_alloc_stats.free_bytes += payload_size;
    if (s_alloc_stats.live_bytes >= payload_size)
    {
        s_alloc_stats.live_bytes -= payload_size;
    }
    else
    {
        s_alloc_stats.live_bytes = 0U;
    }

    free((void *)header);
}
#endif

#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
static void test_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void test_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}
#endif

static void test_assert_handler(const char *file, int line)
{
    printf("ASSERT: %s:%d\n", file, line);
}

static uint32_t test_get_tick_ms(void)
{
    return 0;
}

static void test_delay(uint32_t ms)
{
    EGUI_UNUSED(ms);
}

#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
static void test_memset_fast(void *s, int c, int n)
{
    memset(s, c, n);
}

static void test_memcpy_fast(void *dst, const void *src, int n)
{
    memcpy(dst, src, (size_t)n);
}
#endif

static egui_base_t test_interrupt_disable(void)
{
    return 0;
}

static void test_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

static const egui_platform_ops_t test_platform_ops = {
        .assert_handler = test_assert_handler,
        .delay = test_delay,
        .get_tick_ms = test_get_tick_ms,
        .interrupt_disable = test_interrupt_disable,
        .interrupt_enable = test_interrupt_enable,
        .load_external_resource = NULL,
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
        .watchdog_feed = NULL,
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
        .malloc = test_malloc,
        .free = test_free,
#endif
#if EGUI_CONFIG_PLATFORM_CUSTOM_PRINTF
        .vlog = test_vlog,
        .vsprintf = test_vsprintf,
#endif
#if EGUI_CONFIG_PLATFORM_CUSTOM_MEMORY_OP
        .memset_fast = test_memset_fast,
        .memcpy_fast = test_memcpy_fast,
#endif
};

static egui_platform_t test_platform = {
        .ops = &test_platform_ops,
};

void egui_port_reset_alloc_stats(void)
{
#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    memset(&s_alloc_stats, 0, sizeof(s_alloc_stats));
#endif
}

void egui_port_get_alloc_stats(egui_port_alloc_stats_t *out_stats)
{
    if (out_stats == NULL)
    {
        return;
    }

#if EGUI_CONFIG_PLATFORM_CUSTOM_MALLOC
    *out_stats = s_alloc_stats;
#else
    memset(out_stats, 0, sizeof(*out_stats));
#endif
}

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_hal_lcd_config_t lcd_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .color_depth = EGUI_CONFIG_COLOR_DEPTH,
            .color_swap = 0,
            .x_offset = 0,
            .y_offset = 0,
            .invert_color = 0,
            .mirror_x = 0,
            .mirror_y = 0,
            .custom_init = NULL,
    };

    test_lcd_setup(&s_test_lcd_driver);
    egui_hal_lcd_register(&port_display_driver, &s_test_lcd_driver, &lcd_config);

    egui_platform_register(&test_platform);
}
