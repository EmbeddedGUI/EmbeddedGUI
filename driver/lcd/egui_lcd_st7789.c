/**
 * @file egui_lcd_st7789.c
 * @brief ST7789 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_st7789.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7789-specific commands (not in MIPI DCS common set) */
#define ST7789_RAMCTRL   0xB0

/* MADCTL bits */
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08

/* Color modes */
#define ST7789_COLOR_MODE_16BIT 0x55
#define ST7789_COLOR_MODE_18BIT 0x66

/* Driver: init */
static int st7789_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* If custom init is provided, use it instead of default sequence */
    if (config->custom_init)
    {
        return config->custom_init(self->io, config);
    }

    /* Software reset */
    self->io->tx_param(self->io, LCD_CMD_SWRESET, NULL, 0);
    egui_api_delay(120);

    /* Sleep out */
    self->io->tx_param(self->io, LCD_CMD_SLPOUT, NULL, 0);
    egui_api_delay(120);

    /* Set color mode to 16-bit RGB565 */
    {
        uint8_t colmod = ST7789_COLOR_MODE_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Set RAM control for little endian (optional, depends on MCU) */
    {
        uint8_t data[] = {0x00, 0xF8};
        self->io->tx_param(self->io, ST7789_RAMCTRL, data, sizeof(data));
    }

    /* Set rotation (default 0) */
    {
        uint8_t madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB;
        self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl, 1);
    }

    /* Inversion control (ST7789 typically needs inversion on) */
    if (config->invert_color)
    {
        self->io->tx_param(self->io, LCD_CMD_INVON, NULL, 0);
    }
    else
    {
        self->io->tx_param(self->io, LCD_CMD_INVOFF, NULL, 0);
    }

    /* Display on */
    self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);

    return 0;
}

/* Driver: reset */
static int st7789_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void st7789_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void st7789_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                             int16_t w, int16_t h, const void *data, uint32_t len)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    uint8_t caset[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    self->io->tx_param(self->io, LCD_CMD_CASET, caset, sizeof(caset));

    uint8_t raset[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
    self->io->tx_param(self->io, LCD_CMD_RASET, raset, sizeof(raset));

    self->io->tx_color(self->io, LCD_CMD_RAMWR, data, len);
}

/* Driver: set_power */
static void st7789_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        self->io->tx_param(self->io, LCD_CMD_SLPOUT, NULL, 0);
        egui_api_delay(120);
        self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);
    }
    else
    {
        self->io->tx_param(self->io, LCD_CMD_DISPOFF, NULL, 0);
        self->io->tx_param(self->io, LCD_CMD_SLPIN, NULL, 0);
    }
}

/* Driver: set_invert */
static void st7789_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void st7789_setup_driver(egui_hal_lcd_driver_t *driver,
                                 egui_panel_io_handle_t io,
                                 void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7789";

    driver->reset = st7789_reset;
    driver->init = st7789_init;
    driver->del = st7789_del;
    driver->draw_area = st7789_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = st7789_set_power;
    driver->set_invert = st7789_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_st7789_init(egui_hal_lcd_driver_t *storage,
                           egui_panel_io_handle_t io,
                           void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    st7789_setup_driver(storage, io, set_rst);
}
