#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "egui.h"
#include "egui_lcd.h"
#include "egui_touch.h"

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
 * Display HAL driver
 * ============================================================================ */

static egui_hal_lcd_driver_t designer_lcd_driver;

static egui_display_driver_ops_t port_display_ops = {0};

static egui_display_driver_t port_display_driver = {
        .ops = &port_display_ops,
        .physical_width = EGUI_CONFIG_SCEEN_WIDTH,
        .physical_height = EGUI_CONFIG_SCEEN_HEIGHT,
        .rotation = EGUI_DISPLAY_ROTATION_0,
        .brightness = 255,
        .power_on = 1,
};

static int designer_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    memset(designer_fb, 0, sizeof(designer_fb));
    return 0;
}

static int designer_lcd_reset(egui_hal_lcd_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset */
}

static void designer_lcd_del(egui_hal_lcd_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

static void designer_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
{
    EGUI_UNUSED(self);

    const egui_color_int_t *pixels = (const egui_color_int_t *)data;
    uint32_t pixel_count = len / sizeof(egui_color_int_t);
    uint32_t index = 0;

    for (int16_t row = 0; row < h && index < pixel_count; row++)
    {
        for (int16_t col = 0; col < w && index < pixel_count; col++)
        {
            designer_fb[(y + row) * EGUI_CONFIG_SCEEN_WIDTH + (x + col)] = pixels[index++];
        }
    }
}

static void designer_lcd_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(on);
}

static void designer_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "DESIGNER_FB";
    storage->reset = designer_lcd_reset;
    storage->init = designer_lcd_init;
    storage->del = designer_lcd_del;
    storage->draw_area = designer_lcd_draw_area;
    storage->mirror = NULL;
    storage->swap_xy = NULL;
    storage->set_power = designer_lcd_set_power;
    storage->set_invert = NULL;
    storage->io = NULL;
    storage->set_rst = NULL;
}


#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/* ============================================================================
 * Touch HAL driver
 * ============================================================================ */

static egui_hal_touch_driver_t designer_touch_driver;
static struct {
    uint8_t pressed;
    int16_t x;
    int16_t y;
} designer_touch_state;

static int designer_touch_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    designer_touch_state.pressed = 0;
    designer_touch_state.x = 0;
    designer_touch_state.y = 0;
    return 0;
}

static int designer_touch_reset(egui_hal_touch_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset */
}

static void designer_touch_del(egui_hal_touch_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

static int designer_touch_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    EGUI_UNUSED(self);

    memset(data, 0, sizeof(*data));
    if (!designer_touch_state.pressed)
    {
        return 0;
    }

    data->point_count = 1;
    data->points[0].x = designer_touch_state.x;
    data->points[0].y = designer_touch_state.y;
    data->points[0].id = 0;
    data->points[0].pressure = 1;
    return 0;
}

static void designer_touch_setup(egui_hal_touch_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "DESIGNER_TOUCH";
    storage->max_points = 1;
    storage->reset = designer_touch_reset;
    storage->init = designer_touch_init;
    storage->del = designer_touch_del;
    storage->read = designer_touch_read;
    storage->io = NULL;
    storage->set_rst = NULL;
    storage->set_int = NULL;
    storage->get_int = NULL;
}

void designer_touch_set_state(uint8_t pressed, int16_t x, int16_t y)
{
    designer_touch_state.pressed = pressed;
    designer_touch_state.x = x;
    designer_touch_state.y = y;
}
#endif

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
            .custom_init = NULL,
    };

    designer_lcd_setup(&designer_lcd_driver);
    egui_hal_lcd_register(&port_display_driver, &designer_lcd_driver, &lcd_config);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_hal_touch_config_t touch_config = {
            .width = EGUI_CONFIG_SCEEN_WIDTH,
            .height = EGUI_CONFIG_SCEEN_HEIGHT,
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
    };

    designer_touch_setup(&designer_touch_driver);
    egui_hal_touch_register(&designer_touch_driver, &touch_config);
#endif

    egui_platform_register(&designer_platform);
}
