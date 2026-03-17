/**
 * @file egui_lcd_st7703.h
 * @brief ST7703 LCD driver
 *
 * ST7703 is an RGB interface LCD controller similar to ST7701.
 * Current HAL implementation only models the SPI command phase used during init.
 * The RGB pixel streaming path still needs dedicated bus support and is not production-verified.
 */
#ifndef _EGUI_LCD_ST7703_H_
#define _EGUI_LCD_ST7703_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7703 driver in user-provided storage.
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
void egui_lcd_st7703_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7703_H_ */
