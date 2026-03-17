/**
 * @file egui_touch_gt911.h
 * @brief GT911 capacitive touch driver
 *
 * GT911 is a Goodix I2C capacitive touch controller supporting up to 5 touch points.
 * I2C address: 0x5D (default) or 0x14 (backup, depends on INT pin state during reset).
 */

#ifndef _EGUI_TOUCH_GT911_H_
#define _EGUI_TOUCH_GT911_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GT911 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_gt911_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_GT911_H_ */
