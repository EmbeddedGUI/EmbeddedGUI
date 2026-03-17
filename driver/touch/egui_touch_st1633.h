/**
 * @file egui_touch_st1633.h
 * @brief ST1633 capacitive touch driver
 *
 * ST1633 is a Sitronix I2C capacitive touch controller supporting up to 5 touch points.
 */

#ifndef _EGUI_TOUCH_ST1633_H_
#define _EGUI_TOUCH_ST1633_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST1633 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_st1633_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_ST1633_H_ */
