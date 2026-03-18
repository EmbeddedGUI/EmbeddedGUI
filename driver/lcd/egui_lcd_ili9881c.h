/**
 * @file egui_lcd_ili9881c.h
 * @brief ILI9881C LCD driver
 *
 * ILI9881C is a MIPI DSI interface LCD controller from ILI Technology.
 * Current HAL implementation only models the command side for initialization/debug bring-up.
 * The real DSI DPI video path still needs dedicated bus support and is not production-verified.
 *
 * Uses unified Panel IO interface for bus communication.
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
 * @param io       Panel IO handle for bus communication (must not be NULL)
 * @param set_rst  RST pin control function (may be NULL if not available)
 *
 * Use this in environments without malloc.
 *
 * Note: Brightness/backlight control is handled externally by the
 * porting layer or bridge layer.
 */
void egui_lcd_ili9881c_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ILI9881C_H_ */
