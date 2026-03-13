/**
 * @file egui_touch.h
 * @brief Touch Driver common interface
 *
 * This file defines the common interface for all touch drivers.
 * Supports multi-touch (up to 5 points) with coordinate transformation.
 *
 * Note: Uses egui_hal_touch_* prefix to avoid conflict with Core layer types.
 */

#ifndef _EGUI_HAL_TOUCH_H_
#define _EGUI_HAL_TOUCH_H_

#include <stdint.h>
#include "egui_bus.h"
#include "egui_bus_spi.h"
#include "egui_bus_i2c.h"
#include "egui_bus_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of touch points supported */
#define EGUI_HAL_TOUCH_MAX_POINTS 5

/**
 * Single touch point data
 */
typedef struct egui_hal_touch_point {
    int16_t x;          /**< X coordinate */
    int16_t y;          /**< Y coordinate */
    uint8_t id;         /**< Touch point ID (for tracking) */
    uint8_t pressure;   /**< Pressure value (0 = not supported) */
} egui_hal_touch_point_t;

/**
 * Touch data structure (multi-touch)
 */
typedef struct egui_hal_touch_data {
    uint8_t point_count;                          /**< Number of active touch points */
    uint8_t gesture;                              /**< Gesture code (driver-specific) */
    egui_hal_touch_point_t points[EGUI_HAL_TOUCH_MAX_POINTS];  /**< Touch point array */
} egui_hal_touch_data_t;

/**
 * Touch configuration structure
 */
typedef struct egui_hal_touch_config {
    uint16_t width;       /**< Touch area width (for coordinate mapping) */
    uint16_t height;      /**< Touch area height (for coordinate mapping) */
    uint8_t swap_xy;      /**< Swap X and Y coordinates */
    uint8_t mirror_x;     /**< Mirror X axis */
    uint8_t mirror_y;     /**< Mirror Y axis */
} egui_hal_touch_config_t;

/**
 * Touch driver instance (forward declaration)
 */
typedef struct egui_hal_touch_driver egui_hal_touch_driver_t;

/**
 * Touch driver structure
 *
 * Uses union for bus pointers to save memory.
 */
struct egui_hal_touch_driver {
    const char *name;                    /**< Driver name (e.g., "FT6336") */
    egui_bus_type_t bus_type;           /**< Bus type used by this driver */
    uint8_t max_points;                 /**< Maximum touch points supported */

    /* Lifecycle */
    int (*init)(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config);
    void (*deinit)(egui_hal_touch_driver_t *self);

    /* Read touch data */
    int (*read)(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data);

    /* Optional features */
    void (*set_rotation)(egui_hal_touch_driver_t *self, uint8_t rotation);
    void (*enter_sleep)(egui_hal_touch_driver_t *self);
    void (*exit_sleep)(egui_hal_touch_driver_t *self);

    /* Internal data */
    void *priv;                          /**< Driver-specific private data */
    union {
        const egui_bus_i2c_ops_t *i2c;
        const egui_bus_spi_ops_t *spi;
    } bus;
    const egui_touch_gpio_ops_t *gpio;    /**< Touch GPIO control (RST/INT) */
    egui_hal_touch_config_t config;         /**< Current configuration */
};

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_TOUCH_H_ */
