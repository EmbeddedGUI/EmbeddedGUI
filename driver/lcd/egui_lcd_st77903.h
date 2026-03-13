/**
 * @file egui_lcd_st77903.h
 * @brief ST77903 LCD driver
 *
 * ST77903 is an RGB interface LCD controller supporting RGB565, RGB666, RGB888 color modes.
 * Supports SPI interface for command/data transfer.
 * The real pixel path is RGB timing output rather than SPI RAMWR flushes.
 * Current EGUI implementation should therefore be treated as command-side only,
 * not a production-verified full RGB panel driver.
 */

#ifndef _EGUI_LCD_ST77903_H_
#define _EGUI_LCD_ST77903_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST77903 driver in user-provided storage.
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
void egui_lcd_st77903_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST77903_H_ */
