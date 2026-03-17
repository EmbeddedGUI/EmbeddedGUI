/**
 * @file egui_touch_tt21100.h
 * @brief TT21100 capacitive touch driver
 *
 * TT21100 is a Cypress/Infineon I2C capacitive touch controller supporting up to 10 touch points.
 * I2C address: 0x24 (7-bit).
 * The reference protocol reads report payloads through raw I2C reads without a normal register phase.
 * The Panel IO handle must support rx_param with cmd=-1 for raw reads (no register address phase).
 */

#ifndef _EGUI_TOUCH_TT21100_H_
#define _EGUI_TOUCH_TT21100_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize TT21100 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for I2C communication (must not be NULL)
 * @param set_rst  RST pin control callback (NULL if not available)
 * @param set_int  INT pin control callback (NULL if not used)
 * @param get_int  INT pin read callback (NULL if not used)
 */
void egui_touch_tt21100_init(egui_hal_touch_driver_t *storage,
                              egui_panel_io_handle_t io,
                              void (*set_rst)(uint8_t level),
                              void (*set_int)(uint8_t level),
                              uint8_t (*get_int)(void));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_TT21100_H_ */
