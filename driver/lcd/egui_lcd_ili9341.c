/**
 * @file egui_lcd_ili9341.c
 * @brief ILI9341 LCD driver implementation
 *
 * Uses unified Panel IO interface for all bus communication.
 */

#include "egui_lcd_ili9341.h"
#include "egui_lcd_common.h"
#include <string.h>
#include "core/egui_api.h"

/* ILI9341-specific commands (not in MIPI DCS standard) */
#define ILI9341_FRMCTR1   0xB1  /* Frame Rate Control */
#define ILI9341_DFUNCTR   0xB6  /* Display Function Control */
#define ILI9341_PWCTR1    0xC0  /* Power Control 1 */
#define ILI9341_PWCTR2    0xC1  /* Power Control 2 */
#define ILI9341_VMCTR1    0xC5  /* VCOM Control 1 */
#define ILI9341_VMCTR2    0xC7  /* VCOM Control 2 */
#define ILI9341_GMCTRP1   0xE0  /* Positive Gamma Correction */
#define ILI9341_GMCTRN1   0xE1  /* Negative Gamma Correction */
#define ILI9341_GAMMASET  0x26  /* Gamma Set */

/* Default vendor initialization commands for ILI9341 */
static const egui_lcd_vendor_init_cmd_t ili9341_default_vendor_cmds[] = {
    /* Power control A */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02),
    /* Power control B */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xCF, 0x00, 0xC1, 0x30),
    /* Driver timing control A */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xE8, 0x85, 0x00, 0x78),
    /* Driver timing control B */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xEA, 0x00, 0x00),
    /* Power on sequence control */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xED, 0x64, 0x03, 0x12, 0x81),
    /* Pump ratio control */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF7, 0x20),
    /* Power control 1 */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_PWCTR1, 0x23),
    /* Power control 2 */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_PWCTR2, 0x10),
    /* VCOM control 1 */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_VMCTR1, 0x3E, 0x28),
    /* VCOM control 2 */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_VMCTR2, 0x86),
    /* Frame rate control */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_FRMCTR1, 0x00, 0x18),
    /* Display function control */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_DFUNCTR, 0x08, 0x82, 0x27),
    /* Gamma function disable */
    EGUI_LCD_CMD_WITH_PARAM(0, 0xF2, 0x00),
    /* Gamma curve selected */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_GAMMASET, 0x01),
    /* Positive gamma correction */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_GMCTRP1,
                            0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                            0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00),
    /* Negative gamma correction */
    EGUI_LCD_CMD_WITH_PARAM(0, ILI9341_GMCTRN1,
                            0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                            0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F),
};

/* Driver: init */
static int ili9341_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* Custom init takes full control */
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
        uint8_t colmod = LCD_COLMOD_16BIT;
        self->io->tx_param(self->io, LCD_CMD_COLMOD, &colmod, 1);
    }

    /* Memory access control */
    {
        uint8_t madctl = LCD_MADCTL_MX | LCD_MADCTL_BGR;
        self->io->tx_param(self->io, LCD_CMD_MADCTL, &madctl, 1);
    }

    /* Send vendor-specific init commands */
    egui_lcd_send_vendor_init_cmds(self->io, ili9341_default_vendor_cmds,
                                   sizeof(ili9341_default_vendor_cmds) / sizeof(ili9341_default_vendor_cmds[0]));

    /* Inversion control */
    if (config->invert_color)
    {
        self->io->tx_param(self->io, LCD_CMD_INVON, NULL, 0);
    }
    else
    {
        self->io->tx_param(self->io, LCD_CMD_INVOFF, NULL, 0);
    }

    /* Normal display mode */
    self->io->tx_param(self->io, LCD_CMD_NORON, NULL, 0);

    /* Display on */
    self->io->tx_param(self->io, LCD_CMD_DISPON, NULL, 0);

    return 0;
}

/* Driver: reset */
static int ili9341_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void ili9341_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void ili9341_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                              int16_t w, int16_t h, const void *data, uint32_t len)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    {
        uint8_t caset[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        self->io->tx_param(self->io, LCD_CMD_CASET, caset, sizeof(caset));
    }

    /* Row address set */
    {
        uint8_t raset[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        self->io->tx_param(self->io, LCD_CMD_RASET, raset, sizeof(raset));
    }

    /* Write pixels */
    self->io->tx_color(self->io, LCD_CMD_RAMWR, data, len);
}

/* Driver: set_power */
static void ili9341_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
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
static void ili9341_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    self->io->tx_param(self->io, invert ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

/* Internal: setup driver function pointers */
static void ili9341_setup_driver(egui_hal_lcd_driver_t *driver,
                                  egui_panel_io_handle_t io,
                                  void (*set_rst)(uint8_t level))
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ILI9341";

    driver->reset = ili9341_reset;
    driver->init = ili9341_init;
    driver->del = ili9341_del;
    driver->draw_area = ili9341_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = ili9341_set_power;
    driver->set_invert = ili9341_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;
}

/* Public: init */
void egui_lcd_ili9341_init(egui_hal_lcd_driver_t *storage,
                            egui_panel_io_handle_t io,
                            void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    ili9341_setup_driver(storage, io, set_rst);
}
