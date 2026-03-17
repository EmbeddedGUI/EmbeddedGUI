/**
 * @file egui_lcd_gc9a01.h
 * @brief GC9A01 LCD driver
 *
 * GC9A01 is a 240x240 round TFT LCD controller commonly used in circular displays.
 * Supports SPI interface with 16-bit RGB565 color.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_GC9A01_H_
#define _EGUI_LCD_GC9A01_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GC9A01 driver in user-provided storage.
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
void egui_lcd_gc9a01_init(egui_hal_lcd_driver_t *storage,
                             egui_panel_io_handle_t io,
                             void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_GC9A01_H_ */
