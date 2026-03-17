/**
 * @file egui_touch_stmpe610.h
 * @brief STMPE610 resistive touch driver
 *
 * STMicroelectronics STMPE610 SPI resistive touch controller.
 * Single point touch support with pressure sensing.
 * Chip ID: 0x0811
 */

#ifndef _EGUI_TOUCH_STMPE610_H_
#define _EGUI_TOUCH_STMPE610_H_

#include "egui_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * STMPE610 calibration data structure
 *
 * Used to map raw ADC values to screen coordinates.
 * Formula: screen_coord = (raw_value - offset) * scale / 4096
 */
typedef struct egui_touch_stmpe610_calibration
{
    int16_t x_min; /**< Raw X value at left edge */
    int16_t x_max; /**< Raw X value at right edge */
    int16_t y_min; /**< Raw Y value at top edge */
    int16_t y_max; /**< Raw Y value at bottom edge */
} egui_touch_stmpe610_calibration_t;

/**
 * STMPE610 private data structure
 *
 * User must provide storage for this structure when calling init.
 */
typedef struct egui_touch_stmpe610_priv
{
    egui_touch_stmpe610_calibration_t cal;
    uint8_t pressure_threshold;
} egui_touch_stmpe610_priv_t;

/**
 * Initialize STMPE610 driver in user-provided storage.
 *
 * @param storage      User-provided storage for driver instance
 * @param priv_storage User-provided storage for private data
 * @param spi          SPI bus operations (must not be NULL)
 * @param gpio         GPIO operations (may be NULL if CS/IRQ not needed)
 */
void egui_touch_stmpe610_init(egui_hal_touch_driver_t *storage, egui_touch_stmpe610_priv_t *priv_storage, const egui_bus_spi_ops_t *spi,
                              const egui_touch_gpio_ops_t *gpio);

/**
 * Set calibration data for coordinate mapping.
 *
 * @param driver  Driver instance
 * @param cal     Calibration data
 *
 * Default calibration assumes raw values 0-4095 map to screen coordinates.
 * Call this after init() to set proper calibration for your touch panel.
 */
void egui_touch_stmpe610_set_calibration(egui_hal_touch_driver_t *driver, const egui_touch_stmpe610_calibration_t *cal);

/**
 * Set pressure threshold for touch detection.
 *
 * @param driver     Driver instance
 * @param threshold  Pressure threshold (0-255, default 30)
 *
 * Lower values = more sensitive, higher values = less sensitive.
 */
void egui_touch_stmpe610_set_pressure_threshold(egui_hal_touch_driver_t *driver, uint8_t threshold);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_STMPE610_H_ */
