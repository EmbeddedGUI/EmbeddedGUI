/**
 * @file egui_lcd_gc9b71.h
 * @brief GC9B71 LCD driver
 *
 * GC9B71 is a 320x386 TFT LCD controller.
 * Supports SPI interface with 16-bit RGB565 color.
 */

#ifndef _EGUI_LCD_GC9B71_H_
#define _EGUI_LCD_GC9B71_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GC9B71 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_gc9b71_init(egui_hal_lcd_driver_t *storage,
                          const egui_bus_spi_ops_t *spi,
                          const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_GC9B71_H_ */
