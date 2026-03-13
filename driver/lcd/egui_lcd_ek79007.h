/**
 * @file egui_lcd_ek79007.h
 * @brief EK79007 LCD driver
 *
 * EK79007 is an RGB interface LCD controller.
 * Current HAL implementation only models the command side used during init.
 * The RGB pixel streaming path still needs dedicated bus support and is not production-verified.
 */
#ifndef _EGUI_LCD_EK79007_H_
#define _EGUI_LCD_EK79007_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize EK79007 driver in user-provided storage.
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
void egui_lcd_ek79007_init(egui_hal_lcd_driver_t *storage,
                           const egui_bus_spi_ops_t *spi,
                           const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_EK79007_H_ */
