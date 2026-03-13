/**
 * @file egui_touch_chsc6540.h
 * @brief CHSC6540 capacitive touch driver
 *
 * CHSC6540 is an I2C capacitive touch controller supporting single touch point.
 */

#ifndef _EGUI_TOUCH_CHSC6540_H_
#define _EGUI_TOUCH_CHSC6540_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize CHSC6540 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_chsc6540_init(egui_hal_touch_driver_t *storage,
                              const egui_bus_i2c_ops_t *i2c,
                              const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_CHSC6540_H_ */
