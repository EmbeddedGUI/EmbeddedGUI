/**
 * @file egui_lcd_st7262.h
 * @brief ST7262 LCD driver
 *
 * ST7262 is an RGB interface LCD controller supporting RGB565, RGB666, RGB888 color modes.
 * Supports SPI interface for command/data transfer.
 * Like other RGB-interface panels, the real pixel path is timing/video output rather than
 * SPI RAMWR flushes.
 * Current EGUI implementation is command-side only and is not production-verified as a
 * full RGB panel driver.
 */

#ifndef _EGUI_LCD_ST7262_H_
#define _EGUI_LCD_ST7262_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7262 driver in user-provided storage.
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
void egui_lcd_st7262_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7262_H_ */
