/**
 * @file egui_touch_xpt2046.h
 * @brief XPT2046 resistive touch driver
 *
 * XPT2046 is a common SPI resistive touch controller supporting single touch point.
 * Requires ADC reading and calibration for accurate coordinates.
 */

#ifndef _EGUI_TOUCH_XPT2046_H_
#define _EGUI_TOUCH_XPT2046_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XPT2046 calibration data structure
 *
 * Used to map raw ADC values to screen coordinates.
 * Formula: screen_coord = (raw_value - offset) * scale / 4096
 */
typedef struct egui_touch_xpt2046_calibration {
    int16_t x_min;      /**< Raw X value at left edge */
    int16_t x_max;      /**< Raw X value at right edge */
    int16_t y_min;      /**< Raw Y value at top edge */
    int16_t y_max;      /**< Raw Y value at bottom edge */
} egui_touch_xpt2046_calibration_t;

/**
 * XPT2046 private data structure
 *
 * User must provide storage for this structure when calling init.
 */
typedef struct egui_touch_xpt2046_priv {
    egui_touch_xpt2046_calibration_t cal;
    uint16_t pressure_threshold;
} egui_touch_xpt2046_priv_t;

/**
 * Initialize XPT2046 driver in user-provided storage.
 *
 * @param storage      User-provided storage for driver instance
 * @param priv_storage User-provided storage for private data
 * @param io           Panel IO handle for SPI communication (must not be NULL)
 * @param set_rst      RST pin control callback (NULL if not available)
 * @param set_int      INT pin control callback (NULL if not used)
 * @param get_int      INT pin read callback (NULL if not used)
 */
void egui_touch_xpt2046_init(egui_hal_touch_driver_t *storage,
                              egui_touch_xpt2046_priv_t *priv_storage,
                              egui_panel_io_handle_t io,
                              void (*set_rst)(uint8_t level),
                              void (*set_int)(uint8_t level),
                              uint8_t (*get_int)(void));

/**
 * Set calibration data for coordinate mapping.
 *
 * @param driver  Driver instance
 * @param cal     Calibration data
 *
 * Default calibration assumes raw values 0-4095 map to screen coordinates.
 * Call this after init() to set proper calibration for your touch panel.
 */
void egui_touch_xpt2046_set_calibration(egui_hal_touch_driver_t *driver,
                                         const egui_touch_xpt2046_calibration_t *cal);

/**
 * Set pressure threshold for touch detection.
 *
 * @param driver     Driver instance
 * @param threshold  Pressure threshold (0-4095, default 100)
 *
 * Lower values = more sensitive, higher values = less sensitive.
 */
void egui_touch_xpt2046_set_pressure_threshold(egui_hal_touch_driver_t *driver,
                                                uint16_t threshold);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_XPT2046_H_ */
