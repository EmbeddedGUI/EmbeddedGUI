/**
 * @file egui_lcd_ssd1306.h
 * @brief SSD1306 OLED driver
 *
 * SSD1306 is a 128x64 monochrome OLED controller.
 * Supports both I2C and SPI interfaces with 1-bit color depth.
 */

#ifndef _EGUI_LCD_SSD1306_H_
#define _EGUI_LCD_SSD1306_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SSD1306 driver via I2C in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if reset pin not used)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_ssd1306_init_i2c(egui_hal_lcd_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_lcd_gpio_ops_t *gpio);

/**
 * Initialize SSD1306 driver via SPI in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param spi      SPI bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if all pins hardware-controlled)
 *
 * Use this in environments without malloc.
 */
void egui_lcd_ssd1306_init_spi(egui_hal_lcd_driver_t *storage, const egui_bus_spi_ops_t *spi, const egui_lcd_gpio_ops_t *gpio);

/**
 * Set display contrast.
 *
 * @param self      Driver instance
 * @param contrast  Contrast value (0-255, default 128)
 */
void egui_lcd_ssd1306_set_contrast(egui_hal_lcd_driver_t *self, uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_SSD1306_H_ */
