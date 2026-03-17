/**
 * @file egui_touch_ft6336.h
 * @brief FT6336 capacitive touch driver
 *
 * FT6336 is a common I2C capacitive touch controller supporting up to 2 touch points.
 * Compatible with FT6236, FT6336G, FT6336U variants.
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
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_ft6336_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_FT6336_H_ */
