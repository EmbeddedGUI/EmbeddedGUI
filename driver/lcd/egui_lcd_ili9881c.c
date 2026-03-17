/**
 * @file egui_lcd_ili9881c.c
 * @brief ILI9881C LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_ili9881c.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* ILI9881C-specific commands (not in MIPI DCS common set) */

/* MADCTL bits */
#define ILI9881C_MADCTL_MY  0x80
#define ILI9881C_MADCTL_MX  0x40
#define ILI9881C_MADCTL_MV  0x20
#define ILI9881C_MADCTL_ML  0x10
#define ILI9881C_MADCTL_BGR 0x08
#define ILI9881C_MADCTL_RGB 0x00

/* Color modes */
#define ILI9881C_COLOR_MODE_16BIT 0x55  /* RGB565 */
#define ILI9881C_COLOR_MODE_18BIT 0x66  /* RGB666 */
#define ILI9881C_COLOR_MODE_24BIT 0x77  /* RGB888 */

/* Driver: init */
static int ili9881c_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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

    /* Set color mode based on config */
    {
        uint8_t colmod = ILI9881C_COLOR_MODE_16BIT;
        if (config->color_depth == 24)
        {
            colmod = ILI9881C_COLOR_MODE_24BIT;
        }
        else if (config->color_depth == 18)
        {
            colmod = ILI9881C_COLOR_MODE_18BIT;
        }
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = ILI9881C_MADCTL_MX | ILI9881C_MADCTL_BGR;
        self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl, 1);
    }

    /* Inversion control */
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
static int ili9881c_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void ili9881c_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void ili9881c_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
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
static void ili9881c_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void ili9881c_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void ili9881c_setup_driver(egui_hal_lcd_driver_t *driver,
                                 egui_panel_io_handle_t io,
                                 void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ILI9881C";

    driver->reset = ili9881c_reset;
    driver->init = ili9881c_init;
    driver->del = ili9881c_del;
    driver->draw_area = ili9881c_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = ili9881c_set_power;
    driver->set_invert = ili9881c_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_ili9881c_init(egui_hal_lcd_driver_t *storage,
                            egui_panel_io_handle_t io,
                            void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    ili9881c_setup_driver(storage, io, set_rst);
}
