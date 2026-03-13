/**
 * @file egui_lcd_spd2010.h
 * @brief SPD2010 LCD driver
 *
 * SPD2010 is a SPI interface LCD controller.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The ESP32 Panel reference marks swap_xy as unsupported for this panel.
 * Current EGUI port therefore leaves set_rotation unset to avoid exposing
 * fake 90/270-degree rotation support.
 */

#ifndef _EGUI_LCD_SPD2010_H_
#define _EGUI_LCD_SPD2010_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SPD2010 driver in user-provided storage.
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
void egui_lcd_spd2010_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_SPD2010_H_ */
