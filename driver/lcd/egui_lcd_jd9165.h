/**
 * @file egui_lcd_jd9165.h
 * @brief JD9165 LCD driver
 *
 * JD9165 is a MIPI DSI interface LCD controller from Jadard.
 * Supports RGB565, RGB666, RGB888 color modes.
 * The real panel path requires MIPI DSI-style pixel output rather than SPI RAMWR flushes.
 * Current EGUI implementation should be regarded as command-side bring-up code only,
 * not a production-verified DSI driver.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_JD9165_H_
#define _EGUI_LCD_JD9165_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize JD9165 driver in user-provided storage.
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
void egui_lcd_jd9165_init(egui_hal_lcd_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_JD9165_H_ */
