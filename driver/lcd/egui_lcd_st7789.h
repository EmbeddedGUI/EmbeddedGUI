/**
 * @file egui_lcd_st7789.h
 * @brief ST7789 LCD driver
 *
 * ST7789 is a 240x320 TFT LCD controller commonly used in small displays.
 * Supports SPI interface with 16-bit RGB565 color.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_ST7789_H_
#define _EGUI_LCD_ST7789_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7789 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for bus communication (must not be NULL)
 * @param set_rst  RST pin control function (may be NULL if not available)
 *
 * Use this in environments without malloc.
 *
 * Note: Brightness/backlight control is handled externally by the
 * porting layer or bridge layer.
 */
void egui_lcd_st7789_init(egui_hal_lcd_driver_t *storage,
                           egui_panel_io_handle_t io,
                           void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7789_H_ */
