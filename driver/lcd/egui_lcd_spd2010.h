/**
 * @file egui_lcd_spd2010.h
 * @brief SPD2010 LCD driver
 *
 * SPD2010 is a SPI interface LCD controller.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The ESP32 Panel reference marks swap_xy as unsupported for this panel.
 * Current EGUI port therefore leaves set_rotation unset to avoid exposing
 * fake 90/270-degree rotation support.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_SPD2010_H_
#define _EGUI_LCD_SPD2010_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SPD2010 driver in user-provided storage.
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
void egui_lcd_spd2010_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_SPD2010_H_ */
