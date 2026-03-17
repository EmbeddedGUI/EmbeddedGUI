/**
 * @file egui_lcd_ili9881c.h
 * @brief ILI9881C LCD driver
 *
 * ILI9881C is a MIPI DSI interface LCD controller from ILI Technology.
 * Current HAL implementation only models the command side for initialization/debug bring-up.
 * The real DSI DPI video path still needs dedicated bus support and is not production-verified.
 */
#ifndef _EGUI_LCD_ILI9881C_H_
#define _EGUI_LCD_ILI9881C_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ILI9881C driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_ili9881c_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ILI9881C_H_ */
