#if EGUI_DRIVER_LCD_GC9A01_ENABLE

/**
 * @file egui_lcd_gc9a01.c
 * @brief GC9A01 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_gc9a01.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* MADCTL bits */
#define GC9A01_MADCTL_MY  0x80
#define GC9A01_MADCTL_MX  0x40
#define GC9A01_MADCTL_MV  0x20
#define GC9A01_MADCTL_ML  0x10
#define GC9A01_MADCTL_BGR 0x08

/* Color modes */
#define GC9A01_COLOR_MODE_16BIT 0x55

/* Default vendor initialization commands for GC9A01 */
static const egui_lcd_vendor_init_cmd_t gc9a01_default_vendor_cmds[] = {
        EGUI_LCD_CMD_NO_PARAM(0, 0xEF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xEB, 0x14),
        EGUI_LCD_CMD_NO_PARAM(0, 0xFE),
        EGUI_LCD_CMD_NO_PARAM(0, 0xEF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xEB, 0x14),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x84, 0x40),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x85, 0xFF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x86, 0xFF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x87, 0xFF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x88, 0x0A),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x89, 0x21),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8A, 0x00),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8B, 0x80),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8C, 0x01),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8D, 0x01),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8E, 0xFF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x8F, 0xFF),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xB6, 0x00, 0x00),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x90, 0x08, 0x08, 0x08, 0x08),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xBD, 0x06),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xBC, 0x00),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xFF, 0x60, 0x01, 0x04),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xC3, 0x13),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xC4, 0x13),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xC9, 0x22),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xBE, 0x11),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xE1, 0x10, 0x0E),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xDF, 0x21, 0x0C, 0x02),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xF0, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xF1, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xF2, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xF3, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xED, 0x1B, 0x0B),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xAE, 0x77),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xCD, 0x63),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x70, 0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03),
        EGUI_LCD_CMD_WITH_PARAM(0, 0xE8, 0x34),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x62, 0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x63, 0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x64, 0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x66, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x67, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x74, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x98, 0x3E),
        EGUI_LCD_CMD_WITH_PARAM(0, 0x99, 0x3E),
};

/* Driver: init */
static int gc9a01_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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

    /* Set color mode to 16-bit RGB565 */
    {
        uint8_t colmod = GC9A01_COLOR_MODE_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = GC9A01_MADCTL_MX | GC9A01_MADCTL_MY | GC9A01_MADCTL_BGR;
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

    /* Send vendor-specific init commands */
    egui_lcd_send_vendor_init_cmds(self->io, gc9a01_default_vendor_cmds, sizeof(gc9a01_default_vendor_cmds) / sizeof(gc9a01_default_vendor_cmds[0]));

    /* Sleep out */
    self->io->tx_param(self->io, LCD_CMD_SLPOUT, NULL, 0);
    egui_api_delay(120);

    /* Display on */
    self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);

    return 0;
}

/* Driver: reset */
static int gc9a01_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void gc9a01_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void gc9a01_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
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
static void gc9a01_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void gc9a01_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void gc9a01_setup_driver(egui_hal_lcd_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "GC9A01";

    driver->reset = gc9a01_reset;
    driver->init = gc9a01_init;
    driver->del = gc9a01_del;
    driver->draw_area = gc9a01_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = gc9a01_set_power;
    driver->set_invert = gc9a01_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_gc9a01_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    gc9a01_setup_driver(storage, io, set_rst);
}

#endif /* EGUI_DRIVER_LCD_GC9A01_ENABLE */
