#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"
#include "sdl_port.h"

/**
 * PC port: display and platform driver registration.
 * Replaces the old api_pc.c which directly implemented egui_api_* functions.
 */

// ============================================================================
// Display driver
// ============================================================================

static void pc_display_init(void)
{
    // SDL init is handled separately in main.c (VT_init)
}

static void pc_display_draw_area(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data)
{
    VT_Fill_Multiple_Colors(x, y, x + w - 1, y + h - 1, (egui_color_int_t *)data);
}

static void pc_display_flush(void)
{
    VT_sdl_flush(1);
}

static void pc_display_set_brightness(uint8_t level)
{
    // PC simulator: no-op for brightness
    EGUI_UNUSED(level);
}

static void pc_display_set_power(uint8_t on)
{
    // PC simulator: no-op for power
    EGUI_UNUSED(on);
}

static const egui_display_driver_ops_t pc_display_ops = {
        .init = pc_display_init,
        .draw_area = pc_display_draw_area,
        .wait_draw_complete = NULL,
        .flush = pc_display_flush,
        .set_brightness = pc_display_set_brightness,
        .set_power = pc_display_set_power,
        .set_rotation = NULL, // Use software rotation
        .fill_rect = NULL,
        .blit = NULL,
        .blend = NULL,
        .wait_vsync = NULL,
};

static egui_display_driver_t pc_display_driver = {
        .ops = &pc_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

// ============================================================================
// Platform driver
// ============================================================================

static void *pc_malloc(int size)
{
    return malloc(size);
}

static void pc_free(void *ptr)
{
    free(ptr);
}

static void pc_vlog(const char *format, va_list args)
{
    vprintf(format, args);
}

static void pc_assert_handler(const char *file, int line)
{
#if EGUI_CONFIG_DEBUG_LOG_LEVEL >= EGUI_LOG_IMPL_LEVEL_DBG
    char s_buf[0x200];
    memset(s_buf, 0, sizeof(s_buf));
    sprintf(s_buf, "vvvvvvvvvvvvvvvvvvvvvvvvvvvv\n\nAssert@ file = %s, line = %d\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", file, line);
    printf("%s", s_buf);
#else
    printf("Assert@ file = %s, line = %d\n", file, line);
#endif
    while (1)
        ;
}

static void pc_vsprintf(char *str, const char *format, va_list args)
{
    vsprintf(str, format, args);
}

static void pc_delay(uint32_t ms)
{
    sdl_port_sleep(ms);
}

static uint32_t pc_get_tick_ms(void)
{
    return sdl_get_system_timestamp_ms();
}

static void pc_pfb_clear(void *s, int n)
{
    memset(s, 0, n);
}

static SDL_mutex *pc_isr_mutex = NULL;

static egui_base_t pc_interrupt_disable(void)
{
    if (pc_isr_mutex == NULL)
    {
        pc_isr_mutex = SDL_CreateMutex();
    }
    SDL_LockMutex(pc_isr_mutex);
    return 0;
}

static void pc_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
    if (pc_isr_mutex != NULL)
    {
        SDL_UnlockMutex(pc_isr_mutex);
    }
}

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
static void pc_load_external_resource(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    FILE *file;
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;

    extern char *pc_get_input_file_path(void);
    file = fopen(pc_get_input_file_path(), "rb");
    if (file == NULL)
    {
        EGUI_LOG_ERR("Error opening file\r\n");
        return;
    }

    if (fseek(file, res_real_offset, SEEK_SET) != 0)
    {
        EGUI_LOG_ERR("Error seeking in file");
        fclose(file);
        return;
    }

    int read_size = fread(dest, 1, size, file);
    while (read_size != (int)size)
    {
        EGUI_LOG_ERR("Error reading file, read_size: %d, size: %d\r\n", read_size, (int)size);

        if (feof(file))
        {
            EGUI_LOG_ERR("Reached end of file.\n");
            while (1)
                ;
        }
        else if (ferror(file))
        {
            EGUI_LOG_ERR("Error reading file.\n");
            while (1)
                ;
        }
        fclose(file);
        return;
    }

    fclose(file);
}
#endif

static const egui_platform_ops_t pc_platform_ops = {
        .malloc = pc_malloc,
        .free = pc_free,
        .vlog = pc_vlog,
        .assert_handler = pc_assert_handler,
        .vsprintf = pc_vsprintf,
        .delay = pc_delay,
        .get_tick_ms = pc_get_tick_ms,
        .pfb_clear = pc_pfb_clear,
        .interrupt_disable = pc_interrupt_disable,
        .interrupt_enable = pc_interrupt_enable,
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
        .load_external_resource = pc_load_external_resource,
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

static egui_platform_t pc_platform = {
        .ops = &pc_platform_ops,
};

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_display_driver_register(&pc_display_driver);
    egui_platform_register(&pc_platform);
}
