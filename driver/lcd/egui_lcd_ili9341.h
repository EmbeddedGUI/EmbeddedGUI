/**
 * @file egui_lcd_ili9341.h
 * @brief ILI9341 LCD driver
 *
 * ILI9341 is a 240x320 TFT LCD controller commonly used in small displays.
 * Supports SPI interface with 16-bit RGB565 color.
 */

#ifndef _EGUI_LCD_ILI9341_H_
#define _EGUI_LCD_ILI9341_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ILI9341 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_ili9341_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ILI9341_H_ */
