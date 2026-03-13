#include <string.h>

#include "egui_hal_sdl_sim.h"
#include "sdl_port.h"

typedef struct egui_hal_sdl_lcd_priv {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} egui_hal_sdl_lcd_priv_t;

static egui_hal_sdl_lcd_priv_t s_sdl_lcd_priv;

static int sdl_lcd_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    memset(&s_sdl_lcd_priv, 0, sizeof(s_sdl_lcd_priv));
    return 0;
}

static void sdl_lcd_deinit(egui_hal_lcd_driver_t *self)
{
    EGUI_UNUSED(self);
}

static void sdl_lcd_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    egui_hal_sdl_lcd_priv_t *priv = (egui_hal_sdl_lcd_priv_t *)self->priv;
    priv->x = x;
    priv->y = y;
    priv->w = w;
    priv->h = h;
}

static void sdl_lcd_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    egui_hal_sdl_lcd_priv_t *priv = (egui_hal_sdl_lcd_priv_t *)self->priv;
    uint32_t expected_len = (uint32_t)priv->w * (uint32_t)priv->h * sizeof(egui_color_int_t);

    if (priv->w <= 0 || priv->h <= 0 || data == NULL)
    {
        return;
    }

    if (len < expected_len)
    {
        return;
    }

    VT_Fill_Multiple_Colors(priv->x, priv->y, priv->x + priv->w - 1, priv->y + priv->h - 1, (egui_color_int_t *)data);
    sdl_port_request_refresh();
}

static void sdl_lcd_set_brightness(egui_hal_lcd_driver_t *self, uint8_t level)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(level);
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
    storage->bus_type = EGUI_BUS_TYPE_SPI;
    storage->init = sdl_lcd_init;
    storage->deinit = sdl_lcd_deinit;
    storage->set_window = sdl_lcd_set_window;
    storage->write_pixels = sdl_lcd_write_pixels;
    storage->wait_dma_complete = NULL;
    storage->set_rotation = NULL;
    storage->set_brightness = sdl_lcd_set_brightness;
    storage->set_power = sdl_lcd_set_power;
    storage->set_invert = sdl_lcd_set_invert;
    storage->priv = &s_sdl_lcd_priv;
}

static int sdl_touch_init(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config)
{
    memcpy(&self->config, config, sizeof(*config));
    return 0;
}

static void sdl_touch_deinit(egui_hal_touch_driver_t *self)
{
    EGUI_UNUSED(self);
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
    storage->bus_type = EGUI_BUS_TYPE_SPI;
    storage->max_points = 1;
    storage->init = sdl_touch_init;
    storage->deinit = sdl_touch_deinit;
    storage->read = sdl_touch_read;
}
