/**
 * @file egui_lcd_st7735.h
 * @brief ST7735 LCD driver
 *
 * ST7735 is a 128x160 TFT LCD controller commonly used in small displays.
 * Supports SPI interface with 16-bit RGB565 color.
 */

#ifndef _EGUI_LCD_ST7735_H_
#define _EGUI_LCD_ST7735_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7735 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_st7735_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7735_H_ */
