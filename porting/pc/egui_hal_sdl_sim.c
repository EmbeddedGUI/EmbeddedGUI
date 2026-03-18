#include <string.h>

#include "egui_hal_sdl_sim.h"
#include "sdl_port.h"

static int sdl_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static int sdl_lcd_reset(egui_hal_lcd_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset */
}

static void sdl_lcd_del(egui_hal_lcd_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

static void sdl_lcd_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
{
    EGUI_UNUSED(self);

    uint32_t expected_len = (uint32_t)w * (uint32_t)h * sizeof(egui_color_int_t);

    if (w <= 0 || h <= 0 || data == NULL)
    {
        return;
    }

    if (len < expected_len)
    {
        return;
    }

    VT_Fill_Multiple_Colors(x, y, x + w - 1, y + h - 1, (egui_color_int_t *)data);
    sdl_port_request_refresh();
}

static void sdl_lcd_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(on);
}

static void sdl_lcd_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(invert);
}

void egui_hal_sdl_lcd_setup(egui_hal_lcd_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "SDL_LCD";
    storage->reset = sdl_lcd_reset;
    storage->init = sdl_lcd_init;
    storage->del = sdl_lcd_del;
    storage->draw_area = sdl_lcd_draw_area;
    storage->mirror = NULL;
    storage->swap_xy = NULL;
    storage->set_power = sdl_lcd_set_power;
    storage->set_invert = sdl_lcd_set_invert;
    storage->io = NULL; /* SDL doesn't use real bus IO */
    storage->set_rst = NULL;
}

static int sdl_touch_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static int sdl_touch_reset(egui_hal_touch_driver_t *self)
{
    (void)self;
    return 0; /* No hardware to reset */
}

static void sdl_touch_del(egui_hal_touch_driver_t *self)
{
    memset(self, 0, sizeof(egui_hal_touch_driver_t));
}

static int sdl_touch_read(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data)
{
    uint8_t pressed = 0;
    int16_t x = 0;
    int16_t y = 0;

    EGUI_UNUSED(self);

    memset(data, 0, sizeof(*data));
    sdl_port_touch_read(&pressed, &x, &y);

    if (!pressed)
    {
        data->has_release_point = 1;
        data->release_point.x = x;
        data->release_point.y = y;
        return 0;
    }

    data->point_count = 1;
    data->points[0].x = x;
    data->points[0].y = y;
    data->points[0].id = 0;
    data->points[0].pressure = 1;
    return 0;
}

void egui_hal_sdl_touch_setup(egui_hal_touch_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "SDL_TOUCH";
    storage->max_points = 1;
    storage->reset = sdl_touch_reset;
    storage->init = sdl_touch_init;
    storage->del = sdl_touch_del;
    storage->read = sdl_touch_read;
    storage->io = NULL; /* SDL doesn't use real bus IO */
    storage->set_rst = NULL;
    storage->set_int = NULL;
    storage->get_int = NULL;
}
