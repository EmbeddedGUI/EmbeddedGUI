/**
 * @file egui_lcd_gc9503.h
 * @brief GC9503 LCD driver
 *
 * GC9503 is an RGB interface LCD controller from GalaxyCore.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The ESP32 Panel reference uses RGB timing output plus a command-side init path,
 * not SPI RAMWR flushes.
 * Current EGUI implementation is therefore command-side only and should not be
 * treated as a production-verified RGB flush driver.
 */

#ifndef _EGUI_LCD_GC9503_H_
#define _EGUI_LCD_GC9503_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GC9503 driver in user-provided storage.
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
void egui_lcd_gc9503_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_GC9503_H_ */
