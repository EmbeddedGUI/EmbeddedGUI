/**
 * @file egui_lcd_sh8601.c
 * @brief SH8601 AMOLED driver implementation
 */

#include "egui_lcd_sh8601.h"
#include <string.h>
#include "core/egui_api.h"

/* SH8601 Commands */
#define SH8601_SWRESET   0x01
#define SH8601_SLPIN     0x10
#define SH8601_SLPOUT    0x11
#define SH8601_INVOFF    0x20
#define SH8601_INVON     0x21
#define SH8601_DISPOFF   0x28
#define SH8601_DISPON    0x29
#define SH8601_CASET     0x2A
#define SH8601_RASET     0x2B
#define SH8601_RAMWR     0x2C
#define SH8601_MADCTL    0x36
#define SH8601_COLMOD    0x3A

/* MADCTL bits */
#define SH8601_MADCTL_MY  0x80
#define SH8601_MADCTL_MX  0x40
#define SH8601_MADCTL_MV  0x20
#define SH8601_MADCTL_BGR 0x08

/* Color modes */
#define SH8601_COLOR_MODE_RGB565 0x55
#define SH8601_COLOR_MODE_RGB666 0x66
#define SH8601_COLOR_MODE_RGB888 0x77

/* Vendor specific commands */
#define SH8601_VENDOR_TEON   0x35
#define SH8601_VENDOR_WRCTRLD 0x53
#define SH8601_VENDOR_SETSCROLL 0x44

/* Helper: write command */
static void sh8601_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void sh8601_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void sh8601_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void sh8601_hw_reset(egui_hal_lcd_driver_t *self)
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
static int sh8601_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    sh8601_hw_reset(self);

    /* Software reset */
    sh8601_write_cmd(self, SH8601_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    sh8601_write_cmd(self, SH8601_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Vendor specific init: Set scroll area */
    sh8601_write_cmd(self, SH8601_VENDOR_SETSCROLL);
    {
        uint8_t data[] = {0x00, 0xc8};
        sh8601_write_data(self, data, sizeof(data));
    }

    /* Vendor specific init: Tearing effect line on */
    sh8601_write_cmd(self, SH8601_VENDOR_TEON);
    sh8601_write_data_byte(self, 0x00);

    /* Vendor specific init: Write control display */
    sh8601_write_cmd(self, SH8601_VENDOR_WRCTRLD);
    sh8601_write_data_byte(self, 0x20);

    /* Set color mode to 16-bit RGB565 */
    sh8601_write_cmd(self, SH8601_COLMOD);
    sh8601_write_data_byte(self, SH8601_COLOR_MODE_RGB565);

    /* Set rotation (default 0) */
    sh8601_write_cmd(self, SH8601_MADCTL);
    sh8601_write_data_byte(self, SH8601_MADCTL_MX | SH8601_MADCTL_MY);

    /* Inversion control */
    if (config->invert_color) {
        sh8601_write_cmd(self, SH8601_INVON);
    } else {
        sh8601_write_cmd(self, SH8601_INVOFF);
    }

    /* Display on */
    sh8601_write_cmd(self, SH8601_DISPON);

    /* Brightness control - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void sh8601_deinit(egui_hal_lcd_driver_t *self)
{
    /* Brightness off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    sh8601_write_cmd(self, SH8601_DISPOFF);

    /* Sleep in */
    sh8601_write_cmd(self, SH8601_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void sh8601_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                               int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    sh8601_write_cmd(self, SH8601_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        sh8601_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    sh8601_write_cmd(self, SH8601_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        sh8601_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    sh8601_write_cmd(self, SH8601_RAMWR);
}

/* Driver: write_pixels */
static void sh8601_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void sh8601_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_power */
static void sh8601_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        sh8601_write_cmd(self, SH8601_SLPOUT);
        egui_api_delay(120);
        sh8601_write_cmd(self, SH8601_DISPON);
    } else {
        sh8601_write_cmd(self, SH8601_DISPOFF);
        sh8601_write_cmd(self, SH8601_SLPIN);
    }
}

/* Driver: set_invert */
static void sh8601_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    sh8601_write_cmd(self, invert ? SH8601_INVON : SH8601_INVOFF);
}

/* Internal: setup driver function pointers */
static void sh8601_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "SH8601";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = sh8601_init;
    driver->deinit = sh8601_deinit;
    driver->set_window = sh8601_set_window;
    driver->write_pixels = sh8601_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? sh8601_wait_dma_complete : NULL;
    driver->set_rotation = NULL;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = sh8601_set_power;
    driver->set_invert = sh8601_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_sh8601_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    sh8601_setup_driver(storage, spi, gpio);
}
