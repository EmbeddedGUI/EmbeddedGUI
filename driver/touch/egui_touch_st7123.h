/**
 * @file egui_touch_st7123.h
 * @brief ST7123 capacitive touch driver
 *
 * ST7123 is a Sitronix I2C capacitive touch controller supporting multi-touch.
 * I2C address: 0x55.
 */

#ifndef _EGUI_TOUCH_ST7123_H_
#define _EGUI_TOUCH_ST7123_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize ST7123 driver in user-provided storage.
 *
 * @param storage  User-provided storage for driver instance
 * @param i2c      I2C bus operations (must not be NULL)
 * @param gpio     GPIO operations (may be NULL if RST not needed)
 */
void egui_touch_st7123_init(egui_hal_touch_driver_t *storage, const egui_bus_i2c_ops_t *i2c, const egui_touch_gpio_ops_t *gpio);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_ST7123_H_ */
