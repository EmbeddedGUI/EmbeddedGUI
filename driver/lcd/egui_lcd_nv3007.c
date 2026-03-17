/**
 * @file egui_lcd_nv3007.c
 * @brief NV3007 LCD driver implementation
 */

#include "egui_lcd_nv3007.h"
#include <string.h>
#include "core/egui_api.h"

/* NV3007 Commands */
#define NV3007_NOP     0x00
#define NV3007_SWRESET 0x01
#define NV3007_SLPIN   0x10
#define NV3007_SLPOUT  0x11
#define NV3007_NORON   0x13
#define NV3007_INVOFF  0x20
#define NV3007_INVON   0x21
#define NV3007_DISPOFF 0x28
#define NV3007_DISPON  0x29
#define NV3007_CASET   0x2A
#define NV3007_RASET   0x2B
#define NV3007_RAMWR   0x2C
#define NV3007_MADCTL  0x36
#define NV3007_COLMOD  0x3A

/* MADCTL bits */
#define NV3007_MADCTL_MY  0x80
#define NV3007_MADCTL_MX  0x40
#define NV3007_MADCTL_MV  0x20
#define NV3007_MADCTL_ML  0x10
#define NV3007_MADCTL_RGB 0x00
#define NV3007_MADCTL_BGR 0x08

/* Color modes */
#define NV3007_COLOR_MODE_16BIT 0x55

/* Helper: write command */
static void nv3007_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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

/* Helper: write data byte */
static void nv3007_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write(&data, 1);
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Helper: write data buffer */
static void nv3007_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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

/* Helper: hardware reset */
static void nv3007_hw_reset(egui_hal_lcd_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst)
    {
        self->gpio->set_rst(0);
        /* Simple delay - platform should provide proper delay */
        egui_api_delay(10);
        self->gpio->set_rst(1);
        egui_api_delay(10);
    }
}

/* Driver: init */
static int nv3007_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.spi->init)
    {
        self->bus.spi->init();
    }
    if (self->gpio && self->gpio->init)
    {
        self->gpio->init();
    }

    /* Hardware reset */
    nv3007_hw_reset(self);

    /* Software reset */
    nv3007_write_cmd(self, NV3007_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    nv3007_write_cmd(self, NV3007_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    nv3007_write_cmd(self, NV3007_COLMOD);
    nv3007_write_data_byte(self, NV3007_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    nv3007_write_cmd(self, NV3007_MADCTL);
    nv3007_write_data_byte(self, NV3007_MADCTL_MX | NV3007_MADCTL_MY | NV3007_MADCTL_RGB);

    /* Inversion control */
    if (config->invert_color)
    {
        nv3007_write_cmd(self, NV3007_INVON);
    }
    else
    {
        nv3007_write_cmd(self, NV3007_INVOFF);
    }

    /* Normal display mode */
    nv3007_write_cmd(self, NV3007_NORON);

    /* Display on */
    nv3007_write_cmd(self, NV3007_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void nv3007_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    nv3007_write_cmd(self, NV3007_DISPOFF);

    /* Sleep in */
    nv3007_write_cmd(self, NV3007_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit)
    {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit)
    {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void nv3007_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    nv3007_write_cmd(self, NV3007_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        nv3007_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    nv3007_write_cmd(self, NV3007_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        nv3007_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    nv3007_write_cmd(self, NV3007_RAMWR);
}

/* Driver: write_pixels */
static void nv3007_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void nv3007_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void nv3007_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = NV3007_MADCTL_RGB;

    switch (rotation)
    {
    case 0:
        madctl |= NV3007_MADCTL_MX | NV3007_MADCTL_MY;
        break;
    case 1:
        madctl |= NV3007_MADCTL_MY | NV3007_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl |= NV3007_MADCTL_MX | NV3007_MADCTL_MV;
        break;
    }

    nv3007_write_cmd(self, NV3007_MADCTL);
    nv3007_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void nv3007_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        nv3007_write_cmd(self, NV3007_SLPOUT);
        egui_api_delay(120);
        nv3007_write_cmd(self, NV3007_DISPON);
    }
    else
    {
        nv3007_write_cmd(self, NV3007_DISPOFF);
        nv3007_write_cmd(self, NV3007_SLPIN);
    }
}

/* Driver: set_invert */
static void nv3007_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    nv3007_write_cmd(self, invert ? NV3007_INVON : NV3007_INVOFF);
}

/* Internal: setup driver function pointers */
static void nv3007_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "NV3007";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = nv3007_init;
    driver->deinit = nv3007_deinit;
    driver->set_window = nv3007_set_window;
    driver->write_pixels = nv3007_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? nv3007_wait_dma_complete : NULL;
    driver->set_rotation = nv3007_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = nv3007_set_power;
    driver->set_invert = nv3007_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_nv3007_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    nv3007_setup_driver(storage, spi, gpio);
}
