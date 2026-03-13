/**
 * @file egui_lcd_ek79007.c
 * @brief EK79007 LCD driver implementation
 */

#include "egui_lcd_ek79007.h"
#include <string.h>
#include "core/egui_api.h"

/* EK79007 Commands */
#define EK79007_SWRESET  0x01
#define EK79007_SLPIN    0x10
#define EK79007_SLPOUT   0x11
#define EK79007_INVOFF   0x20
#define EK79007_INVON    0x21
#define EK79007_DISPOFF  0x28
#define EK79007_DISPON   0x29
#define EK79007_CASET    0x2A
#define EK79007_RASET    0x2B
#define EK79007_RAMWR    0x2C
#define EK79007_MADCTL   0x36
#define EK79007_COLMOD   0x3A

/* MADCTL bits */
#define EK79007_MADCTL_MY  0x80
#define EK79007_MADCTL_MX  0x40
#define EK79007_MADCTL_MV  0x20
#define EK79007_MADCTL_BGR 0x08

/* Color modes */
#define EK79007_COLOR_MODE_RGB565 0x55
#define EK79007_COLOR_MODE_RGB666 0x66
#define EK79007_COLOR_MODE_RGB888 0x77

/* Helper: write command */
static void ek79007_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(0);  /* Command mode */
    }
    self->bus.spi->write(&cmd, 1);
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Helper: write data byte */
static void ek79007_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write(&data, 1);
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Helper: write data buffer */
static void ek79007_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write(data, len);
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Helper: hardware reset */
static void ek79007_hw_reset(egui_hal_lcd_driver_t *self)
{
    if (self->gpio && self->gpio->set_rst) {
        self->gpio->set_rst(0);
        /* Simple delay - platform should provide proper delay */
        egui_api_delay(10);
        self->gpio->set_rst(1);
        egui_api_delay(10);
    }
}

/* Driver: init */
static int ek79007_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
{
    /* Save config */
    memcpy(&self->config, config, sizeof(egui_hal_lcd_config_t));

    /* Initialize bus and GPIO */
    if (self->bus.spi->init) {
        self->bus.spi->init();
    }
    if (self->gpio && self->gpio->init) {
        self->gpio->init();
    }

    /* Hardware reset */
    ek79007_hw_reset(self);

    /* Software reset */
    ek79007_write_cmd(self, EK79007_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    ek79007_write_cmd(self, EK79007_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Set color mode based on config */
    ek79007_write_cmd(self, EK79007_COLMOD);
    if (config->color_depth == 24) {
        ek79007_write_data_byte(self, EK79007_COLOR_MODE_RGB888);
    } else if (config->color_depth == 18) {
        ek79007_write_data_byte(self, EK79007_COLOR_MODE_RGB666);
    } else {
        ek79007_write_data_byte(self, EK79007_COLOR_MODE_RGB565);
    }

    /* Set rotation (default 0) */
    ek79007_write_cmd(self, EK79007_MADCTL);
    ek79007_write_data_byte(self, EK79007_MADCTL_MX | EK79007_MADCTL_MY);

    /* Set color inversion */
    if (config->invert_color) {
        ek79007_write_cmd(self, EK79007_INVON);
    } else {
        ek79007_write_cmd(self, EK79007_INVOFF);
    }

    /* Display on */
    ek79007_write_cmd(self, EK79007_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void ek79007_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    ek79007_write_cmd(self, EK79007_DISPOFF);

    /* Sleep in */
    ek79007_write_cmd(self, EK79007_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void ek79007_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                               int16_t w, int16_t h)
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
    ek79007_write_cmd(self, EK79007_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        ek79007_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    ek79007_write_cmd(self, EK79007_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        ek79007_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    ek79007_write_cmd(self, EK79007_RAMWR);
}

/* Driver: write_pixels */
static void ek79007_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void ek79007_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void ek79007_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation) {
    case 0:
        madctl = EK79007_MADCTL_MX | EK79007_MADCTL_MY;
        break;
    case 1:
        madctl = EK79007_MADCTL_MY | EK79007_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = EK79007_MADCTL_MX | EK79007_MADCTL_MV;
        break;
    }

    ek79007_write_cmd(self, EK79007_MADCTL);
    ek79007_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void ek79007_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        ek79007_write_cmd(self, EK79007_SLPOUT);
        egui_api_delay(120);
        ek79007_write_cmd(self, EK79007_DISPON);
    } else {
        ek79007_write_cmd(self, EK79007_DISPOFF);
        ek79007_write_cmd(self, EK79007_SLPIN);
    }
}

/* Driver: set_invert */
static void ek79007_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    ek79007_write_cmd(self, invert ? EK79007_INVON : EK79007_INVOFF);
}

/* Internal: setup driver function pointers */
static void ek79007_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "EK79007";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = ek79007_init;
    driver->deinit = ek79007_deinit;
    driver->set_window = ek79007_set_window;
    driver->write_pixels = ek79007_write_pixels;
    driver->wait_dma_complete = ek79007_wait_dma_complete;
    driver->set_rotation = ek79007_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = ek79007_set_power;
    driver->set_invert = ek79007_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_ek79007_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    ek79007_setup_driver(storage, spi, gpio);
}
