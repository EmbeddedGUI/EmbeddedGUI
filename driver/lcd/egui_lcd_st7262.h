/**
 * @file egui_lcd_st7262.h
 * @brief ST7262 LCD driver
 *
 * ST7262 is an RGB interface LCD controller supporting RGB565, RGB666, RGB888 color modes.
 * Supports SPI interface for command/data transfer.
 * Like other RGB-interface panels, the real pixel path is timing/video output rather than
 * SPI RAMWR flushes.
 * Current EGUI implementation is command-side only and is not production-verified as a
 * full RGB panel driver.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_ST7262_H_
#define _EGUI_LCD_ST7262_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7262 driver in user-provided storage.
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
void egui_lcd_st7262_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7262_H_ */
