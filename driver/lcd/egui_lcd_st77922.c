/**
 * @file egui_lcd_st77922.c
 * @brief ST77922 LCD driver implementation
 */

#include "egui_lcd_st77922.h"
#include <string.h>
#include "core/egui_api.h"

/* ST77922 Commands */
#define ST77922_SWRESET 0x01
#define ST77922_SLPIN   0x10
#define ST77922_SLPOUT  0x11
#define ST77922_INVOFF  0x20
#define ST77922_INVON   0x21
#define ST77922_DISPOFF 0x28
#define ST77922_DISPON  0x29
#define ST77922_CASET   0x2A
#define ST77922_RASET   0x2B
#define ST77922_RAMWR   0x2C
#define ST77922_MADCTL  0x36
#define ST77922_COLMOD  0x3A

/* MADCTL bits */
#define ST77922_MADCTL_MY  0x80
#define ST77922_MADCTL_MX  0x40
#define ST77922_MADCTL_MV  0x20
#define ST77922_MADCTL_BGR 0x08

/* Color modes */
#define ST77922_COLOR_MODE_RGB565 0x55
#define ST77922_COLOR_MODE_RGB666 0x66
#define ST77922_COLOR_MODE_RGB888 0x77

/* Helper: write command */
static void st77922_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void st77922_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void st77922_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void st77922_hw_reset(egui_hal_lcd_driver_t *self)
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
static int st77922_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    st77922_hw_reset(self);

    /* Software reset */
    st77922_write_cmd(self, ST77922_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    st77922_write_cmd(self, ST77922_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    st77922_write_cmd(self, ST77922_COLMOD);
    st77922_write_data_byte(self, ST77922_COLOR_MODE_RGB565);

    /* Set rotation (default 0) */
    st77922_write_cmd(self, ST77922_MADCTL);
    st77922_write_data_byte(self, ST77922_MADCTL_MX | ST77922_MADCTL_MY);

    /* Set inversion */
    if (config->invert_color)
    {
        st77922_write_cmd(self, ST77922_INVON);
    }
    else
    {
        st77922_write_cmd(self, ST77922_INVOFF);
    }

    /* Display on */
    st77922_write_cmd(self, ST77922_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void st77922_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    st77922_write_cmd(self, ST77922_DISPOFF);

    /* Sleep in */
    st77922_write_cmd(self, ST77922_SLPIN);

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
static void st77922_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    self->window.x = x;
    self->window.y = y;
    self->window.w = w;
    self->window.h = h;

    /* Column address set */
    st77922_write_cmd(self, ST77922_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        st77922_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    st77922_write_cmd(self, ST77922_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        st77922_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    st77922_write_cmd(self, ST77922_RAMWR);
}

/* Driver: write_pixels */
static void st77922_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void st77922_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void st77922_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation)
    {
    case 0:
        madctl = ST77922_MADCTL_MX | ST77922_MADCTL_MY;
        break;
    case 1:
        madctl = ST77922_MADCTL_MY | ST77922_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = ST77922_MADCTL_MX | ST77922_MADCTL_MV;
        break;
    }

    st77922_write_cmd(self, ST77922_MADCTL);
    st77922_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void st77922_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        st77922_write_cmd(self, ST77922_SLPOUT);
        egui_api_delay(120);
        st77922_write_cmd(self, ST77922_DISPON);
    }
    else
    {
        st77922_write_cmd(self, ST77922_DISPOFF);
        st77922_write_cmd(self, ST77922_SLPIN);
    }
}

/* Driver: set_invert */
static void st77922_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    st77922_write_cmd(self, invert ? ST77922_INVON : ST77922_INVOFF);
}

/* Internal: setup driver function pointers */
static void st77922_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST77922";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = st77922_init;
    driver->deinit = st77922_deinit;
    driver->set_window = st77922_set_window;
    driver->write_pixels = st77922_write_pixels;
    driver->wait_dma_complete = st77922_wait_dma_complete;
    driver->set_rotation = st77922_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = st77922_set_power;
    driver->set_invert = st77922_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_st77922_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    st77922_setup_driver(storage, spi, gpio);
}
