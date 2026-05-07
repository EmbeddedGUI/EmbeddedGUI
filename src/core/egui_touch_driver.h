#ifndef _EGUI_TOUCH_DRIVER_H_
#define _EGUI_TOUCH_DRIVER_H_

/**
 * @file egui_touch_driver.h
 * @brief Port-side touch driver callbacks that feed the core input layer.
 */

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
#define EGUI_TOUCH_DRIVER_MAX_POINTS EGUI_CONFIG_TOUCH_MAX_POINTS
#else
#define EGUI_TOUCH_DRIVER_MAX_POINTS 1
#endif

typedef struct egui_touch_driver_point
{
    int16_t x;
    int16_t y;
    uint8_t id;
    uint8_t pressure;
} egui_touch_driver_point_t;

typedef struct egui_touch_driver_data
{
    uint8_t point_count; /* Active points in this sample; 0 means all released. */
    egui_touch_driver_point_t points[EGUI_TOUCH_DRIVER_MAX_POINTS];
} egui_touch_driver_data_t;

/**
 * Touch driver operations (vtable).
 * The port implements this table to expose raw touch state.
 * `read()` is polled from `egui_input_polling_work()`,
 * and the core converts
 * the active-point sample into motion events automatically.
 */
struct egui_touch_driver_ops
{
    /** Initialize touch hardware and any driver-private state. Called once when the driver is registered. */
    void (*init)(egui_core_t *core);

    /**
     * The driver reports only the points that are active in the current sample.
     * `point_count == 0` means all points are released. The input
     * layer compares
     * consecutive samples, uses stable point IDs for continuity when available,
     * and emits DOWN, POINTER_DOWN, MOVE, POINTER_UP,
     * and UP automatically.
     */
    int (*read)(egui_core_t *core, egui_touch_driver_data_t *data);
};

/**
 * One touch-driver instance bound to one core.
 * The port usually allocates this structure statically.
 */
struct egui_touch_driver
{
    const egui_touch_driver_ops_t *ops; // driver callback table implemented by the port
};

/** Register a touch driver with one core and run its init callback immediately when present. */
void egui_touch_driver_register(egui_core_t *core, egui_touch_driver_t *driver);

/** Get the touch driver currently bound to one core, or `NULL` when no driver is registered. */
egui_touch_driver_t *egui_touch_driver_get(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_DRIVER_H_ */
