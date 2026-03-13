#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_driver_bridge.h"
#include "egui_hal_sdl_sim.h"
#include "sdl_port.h"

/**
 * PC port: display and platform driver registration.
 * Replaces the old api_pc.c which directly implemented egui_api_* functions.
 */

static egui_hal_lcd_driver_t s_pc_lcd_driver;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static egui_hal_touch_driver_t s_pc_touch_driver;
#endif

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

    egui_hal_sdl_lcd_setup(&s_pc_lcd_driver);
    s_pc_lcd_driver.init(&s_pc_lcd_driver, &lcd_config);
    egui_display_driver_register(egui_display_driver_from_lcd(&s_pc_lcd_driver));

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_hal_touch_config_t touch_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };

    egui_hal_sdl_touch_setup(&s_pc_touch_driver);
    s_pc_touch_driver.init(&s_pc_touch_driver, &touch_config);
    egui_touch_driver_bridge_register(&s_pc_touch_driver);
#endif

    egui_platform_register(&pc_platform);
}
