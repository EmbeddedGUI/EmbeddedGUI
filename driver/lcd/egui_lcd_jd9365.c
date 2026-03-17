/**
 * @file egui_lcd_jd9365.c
 * @brief JD9365 LCD driver implementation
 */

#include "egui_lcd_jd9365.h"
#include <string.h>
#include "core/egui_api.h"

/* JD9365 Commands */
#define JD9365_SWRESET 0x01
#define JD9365_SLPIN   0x10
#define JD9365_SLPOUT  0x11
#define JD9365_INVOFF  0x20
#define JD9365_INVON   0x21
#define JD9365_DISPOFF 0x28
#define JD9365_DISPON  0x29
#define JD9365_CASET   0x2A
#define JD9365_RASET   0x2B
#define JD9365_RAMWR   0x2C
#define JD9365_MADCTL  0x36
#define JD9365_COLMOD  0x3A

/* MADCTL bits */
#define JD9365_MADCTL_MY  0x80
#define JD9365_MADCTL_MX  0x40
#define JD9365_MADCTL_MV  0x20
#define JD9365_MADCTL_BGR 0x08

/* Color modes */
#define JD9365_COLOR_MODE_16BIT 0x55 /* RGB565 */
#define JD9365_COLOR_MODE_18BIT 0x66 /* RGB666 */
#define JD9365_COLOR_MODE_24BIT 0x77 /* RGB888 */

/* Helper: write command */
static void jd9365_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void jd9365_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void jd9365_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void jd9365_hw_reset(egui_hal_lcd_driver_t *self)
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
static int jd9365_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    jd9365_hw_reset(self);

    /* Software reset */
    jd9365_write_cmd(self, JD9365_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    jd9365_write_cmd(self, JD9365_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    jd9365_write_cmd(self, JD9365_COLMOD);
    jd9365_write_data_byte(self, JD9365_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    jd9365_write_cmd(self, JD9365_MADCTL);
    jd9365_write_data_byte(self, JD9365_MADCTL_MX | JD9365_MADCTL_MY);

    /* Color inversion */
    if (config->invert_color)
    {
        jd9365_write_cmd(self, JD9365_INVON);
    }
    else
    {
        jd9365_write_cmd(self, JD9365_INVOFF);
    }

    /* Display on */
    jd9365_write_cmd(self, JD9365_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void jd9365_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    jd9365_write_cmd(self, JD9365_DISPOFF);

    /* Sleep in */
    jd9365_write_cmd(self, JD9365_SLPIN);

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
static void jd9365_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
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
    jd9365_write_cmd(self, JD9365_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        jd9365_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    jd9365_write_cmd(self, JD9365_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        jd9365_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    jd9365_write_cmd(self, JD9365_RAMWR);
}

/* Driver: write_pixels */
static void jd9365_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void jd9365_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void jd9365_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation)
    {
    case 0:
        madctl = JD9365_MADCTL_MX | JD9365_MADCTL_MY;
        break;
    case 1:
        madctl = JD9365_MADCTL_MY | JD9365_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = JD9365_MADCTL_MX | JD9365_MADCTL_MV;
        break;
    }

    jd9365_write_cmd(self, JD9365_MADCTL);
    jd9365_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void jd9365_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        jd9365_write_cmd(self, JD9365_SLPOUT);
        egui_api_delay(120);
        jd9365_write_cmd(self, JD9365_DISPON);
    }
    else
    {
        jd9365_write_cmd(self, JD9365_DISPOFF);
        jd9365_write_cmd(self, JD9365_SLPIN);
    }
}

/* Driver: set_invert */
static void jd9365_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    jd9365_write_cmd(self, invert ? JD9365_INVON : JD9365_INVOFF);
}

/* Internal: setup driver function pointers */
static void jd9365_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "JD9365";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = jd9365_init;
    driver->deinit = jd9365_deinit;
    driver->set_window = jd9365_set_window;
    driver->write_pixels = jd9365_write_pixels;
    driver->wait_dma_complete = jd9365_wait_dma_complete;
    driver->set_rotation = jd9365_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = jd9365_set_power;
    driver->set_invert = jd9365_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_jd9365_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    jd9365_setup_driver(storage, spi, gpio);
}
