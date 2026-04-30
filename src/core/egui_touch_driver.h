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

/**
 * Touch driver operations (vtable).
 * The port implements this table to expose raw touch state.
 * `read()` is polled from `egui_input_polling_work()`, and the core converts the reported pressed/released state into motion events automatically.
 */
struct egui_touch_driver_ops
{
    /** Initialize touch hardware and any driver-private state. Called once when the driver is registered. */
    void (*init)(egui_core_t *core);

    /**
     * Read the current touch state.
     * This callback is the required baseline polling hook; the input layer
     * checks it before consulting `read_ex`.
     *
     * @param[out] pressed  1 if touch is active, 0 if released
     * @param[out] x        touch X coordinate (only valid when pressed=1)
     * @param[out] y        touch Y coordinate (only valid when pressed=1)
     */
    void (*read)(egui_core_t *core, uint8_t *pressed, int16_t *x, int16_t *y);

    /**
     * Extended read with explicit release-coordinate validity.
     * When implemented, x/y may still be valid after release and has_position
     * tells the input layer whether it should trust them for `ACTION_UP`.
     * Ports that cannot report release coordinates can leave this `NULL`.
     */
    void (*read_ex)(egui_core_t *core, uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position);
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
