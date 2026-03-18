/**
 * @file egui_touch_ft6336.h
 * @brief FT6336 capacitive touch driver
 *
 * FT6336 is a common I2C capacitive touch controller supporting up to 2 touch points.
 * Compatible with FT6236, FT6336G, FT6336U variants.
 *
 * Uses unified Panel IO interface for bus communication.
 */

#ifndef _EGUI_TOUCH_FT6336_H_
#define _EGUI_TOUCH_FT6336_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize FT6336 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for I2C communication (must not be NULL)
 * @param set_rst  RST pin control function (may be NULL if not available)
 * @param set_int  INT pin control function (may be NULL)
 * @param get_int  INT pin read function (may be NULL)
 */
void egui_touch_ft6336_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_FT6336_H_ */
