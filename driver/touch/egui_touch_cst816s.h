/**
 * @file egui_touch_cst816s.h
 * @brief CST816S capacitive touch driver
 *
 * CST816S is a low-power I2C capacitive touch controller commonly used in
 * smartwatches. Single touch point support only.
 */

#ifndef _EGUI_TOUCH_CST816S_H_
#define _EGUI_TOUCH_CST816S_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize CST816S driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_cst816s_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_CST816S_H_ */
