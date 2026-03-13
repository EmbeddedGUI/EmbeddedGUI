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
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_ft5x06_init(egui_hal_touch_driver_t *storage,
                            const egui_bus_i2c_ops_t *i2c,
                            const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_FT5X06_H_ */
