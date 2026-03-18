#ifndef _EGUI_TOUCH_DRIVER_H_
#define _EGUI_TOUCH_DRIVER_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Touch driver operations (vtable).
 * External touch hardware driver implements this.
 * read() is called periodically from egui_input_polling_work();
 * it should report the current touch state (pressed/released + coordinates).
 * The core framework converts raw touch data into motion events automatically.
 */
typedef struct egui_touch_driver_ops egui_touch_driver_ops_t;
struct egui_touch_driver_ops
{
    /** Initialize touch hardware. Called once during init. */
    void (*init)(void);

    /**
     * Read current touch state.
     * @param[out] pressed  1 if touch is active, 0 if released
     * @param[out] x        touch X coordinate (only valid when pressed=1)
     * @param[out] y        touch Y coordinate (only valid when pressed=1)
     */
    void (*read)(uint8_t *pressed, int16_t *x, int16_t *y);

    /**
     * Extended read with coordinate validity.
     * When implemented, x/y may still be valid after release and has_position
     * tells the input layer whether it should trust them for ACTION_UP.
     */
    void (*read_ex)(uint8_t *pressed, int16_t *x, int16_t *y, uint8_t *has_position);
};

/**
 * Touch driver instance.
 * Statically allocated by the port.
 */
typedef struct egui_touch_driver egui_touch_driver_t;
struct egui_touch_driver
{
    const egui_touch_driver_ops_t *ops;
};

/** Register touch driver with core. Called before egui_init(). */
void egui_touch_driver_register(egui_touch_driver_t *driver);

/** Get current touch driver. Returns NULL if not registered. */
egui_touch_driver_t *egui_touch_driver_get(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_TOUCH_DRIVER_H_ */
