#ifndef _EGUI_DISPLAY_DRIVER_H_
#define _EGUI_DISPLAY_DRIVER_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Runtime display rotation requested by the core.
 * The values follow clockwise quarter turns relative to the panel's native orientation.
 */
typedef enum egui_display_rotation
{
    EGUI_DISPLAY_ROTATION_0 = 0,   // native panel orientation
    EGUI_DISPLAY_ROTATION_90 = 1,  // rotate logical output 90 degrees clockwise
    EGUI_DISPLAY_ROTATION_180 = 2, // rotate logical output 180 degrees
    EGUI_DISPLAY_ROTATION_270 = 3, // rotate logical output 270 degrees clockwise
} egui_display_rotation_t;

/**
 * Display driver operations implemented by the port layer.
 * Each callback is optional unless otherwise noted.
 */
struct egui_display_driver_ops
{
    /** Initialize panel hardware and any driver-private state. Called once when the driver is registered. */
    void (*init)(egui_core_t *core);

    /**
     * Push one rendered tile to the display.
     * `data` is row-major pixel data for the rectangle `[x, x + w) x [y, y + h)`.
     * The transfer may be blocking or asynchronous.
     * In multi-buffer mode, asynchronous ports must notify PFB completion separately.
     */
    void (*draw_area)(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data);

    /**
     * Wait until the most recent asynchronous `draw_area` call has finished.
     * Leave this `NULL` when `draw_area` is already blocking.
     */
    void (*wait_draw_complete)(egui_core_t *core);

    /** Called after all dirty tiles for one frame have been submitted. */
    void (*flush)(egui_core_t *core);

    /** Apply display brightness where `0` is off and `255` is full brightness. */
    void (*set_brightness)(egui_core_t *core, uint8_t level);

    /** Turn the panel on or off. */
    void (*set_power)(egui_core_t *core, uint8_t on);

    /**
     * Apply hardware rotation when the controller supports it.
     * Leave this `NULL` to let the core fall back to software rotation.
     */
    void (*set_rotation)(egui_core_t *core, egui_display_rotation_t rotation);

    /* ---- Optional 2D acceleration hooks. `NULL` means software fallback. ---- */

    /** Accelerated solid rectangle fill. */
    void (*fill_rect)(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h, egui_color_int_t color);

    /** Accelerated block copy from a source pixel buffer. */
    void (*blit)(egui_core_t *core, int16_t dx, int16_t dy, int16_t w, int16_t h, const egui_color_int_t *src);

    /** Accelerated uniform-alpha blend of one foreground buffer onto the active target. */
    void (*blend)(egui_core_t *core, int16_t dx, int16_t dy, int16_t w, int16_t h, const egui_color_int_t *fg, egui_alpha_t alpha);

    /** Block until the next VSync or TE edge when explicit frame pacing is needed. */
    void (*wait_vsync)(egui_core_t *core);
};

/**
 * One display-driver instance bound to one core/display.
 * The port usually allocates this structure statically and fills in the desired defaults.
 */
struct egui_display_driver
{
    const egui_display_driver_ops_t *ops; // callback table implemented by the port

    int16_t physical_width;  // native panel width before runtime rotation is applied
    int16_t physical_height; // native panel height before runtime rotation is applied

    egui_display_rotation_t rotation; // current runtime rotation requested by the core

    uint8_t brightness; // last brightness level requested through the core API

    uint8_t power_on; // cached power state where non-zero means panel enabled

    /**
     * Non-blocking frame synchronization flags.
     * When enabled, the core waits for `frame_sync_ready` before rendering the next frame.
     */
    uint8_t frame_sync_enabled;        // non-zero to use ISR-driven frame sync instead of blocking waits
    volatile uint8_t frame_sync_ready; // set by the port when a fresh frame may start
    void *user_data;                   // optional port-private pointer carried with the driver instance
};

/** Bind one display driver to the core, call its init hook, and apply the instance's default rotation/brightness. */
void egui_display_driver_register(egui_core_t *core, egui_display_driver_t *driver);

/** Get the display driver currently registered on this core, or `NULL` when display setup is incomplete. */
egui_display_driver_t *egui_display_driver_get(egui_core_t *core);

/** Cache and apply a new brightness level through the registered driver when supported. */
void egui_display_set_brightness(egui_core_t *core, uint8_t level);
/** Cache and apply a new display power state through the registered driver when supported. */
void egui_display_set_power(egui_core_t *core, uint8_t on);
/** Update rotation, delegate to hardware when available, and refresh the core's logical screen size if needed. */
void egui_display_set_rotation(egui_core_t *core, egui_display_rotation_t rotation);
/** Get the driver's current rotation, or `EGUI_DISPLAY_ROTATION_0` when no driver is registered. */
egui_display_rotation_t egui_display_get_rotation(egui_core_t *core);

/**
 * Get the logical display width seen by layout and input code.
 * When software rotation is active, 90/270-degree rotation swaps the physical dimensions.
 */
int16_t egui_display_get_width(egui_core_t *core);
/** Get the logical display height seen by layout and input code. */
int16_t egui_display_get_height(egui_core_t *core);

/**
 * Notify the core that a VSync or TE event has arrived.
 * Ports normally call this from an ISR when `frame_sync_enabled` is in use.
 */
void egui_display_notify_vsync(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DISPLAY_DRIVER_H_ */
