/**
 * @file egui_lcd_ek9716b.h
 * @brief EK9716B LCD driver
 *
 * EK9716B is an RGB interface LCD controller.
 * Supports RGB565, RGB666, RGB888 color modes.
 * Like other RGB-interface panels, the real pixel path is timing/video output rather than
 * SPI RAMWR flushes.
 * Current EGUI implementation is command-side only and is not production-verified as a
 * full RGB panel driver.
 */

#ifndef _EGUI_LCD_EK9716B_H_
#define _EGUI_LCD_EK9716B_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize EK9716B driver in user-provided storage.
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
void egui_lcd_ek9716b_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_EK9716B_H_ */
