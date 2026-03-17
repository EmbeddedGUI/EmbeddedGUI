/**
 * @file egui_lcd_gc9a01.h
 * @brief GC9A01 LCD driver
 *
 * GC9A01 is a 240x240 round TFT LCD controller commonly used in circular displays.
 * Supports SPI interface with 16-bit RGB565 color.
 */

#ifndef _EGUI_LCD_GC9A01_H_
#define _EGUI_LCD_GC9A01_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GC9A01 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 *
 * Note: set_brightness is NULL by default. Porting layer should set
 * driver->set_brightness after init if backlight control is needed.
 */
void egui_lcd_gc9a01_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_GC9A01_H_ */
