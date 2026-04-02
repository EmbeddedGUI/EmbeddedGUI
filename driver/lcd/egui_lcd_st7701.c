#if EGUI_DRIVER_LCD_ST7701_ENABLE

/**
 * @file egui_lcd_st7701.c
 * @brief ST7701 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_st7701.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* Vendor-specific command page select */
#define ST7701_CMD_PAGE 0xFF

/* MADCTL bits */
#define ST7701_MADCTL_MY  0x80
#define ST7701_MADCTL_MX  0x40
#define ST7701_MADCTL_MV  0x20
#define ST7701_MADCTL_BGR 0x08

/* Color modes */
#define ST7701_COLOR_MODE_16BIT 0x55 /* RGB565 */
#define ST7701_COLOR_MODE_18BIT 0x66 /* RGB666 */
#define ST7701_COLOR_MODE_24BIT 0x77 /* RGB888 */

/* Default vendor initialization commands for ST7701 */
static const egui_lcd_vendor_init_cmd_t st7701_default_vendor_cmds[] = {
        /* Select command page 1 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7701_CMD_PAGE, 0x77, 0x01, 0x00, 0x00, 0x10),
};

/* Driver: init */
static int st7701_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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

    /* Send vendor-specific init commands */
    egui_lcd_send_vendor_init_cmds(self->io, st7701_default_vendor_cmds, sizeof(st7701_default_vendor_cmds) / sizeof(st7701_default_vendor_cmds[0]));

    /* Set color mode to 16-bit RGB565 */
    {
        uint8_t colmod = ST7701_COLOR_MODE_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = ST7701_MADCTL_MX | ST7701_MADCTL_MY;
        self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl, 1);
    }

    /* Color inversion */
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
static int st7701_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void st7701_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void st7701_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
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
static void st7701_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void st7701_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void st7701_setup_driver(egui_hal_lcd_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7701";

    driver->reset = st7701_reset;
    driver->init = st7701_init;
    driver->del = st7701_del;
    driver->draw_area = st7701_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = st7701_set_power;
    driver->set_invert = st7701_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_st7701_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    st7701_setup_driver(storage, io, set_rst);
}

#endif /* EGUI_DRIVER_LCD_ST7701_ENABLE */
