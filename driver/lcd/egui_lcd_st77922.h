/**
 * @file egui_lcd_st77922.h
 * @brief ST77922 LCD driver
 *
 * ST77922 is a SPI/RGB interface LCD controller.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The upstream driver supports multiple interface variants (general SPI, RGB, MIPI/QSPI depending on SoC).
 * Current EGUI implementation only models the generic SPI-style command/RAMWR path,
 * so RGB/MIPI-specific paths are not production-verified here.
 */

#ifndef _EGUI_LCD_ST77922_H_
#define _EGUI_LCD_ST77922_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST77922 driver in user-provided storage.
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
void egui_lcd_st77922_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST77922_H_ */
