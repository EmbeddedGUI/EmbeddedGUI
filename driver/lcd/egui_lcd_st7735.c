/**
 * @file egui_lcd_st7735.c
 * @brief ST7735 LCD driver implementation
 */

#include "egui_lcd_st7735.h"
#include <string.h>
#include "core/egui_api.h"

/* ST7735 Commands */
#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_NORON   0x13
#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

/* Frame rate control commands */
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

/* MADCTL bits */
#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08

/* Color modes */
#define ST7735_COLOR_MODE_16BIT 0x55
#define ST7735_COLOR_MODE_18BIT 0x66

/* Helper: write command */
static void st7735_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void st7735_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void st7735_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void st7735_hw_reset(egui_hal_lcd_driver_t *self)
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
static int st7735_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    st7735_hw_reset(self);

    /* Software reset */
    st7735_write_cmd(self, ST7735_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    st7735_write_cmd(self, ST7735_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Frame rate control - normal mode */
    st7735_write_cmd(self, ST7735_FRMCTR1);
    {
        uint8_t data[] = {0x01, 0x2C, 0x2D};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Frame rate control - idle mode */
    st7735_write_cmd(self, ST7735_FRMCTR2);
    {
        uint8_t data[] = {0x01, 0x2C, 0x2D};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Frame rate control - partial mode */
    st7735_write_cmd(self, ST7735_FRMCTR3);
    {
        uint8_t data[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Inversion control */
    st7735_write_cmd(self, ST7735_INVCTR);
    st7735_write_data_byte(self, 0x03);

    /* Power control sequence aligned with LVGL ST7735 reference */
    st7735_write_cmd(self, ST7735_PWCTR1);
    {
        uint8_t data[] = {0x28, 0x08, 0x04};
        st7735_write_data(self, data, sizeof(data));
    }

    st7735_write_cmd(self, ST7735_PWCTR2);
    st7735_write_data_byte(self, 0xC0);

    st7735_write_cmd(self, ST7735_PWCTR3);
    {
        uint8_t data[] = {0x0D, 0x00};
        st7735_write_data(self, data, sizeof(data));
    }

    st7735_write_cmd(self, ST7735_PWCTR4);
    {
        uint8_t data[] = {0x8D, 0x2A};
        st7735_write_data(self, data, sizeof(data));
    }

    st7735_write_cmd(self, ST7735_PWCTR5);
    {
        uint8_t data[] = {0x8D, 0xEE};
        st7735_write_data(self, data, sizeof(data));
    }

    st7735_write_cmd(self, ST7735_VMCTR1);
    st7735_write_data_byte(self, 0x10);

    st7735_write_cmd(self, ST7735_GMCTRP1);
    {
        uint8_t data[] = {0x04, 0x22, 0x07, 0x0A, 0x2E, 0x30, 0x25, 0x2A, 0x28, 0x26, 0x2E, 0x3A, 0x00, 0x01, 0x03, 0x13};
        st7735_write_data(self, data, sizeof(data));
    }

    st7735_write_cmd(self, ST7735_GMCTRN1);
    {
        uint8_t data[] = {0x04, 0x16, 0x06, 0x0D, 0x2D, 0x26, 0x23, 0x27, 0x27, 0x25, 0x2D, 0x3B, 0x00, 0x01, 0x04, 0x13};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Set color mode to 16-bit RGB565 */
    st7735_write_cmd(self, ST7735_COLMOD);
    st7735_write_data_byte(self, ST7735_COLOR_MODE_16BIT);

    /* Set rotation (default 0) */
    st7735_write_cmd(self, ST7735_MADCTL);
    st7735_write_data_byte(self, ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB);

    /* Inversion control */
    if (config->invert_color)
    {
        st7735_write_cmd(self, ST7735_INVON);
    }
    else
    {
        st7735_write_cmd(self, ST7735_INVOFF);
    }

    /* Normal display mode */
    st7735_write_cmd(self, ST7735_NORON);

    /* Display on */
    st7735_write_cmd(self, ST7735_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void st7735_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    st7735_write_cmd(self, ST7735_DISPOFF);

    /* Sleep in */
    st7735_write_cmd(self, ST7735_SLPIN);

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
static void st7735_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    st7735_write_cmd(self, ST7735_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    st7735_write_cmd(self, ST7735_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        st7735_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    st7735_write_cmd(self, ST7735_RAMWR);
}

/* Driver: write_pixels */
static void st7735_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void st7735_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void st7735_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = ST7735_MADCTL_RGB;

    switch (rotation)
    {
    case 0:
        madctl |= ST7735_MADCTL_MX | ST7735_MADCTL_MY;
        break;
    case 1:
        madctl |= ST7735_MADCTL_MY | ST7735_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl |= ST7735_MADCTL_MX | ST7735_MADCTL_MV;
        break;
    }

    st7735_write_cmd(self, ST7735_MADCTL);
    st7735_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void st7735_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        st7735_write_cmd(self, ST7735_SLPOUT);
        egui_api_delay(120);
        st7735_write_cmd(self, ST7735_DISPON);
    }
    else
    {
        st7735_write_cmd(self, ST7735_DISPOFF);
        st7735_write_cmd(self, ST7735_SLPIN);
    }
}

/* Driver: set_invert */
static void st7735_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    st7735_write_cmd(self, invert ? ST7735_INVON : ST7735_INVOFF);
}

/* Internal: setup driver function pointers */
static void st7735_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ST7735";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = st7735_init;
    driver->deinit = st7735_deinit;
    driver->set_window = st7735_set_window;
    driver->write_pixels = st7735_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? st7735_wait_dma_complete : NULL;
    driver->set_rotation = st7735_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = st7735_set_power;
    driver->set_invert = st7735_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_st7735_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    st7735_setup_driver(storage, spi, gpio);
}
