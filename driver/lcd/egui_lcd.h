/**
 * @file egui_lcd.h
 * @brief LCD Driver common interface
 *
 * This file defines the common interface for all LCD drivers.
 * Each LCD controller (ST7789, ILI9341, etc.) implements this interface.
 *
 * Note: Uses egui_hal_lcd_* prefix to avoid conflict with Core layer types.
 */

#ifndef _EGUI_HAL_LCD_H_
#define _EGUI_HAL_LCD_H_

#include <stdint.h>
#include "egui_bus.h"
#include "egui_bus_spi.h"
#include "egui_bus_i2c.h"
#include "egui_bus_8080.h"
#include "egui_bus_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * LCD configuration structure
 */
typedef struct egui_hal_lcd_config {
    uint16_t width;           /**< Screen width in pixels */
    uint16_t height;          /**< Screen height in pixels */
    uint8_t color_depth;      /**< Color depth: 1, 8, 16, or 24 bits */
    uint8_t color_swap;       /**< Byte order swap (for RGB565) */
    int16_t x_offset;         /**< X offset for partial displays */
    int16_t y_offset;         /**< Y offset for partial displays */
    uint8_t invert_color;     /**< Color inversion (0 = normal, 1 = inverted) */
    uint8_t mirror_x;         /**< X axis mirror */
    uint8_t mirror_y;         /**< Y axis mirror */
} egui_hal_lcd_config_t;

/**
 * LCD driver instance (forward declaration)
 */
typedef struct egui_hal_lcd_driver egui_hal_lcd_driver_t;

/**
 * LCD driver structure
 *
 * Uses union for bus pointers to save memory - only one bus type is used per driver.
 */
struct egui_hal_lcd_driver {
    const char *name;                    /**< Driver name (e.g., "ST7789") */
    egui_bus_type_t bus_type;           /**< Bus type used by this driver */

    /* Lifecycle */
    int (*init)(egui_hal_lcd_driver_t *self, const egui_hal_lcd_config_t *config);
    void (*deinit)(egui_hal_lcd_driver_t *self);

    /* Core drawing */
    void (*set_window)(egui_hal_lcd_driver_t *self, int16_t x, int16_t y,
                       int16_t w, int16_t h);
    void (*write_pixels)(egui_hal_lcd_driver_t *self, const void *data, uint32_t len);
    void (*wait_dma_complete)(egui_hal_lcd_driver_t *self);  /**< NULL = sync mode */

    /* Optional features - porting layer can override these */
    void (*set_rotation)(egui_hal_lcd_driver_t *self, uint8_t rotation);
    void (*set_brightness)(egui_hal_lcd_driver_t *self, uint8_t level);
    void (*set_power)(egui_hal_lcd_driver_t *self, uint8_t on);
    void (*set_invert)(egui_hal_lcd_driver_t *self, uint8_t invert);

    struct {
        int16_t x;
        int16_t y;
        int16_t w;
        int16_t h;
    } window;

    /* Internal data */
    void *priv;                          /**< Driver-specific private data */
    union {
        const egui_bus_spi_ops_t *spi;
        const egui_bus_i2c_ops_t *i2c;
        const egui_bus_8080_ops_t *bus_8080;
    } bus;
    const egui_lcd_gpio_ops_t *gpio;    /**< LCD GPIO control (RST, DC, CS) */
    egui_hal_lcd_config_t config;       /**< Current configuration */
};

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_LCD_H_ */
