/**
 * @file egui_lcd_hx8399.c
 * @brief HX8399 LCD driver implementation
 */

#include "egui_lcd_hx8399.h"
#include <string.h>
#include "core/egui_api.h"

/* HX8399 Commands */
#define HX8399_SWRESET    0x01
#define HX8399_SLPIN      0x10
#define HX8399_SLPOUT     0x11
#define HX8399_INVOFF     0x20
#define HX8399_INVON      0x21
#define HX8399_DISPOFF    0x28
#define HX8399_DISPON     0x29
#define HX8399_CASET      0x2A
#define HX8399_RASET      0x2B
#define HX8399_RAMWR      0x2C
#define HX8399_MADCTL     0x36
#define HX8399_COLMOD     0x3A

/* MADCTL bits */
#define HX8399_MADCTL_MY  0x80
#define HX8399_MADCTL_MX  0x40
#define HX8399_MADCTL_MV  0x20
#define HX8399_MADCTL_BGR 0x08

/* Color modes */
#define HX8399_COLOR_MODE_RGB565 0x55
#define HX8399_COLOR_MODE_RGB666 0x66
#define HX8399_COLOR_MODE_RGB888 0x77

/* Helper: write command */
static void hx8399_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void hx8399_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void hx8399_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void hx8399_hw_reset(egui_hal_lcd_driver_t *self)
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
static int hx8399_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    hx8399_hw_reset(self);

    /* Software reset */
    hx8399_write_cmd(self, HX8399_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    hx8399_write_cmd(self, HX8399_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Memory access control */
    hx8399_write_cmd(self, HX8399_MADCTL);
    hx8399_write_data_byte(self, HX8399_MADCTL_MX | HX8399_MADCTL_BGR);

    /* Set color mode based on config */
    hx8399_write_cmd(self, HX8399_COLMOD);
    if (config->color_depth == 24) {
        hx8399_write_data_byte(self, HX8399_COLOR_MODE_RGB888);
    } else if (config->color_depth == 18) {
        hx8399_write_data_byte(self, HX8399_COLOR_MODE_RGB666);
    } else {
        hx8399_write_data_byte(self, HX8399_COLOR_MODE_RGB565);
    }

    /* Inversion control */
    if (config->invert_color) {
        hx8399_write_cmd(self, HX8399_INVON);
    } else {
        hx8399_write_cmd(self, HX8399_INVOFF);
    }

    /* Display on */
    hx8399_write_cmd(self, HX8399_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void hx8399_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    hx8399_write_cmd(self, HX8399_DISPOFF);

    /* Sleep in */
    hx8399_write_cmd(self, HX8399_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void hx8399_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
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
    hx8399_write_cmd(self, HX8399_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        hx8399_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    hx8399_write_cmd(self, HX8399_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        hx8399_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    hx8399_write_cmd(self, HX8399_RAMWR);
}

/* Driver: write_pixels */
static void hx8399_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void hx8399_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void hx8399_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = HX8399_MADCTL_BGR;

    switch (rotation) {
    case 0:
        madctl |= HX8399_MADCTL_MX;
        break;
    case 1:
        madctl |= HX8399_MADCTL_MV;
        break;
    case 2:
        madctl |= HX8399_MADCTL_MY;
        break;
    case 3:
        madctl |= HX8399_MADCTL_MX | HX8399_MADCTL_MY | HX8399_MADCTL_MV;
        break;
    }

    hx8399_write_cmd(self, HX8399_MADCTL);
    hx8399_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void hx8399_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        hx8399_write_cmd(self, HX8399_SLPOUT);
        egui_api_delay(120);
        hx8399_write_cmd(self, HX8399_DISPON);
    } else {
        hx8399_write_cmd(self, HX8399_DISPOFF);
        hx8399_write_cmd(self, HX8399_SLPIN);
    }
}

/* Driver: set_invert */
static void hx8399_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    hx8399_write_cmd(self, invert ? HX8399_INVON : HX8399_INVOFF);
}

/* Internal: setup driver function pointers */
static void hx8399_setup_driver(egui_hal_lcd_driver_t *driver,
                                const egui_bus_spi_ops_t *spi,
                                const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "HX8399";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = hx8399_init;
    driver->deinit = hx8399_deinit;
    driver->set_window = hx8399_set_window;
    driver->write_pixels = hx8399_write_pixels;
    driver->wait_dma_complete = hx8399_wait_dma_complete;
    driver->set_rotation = hx8399_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = hx8399_set_power;
    driver->set_invert = hx8399_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_hx8399_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    hx8399_setup_driver(storage, spi, gpio);
}
