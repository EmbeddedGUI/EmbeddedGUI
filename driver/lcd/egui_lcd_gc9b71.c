/**
 * @file egui_lcd_gc9b71.c
 * @brief GC9B71 LCD driver implementation
 */

#include "egui_lcd_gc9b71.h"
#include <string.h>
#include "core/egui_api.h"

/* GC9B71 Commands */
#define GC9B71_NOP       0x00
#define GC9B71_SWRESET   0x01
#define GC9B71_SLPIN     0x10
#define GC9B71_SLPOUT    0x11
#define GC9B71_NORON     0x13
#define GC9B71_INVOFF    0x20
#define GC9B71_INVON     0x21
#define GC9B71_DISPOFF   0x28
#define GC9B71_DISPON    0x29
#define GC9B71_CASET     0x2A
#define GC9B71_RASET     0x2B
#define GC9B71_RAMWR     0x2C
#define GC9B71_MADCTL    0x36
#define GC9B71_COLMOD    0x3A

/* MADCTL bits */
#define GC9B71_MADCTL_MY  0x80
#define GC9B71_MADCTL_MX  0x40
#define GC9B71_MADCTL_MV  0x20
#define GC9B71_MADCTL_ML  0x10
#define GC9B71_MADCTL_RGB 0x00
#define GC9B71_MADCTL_BGR 0x08

/* Color modes */
#define GC9B71_COLOR_MODE_16BIT 0x55
#define GC9B71_COLOR_MODE_18BIT 0x66

/* Helper: write command */
static void gc9b71_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void gc9b71_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void gc9b71_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void gc9b71_hw_reset(egui_hal_lcd_driver_t *self)
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
static int gc9b71_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    gc9b71_hw_reset(self);

    /* Software reset */
    gc9b71_write_cmd(self, GC9B71_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    gc9b71_write_cmd(self, GC9B71_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    gc9b71_write_cmd(self, GC9B71_COLMOD);
    gc9b71_write_data_byte(self, GC9B71_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    gc9b71_write_cmd(self, GC9B71_MADCTL);
    gc9b71_write_data_byte(self, GC9B71_MADCTL_MX | GC9B71_MADCTL_MY | GC9B71_MADCTL_RGB);

    /* Inversion control */
    if (config->invert_color) {
        gc9b71_write_cmd(self, GC9B71_INVON);
    } else {
        gc9b71_write_cmd(self, GC9B71_INVOFF);
    }

    /* Normal display mode */
    gc9b71_write_cmd(self, GC9B71_NORON);

    /* Display on */
    gc9b71_write_cmd(self, GC9B71_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void gc9b71_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    gc9b71_write_cmd(self, GC9B71_DISPOFF);

    /* Sleep in */
    gc9b71_write_cmd(self, GC9B71_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void gc9b71_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                               int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    gc9b71_write_cmd(self, GC9B71_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        gc9b71_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    gc9b71_write_cmd(self, GC9B71_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        gc9b71_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    gc9b71_write_cmd(self, GC9B71_RAMWR);
}

/* Driver: write_pixels */
static void gc9b71_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void gc9b71_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void gc9b71_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = GC9B71_MADCTL_RGB;

    switch (rotation) {
    case 0:
        madctl |= GC9B71_MADCTL_MX | GC9B71_MADCTL_MY;
        break;
    case 1:
        madctl |= GC9B71_MADCTL_MY | GC9B71_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl |= GC9B71_MADCTL_MX | GC9B71_MADCTL_MV;
        break;
    }

    gc9b71_write_cmd(self, GC9B71_MADCTL);
    gc9b71_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void gc9b71_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        gc9b71_write_cmd(self, GC9B71_SLPOUT);
        egui_api_delay(120);
        gc9b71_write_cmd(self, GC9B71_DISPON);
    } else {
        gc9b71_write_cmd(self, GC9B71_DISPOFF);
        gc9b71_write_cmd(self, GC9B71_SLPIN);
    }
}

/* Driver: set_invert */
static void gc9b71_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    gc9b71_write_cmd(self, invert ? GC9B71_INVON : GC9B71_INVOFF);
}

/* Internal: setup driver function pointers */
static void gc9b71_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "GC9B71";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = gc9b71_init;
    driver->deinit = gc9b71_deinit;
    driver->set_window = gc9b71_set_window;
    driver->write_pixels = gc9b71_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? gc9b71_wait_dma_complete : NULL;
    driver->set_rotation = gc9b71_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = gc9b71_set_power;
    driver->set_invert = gc9b71_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_gc9b71_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    gc9b71_setup_driver(storage, spi, gpio);
}
