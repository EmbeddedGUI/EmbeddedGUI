/**
 * @file egui_touch_ft5x06.h
 * @brief FT5x06 capacitive touch driver
 *
 * FocalTech FT5x06 series I2C capacitive touch controller supporting up to 5 touch points.
 * Compatible with FT5206, FT5306, FT5406, FT5506 variants.
 * Default I2C address: 0x38
 */

#ifndef _EGUI_TOUCH_FT5X06_H_
#define _EGUI_TOUCH_FT5X06_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize FT5x06 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for register read/write (must not be NULL)
 * @param set_rst  RST pin control callback (NULL if not available)
 * @param set_int  INT pin control callback (NULL if not used)
 * @param get_int  INT pin read callback (NULL if not used)
 */
void egui_touch_ft5x06_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_FT5X06_H_ */
