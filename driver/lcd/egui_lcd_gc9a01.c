/**
 * @file egui_lcd_gc9a01.c
 * @brief GC9A01 LCD driver implementation
 */

#include "egui_lcd_gc9a01.h"
#include <string.h>
#include "core/egui_api.h"

/* GC9A01 Commands */
#define GC9A01_SWRESET   0x01
#define GC9A01_SLPIN     0x10
#define GC9A01_SLPOUT    0x11
#define GC9A01_INVOFF    0x20
#define GC9A01_INVON     0x21
#define GC9A01_DISPOFF   0x28
#define GC9A01_DISPON    0x29
#define GC9A01_CASET     0x2A
#define GC9A01_RASET     0x2B
#define GC9A01_RAMWR     0x2C
#define GC9A01_MADCTL    0x36
#define GC9A01_COLMOD    0x3A

/* MADCTL bits */
#define GC9A01_MADCTL_MY  0x80
#define GC9A01_MADCTL_MX  0x40
#define GC9A01_MADCTL_MV  0x20
#define GC9A01_MADCTL_ML  0x10
#define GC9A01_MADCTL_BGR 0x08

/* Color modes */
#define GC9A01_COLOR_MODE_16BIT 0x55

/* Helper: write command */
static void gc9a01_write_cmd(egui_hal_lcd_driver_t *self, uint8_t cmd)
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
static void gc9a01_write_data_byte(egui_hal_lcd_driver_t *self, uint8_t data)
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
static void gc9a01_write_data(egui_hal_lcd_driver_t *self, const uint8_t *data, uint32_t len)
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
static void gc9a01_hw_reset(egui_hal_lcd_driver_t *self)
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
static int gc9a01_init(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config)
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
    gc9a01_hw_reset(self);

    /* Software reset */
    gc9a01_write_cmd(self, GC9A01_SWRESET);
    egui_api_delay(120);  /* Wait 150ms */

    /* GC9A01 specific initialization sequence */
    gc9a01_write_cmd(self, 0xEF);

    gc9a01_write_cmd(self, 0xEB);
    gc9a01_write_data_byte(self, 0x14);

    gc9a01_write_cmd(self, 0xFE);
    gc9a01_write_cmd(self, 0xEF);

    gc9a01_write_cmd(self, 0xEB);
    gc9a01_write_data_byte(self, 0x14);

    gc9a01_write_cmd(self, 0x84);
    gc9a01_write_data_byte(self, 0x40);

    gc9a01_write_cmd(self, 0x85);
    gc9a01_write_data_byte(self, 0xFF);

    gc9a01_write_cmd(self, 0x86);
    gc9a01_write_data_byte(self, 0xFF);

    gc9a01_write_cmd(self, 0x87);
    gc9a01_write_data_byte(self, 0xFF);

    gc9a01_write_cmd(self, 0x88);
    gc9a01_write_data_byte(self, 0x0A);

    gc9a01_write_cmd(self, 0x89);
    gc9a01_write_data_byte(self, 0x21);

    gc9a01_write_cmd(self, 0x8A);
    gc9a01_write_data_byte(self, 0x00);

    gc9a01_write_cmd(self, 0x8B);
    gc9a01_write_data_byte(self, 0x80);

    gc9a01_write_cmd(self, 0x8C);
    gc9a01_write_data_byte(self, 0x01);

    gc9a01_write_cmd(self, 0x8D);
    gc9a01_write_data_byte(self, 0x01);

    gc9a01_write_cmd(self, 0x8E);
    gc9a01_write_data_byte(self, 0xFF);

    gc9a01_write_cmd(self, 0x8F);
    gc9a01_write_data_byte(self, 0xFF);

    gc9a01_write_cmd(self, 0xB6);
    {
        uint8_t data[] = {0x00, 0x00};
        gc9a01_write_data(self, data, sizeof(data));
    }

    /* Set color mode to 16-bit RGB565 */
    gc9a01_write_cmd(self, GC9A01_COLMOD);
    gc9a01_write_data_byte(self, GC9A01_COLOR_MODE_16BIT);

    gc9a01_write_cmd(self, 0x90);
    {
        uint8_t data[] = {0x08, 0x08, 0x08, 0x08};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xBD);
    gc9a01_write_data_byte(self, 0x06);

    gc9a01_write_cmd(self, 0xBC);
    gc9a01_write_data_byte(self, 0x00);

    gc9a01_write_cmd(self, 0xFF);
    {
        uint8_t data[] = {0x60, 0x01, 0x04};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xC3);
    gc9a01_write_data_byte(self, 0x13);

    gc9a01_write_cmd(self, 0xC4);
    gc9a01_write_data_byte(self, 0x13);

    gc9a01_write_cmd(self, 0xC9);
    gc9a01_write_data_byte(self, 0x22);

    gc9a01_write_cmd(self, 0xBE);
    gc9a01_write_data_byte(self, 0x11);

    gc9a01_write_cmd(self, 0xE1);
    {
        uint8_t data[] = {0x10, 0x0E};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xDF);
    {
        uint8_t data[] = {0x21, 0x0C, 0x02};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xF0);
    {
        uint8_t data[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xF1);
    {
        uint8_t data[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xF2);
    {
        uint8_t data[] = {0x45, 0x09, 0x08, 0x08, 0x26, 0x2A};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xF3);
    {
        uint8_t data[] = {0x43, 0x70, 0x72, 0x36, 0x37, 0x6F};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xED);
    {
        uint8_t data[] = {0x1B, 0x0B};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xAE);
    gc9a01_write_data_byte(self, 0x77);

    gc9a01_write_cmd(self, 0xCD);
    gc9a01_write_data_byte(self, 0x63);

    gc9a01_write_cmd(self, 0x70);
    {
        uint8_t data[] = {0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0xE8);
    gc9a01_write_data_byte(self, 0x34);

    gc9a01_write_cmd(self, 0x62);
    {
        uint8_t data[] = {0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x63);
    {
        uint8_t data[] = {0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x64);
    {
        uint8_t data[] = {0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x66);
    {
        uint8_t data[] = {0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x67);
    {
        uint8_t data[] = {0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x74);
    {
        uint8_t data[] = {0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00};
        gc9a01_write_data(self, data, sizeof(data));
    }

    gc9a01_write_cmd(self, 0x98);
    gc9a01_write_data_byte(self, 0x3E);

    gc9a01_write_cmd(self, 0x99);
    gc9a01_write_data_byte(self, 0x3E);

    /* Set rotation (default 0) */
    gc9a01_write_cmd(self, GC9A01_MADCTL);
    gc9a01_write_data_byte(self, GC9A01_MADCTL_MX | GC9A01_MADCTL_MY | GC9A01_MADCTL_BGR);

    /* Inversion control */
    if (config->invert_color) {
        gc9a01_write_cmd(self, GC9A01_INVON);
    } else {
        gc9a01_write_cmd(self, GC9A01_INVOFF);
    }

    /* Sleep out */
    gc9a01_write_cmd(self, GC9A01_SLPOUT);
    egui_api_delay(120);  /* Wait 120ms */

    /* Display on */
    gc9a01_write_cmd(self, GC9A01_DISPON);
    egui_api_delay(10);  /* Wait 20ms */

    /* Backlight on - porting layer should set driver->set_brightness */
    if (self->set_brightness) {
        self->set_brightness(self, 255);
    }

    return 0;
}

/* Driver: deinit */
static void gc9a01_deinit(egui_hal_lcd_driver_t *self)
{
    /* Backlight off */
    if (self->set_brightness) {
        self->set_brightness(self, 0);
    }

    /* Display off */
    gc9a01_write_cmd(self, GC9A01_DISPOFF);

    /* Sleep in */
    gc9a01_write_cmd(self, GC9A01_SLPIN);

    /* Deinit bus and GPIO */
    if (self->bus.spi->deinit) {
        self->bus.spi->deinit();
    }
    if (self->gpio && self->gpio->deinit) {
        self->gpio->deinit();
    }
}

/* Driver: set_window */
static void gc9a01_set_window(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                               int16_t w, int16_t h)
{
    uint16_t x0 = x + self->config.x_offset;
    uint16_t y0 = y + self->config.y_offset;
    uint16_t x1 = x0 + w - 1;
    uint16_t y1 = y0 + h - 1;

    /* Column address set */
    gc9a01_write_cmd(self, GC9A01_CASET);
    {
        uint8_t data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
        gc9a01_write_data(self, data, sizeof(data));
    }

    /* Row address set */
    gc9a01_write_cmd(self, GC9A01_RASET);
    {
        uint8_t data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
        gc9a01_write_data(self, data, sizeof(data));
    }

    /* Write to RAM */
    gc9a01_write_cmd(self, GC9A01_RAMWR);
}

/* Driver: write_pixels */
static void gc9a01_write_pixels(egui_hal_lcd_driver_t *self, const void *data, uint32_t len)
{
    if (self->gpio && self->gpio->set_dc) {
        self->gpio->set_dc(1);  /* Data mode */
    }
    self->bus.spi->write((const uint8_t *)data, len);
    /* Note: don't wait here - let caller decide via wait_dma_complete */
}

/* Driver: wait_dma_complete */
static void gc9a01_wait_dma_complete(egui_hal_lcd_driver_t *self)
{
    if (self->bus.spi->wait_complete) {
        self->bus.spi->wait_complete();
    }
}

/* Driver: set_rotation */
static void gc9a01_set_rotation(egui_hal_lcd_driver_t *self, uint8_t rotation)
{
    uint8_t madctl = GC9A01_MADCTL_BGR;

    switch (rotation) {
    case 0:
        madctl |= GC9A01_MADCTL_MX | GC9A01_MADCTL_MY;
        break;
    case 1:
        madctl |= GC9A01_MADCTL_MY | GC9A01_MADCTL_MV;
        break;
    case 2:
        /* No additional flags */
        break;
    case 3:
        madctl |= GC9A01_MADCTL_MX | GC9A01_MADCTL_MV;
        break;
    }

    gc9a01_write_cmd(self, GC9A01_MADCTL);
    gc9a01_write_data_byte(self, madctl);
}

/* Driver: set_power */
static void gc9a01_set_power(egui_hal_lcd_driver_t *self, uint8_t on)
{
    if (on) {
        gc9a01_write_cmd(self, GC9A01_SLPOUT);
        egui_api_delay(120);
        gc9a01_write_cmd(self, GC9A01_DISPON);
    } else {
        gc9a01_write_cmd(self, GC9A01_DISPOFF);
        gc9a01_write_cmd(self, GC9A01_SLPIN);
    }
}

/* Driver: set_invert */
static void gc9a01_set_invert(egui_hal_lcd_driver_t *self, uint8_t invert)
{
    gc9a01_write_cmd(self, invert ? GC9A01_INVON : GC9A01_INVOFF);
}

/* Internal: setup driver function pointers */
static void gc9a01_setup_driver(egui_hal_lcd_driver_t *driver,
                                 const egui_bus_spi_ops_t *spi,
                                 const egui_lcd_gpio_ops_t *gpio)
{
    memset(driver, 0, sizeof(egui_hal_lcd_driver_t));

    driver->name = "GC9A01";
    driver->bus_type = EGUI_BUS_TYPE_SPI;

    driver->init = gc9a01_init;
    driver->deinit = gc9a01_deinit;
    driver->set_window = gc9a01_set_window;
    driver->write_pixels = gc9a01_write_pixels;
    driver->wait_dma_complete = spi->wait_complete ? gc9a01_wait_dma_complete : NULL;
    driver->set_rotation = gc9a01_set_rotation;
    driver->set_brightness = NULL;  /* Porting layer should set this */
    driver->set_power = gc9a01_set_power;
    driver->set_invert = gc9a01_set_invert;

    driver->bus.spi = spi;
    driver->gpio = gpio;
}

/* Public: init */
void egui_lcd_gc9a01_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio)
{
    if (!storage || !spi || !spi->write) {
        return;
    }

    gc9a01_setup_driver(storage, spi, gpio);
}
