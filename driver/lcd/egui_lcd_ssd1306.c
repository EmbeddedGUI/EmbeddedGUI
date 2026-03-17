/**
 * @file egui_lcd_ssd1306.c
 * @brief SSD1306 OLED driver implementation
 *
 * SSD1306 is a 128x64 monochrome OLED controller.
 * Supports both I2C and SPI interfaces with 1-bit color depth.
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

/* I2C address and control bytes */
#define SSD1306_I2C_ADDR 0x3C /* Default I2C address (can also be 0x3D) */
#define SSD1306_I2C_CMD  0x00 /* Co=0, D/C#=0: Command */
#define SSD1306_I2C_DATA 0x40 /* Co=0, D/C#=1: Data */

/* ============================================================
 * SPI Interface Implementation
 * ============================================================ */

/* Helper: write command via SPI */
static void ssd1306_spi_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(0); /* Command mode */
    }
    self->bus.spi->write(&cmd, 1);
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Helper: write data via SPI */
static void ssd1306_spi_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write(data, len);
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* ============================================================
 * I2C Interface Implementation
 * ============================================================ */

/* Helper: write command via I2C */
static void ssd1306_i2c_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
{
    /* Use control byte as register address, command as data */
    self->bus.i2c->write_reg(SSD1306_I2C_ADDR, SSD1306_I2C_CMD, &cmd, 1);
}

/* Helper: write data via I2C */
static void ssd1306_i2c_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
{
    /* Use control byte as register address, pixel data as data */
    /* Write in chunks to avoid large stack buffers */
    while (len > 0)
    {
        uint16_t chunk = (len > 128) ? 128 : (uint16_t)len;
        self->bus.i2c->write_reg(SSD1306_I2C_ADDR, SSD1306_I2C_DATA, data, chunk);
        data += chunk;
        len -= chunk;
    }
}

/* ============================================================
 * Common Implementation
 * ============================================================ */

/* Helper: hardware reset */
static void ssd1306_hw_reset(egui_hal_lcd_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst)
    {
        self->gpio->set_rst(0);
        egui_api_delay(10);
        self->gpio->set_rst(1);
        egui_api_delay(10);
    }
}

/* Helper: write command (dispatch based on bus type) */
static void ssd1306_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
{
    if (self->bus_type == EGUI_BUS_TYPE_SPI)
    {
        ssd1306_spi_write_cmd(self, cmd);
    }
    else
    {
        ssd1306_i2c_write_cmd(self, cmd);
    }
}

/* Helper: write data (dispatch based on bus type) */
static void ssd1306_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
{
    if (self->bus_type == EGUI_BUS_TYPE_SPI)
    {
        ssd1306_spi_write_data(self, data, len);
    }
    else
    {
        ssd1306_i2c_write_data(self, data, len);
    }
}

/* Driver: init */
static int ssd1306_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* Initialize bus and GPIO */
    if (self->bus_type == EGUI_BUS_TYPE_SPI)
    {
        if (self->bus.spi->init)
        {
            self->bus.spi->init();
        }
    }
    else
    {
        if (self->bus.i2c->init)
        {
            self->bus.i2c->init();
        }
    }
    if (self->gpio && self->gpio->init)
    {
        self->gpio->init();
    }

    /* Hardware reset */
    ssd1306_hw_reset(self);

    /* Initialization sequence */
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

/* Driver: deinit */
static void ssd1306_deinit(egui_hal_lcd_driver_t *self)
{
    /* Display off */
    ssd1306_write_cmd(self, SSD1306_DISPLAYOFF);

    /* Deinit bus and GPIO */
    if (self->bus_type == EGUI_BUS_TYPE_SPI)
    {
        if (self->bus.spi->deinit)
        {
            self->bus.spi->deinit();
        }
    }
    else
    {
        if (self->bus.i2c->deinit)
        {
            self->bus.i2c->deinit();
        }
    }
    if (self->gpio && self->gpio->deinit)
    {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void ssd1306_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
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
}

/* Driver: write_pixels */
static void ssd1306_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    ssd1306_write_data(self, (const uint8_t *)data, len);
}

/* Driver: wait_dma_complete (SPI only) */
static void ssd1306_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus_type == EGUI_BUS_TYPE_SPI && self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void ssd1306_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    switch (rotation)
    {
    case 0:
        ssd1306_write_cmd(self, SSD1306_SEGREMAP | 0x01);
        ssd1306_write_cmd(self, SSD1306_COMSCANDEC);
        break;
    case 1:
        /* 90 degrees - not directly supported, would need software rotation */
        break;
    case 2:
        ssd1306_write_cmd(self, SSD1306_SEGREMAP | 0x00);
        ssd1306_write_cmd(self, SSD1306_COMSCANINC);
        break;
    case 3:
        /* 270 degrees - not directly supported */
        break;
    }
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
 * Public API - SPI Interface
 * ============================================================ */

static void ssd1306_setup_driver_spi(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "SSD1306";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = ssd1306_init;
    driver->deinit = ssd1306_deinit;
    driver->set_window = ssd1306_set_window;
    driver->write_pixels = ssd1306_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? ssd1306_wait_dma_complete : NULL;
    driver->set_rotation = ssd1306_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = ssd1306_set_power;
    driver->set_invert = ssd1306_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

void egui_lcd_ssd1306_init_spi(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    ssd1306_setup_driver_spi(storage, spi, gpio);
}

/* ============================================================
 * Public API - I2C Interface
 * ============================================================ */

static void ssd1306_setup_driver_i2c(egui_hal_lcd_driver_t *driver, const egui_bus_i2c_ops_t *i2c, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "SSD1306";
    driver->bus_type = EGUI_BUS_TYPE_I2C;

    driver->init = ssd1306_init;
    driver->deinit = ssd1306_deinit;
    driver->set_window = ssd1306_set_window;
    driver->write_pixels = ssd1306_write_pixels;
    driver->wait_dma_complete = NULL; /* I2C is synchronous */
    driver->set_rotation = ssd1306_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = ssd1306_set_power;
    driver->set_invert = ssd1306_set_invert;

    driver->bus.i2c = i2c;
    driver->gpio = gpio;
}

void egui_lcd_ssd1306_init_i2c(egui_hal_lcd_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !i2c || !i2c->write_reg)
    {
        return;
    }

    ssd1306_setup_driver_i2c(storage, i2c, gpio);
}

/* ============================================================
 * Additional API
 * ============================================================ */

void egui_lcd_ssd1306_set_contrast(egui_hal_lcd_driver_t *self, uint8_t contrast)
{
    if (!self)
    {
        return;
    }
    ssd1306_write_cmd(self, SSD1306_SETCONTRAST);
    ssd1306_write_cmd(self, contrast);
}
