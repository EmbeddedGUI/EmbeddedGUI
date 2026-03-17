/**
 * @file egui_lcd_spd2010.c
 * @brief SPD2010 LCD driver implementation
 */

#include "egui_lcd_spd2010.h"
#include <string.h>
#include "core/egui_api.h"

/* SPD2010 Commands */
#define SPD2010_SWRESET 0x01
#define SPD2010_SLPIN   0x10
#define SPD2010_SLPOUT  0x11
#define SPD2010_INVOFF  0x20
#define SPD2010_INVON   0x21
#define SPD2010_DISPOFF 0x28
#define SPD2010_DISPON  0x29
#define SPD2010_CASET   0x2A
#define SPD2010_RASET   0x2B
#define SPD2010_RAMWR   0x2C
#define SPD2010_MADCTL  0x36
#define SPD2010_COLMOD  0x3A

/* MADCTL bits */
#define SPD2010_MADCTL_MY  0x80
#define SPD2010_MADCTL_MX  0x40
#define SPD2010_MADCTL_MV  0x20
#define SPD2010_MADCTL_BGR 0x08

/* Color modes */
#define SPD2010_COLOR_MODE_RGB565 0x55 /* 16-bit RGB565 */
#define SPD2010_COLOR_MODE_RGB666 0x66 /* 18-bit RGB666 */
#define SPD2010_COLOR_MODE_RGB888 0x77 /* 24-bit RGB888 */

/* Helper: write command */
static void spd2010_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void spd2010_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void spd2010_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void spd2010_hw_reset(egui_hal_lcd_driver_t *self)
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
static int spd2010_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    spd2010_hw_reset(self);

    /* Software reset */
    spd2010_write_cmd(self, SPD2010_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    spd2010_write_cmd(self, SPD2010_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Set color mode based on config */
    spd2010_write_cmd(self, SPD2010_COLMOD);
    if (config->color_depth == 24)
    {
        spd2010_write_data_byte(self, SPD2010_COLOR_MODE_RGB888);
    }
    else if (config->color_depth == 18)
    {
        spd2010_write_data_byte(self, SPD2010_COLOR_MODE_RGB666);
    }
    else
    {
        spd2010_write_data_byte(self, SPD2010_COLOR_MODE_RGB565);
    }

    /* Set rotation (default 0) */
    spd2010_write_cmd(self, SPD2010_MADCTL);
    spd2010_write_data_byte(self, SPD2010_MADCTL_MX | SPD2010_MADCTL_MY);

    /* Color inversion */
    if (config->invert_color)
    {
        spd2010_write_cmd(self, SPD2010_INVON);
    }
    else
    {
        spd2010_write_cmd(self, SPD2010_INVOFF);
    }

    /* Display on */
    spd2010_write_cmd(self, SPD2010_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void spd2010_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    spd2010_write_cmd(self, SPD2010_DISPOFF);

    /* Sleep in */
    spd2010_write_cmd(self, SPD2010_SLPIN);

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
static void spd2010_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    spd2010_write_cmd(self, SPD2010_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        spd2010_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    spd2010_write_cmd(self, SPD2010_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        spd2010_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    spd2010_write_cmd(self, SPD2010_RAMWR);
}

/* Driver: write_pixels */
static void spd2010_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void spd2010_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_power */
static void spd2010_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        spd2010_write_cmd(self, SPD2010_SLPOUT);
        egui_api_delay(120);
        spd2010_write_cmd(self, SPD2010_DISPON);
    }
    else
    {
        spd2010_write_cmd(self, SPD2010_DISPOFF);
        spd2010_write_cmd(self, SPD2010_SLPIN);
    }
}

/* Driver: set_invert */
static void spd2010_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    spd2010_write_cmd(self, invert ? SPD2010_INVON : SPD2010_INVOFF);
}

/* Internal: setup driver function pointers */
static void spd2010_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "SPD2010";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = spd2010_init;
    driver->deinit = spd2010_deinit;
    driver->set_window = spd2010_set_window;
    driver->write_pixels = spd2010_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? spd2010_wait_dma_complete : NULL;
    driver->set_rotation = NULL;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = spd2010_set_power;
    driver->set_invert = spd2010_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_spd2010_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    spd2010_setup_driver(storage, spi, gpio);
}
