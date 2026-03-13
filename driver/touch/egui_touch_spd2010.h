/**
 * @file egui_touch_spd2010.h
 * @brief SPD2010 capacitive touch driver
 *
 * SPD2010 is an I2C capacitive touch controller supporting multi-touch.
 * I2C address: 0x2E.
 * The reference protocol uses raw I2C command/read transactions instead of normal register-addressed reads.
 * This driver requires raw-I2C transactions (`write_raw` and `read_raw`) because the reference protocol is command-stream based.
 */

#ifndef _EGUI_TOUCH_SPD2010_H_
#define _EGUI_TOUCH_SPD2010_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SPD2010 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL, requires `write_raw` and `read_raw`)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_spd2010_init(egui_hal_touch_driver_t *storage,
                             const egui_bus_i2c_ops_t *i2c,
                             const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_SPD2010_H_ */
