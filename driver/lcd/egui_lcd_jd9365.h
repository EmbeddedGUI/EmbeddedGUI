/**
 * @file egui_lcd_jd9365.h
 * @brief JD9365 LCD driver
 *
 * JD9365 is a MIPI DSI interface LCD controller from Jadard.
 * Current HAL implementation only models the command side for initialization/debug bring-up.
 * The real DSI video path still needs dedicated bus support and is not production-verified.
 */
#ifndef _EGUI_LCD_JD9365_H_
#define _EGUI_LCD_JD9365_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize JD9365 driver in user-provided storage.
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
void egui_lcd_jd9365_init(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_JD9365_H_ */
