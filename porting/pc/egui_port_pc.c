#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_touch.h"
#include "egui_hal_sdl_sim.h"
#include "sdl_port.h"

/**
 * PC port: display and platform driver registration.
 * Replaces the old api_pc.c which directly implemented egui_api_* functions.
 */

static egui_hal_lcd_driver_t s_pc_lcd_driver;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static egui_hal_touch_driver_t s_pc_touch_drivers[EGUI_CONFIG_MAX_DISPLAY_COUNT];

static void pc_touch_register(egui_core_t *core)
{
    int display_id;
    egui_hal_touch_driver_t *touch_driver;
    egui_hal_touch_config_t touch_config = {
            .width = (uint16_t)egui_display_get_width(core),
            .height = (uint16_t)egui_display_get_height(core),
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };

    EGUI_ASSERT(core != NULL);

    display_id = (int)core->id;
    EGUI_ASSERT(display_id >= 0 && display_id < EGUI_CONFIG_MAX_DISPLAY_COUNT);

    touch_driver = &s_pc_touch_drivers[display_id];
    egui_hal_sdl_touch_setup(touch_driver);
    egui_hal_touch_register(core, touch_driver, &touch_config);
}
#endif

static egui_display_driver_ops_t port_display_ops = {0};

static egui_display_driver_t port_display_driver = {
        .ops = &port_display_ops,
        .physical_width = EGUI_CONFIG_SCREEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCREEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

static void pc_display_flush(egui_core_t *core)
{
    VT_sdl_flush_core(core, 1);
}

// ============================================================================
// Platform driver
// ============================================================================

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

static void pc_delay(uint32_t ms)
{
    sdl_port_sleep(ms);
}

static uint32_t pc_get_tick_ms(void)
{
    return sdl_get_system_timestamp_ms();
}

static SDL_mutex *pc_isr_mutex = NULL;
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
typedef struct pc_resource_stream
{
    FILE *file;
    char path[512];
    uint32_t offset;
    uint8_t offset_valid;
    SDL_mutex *mutex;
} pc_resource_stream_t;

#define PC_RESOURCE_STREAM_NULL_SLOT  EGUI_CONFIG_MAX_DISPLAY_COUNT
#define PC_RESOURCE_STREAM_SLOT_COUNT (EGUI_CONFIG_MAX_DISPLAY_COUNT + 1)

static pc_resource_stream_t s_pc_resource_streams[PC_RESOURCE_STREAM_SLOT_COUNT];

static int pc_get_resource_stream_index(egui_core_t *core)
{
    if (core == NULL)
    {
        return PC_RESOURCE_STREAM_NULL_SLOT;
    }

    EGUI_ASSERT(core->id < EGUI_CONFIG_MAX_DISPLAY_COUNT);
    return (int)core->id;
}

static pc_resource_stream_t *pc_get_resource_stream(egui_core_t *core)
{
    int slot = pc_get_resource_stream_index(core);
    pc_resource_stream_t *stream = &s_pc_resource_streams[slot];

    if (stream->mutex == NULL)
    {
        stream->mutex = SDL_CreateMutex();
    }

    EGUI_ASSERT(stream->mutex != NULL);
    return stream;
}

static void pc_resource_stream_lock(pc_resource_stream_t *stream)
{
    if (stream != NULL && stream->mutex != NULL)
    {
        SDL_LockMutex(stream->mutex);
    }
}

static void pc_resource_stream_unlock(pc_resource_stream_t *stream)
{
    if (stream != NULL && stream->mutex != NULL)
    {
        SDL_UnlockMutex(stream->mutex);
    }
}
#endif

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
static void pc_close_external_resource_file(pc_resource_stream_t *stream)
{
    if (stream == NULL)
    {
        return;
    }

    if (stream->file != NULL)
    {
        fclose(stream->file);
        stream->file = NULL;
        stream->path[0] = '\0';
    }
    stream->offset = 0;
    stream->offset_valid = 0;
}

static FILE *pc_get_external_resource_file(pc_resource_stream_t *stream)
{
    extern char *pc_get_input_file_path(void);
    const char *path = pc_get_input_file_path();

    if (stream == NULL || path == NULL)
    {
        return NULL;
    }

    if (stream->file != NULL && strcmp(stream->path, path) == 0)
    {
        return stream->file;
    }

    pc_close_external_resource_file(stream);

    stream->file = fopen(path, "rb");
    if (stream->file == NULL)
    {
        EGUI_LOG_ERR("Error opening file\r\n");
        return NULL;
    }

    strncpy(stream->path, path, sizeof(stream->path) - 1);
    stream->path[sizeof(stream->path) - 1] = '\0';
    setvbuf(stream->file, NULL, _IOFBF, 64 * 1024);

    return stream->file;
}

static void pc_load_external_resource(egui_core_t *core, void *dest, uint32_t res_id, uint32_t start_offset, uint32_t size)
{
    extern const uint32_t egui_ext_res_id_map[];
    pc_resource_stream_t *stream = pc_get_resource_stream(core);
    uint32_t res_offset = egui_ext_res_id_map[res_id];
    uint32_t res_real_offset = res_offset + start_offset;
    FILE *file;

    pc_resource_stream_lock(stream);
    file = pc_get_external_resource_file(stream);
    if (file == NULL)
    {
        pc_resource_stream_unlock(stream);
        return;
    }

    if (!stream->offset_valid || stream->offset != res_real_offset)
    {
        if (fseek(file, res_real_offset, SEEK_SET) != 0)
        {
            EGUI_LOG_ERR("Error seeking in file");
            pc_close_external_resource_file(stream);
            pc_resource_stream_unlock(stream);
            return;
        }
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
        pc_close_external_resource_file(stream);
        pc_resource_stream_unlock(stream);
        return;
    }

    stream->offset = res_real_offset + size;
    stream->offset_valid = 1;
    pc_resource_stream_unlock(stream);
}
#endif

static const egui_platform_ops_t pc_platform_ops = {
        .assert_handler = pc_assert_handler,
        .delay = pc_delay,
        .get_tick_ms = pc_get_tick_ms,
        .interrupt_disable = pc_interrupt_disable,
        .interrupt_enable = pc_interrupt_enable,
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
        .load_external_resource = pc_load_external_resource,
#else
        .load_external_resource = NULL,
#endif
        .timer_start = NULL,
        .timer_stop = NULL,
};

static egui_platform_t pc_platform = {
        .ops = &pc_platform_ops,
};

// ============================================================================
// Port initialization
// ============================================================================

void egui_port_init(void)
{
    egui_platform_register(&pc_platform);

    if (pc_isr_mutex == NULL)
    {
        pc_isr_mutex = SDL_CreateMutex();
    }
#if EGUI_CONFIG_FUNCTION_RESOURCE_MANAGER
    for (int i = 0; i < PC_RESOURCE_STREAM_SLOT_COUNT; i++)
    {
        if (s_pc_resource_streams[i].mutex == NULL)
        {
            s_pc_resource_streams[i].mutex = SDL_CreateMutex();
        }
    }
#endif

    egui_hal_lcd_config_t lcd_config = {
            .width = EGUI_CONFIG_SCREEN_WIDTH,
            .height = EGUI_CONFIG_SCREEN_HEIGHT,
            .color_depth = EGUI_CONFIG_COLOR_DEPTH,
            .color_swap = 0,
            .x_offset = 0,
            .y_offset = 0,
            .invert_color = 0,
            .mirror_x = 0,
            .mirror_y = 0,
            .custom_init = NULL,
    };

    egui_hal_sdl_lcd_setup(&s_pc_lcd_driver);
    port_display_ops.flush = pc_display_flush;
    egui_hal_lcd_register(&port_display_driver, &s_pc_lcd_driver, &lcd_config);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#endif
}

egui_display_driver_t *egui_port_get_display_driver(void)
{
    return &port_display_driver;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_port_register_touch_driver(egui_core_t *core)
{
    pc_touch_register(core);
}
#endif

#if EGUI_CONFIG_MAX_DISPLAY_COUNT > 1

/**
 * Create a sub-display driver for multi-display PC simulation.
 * Allocates a new display driver and creates an SDL window.
 *
 * @param display_id  Display ID (1, 2, ...)
 * @param w           Screen width
 * @param h           Screen height
 * @return            Display driver pointer, or NULL on failure
 */
egui_display_driver_t *egui_port_create_sub_display(egui_core_t *core, int display_id, int16_t w, int16_t h)
{
    egui_display_driver_t *drv = (egui_display_driver_t *)egui_api_malloc(core, sizeof(egui_display_driver_t));
    if (drv == NULL)
    {
        return NULL;
    }
    memset(drv, 0, sizeof(egui_display_driver_t));

    drv->ops = &port_display_ops;
    drv->physical_width = w;
    drv->physical_height = h;
    drv->rotation = EGUI_DISPLAY_ROTATION_0;
    drv->brightness = 255;
    drv->power_on = 1;

    /* Reuse the same SDL LCD HAL setup — draw_area routes via active display ID */
    egui_hal_lcd_driver_t *lcd = (egui_hal_lcd_driver_t *)egui_api_malloc(core, sizeof(egui_hal_lcd_driver_t));
    if (lcd == NULL)
    {
        egui_api_free(core, drv);
        return NULL;
    }
    egui_hal_sdl_lcd_setup(lcd);

    egui_hal_lcd_config_t lcd_config = {
            .width = w,
            .height = h,
            .color_depth = EGUI_CONFIG_COLOR_DEPTH,
            .color_swap = 0,
            .x_offset = 0,
            .y_offset = 0,
            .invert_color = 0,
            .mirror_x = 0,
            .mirror_y = 0,
            .custom_init = NULL,
    };
    egui_hal_lcd_register(drv, lcd, &lcd_config);

    /* Create SDL window for this display */
    sdl_port_add_display(display_id, w, h);

    return drv;
}

#endif /* EGUI_CONFIG_MAX_DISPLAY_COUNT > 1 */
