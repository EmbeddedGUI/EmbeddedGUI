/**
 * @file egui_lcd_ek9716b.c
 * @brief EK9716B LCD driver implementation
 */

#include "egui_lcd_ek9716b.h"
#include <string.h>
#include "core/egui_api.h"

/* EK9716B Commands */
#define EK9716B_SWRESET  0x01
#define EK9716B_SLPIN    0x10
#define EK9716B_SLPOUT   0x11
#define EK9716B_INVOFF   0x20
#define EK9716B_INVON    0x21
#define EK9716B_DISPOFF  0x28
#define EK9716B_DISPON   0x29
#define EK9716B_CASET    0x2A
#define EK9716B_RASET    0x2B
#define EK9716B_RAMWR    0x2C
#define EK9716B_MADCTL   0x36
#define EK9716B_COLMOD   0x3A

/* MADCTL bits */
#define EK9716B_MADCTL_MY  0x80
#define EK9716B_MADCTL_MX  0x40
#define EK9716B_MADCTL_MV  0x20
#define EK9716B_MADCTL_BGR 0x08

/* Color modes */
#define EK9716B_COLOR_MODE_RGB565 0x55
#define EK9716B_COLOR_MODE_RGB666 0x66
#define EK9716B_COLOR_MODE_RGB888 0x77

/* Helper: write command */
static void ek9716b_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void ek9716b_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void ek9716b_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void ek9716b_hw_reset(egui_hal_lcd_driver_t *self)
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
static int ek9716b_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    ek9716b_hw_reset(self);

    /* Software reset */
    ek9716b_write_cmd(self, EK9716B_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    ek9716b_write_cmd(self, EK9716B_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    ek9716b_write_cmd(self, EK9716B_COLMOD);
    ek9716b_write_data_byte(self, EK9716B_COLOR_MODE_RGB565);

    /* Set rotation (default 0) */
    ek9716b_write_cmd(self, EK9716B_MADCTL);
    ek9716b_write_data_byte(self, EK9716B_MADCTL_MX | EK9716B_MADCTL_MY);

    /* Inversion control */
    if (config->invert_color) {
        ek9716b_write_cmd(self, EK9716B_INVON);
    } else {
        ek9716b_write_cmd(self, EK9716B_INVOFF);
    }

    /* Display on */
    ek9716b_write_cmd(self, EK9716B_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void ek9716b_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    ek9716b_write_cmd(self, EK9716B_DISPOFF);

    /* Sleep in */
    ek9716b_write_cmd(self, EK9716B_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void ek9716b_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
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
    ek9716b_write_cmd(self, EK9716B_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        ek9716b_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    ek9716b_write_cmd(self, EK9716B_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        ek9716b_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    ek9716b_write_cmd(self, EK9716B_RAMWR);
}

/* Driver: write_pixels */
static void ek9716b_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void ek9716b_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void ek9716b_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation) {
    case 0:
        madctl = EK9716B_MADCTL_MX | EK9716B_MADCTL_MY;
        break;
    case 1:
        madctl = EK9716B_MADCTL_MY | EK9716B_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = EK9716B_MADCTL_MX | EK9716B_MADCTL_MV;
        break;
    }

    ek9716b_write_cmd(self, EK9716B_MADCTL);
    ek9716b_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void ek9716b_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        ek9716b_write_cmd(self, EK9716B_SLPOUT);
        egui_api_delay(120);
        ek9716b_write_cmd(self, EK9716B_DISPON);
    } else {
        ek9716b_write_cmd(self, EK9716B_DISPOFF);
        ek9716b_write_cmd(self, EK9716B_SLPIN);
    }
}

/* Driver: set_invert */
static void ek9716b_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    ek9716b_write_cmd(self, invert ? EK9716B_INVON : EK9716B_INVOFF);
}

/* Internal: setup driver function pointers */
static void ek9716b_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "EK9716B";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = ek9716b_init;
    driver->deinit = ek9716b_deinit;
    driver->set_window = ek9716b_set_window;
    driver->write_pixels = ek9716b_write_pixels;
    driver->wait_dma_complete = ek9716b_wait_dma_complete;
    driver->set_rotation = ek9716b_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = ek9716b_set_power;
    driver->set_invert = ek9716b_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_ek9716b_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    ek9716b_setup_driver(storage, spi, gpio);
}
