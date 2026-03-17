/**
 * @file egui_lcd_ili9341.c
 * @brief ILI9341 LCD driver implementation
 */

#include "egui_lcd_ili9341.h"
#include <string.h>
#include "core/egui_api.h"

/* ILI9341 Commands */
#define ILI9341_NOP      0x00
#define ILI9341_SWRESET  0x01
#define ILI9341_SLPIN    0x10
#define ILI9341_SLPOUT   0x11
#define ILI9341_NORON    0x13
#define ILI9341_INVOFF   0x20
#define ILI9341_INVON    0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF  0x28
#define ILI9341_DISPON   0x29
#define ILI9341_CASET    0x2A
#define ILI9341_RASET    0x2B
#define ILI9341_RAMWR    0x2C
#define ILI9341_MADCTL   0x36
#define ILI9341_COLMOD   0x3A
#define ILI9341_FRMCTR1  0xB1
#define ILI9341_DFUNCTR  0xB6
#define ILI9341_PWCTR1   0xC0
#define ILI9341_PWCTR2   0xC1
#define ILI9341_VMCTR1   0xC5
#define ILI9341_VMCTR2   0xC7
#define ILI9341_GMCTRP1  0xE0
#define ILI9341_GMCTRN1  0xE1

/* MADCTL bits */
#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04
#define ILI9341_MADCTL_RGB 0x00

/* Color modes */
#define ILI9341_COLOR_MODE_16BIT 0x55
#define ILI9341_COLOR_MODE_18BIT 0x66

/* Helper: write command */
static void ili9341_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void ili9341_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void ili9341_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void ili9341_hw_reset(egui_hal_lcd_driver_t *self)
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
static int ili9341_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    ili9341_hw_reset(self);

    /* Software reset */
    ili9341_write_cmd(self, ILI9341_SWRESET);
    egui_api_delay(120); /* Wait 150ms */

    /* Sleep out */
    ili9341_write_cmd(self, ILI9341_SLPOUT);
    egui_api_delay(120); /* Wait 500ms */

    /* Power control A */
    ili9341_write_cmd(self, 0xCB);
    {
        uint8_t data[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Power control B */
    ili9341_write_cmd(self, 0xCF);
    {
        uint8_t data[] = {0x00, 0xC1, 0x30};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Driver timing control A */
    ili9341_write_cmd(self, 0xE8);
    {
        uint8_t data[] = {0x85, 0x00, 0x78};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Driver timing control B */
    ili9341_write_cmd(self, 0xEA);
    {
        uint8_t data[] = {0x00, 0x00};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Power on sequence control */
    ili9341_write_cmd(self, 0xED);
    {
        uint8_t data[] = {0x64, 0x03, 0x12, 0x81};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Pump ratio control */
    ili9341_write_cmd(self, 0xF7);
    ili9341_write_data_byte(self, 0x20);

    /* Power control 1 */
    ili9341_write_cmd(self, ILI9341_PWCTR1);
    ili9341_write_data_byte(self, 0x23); /* VRH[5:0] */

    /* Power control 2 */
    ili9341_write_cmd(self, ILI9341_PWCTR2);
    ili9341_write_data_byte(self, 0x10); /* SAP[2:0]; BT[3:0] */

    /* VCOM control 1 */
    ili9341_write_cmd(self, ILI9341_VMCTR1);
    {
        uint8_t data[] = {0x3E, 0x28};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* VCOM control 2 */
    ili9341_write_cmd(self, ILI9341_VMCTR2);
    ili9341_write_data_byte(self, 0x86);

    /* Memory access control */
    ili9341_write_cmd(self, ILI9341_MADCTL);
    ili9341_write_data_byte(self, ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);

    /* Set color mode to 16-bit RGB565 */
    ili9341_write_cmd(self, ILI9341_COLMOD);
    ili9341_write_data_byte(self, ILI9341_COLOR_MODE_16BIT);

    /* Frame rate control */
    ili9341_write_cmd(self, ILI9341_FRMCTR1);
    {
        uint8_t data[] = {0x00, 0x18}; /* 79Hz */
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Display function control */
    ili9341_write_cmd(self, ILI9341_DFUNCTR);
    {
        uint8_t data[] = {0x08, 0x82, 0x27};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Gamma function disable */
    ili9341_write_cmd(self, 0xF2);
    ili9341_write_data_byte(self, 0x00);

    /* Gamma curve selected */
    ili9341_write_cmd(self, ILI9341_GAMMASET);
    ili9341_write_data_byte(self, 0x01);

    /* Positive gamma correction */
    ili9341_write_cmd(self, ILI9341_GMCTRP1);
    {
        uint8_t data[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Negative gamma correction */
    ili9341_write_cmd(self, ILI9341_GMCTRN1);
    {
        uint8_t data[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Inversion control */
    if (config->invert_color)
    {
        ili9341_write_cmd(self, ILI9341_INVON);
    }
    else
    {
        ili9341_write_cmd(self, ILI9341_INVOFF);
    }

    /* Normal display mode */
    ili9341_write_cmd(self, ILI9341_NORON);

    /* Display on */
    ili9341_write_cmd(self, ILI9341_DISPON);

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness)
    {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void ili9341_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness)
    {
        self->set_brightness(self, 0);
    }

    /* Display off */
    ili9341_write_cmd(self, ILI9341_DISPOFF);

    /* Sleep in */
    ili9341_write_cmd(self, ILI9341_SLPIN);

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
static void ili9341_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y, int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    ili9341_write_cmd(self, ILI9341_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    ili9341_write_cmd(self, ILI9341_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        ili9341_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    ili9341_write_cmd(self, ILI9341_RAMWR);
}

/* Driver: write_pixels */
static void ili9341_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc)
    {
        self->gpio->set_dc(1); /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void ili9341_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete)
    {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void ili9341_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = ILI9341_MADCTL_BGR;

    switch (rotation)
    {
    case 0:
        madctl |= ILI9341_MADCTL_MX;
        break;
    case 1:
        madctl |= ILI9341_MADCTL_MV;
        break;
    case 2:
        madctl |= ILI9341_MADCTL_MY;
        break;
    case 3:
        madctl |= ILI9341_MADCTL_MX | ILI9341_MADCTL_MY | ILI9341_MADCTL_MV;
        break;
    }

    ili9341_write_cmd(self, ILI9341_MADCTL);
    ili9341_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void ili9341_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on)
    {
        ili9341_write_cmd(self, ILI9341_SLPOUT);
        egui_api_delay(120);
        ili9341_write_cmd(self, ILI9341_DISPON);
    }
    else
    {
        ili9341_write_cmd(self, ILI9341_DISPOFF);
        ili9341_write_cmd(self, ILI9341_SLPIN);
    }
}

/* Driver: set_invert */
static void ili9341_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    ili9341_write_cmd(self, invert ? ILI9341_INVON : ILI9341_INVOFF);
}

/* Internal: setup driver function pointers */
static void ili9341_setup_driver(egui_hal_lcd_driver_t *driver, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "ILI9341";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = ili9341_init;
    driver->deinit = ili9341_deinit;
    driver->set_window = ili9341_set_window;
    driver->write_pixels = ili9341_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? ili9341_wait_dma_complete : NULL;
    driver->set_rotation = ili9341_set_rotation;
    driver->set_brightness = NULL; /* Porting layer should set this */
    driver->set_power = ili9341_set_power;
    driver->set_invert = ili9341_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_ili9341_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write)
    {
        return;
    }

    ili9341_setup_driver(storage, spi, gpio);
}
