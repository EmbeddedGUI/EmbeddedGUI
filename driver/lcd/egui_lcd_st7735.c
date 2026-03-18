/**
 * @file egui_lcd_st7735.c
 * @brief ST7735 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_st7735.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7735-specific commands */
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

/* MADCTL bits */
#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08

/* Color modes */
#define ST7735_COLOR_MODE_16BIT 0x55
#define ST7735_COLOR_MODE_18BIT 0x66

/* Default vendor initialization commands for ST7735 */
static const egui_lcd_vendor_init_cmd_t st7735_default_vendor_cmds[] = {
        /* Frame rate control - normal mode */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_FRMCTR1, 0x01, 0x2C, 0x2D),
        /* Frame rate control - idle mode */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_FRMCTR2, 0x01, 0x2C, 0x2D),
        /* Frame rate control - partial mode */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_FRMCTR3, 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D),
        /* Inversion control */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_INVCTR, 0x03),
        /* Power control 1 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_PWCTR1, 0x28, 0x08, 0x04),
        /* Power control 2 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_PWCTR2, 0xC0),
        /* Power control 3 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_PWCTR3, 0x0D, 0x00),
        /* Power control 4 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_PWCTR4, 0x8D, 0x2A),
        /* Power control 5 */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_PWCTR5, 0x8D, 0xEE),
        /* VCOM control */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_VMCTR1, 0x10),
        /* Positive gamma correction */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_GMCTRP1, 0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A, 0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13),
        /* Negative gamma correction */
        EGUI_LCD_CMD_WITH_PARAM(0, ST7735_GMCTRN1, 0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27, 0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13),
};

/* Driver: init */
static int st7735_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
        uint8_t colmod = ST7735_COLOR_MODE_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB;
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
    egui_lcd_send_vendor_init_cmds(self->io, st7735_default_vendor_cmds, sizeof(st7735_default_vendor_cmds) / sizeof(st7735_default_vendor_cmds[0]));

    /* Normal display mode */
    self->io->tx_param(self->io, LCD_CMD_NORON, NULL, 0);

    /* Display on */
    self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);

    return 0;
}

/* Driver: reset */
static int st7735_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void st7735_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void st7735_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h, const void *data, uint32_t len)
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
static void st7735_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void st7735_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void st7735_setup_driver(egui_hal_lcd_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7735";

    driver->reset = st7735_reset;
    driver->init = st7735_init;
    driver->del = st7735_del;
    driver->draw_area = st7735_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = st7735_set_power;
    driver->set_invert = st7735_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init (static allocation) */
void egui_lcd_st7735_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    st7735_setup_driver(storage, io, set_rst);
}
