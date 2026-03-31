#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <emscripten.h>

#include "egui.h"
#include "egui_touch.h"
#include "egui_hal_sdl_sim.h"
#include "sdl_port.h"

/**
 * Emscripten port: display and platform driver registration.
 * Based on egui_port_pc.c with browser-friendly adaptations.
 */

static egui_hal_lcd_driver_t s_em_lcd_driver;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static egui_hal_touch_driver_t s_em_touch_driver;
#endif

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
// Platform driver
// ============================================================================

static void em_assert_handler(const char *file, int line)
{
    printf("Assert@ file = %s, line = %d\n", file, line);
    // Browser-friendly: stop main loop and exit instead of while(1)
    emscripten_cancel_main_loop();
    emscripten_force_exit(1);
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

static egui_base_t em_interrupt_disable(void)
{
    return 0;
}

static void em_interrupt_enable(egui_base_t level)
{
    EGUI_UNUSED(level);
}

#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
static FILE *s_em_resource_file = NULL;
static char s_em_resource_file_path[512];
static uint32_t s_em_resource_file_offset = 0;
static uint8_t s_em_resource_file_offset_valid = 0;

static void em_close_external_resource_file(void)
{
    if (s_em_resource_file != NULL)
    {
        fclose(s_em_resource_file);
        s_em_resource_file = NULL;
        s_em_resource_file_path[0] = '\0';
    }
    s_em_resource_file_offset = 0;
    s_em_resource_file_offset_valid = 0;
}

static FILE *em_get_external_resource_file(void)
{
    extern char *pc_get_input_file_path(void);
    const char *path = pc_get_input_file_path();

    if (path == NULL)
    {
        return NULL;
    }

    if (s_em_resource_file != NULL && strcmp(s_em_resource_file_path, path) == 0)
    {
        return s_em_resource_file;
    }

    em_close_external_resource_file();

    s_em_resource_file = fopen(path, "rb");
    if (s_em_resource_file == NULL)
    {
        EGUI_LOG_ERR("Error opening resource file\r\n");
        return NULL;
    }

    strncpy(s_em_resource_file_path, path, sizeof(s_em_resource_file_path) - 1);
    s_em_resource_file_path[sizeof(s_em_resource_file_path) - 1] = '\0';
    setvbuf(s_em_resource_file, NULL, _IOFBF, 64 * 1024);

    return s_em_resource_file;
}

static void em_load_external_resource(void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;
    FILE *file = em_get_external_resource_file();
    if (file == NULL)
    {
        return;
    }

    if (!s_em_resource_file_offset_valid || s_em_resource_file_offset != res_real_offset)
    {
        if (fseek(file, res_real_offset, SEEK_SET) != 0)
        {
            EGUI_LOG_ERR("Error seeking in resource file\r\n");
            em_close_external_resource_file();
            return;
        }
    }

    int read_size = fread(dest, 1, size, file);
    if (read_size != (int)size)
    {
        EGUI_LOG_ERR("Error reading resource file, read_size: %d, size: %d\r\n", read_size, (int)size);
        em_close_external_resource_file();
        return;
    }

    s_em_resource_file_offset = res_real_offset + size;
    s_em_resource_file_offset_valid = 1;
}
#endif

static const egui_platform_ops_t em_platform_ops = {
        .assert_handler = em_assert_handler,
        .delay = em_delay,
        .get_tick_ms = em_get_tick_ms,
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

    egui_hal_sdl_lcd_setup(&s_em_lcd_driver);
    egui_hal_lcd_register(&port_display_driver, &s_em_lcd_driver, &lcd_config);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_hal_touch_config_t touch_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };

    egui_hal_sdl_touch_setup(&s_em_touch_driver);
    egui_hal_touch_register(&s_em_touch_driver, &touch_config);
#endif

    egui_platform_register(&em_platform);
}
