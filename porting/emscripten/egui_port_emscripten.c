#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emscripten.h>

#include "egui.h"
#include "sdl_port.h"

/**
 * Emscripten port: display and platform driver registration.
 * Based on egui_port_pc.c with browser-friendly adaptations.
 */

// ============================================================================
// Display driver
// ============================================================================

static void em_display_init(void)
{
    // SDL init is handled separately in main (VT_init)
}

static void em_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    VT_Fill_Multiple_Colors(x, y, x + w - 1, y + h - 1, (egui_color_int_t *)data);
}

static void em_display_flush(void)
{
    VT_sdl_flush(1);
}

static void em_display_set_brightness(uint8_t level)
{
    EGUI_UNUSED(level);
}

static void em_display_set_power(uint8_t on)
{
    EGUI_UNUSED(on);
}

static const egui_display_driver_ops_t em_display_ops = {
        .init = em_display_init,
        .draw_area = em_display_draw_area,
        .wait_draw_complete = NULL,
        .flush = em_display_flush,
        .set_brightness = em_display_set_brightness,
        .set_power = em_display_set_power,
        .set_rotation = NULL,
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t em_display_driver = {
        .ops = &em_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Platform driver
// ============================================================================

static void *em_malloc(int size)
{
    return malloc(size);
}

static void em_free(void *ptr)
{
    free(ptr);
}

static void em_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void em_assert_handler(const char *file, int line)
{
    printf("Assert@ file = %s, line = %d\n", file, line);
    // Browser-friendly: stop main loop and exit instead of while(1)
    emscripten_cancel_main_loop();
    emscripten_force_exit(1);
}

static void em_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static void em_delay(uint32_t ms)
{
    // Single-threaded: SDL_Delay would block the browser.
    // In the emscripten_set_main_loop model, delays are not needed.
    EGUI_UNUSED(ms);
}

static uint32_t em_get_tick_ms(void)
{
    return (uint32_t)emscripten_get_now();
}

static void em_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static egui_base_t em_interrupt_disable(void)
{
    return 0;
}

static void em_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
static void em_load_external_resource(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    FILE *file;
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    extern char *pc_get_input_file_path(void);
    file = fopen(pc_get_input_file_path(), "rb");
    if (file == NULL)
    {
        EGUI_LOG_ERR("Error opening resource file\r\n");
        return;
    }

    if (fseek(file, res_real_offset, SEEK_SET) != 0)
    {
        EGUI_LOG_ERR("Error seeking in resource file\r\n");
        fclose(file);
        return;
    }

    int read_size = fread(dest, 1, size, file);
    if (read_size != (int)size)
    {
        EGUI_LOG_ERR("Error reading resource file, read_size: %d, size: %d\r\n", read_size, (int)size);
        fclose(file);
        return;
    }

    fclose(file);
}
#endif

static const egui_platform_ops_t em_platform_ops = {
        .malloc = em_malloc,
        .free = em_free,
        .vlog = em_vlog,
        .assert_handler = em_assert_handler,
        .vsprintf = em_vsprintf,
        .delay = em_delay,
        .get_tick_ms = em_get_tick_ms,
        .pfb_clear = em_pfb_clear,
        .interrupt_disable = em_interrupt_disable,
        .interrupt_enable = em_interrupt_enable,
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
        .load_external_resource = em_load_external_resource,
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

static egui_platform_t em_platform = {
        .ops = &em_platform_ops,
};

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_display_driver_register(&em_display_driver);
    egui_platform_register(&em_platform);
}
