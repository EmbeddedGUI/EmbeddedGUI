/**
 * @file egui_lcd_st7796.c
 * @brief ST7796 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_st7796.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7796-specific commands */
#define ST7796_PWCTR1    0xC0
#define ST7796_PWCTR2    0xC1
#define ST7796_PWCTR3    0xC2
#define ST7796_VMCTR1    0xC5
#define ST7796_GMCTRP1   0xE0
#define ST7796_GMCTRN1   0xE1

/* ST7796-specific commands (not in MIPI DCS common set) */

/* MADCTL bits */
#define ST7796_MADCTL_MY  0x80
#define ST7796_MADCTL_MX  0x40
#define ST7796_MADCTL_MV  0x20
#define ST7796_MADCTL_ML  0x10
#define ST7796_MADCTL_BGR 0x08
#define ST7796_MADCTL_RGB 0x00

/* Color modes */
#define ST7796_COLOR_MODE_16BIT 0x55  /* RGB565 */

/* Default vendor initialization commands for ST7796 */
static const egui_lcd_vendor_init_cmd_t st7796_default_vendor_cmds[] = {
    /* Command set control */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF0, 0xC3),
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF0, 0x96),
    /* Display inversion control */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xB4, 0x02),
    /* Entry mode set */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xB7, 0xC6),
    EGUI_LCD_CMD_WITH_PARAM(0, 0xB9, 0x02, 0xE0),
    /* Power control 1 */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_PWCTR1, 0x80, 0x71),
    /* Power control 2 */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_PWCTR2, 0x17),
    /* Power control 3 */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_PWCTR3, 0xA7),
    /* VCOM control */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_VMCTR1, 0x16),
    /* Display output ctrl adjust */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xE8, 0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33),
    /* Positive gamma */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_GMCTRP1,
                            0xF0, 0x11, 0x17, 0x0C, 0x0B, 0x08, 0x42, 0x44,
                            0x57, 0x3D, 0x17, 0x18, 0x34, 0x38),
    /* Negative gamma */
    EGUI_LCD_CMD_WITH_PARAM(0, ST7796_GMCTRN1,
                            0xF0, 0x13, 0x1C, 0x0E, 0x0C, 0x15, 0x41, 0x43,
                            0x57, 0x25, 0x13, 0x14, 0x35, 0x36),
    /* Command set control - close */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF0, 0x3C),
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF0, 0x69),
};

/* Driver: init */
static int st7796_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* If custom init is provided, use it instead of default sequence */
    if (config->custom_init)
    {
        return config->custom_init(self->io, config);
    }

    /* Sleep out */
    self->io->tx_param(self->io, LCD_CMD_SLPOUT, NULL, 0);
    egui_api_delay(120);

    /* Set color mode to 16-bit RGB565 */
    {
        uint8_t colmod = ST7796_COLOR_MODE_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = ST7796_MADCTL_BGR;
        self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl, 1);
    }

    /* Send vendor-specific init commands */
    egui_lcd_send_vendor_init_cmds(self->io, st7796_default_vendor_cmds,
                                   sizeof(st7796_default_vendor_cmds) / sizeof(st7796_default_vendor_cmds[0]));

    /* Inversion on (ST7796 typically uses inversion) */
    self->io->tx_param(self->io, LCD_CMD_INVON, NULL, 0);

    /* Display on */
    self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);

    return 0;
}

/* Driver: reset */
static int st7796_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void st7796_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void st7796_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
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

/* Internal: write MADCTL register */
static void st7796_write_madctl(egui_hal_lcd_driver_t *self)
{
    uint16_t x_offset = self->config.x_offset;
    uint16_t y_offset = self->config.y_offset;
    uint8_t madctl_val = ST7796_MADCTL_BGR;

    if (self->config.mirror_x)
    {
        madctl_val |= ST7796_MADCTL_MX;
    }
    if (self->config.mirror_y)
    {
        madctl_val |= ST7796_MADCTL_MY;
    }

    if (self->config.swap_xy)
    {
        /* Swap offsets when X/Y are swapped */
        self->config.x_offset = y_offset;
        self->config.y_offset = x_offset;
        madctl_val |= ST7796_MADCTL_MV;
    }

    self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl_val, 1);
}

/* Driver: mirror */
static void st7796_mirror(egui_hal_lcd_driver_t *self, uint8_t mirror_x, uint8_t mirror_y)
{
    self->config.mirror_x = mirror_x;
    self->config.mirror_y = mirror_y;
    st7796_write_madctl(self);
}

/* Driver: swap_xy */
static void st7796_swap_xy(egui_hal_lcd_driver_t *self, uint8_t swap)
{
    self->config.swap_xy = swap;
    st7796_write_madctl(self);
}

/* Driver: set_power */
static void st7796_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void st7796_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void st7796_setup_driver(egui_hal_lcd_driver_t *driver,
                                 egui_panel_io_handle_t io,
                                 void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7796";

    driver->reset = st7796_reset;
    driver->init = st7796_init;
    driver->del = st7796_del;
    driver->draw_area = st7796_draw_area;
    driver->mirror = st7796_mirror;
    driver->swap_xy = st7796_swap_xy;
    driver->set_power = st7796_set_power;
    driver->set_invert = st7796_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_st7796_init(egui_hal_lcd_driver_t *storage,
                             egui_panel_io_handle_t io,
                             void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    st7796_setup_driver(storage, io, set_rst);
}
