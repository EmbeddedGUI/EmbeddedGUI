/**
 * @file egui_touch_axs15231b.h
 * @brief AXS15231B capacitive touch driver
 *
 * AXS15231B is an I2C capacitive touch controller with multi-touch support.
 * The reference protocol uses raw I2C command/read transactions without a normal register phase.
 * The Panel IO handle must support tx_param/rx_param with cmd=-1 for raw (no register phase) I2C transactions.
 */

#ifndef _EGUI_TOUCH_AXS15231B_H_
#define _EGUI_TOUCH_AXS15231B_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize AXS15231B driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param io       Panel IO handle for I2C communication (must not be NULL)
 * @param set_rst  RST pin control callback (NULL if not available)
 * @param set_int  INT pin control callback (NULL if not used)
 * @param get_int  INT pin read callback (NULL if not used)
 */
void egui_touch_axs15231b_init(egui_hal_touch_driver_t *storage,
                               egui_panel_io_handle_t io,
                               void (*set_rst)(uint8_t level),
                               void (*set_int)(uint8_t level),
                               uint8_t (*get_int)(void));

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_AXS15231B_H_ */
