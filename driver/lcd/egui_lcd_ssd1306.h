/**
 * @file egui_lcd_ssd1306.h
 * @brief SSD1306 OLED driver
 *
 * SSD1306 is a 128x64 monochrome OLED controller.
 * Supports both I2C and SPI interfaces with 1-bit color depth.
 *
 * Uses unified Panel IO interface. Caller decides bus type by passing
 * either egui_panel_io_spi_t.base or egui_panel_io_i2c_t.base as io.
 */

#ifndef _EGUI_LCD_SSD1306_H_
#define _EGUI_LCD_SSD1306_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SSD1306 driver via SPI Panel IO.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Unified Panel IO handle (must not be NULL)
 * @param set_rst  RST pin control function (NULL if not available)
 */
void egui_lcd_ssd1306_init_spi(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

/**
 * Initialize SSD1306 driver via I2C Panel IO.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Unified Panel IO handle (must not be NULL)
 * @param set_rst  RST pin control function (NULL if not available)
 */
void egui_lcd_ssd1306_init_i2c(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

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
