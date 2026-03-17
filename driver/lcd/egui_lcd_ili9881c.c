/**
 * @file egui_lcd_ili9881c.c
 * @brief ILI9881C LCD driver implementation
 */

#include "egui_lcd_ili9881c.h"
#include <string.h>
#include "core/egui_api.h"

/* ILI9881C Commands */
#define ILI9881C_NOP     0x00
#define ILI9881C_SWRESET 0x01
#define ILI9881C_SLPIN   0x10
#define ILI9881C_SLPOUT  0x11
#define ILI9881C_INVOFF  0x20
#define ILI9881C_INVON   0x21
#define ILI9881C_DISPOFF 0x28
#define ILI9881C_DISPON  0x29
#define ILI9881C_CASET   0x2A
#define ILI9881C_RASET   0x2B
#define ILI9881C_RAMWR   0x2C
#define ILI9881C_MADCTL  0x36
#define ILI9881C_COLMOD  0x3A

/* MADCTL bits */
#define ILI9881C_MADCTL_MY  0x80
#define ILI9881C_MADCTL_MX  0x40
#define ILI9881C_MADCTL_MV  0x20
#define ILI9881C_MADCTL_BGR 0x08

/* Color modes */
#define ILI9881C_COLOR_MODE_16BIT 0x55 /* RGB565 */
#define ILI9881C_COLOR_MODE_18BIT 0x66 /* RGB666 */
#define ILI9881C_COLOR_MODE_24BIT 0x77 /* RGB888 */

/* Helper: write command */
static void ili9881c_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void ili9881c_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void ili9881c_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void ili9881c_hw_reset(egui_hal_lcd_driver_t *self)
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
static int ili9881c_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    ili9881c_hw_reset(self);

    /* Software reset */
    ili9881c_write_cmd(self, ILI9881C_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    ili9881c_write_cmd(self, ILI9881C_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Memory access control */
    ili9881c_write_cmd(self, ILI9881C_MADCTL);
    ili9881c_write_data_byte(self, ILI9881C_MADCTL_MX | ILI9881C_MADCTL_BGR);

    /* Set color mode based on config */
    ili9881c_write_cmd(self, ILI9881C_COLMOD);
    if (config->color_depth == 24)
    {
        ili9881c_write_data_byte(self, ILI9881C_COLOR_MODE_24BIT);
    }
    else if (config->color_depth == 18)
    {
        ili9881c_write_data_byte(self, ILI9881C_COLOR_MODE_18BIT);
    }
    else
    {
        ili9881c_write_data_byte(self, ILI9881C_COLOR_MODE_16BIT);
    }

    /* Inversion control */
    if (config->invert_color)
    {
        ili9881c_write_cmd(self, ILI9881C_INVON);
    }
    else
    {
        ili9881c_write_cmd(self, ILI9881C_INVOFF);
    }

    /* Display on */
    ili9881c_write_cmd(self, ILI9881C_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void ili9881c_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    ili9881c_write_cmd(self, ILI9881C_DISPOFF);

    /* Sleep in */
    ili9881c_write_cmd(self, ILI9881C_SLPIN);

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
static void ili9881c_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
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
    ili9881c_write_cmd(self, ILI9881C_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        ili9881c_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    ili9881c_write_cmd(self, ILI9881C_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        ili9881c_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    ili9881c_write_cmd(self, ILI9881C_RAMWR);
}

/* Driver: write_pixels */
static void ili9881c_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{

    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void ili9881c_wait_dma_complete(egui_hal_lcd_driver_t *self)
{

    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void ili9881c_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = ILI9881C_MADCTL_BGR;

    switch (rotation)
    {
    case 0:
        madctl |= ILI9881C_MADCTL_MX;
        break;
    case 1:
        madctl |= ILI9881C_MADCTL_MV;
        break;
    case 2:
        madctl |= ILI9881C_MADCTL_MY;
        break;
    case 3:
        madctl |= ILI9881C_MADCTL_MX | ILI9881C_MADCTL_MY | ILI9881C_MADCTL_MV;
        break;
    }

    ili9881c_write_cmd(self, ILI9881C_MADCTL);
    ili9881c_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void ili9881c_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        ili9881c_write_cmd(self, ILI9881C_SLPOUT);
        egui_api_delay(120);
        ili9881c_write_cmd(self, ILI9881C_DISPON);
    }
    else
    {
        ili9881c_write_cmd(self, ILI9881C_DISPOFF);
        ili9881c_write_cmd(self, ILI9881C_SLPIN);
    }
}

/* Driver: set_invert */
static void ili9881c_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    ili9881c_write_cmd(self, invert ? ILI9881C_INVON : ILI9881C_INVOFF);
}

/* Internal: setup driver function pointers */
static void ili9881c_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ILI9881C";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = ili9881c_init;
    driver->deinit = ili9881c_deinit;
    driver->set_window = ili9881c_set_window;
    driver->write_pixels = ili9881c_write_pixels;
    driver->wait_dma_complete = ili9881c_wait_dma_complete;
    driver->set_rotation = ili9881c_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = ili9881c_set_power;
    driver->set_invert = ili9881c_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_ili9881c_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    ili9881c_setup_driver(storage, spi, gpio);
}
