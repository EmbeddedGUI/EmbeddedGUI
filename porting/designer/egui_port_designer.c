#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"

/**
 * Headless designer port: display and platform driver.
 * Renders to in-memory framebuffer, no SDL dependency.
 * Log output goes to stderr to avoid corrupting stdout IPC pipe.
 */

/* ============================================================================
 * Framebuffer
 * ============================================================================ */

static egui_color_int_t designer_fb[EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT];
static uint8_t designer_rgb888[EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT * 3];

void designer_fb_to_rgb888(void)
{
    int total = EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT;
    for (int i = 0; i < total; i++)
    {
        /* 32-bit color: BGRA on little-endian (see egui_common.h) */
        uint32_t c = designer_fb[i];
        designer_rgb888[i * 3 + 0] = (c >> 16) & 0xFF; /* R */
        designer_rgb888[i * 3 + 1] = (c >> 8) & 0xFF;  /* G */
        designer_rgb888[i * 3 + 2] = c & 0xFF;         /* B */
    }
}

uint8_t *designer_get_rgb888(void)
{
    return designer_rgb888;
}

uint32_t designer_get_rgb888_size(void)
{
    return EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT * 3;
}

/* ============================================================================
 * Display driver
 * ============================================================================ */

static void designer_display_init(void)
{
    memset(designer_fb, 0, sizeof(designer_fb));
}

static void designer_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    for (int16_t row = 0; row < h; row++)
    {
        for (int16_t col = 0; col < w; col++)
        {
            designer_fb[(y + row) * EGUI_CONFIG_SCEEN_WIDTH + (x + col)] = *data++;
        }
    }
}

static void designer_display_flush(void)
{
    /* No-op: frame stays in memory, sent on CMD_RENDER */
}

static void designer_display_set_brightness(uint8_t level)
{
    EGUI_UNUSED(level);
}

static void designer_display_set_power(uint8_t on)
{
    EGUI_UNUSED(on);
}

static const egui_display_driver_ops_t designer_display_ops = {
        .init = designer_display_init,
        .draw_area = designer_display_draw_area,
        .wait_draw_complete = NULL,
        .flush = designer_display_flush,
        .set_brightness = designer_display_set_brightness,
        .set_power = designer_display_set_power,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t designer_display_driver = {
        .ops = &designer_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

/* ============================================================================
 * Platform driver
 * ============================================================================ */

static void *designer_malloc(int size)
{
    return malloc(size);
}

static void designer_free(void *ptr)
{
    free(ptr);
}

static void designer_vlog(const char *format, va_list args)
{
    /* Write to stderr to avoid corrupting stdout IPC pipe */
    vfprintf(stderr, format, args);
}

static void designer_assert_handler(const char *file, int line)
{
    fprintf(stderr, "Assert@ file = %s, line = %d\n", file, line);
    exit(1);
}

static void designer_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

#ifdef _WIN32
#include <windows.h>
static void designer_delay(uint32_t ms)
{
    Sleep(ms);
}

static uint32_t designer_get_tick_ms(void)
{
    return GetTickCount();
}
#else
#include <time.h>
#include <unistd.h>
static void designer_delay(uint32_t ms)
{
    usleep(ms * 1000);
}

static uint32_t designer_get_tick_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

static void designer_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t designer_interrupt_disable(void)
{
    return 0;
}

static void designer_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
static void designer_load_external_resource(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    FILE *file;
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    extern char *pc_get_input_file_path(void);
    file = fopen(pc_get_input_file_path(), "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening resource file\n");
        return;
    }

    if (fseek(file, res_real_offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Error seeking in resource file\n");
        fclose(file);
        return;
    }

    int read_size = fread(dest, 1, size, file);
    if (read_size != (int)size)
    {
        fprintf(stderr, "Error reading resource file, read_size: %d, size: %d\n", read_size, (int)size);
    }

    fclose(file);
}
#endif

static const egui_platform_ops_t designer_platform_ops = {
        .malloc = designer_malloc,
        .free = designer_free,
        .vlog = designer_vlog,
        .assert_handler = designer_assert_handler,
        .vsprintf = designer_vsprintf,
        .delay = designer_delay,
        .get_tick_ms = designer_get_tick_ms,
        .pfb_clear = designer_pfb_clear,
        .interrupt_disable = designer_interrupt_disable,
        .interrupt_enable = designer_interrupt_enable,
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
        .load_external_resource = designer_load_external_resource,
#else
        .load_external_resource = NULL,
#endif
        .mutex_create = NULL,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .mutex_destroy = NULL,
        .timer_start = NULL,
        .timer_stop = NULL,
        .memcpy_fast = NULL,
        .watchdog_feed = NULL,
};

static egui_platform_t designer_platform = {
        .ops = &designer_platform_ops,
};

/* ============================================================================
 * Port initialization
 * ============================================================================ */

void egui_port_init(void)
{
    egui_display_driver_register(&designer_display_driver);
    egui_platform_register(&designer_platform);
}
