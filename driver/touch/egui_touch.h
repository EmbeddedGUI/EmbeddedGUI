/**
 * @file egui_touch.h
 * @brief Touch Driver common interface
 *
 * This file defines the common interface for all touch drivers.
 * Supports multi-touch (up to 5 points) with coordinate transformation.
 *
 * Uses unified Panel IO handle for register read/write.
 * Device-level GPIO (RST, INT) kept separate from IO protocol.
 *
 * Note: Uses egui_hal_touch_* prefix to avoid conflict with Core layer types.
 */

#ifndef _EGUI_HAL_TOUCH_H_
#define _EGUI_HAL_TOUCH_H_

#include <stdint.h>
#include "egui_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of touch points supported */
#define EGUI_HAL_TOUCH_MAX_POINTS 5

/**
 * Single touch point data
 */
typedef struct egui_hal_touch_point
{
    int16_t x;        /**< X coordinate */
    int16_t y;        /**< Y coordinate */
    uint8_t id;       /**< Touch point ID (for tracking) */
    uint8_t pressure; /**< Pressure value (0 = not supported) */
} egui_hal_touch_point_t;

/**
 * Touch data structure (multi-touch)
 */
typedef struct egui_hal_touch_data
{
    uint8_t point_count;                                      /**< Number of active touch points */
    uint8_t gesture;                                          /**< Gesture code (driver-specific) */
    uint8_t has_release_point;                                /**< Release coordinates are valid when point_count=0 */
    egui_hal_touch_point_t release_point;                     /**< Optional release coordinate */
    egui_hal_touch_point_t points[EGUI_HAL_TOUCH_MAX_POINTS]; /**< Touch point array */
} egui_hal_touch_data_t;

/**
 * Touch configuration structure
 */
typedef struct egui_hal_touch_config
{
    uint16_t width;   /**< Touch area width (for coordinate mapping) */
    uint16_t height;  /**< Touch area height (for coordinate mapping) */
    uint8_t swap_xy;  /**< Swap X and Y coordinates */
    uint8_t mirror_x; /**< Mirror X axis */
    uint8_t mirror_y; /**< Mirror Y axis */
} egui_hal_touch_config_t;

/**
 * Touch driver instance (forward declaration)
 */
typedef struct egui_hal_touch_driver egui_hal_touch_driver_t;

/**
 * Touch driver structure
 *
 * Lifecycle: factory_init → reset → init → read/... → del
 *
 * Uses unified Panel IO handle for register read/write.
 * Device-level GPIO (RST, INT) managed internally by reset/del.
 */
struct egui_hal_touch_driver
{
    const char *name;   /**< Driver name (e.g., "FT6336") */
    uint8_t max_points; /**< Maximum touch points supported */

    /* Lifecycle: reset → init → ... → del */
    int (*reset)(egui_hal_touch_driver_t *self); /**< Hardware reset (RST low→high) */
    int (*init)(egui_hal_touch_driver_t *self, const egui_hal_touch_config_t *config);
    void (*del)(egui_hal_touch_driver_t *self); /**< Pull RST low + clear struct */

    /* Read touch data */
    int (*read)(egui_hal_touch_driver_t *self, egui_hal_touch_data_t *data);

    /* IO and GPIO */
    egui_panel_io_handle_t io;      /**< Unified IO handle for register read/write */
    void (*set_rst)(uint8_t level); /**< Device RST pin. Used by reset/del internally */
    void (*set_int)(uint8_t level); /**< INT pin control (some chips need during init) */
    uint8_t (*get_int)(void);       /**< INT pin read (NULL if not used) */

    /* State */
    egui_hal_touch_config_t config; /**< Current configuration */
    void *priv;                     /**< Driver-specific private data */
};

/**
 * Register HAL touch driver with Core.
 *
 * Calls reset(), init(), then registers with Core's touch driver system.
 * Extracts first touch point for single-point events.
 * Checks INT pin (if available) to skip reads when no interrupt pending.
 *
 * @param touch   HAL touch driver instance (factory_init already called)
 * @param config  Touch configuration
 */
void egui_hal_touch_register(egui_hal_touch_driver_t *touch, const egui_hal_touch_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_HAL_TOUCH_H_ */
