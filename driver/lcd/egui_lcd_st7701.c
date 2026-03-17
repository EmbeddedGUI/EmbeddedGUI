/**
 * @file egui_lcd_st7701.c
 * @brief ST7701 LCD driver implementation
 */

#include "egui_lcd_st7701.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7701 Commands */
#define ST7701_NOP     0x00
#define ST7701_SWRESET 0x01
#define ST7701_SLPIN   0x10
#define ST7701_SLPOUT  0x11
#define ST7701_INVOFF  0x20
#define ST7701_INVON   0x21
#define ST7701_DISPOFF 0x28
#define ST7701_DISPON  0x29
#define ST7701_CASET   0x2A
#define ST7701_RASET   0x2B
#define ST7701_RAMWR   0x2C
#define ST7701_MADCTL  0x36
#define ST7701_COLMOD  0x3A

/* Vendor-specific command page select */
#define ST7701_CMD_PAGE 0xFF

/* MADCTL bits */
#define ST7701_MADCTL_MY  0x80
#define ST7701_MADCTL_MX  0x40
#define ST7701_MADCTL_MV  0x20
#define ST7701_MADCTL_BGR 0x08

/* Color modes */
#define ST7701_COLOR_MODE_16BIT 0x55 /* RGB565 */
#define ST7701_COLOR_MODE_18BIT 0x66 /* RGB666 */
#define ST7701_COLOR_MODE_24BIT 0x77 /* RGB888 */

/* Helper: write command */
static void st7701_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void st7701_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void st7701_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void st7701_hw_reset(egui_hal_lcd_driver_t *self)
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

/* Helper: select command page */
static void st7701_select_page(egui_hal_lcd_driver_t *self, uint8_t page1, uint8_t page2, uint8_t page3)
{
    st7701_write_cmd(self, ST7701_CMD_PAGE);
    uint8_t data[] = {page1, page2, page3};
    st7701_write_data(self, data, sizeof(data));
}

/* Driver: init */
static int st7701_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    st7701_hw_reset(self);

    /* Software reset */
    st7701_write_cmd(self, ST7701_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    st7701_write_cmd(self, ST7701_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Select command page 1 (standard commands) */
    st7701_select_page(self, 0x77, 0x01, 0x00);
    st7701_write_data_byte(self, 0x00);
    st7701_write_data_byte(self, 0x10);

    /* Set color mode to 16-bit RGB565 */
    st7701_write_cmd(self, ST7701_COLMOD);
    st7701_write_data_byte(self, ST7701_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    st7701_write_cmd(self, ST7701_MADCTL);
    st7701_write_data_byte(self, ST7701_MADCTL_MX | ST7701_MADCTL_MY);

    /* Color inversion */
    if (config->invert_color)
    {
        st7701_write_cmd(self, ST7701_INVON);
    }
    else
    {
        st7701_write_cmd(self, ST7701_INVOFF);
    }

    /* Display on */
    st7701_write_cmd(self, ST7701_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void st7701_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    st7701_write_cmd(self, ST7701_DISPOFF);

    /* Sleep in */
    st7701_write_cmd(self, ST7701_SLPIN);

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
static void st7701_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
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
    st7701_write_cmd(self, ST7701_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        st7701_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    st7701_write_cmd(self, ST7701_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        st7701_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    st7701_write_cmd(self, ST7701_RAMWR);
}

/* Driver: write_pixels */
static void st7701_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void st7701_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void st7701_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation)
    {
    case 0:
        madctl = ST7701_MADCTL_MX | ST7701_MADCTL_MY;
        break;
    case 1:
        madctl = ST7701_MADCTL_MY | ST7701_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl = ST7701_MADCTL_MX | ST7701_MADCTL_MV;
        break;
    }

    st7701_write_cmd(self, ST7701_MADCTL);
    st7701_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void st7701_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        st7701_write_cmd(self, ST7701_SLPOUT);
        egui_api_delay(120);
        st7701_write_cmd(self, ST7701_DISPON);
    }
    else
    {
        st7701_write_cmd(self, ST7701_DISPOFF);
        st7701_write_cmd(self, ST7701_SLPIN);
    }
}

/* Driver: set_invert */
static void st7701_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    st7701_write_cmd(self, invert ? ST7701_INVON : ST7701_INVOFF);
}

/* Internal: setup driver function pointers */
static void st7701_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7701";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = st7701_init;
    driver->deinit = st7701_deinit;
    driver->set_window = st7701_set_window;
    driver->write_pixels = st7701_write_pixels;
    driver->wait_dma_complete = st7701_wait_dma_complete;
    driver->set_rotation = st7701_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = st7701_set_power;
    driver->set_invert = st7701_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_st7701_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    st7701_setup_driver(storage, spi, gpio);
}
