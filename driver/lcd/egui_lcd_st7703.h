/**
 * @file egui_lcd_st7703.h
 * @brief ST7703 LCD driver
 *
 * ST7703 is an RGB interface LCD controller similar to ST7701.
 * Current HAL implementation only models the SPI command phase used during init.
 * The RGB pixel streaming path still needs dedicated bus support and is not production-verified.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_LCD_ST7703_H_
#define _EGUI_LCD_ST7703_H_

#include "egui_lcd.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7703 driver in user-provided storage.
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
void egui_lcd_st7703_init(egui_hal_lcd_driver_t *storage,
                             egui_panel_io_handle_t io,
                             void (*set_rst)(uint8_t level));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_LCD_ST7703_H_ */
