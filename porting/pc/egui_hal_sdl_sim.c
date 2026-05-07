#include <string.h>

#include "egui_hal_sdl_sim.h"
#include "core/egui_touch_driver.h"
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

    VT_Fill_Multiple_Colors_Core(self->bridge_core, x, y, x + w - 1, y + h - 1, (egui_color_int_t *)data);
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

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

static int sdl_touch_read(egui_hal_touch_driver_t *self, egui_core_t *core, egui_hal_touch_data_t *data)
{
    egui_touch_driver_data_t touch_data;

    EGUI_UNUSED(self);

    memset(data, 0, sizeof(*data));
    memset(&touch_data, 0, sizeof(touch_data));
    if (sdl_port_touch_read(core, &touch_data) != 0)
    {
        return -1;
    }

    data->point_count = touch_data.point_count;
    if (data->point_count > EGUI_HAL_TOUCH_MAX_POINTS)
    {
        data->point_count = EGUI_HAL_TOUCH_MAX_POINTS;
    }

    for (uint8_t i = 0; i < data->point_count; i++)
    {
        data->points[i].x = touch_data.points[i].x;
        data->points[i].y = touch_data.points[i].y;
        data->points[i].id = touch_data.points[i].id;
        data->points[i].pressure = touch_data.points[i].pressure;
    }
    return 0;
}

void egui_hal_sdl_touch_setup(egui_hal_touch_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
    storage->name = "SDL_TOUCH";
    storage->max_points = EGUI_TOUCH_DRIVER_MAX_POINTS;
    storage->reset = sdl_touch_reset;
    storage->init = sdl_touch_init;
    storage->del = sdl_touch_del;
    storage->read = sdl_touch_read;
    storage->io = NULL; /* SDL doesn't use real bus IO */
    storage->set_rst = NULL;
    storage->set_int = NULL;
    storage->get_int = NULL;
}

#else

void egui_hal_sdl_touch_setup(egui_hal_touch_driver_t *storage)
{
    memset(storage, 0, sizeof(*storage));
}

#endif
