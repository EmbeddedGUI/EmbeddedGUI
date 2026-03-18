/**
 * @file egui_touch_cst820.h
 * @brief CST820 capacitive touch driver
 *
 * CST820 is an upgraded version of CST816S, a low-power I2C capacitive touch
 * controller commonly used in smartwatches. Single touch point support only.
 */

#ifndef _EGUI_TOUCH_CST820_H_
#define _EGUI_TOUCH_CST820_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize CST820 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for register read/write (must not be NULL)
 * @param set_rst  RST pin control callback (NULL if not available)
 * @param set_int  INT pin control callback (NULL if not used)
 * @param get_int  INT pin read callback (NULL if not used)
 */
void egui_touch_cst820_init(egui_hal_touch_driver_t *storage, egui_panel_io_handle_t io, void (*set_rst)(uint8_t level), void (*set_int)(uint8_t level),
                            uint8_t (*get_int)(void));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_CST820_H_ */
