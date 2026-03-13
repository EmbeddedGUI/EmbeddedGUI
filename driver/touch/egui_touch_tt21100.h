/**
 * @file egui_touch_tt21100.h
 * @brief TT21100 capacitive touch driver
 *
 * TT21100 is a Cypress/Infineon I2C capacitive touch controller supporting up to 10 touch points.
 * I2C address: 0x24 (7-bit).
 * The reference protocol reads report payloads through raw I2C reads without a normal register phase.
 * This driver requires `i2c->read_raw` support because the report payload is read without a normal register phase.
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
 * @param i2c      I2C bus operations (must not be NULL, requires `read_raw`)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_tt21100_init(egui_hal_touch_driver_t *storage,
                              const egui_bus_i2c_ops_t *i2c,
                              const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_TT21100_H_ */
