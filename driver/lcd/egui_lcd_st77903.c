/**
 * @file egui_lcd_st77903.c
 * @brief ST77903 LCD driver implementation
 */

#include "egui_lcd_st77903.h"
#include <string.h>
#include "core/egui_api.h"

/* ST77903 Commands */
#define ST77903_NOP       0x00
#define ST77903_SWRESET   0x01
#define ST77903_SLPIN     0x10
#define ST77903_SLPOUT    0x11
#define ST77903_NORON     0x13
#define ST77903_INVOFF    0x20
#define ST77903_INVON     0x21
#define ST77903_DISPOFF   0x28
#define ST77903_DISPON    0x29
#define ST77903_CASET     0x2A
#define ST77903_RASET     0x2B
#define ST77903_RAMWR     0x2C
#define ST77903_MADCTL    0x36
#define ST77903_COLMOD    0x3A

/* MADCTL bits */
#define ST77903_MADCTL_MY  0x80
#define ST77903_MADCTL_MX  0x40
#define ST77903_MADCTL_MV  0x20
#define ST77903_MADCTL_BGR 0x08

/* Color modes */
#define ST77903_COLOR_MODE_16BIT 0x55  /* RGB565 */
#define ST77903_COLOR_MODE_18BIT 0x66  /* RGB666 */
#define ST77903_COLOR_MODE_24BIT 0x77  /* RGB888 */

/* Helper: write command */
static void st77903_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void st77903_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void st77903_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void st77903_hw_reset(egui_hal_lcd_driver_t *self)
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
static int st77903_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    st77903_hw_reset(self);

    /* Software reset */
    st77903_write_cmd(self, ST77903_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* Sleep out */
    st77903_write_cmd(self, ST77903_SLPOUT);
    egui_api_delay(120);  /* Wait 500ms */

    /* Set color mode to 16-bit RGB565 */
    st77903_write_cmd(self, ST77903_COLMOD);
    st77903_write_data_byte(self, ST77903_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    st77903_write_cmd(self, ST77903_MADCTL);
    st77903_write_data_byte(self, ST77903_MADCTL_MX | ST77903_MADCTL_MY);

    /* Inversion control */
    if (config->invert_color) {
        st77903_write_cmd(self, ST77903_INVON);
    } else {
        st77903_write_cmd(self, ST77903_INVOFF);
    }

    /* Normal display mode */
    st77903_write_cmd(self, ST77903_NORON);

    /* Display on */
    st77903_write_cmd(self, ST77903_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void st77903_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    st77903_write_cmd(self, ST77903_DISPOFF);

    /* Sleep in */
    st77903_write_cmd(self, ST77903_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void st77903_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
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
    st77903_write_cmd(self, ST77903_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        st77903_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    st77903_write_cmd(self, ST77903_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        st77903_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    st77903_write_cmd(self, ST77903_RAMWR);
}

/* Driver: write_pixels */
static void st77903_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void st77903_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void st77903_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation) {
    case 0:
        madctl = ST77903_MADCTL_MX | ST77903_MADCTL_MY;
        break;
    case 1:
        madctl = ST77903_MADCTL_MY | ST77903_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = ST77903_MADCTL_MX | ST77903_MADCTL_MV;
        break;
    }

    st77903_write_cmd(self, ST77903_MADCTL);
    st77903_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void st77903_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        st77903_write_cmd(self, ST77903_SLPOUT);
        egui_api_delay(120);
        st77903_write_cmd(self, ST77903_DISPON);
    } else {
        st77903_write_cmd(self, ST77903_DISPOFF);
        st77903_write_cmd(self, ST77903_SLPIN);
    }
}

/* Driver: set_invert */
static void st77903_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    st77903_write_cmd(self, invert ? ST77903_INVON : ST77903_INVOFF);
}

/* Internal: setup driver function pointers */
static void st77903_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST77903";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = st77903_init;
    driver->deinit = st77903_deinit;
    driver->set_window = st77903_set_window;
    driver->write_pixels = st77903_write_pixels;
    driver->wait_dma_complete = st77903_wait_dma_complete;
    driver->set_rotation = st77903_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = st77903_set_power;
    driver->set_invert = st77903_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_st77903_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    st77903_setup_driver(storage, spi, gpio);
}
