/**
 * @file egui_lcd_ssd1306.c
 * @brief SSD1306 OLED driver implementation
 *
 * SSD1306 is a 128x64 monochrome OLED controller.
 * Supports both I2C and SPI interfaces via unified Panel IO.
 *
 * The driver uses a private data flag (is_i2c) to select the correct
 * command/data sending pattern:
 * - SPI: io->tx_param(io, cmd, NULL, 0) for commands
 * - I2C: io->tx_param(io, 0x00, &cmd, 1) for commands (0x00 = control byte)
 */

#include "egui_lcd_ssd1306.h"
#include <string.h>
#include "core/egui_api.h"

/* SSD1306 Commands */
#define SSD1306_SETCONTRAST         0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON        0xA5
#define SSD1306_NORMALDISPLAY       0xA6
#define SSD1306_INVERTDISPLAY       0xA7
#define SSD1306_DISPLAYOFF          0xAE
#define SSD1306_DISPLAYON           0xAF
#define SSD1306_SETDISPLAYOFFSET    0xD3
#define SSD1306_SETCOMPINS          0xDA
#define SSD1306_SETVCOMDETECT       0xDB
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5
#define SSD1306_SETPRECHARGE        0xD9
#define SSD1306_SETMULTIPLEX        0xA8
#define SSD1306_SETLOWCOLUMN        0x00
#define SSD1306_SETHIGHCOLUMN       0x10
#define SSD1306_SETSTARTLINE        0x40
#define SSD1306_MEMORYMODE          0x20
#define SSD1306_COLUMNADDR          0x21
#define SSD1306_PAGEADDR            0x22
#define SSD1306_COMSCANINC          0xC0
#define SSD1306_COMSCANDEC          0xC8
#define SSD1306_SEGREMAP            0xA0
#define SSD1306_CHARGEPUMP          0x8D
#define SSD1306_DEACTIVATE_SCROLL   0x2E

/* I2C control bytes */
#define SSD1306_I2C_CMD             0x00  /* Co=0, D/C#=0: Command */
#define SSD1306_I2C_DATA            0x40  /* Co=0, D/C#=1: Data */

/* ============================================================
 * Private Data
 * ============================================================ */

typedef struct ssd1306_priv {
    uint8_t is_i2c;
} ssd1306_priv_t;

static ssd1306_priv_t s_ssd1306_priv;

/* ============================================================
 * IO Helpers
 * ============================================================ */

/**
 * Write a single command byte.
 * SPI: io->tx_param(io, cmd, NULL, 0) - cmd sent with DC=0
 * I2C: io->tx_param(io, 0x00, &cmd, 1) - cmd written to control byte 0x00
 */
static void ssd1306_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
{
    ssd1306_priv_t *priv = (ssd1306_priv_t *)self->priv;
    if (priv->is_i2c)
    {
        self->io->tx_param(self->io, SSD1306_I2C_CMD, &cmd, 1);
    }
    else
    {
        self->io->tx_param(self->io, cmd, NULL, 0);
    }
}

/**
 * Write pixel/data bytes.
 * SPI: io->tx_color(io, -1, data, len) - data sent with DC=1 (no command prefix)
 * I2C: io->tx_color(io, 0x40, data, len) - data written to control byte 0x40
 */
static void ssd1306_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
{
    ssd1306_priv_t *priv = (ssd1306_priv_t *)self->priv;
    if (priv->is_i2c)
    {
        self->io->tx_color(self->io, SSD1306_I2C_DATA, data, len);
    }
    else
    {
        self->io->tx_color(self->io, -1, data, len);
    }
}

/* ============================================================
 * Driver Callbacks
 * ============================================================ */

/* Driver: init */
static int ssd1306_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* Custom init takes full control */
    if (config->custom_init)
    {
        return config->custom_init(self->io, config);
    }

    /* Default initialization sequence */
    ssd1306_write_cmd(self, SSD1306_DISPLAYOFF);

    /* Set display clock divide ratio/oscillator frequency */
    ssd1306_write_cmd(self, SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_write_cmd(self, 0x80);

    /* Set multiplex ratio */
    ssd1306_write_cmd(self, SSD1306_SETMULTIPLEX);
    ssd1306_write_cmd(self, config->height - 1);

    /* Set display offset */
    ssd1306_write_cmd(self, SSD1306_SETDISPLAYOFFSET);
    ssd1306_write_cmd(self, 0x00);

    /* Set start line */
    ssd1306_write_cmd(self, SSD1306_SETSTARTLINE | 0x00);

    /* Charge pump */
    ssd1306_write_cmd(self, SSD1306_CHARGEPUMP);
    ssd1306_write_cmd(self, 0x14); /* Enable charge pump */

    /* Memory mode: horizontal addressing */
    ssd1306_write_cmd(self, SSD1306_MEMORYMODE);
    ssd1306_write_cmd(self, 0x00);

    /* Segment remap */
    ssd1306_write_cmd(self, SSD1306_SEGREMAP | 0x01);

    /* COM scan direction */
    ssd1306_write_cmd(self, SSD1306_COMSCANDEC);

    /* COM pins hardware configuration */
    ssd1306_write_cmd(self, SSD1306_SETCOMPINS);
    if (config->height == 64)
    {
        ssd1306_write_cmd(self, 0x12);
    }
    else if (config->height == 32)
    {
        ssd1306_write_cmd(self, 0x02);
    }
    else
    {
        ssd1306_write_cmd(self, 0x12); /* Default */
    }

    /* Set contrast */
    ssd1306_write_cmd(self, SSD1306_SETCONTRAST);
    ssd1306_write_cmd(self, 0x7F); /* Default contrast */

    /* Set precharge period */
    ssd1306_write_cmd(self, SSD1306_SETPRECHARGE);
    ssd1306_write_cmd(self, 0xF1);

    /* Set VCOMH deselect level */
    ssd1306_write_cmd(self, SSD1306_SETVCOMDETECT);
    ssd1306_write_cmd(self, 0x40);

    /* Entire display on (resume from RAM) */
    ssd1306_write_cmd(self, SSD1306_DISPLAYALLON_RESUME);

    /* Normal display (not inverted) */
    if (config->invert_color)
    {
        ssd1306_write_cmd(self, SSD1306_INVERTDISPLAY);
    }
    else
    {
        ssd1306_write_cmd(self, SSD1306_NORMALDISPLAY);
    }

    /* Deactivate scroll */
    ssd1306_write_cmd(self, SSD1306_DEACTIVATE_SCROLL);

    /* Display on */
    ssd1306_write_cmd(self, SSD1306_DISPLAYON);

    return 0;
}

/* Driver: reset */
static int ssd1306_reset(egui_hal_lcd_driver_t *self)
{
    egui_lcd_hw_reset(self, 10);
    return 0;
}

/* Driver: del */
static void ssd1306_del(egui_hal_lcd_driver_t *self)
{
    if (self->set_rst)
    {
        self->set_rst(0);
    }
    memset(self, 0, sizeof(egui_hal_lcd_driver_t));
}

/* Driver: draw_area */
static void ssd1306_draw_area(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                              int16_t w, int16_t h, const void *data, uint32_t len)
{
    /* SSD1306 uses page addressing for vertical */
    /* Convert pixel coordinates to page/column */
    uint8_t x0 = x + self->config.x_offset;
    uint8_t x1 = x0 + w - 1;
    uint8_t page0 = (y + self->config.y_offset) / 8;
    uint8_t page1 = (y + self->config.y_offset + h - 1) / 8;

    /* Set column address */
    ssd1306_write_cmd(self, SSD1306_COLUMNADDR);
    ssd1306_write_cmd(self, x0);
    ssd1306_write_cmd(self, x1);

    /* Set page address */
    ssd1306_write_cmd(self, SSD1306_PAGEADDR);
    ssd1306_write_cmd(self, page0);
    ssd1306_write_cmd(self, page1);

    /* Write pixels */
    ssd1306_write_data(self, (const uint8_t *)data, len);
}

/* Driver: set_power */
static void ssd1306_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    ssd1306_write_cmd(self, on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

/* Driver: set_invert */
static void ssd1306_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    ssd1306_write_cmd(self, invert ? SSD1306_INVERTDISPLAY : SSD1306_NORMALDISPLAY);
}

/* ============================================================
 * Internal Setup
 * ============================================================ */

static void ssd1306_setup_driver(egui_hal_lcd_driver_t *driver, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level),
                                  uint8_t is_i2c)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "SSD1306";

    driver->reset = ssd1306_reset;
    driver->init = ssd1306_init;
    driver->del = ssd1306_del;
    driver->draw_area = ssd1306_draw_area;
    driver->mirror = NULL;
    driver->swap_xy = NULL;
    driver->set_power = ssd1306_set_power;
    driver->set_invert = ssd1306_set_invert;

    driver->io = io;
    driver->set_rst = set_rst;

    /* Setup private data */
    s_ssd1306_priv.is_i2c = is_i2c;
    driver->priv = &s_ssd1306_priv;
}

/* ============================================================
 * Public API
 * ============================================================ */

void egui_lcd_ssd1306_init_spi(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    ssd1306_setup_driver(storage, io, set_rst, 0);
}

void egui_lcd_ssd1306_init_i2c(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level))
{
    if (!storage || !io || !io->tx_param)
    {
        return;
    }

    ssd1306_setup_driver(storage, io, set_rst, 1);
}

void egui_lcd_ssd1306_set_contrast(egui_hal_lcd_driver_t *self, uint8_t contrast)
{
    if (!self)
    {
        return;
    }
    ssd1306_write_cmd(self, SSD1306_SETCONTRAST);
    ssd1306_write_cmd(self, contrast);
}
