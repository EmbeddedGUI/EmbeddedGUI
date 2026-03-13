#ifndef _EGUI_DISPLAY_DRIVER_H_
#define _EGUI_DISPLAY_DRIVER_H_

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display rotation enum
 */
typedef enum egui_display_rotation
{
    EGUI_DISPLAY_ROTATION_0 = 0,
    EGUI_DISPLAY_ROTATION_90 = 1,
    EGUI_DISPLAY_ROTATION_180 = 2,
    EGUI_DISPLAY_ROTATION_270 = 3,
} egui_display_rotation_t;

/**
 * Display driver operations (vtable).
 * Port implements a static const instance of this struct.
 */
struct egui_display_driver_ops
{
    /** Initialize display hardware. Called once during egui_init. */
    void (*init)(void);

    /**
     * Draw pixel data to a rectangular area on the display.
     * Can be synchronous (blocking) or asynchronous (e.g., DMA).
     * When used with multi-buffer PFB, the DMA completion ISR must call
     * egui_pfb_notify_flush_complete() to advance the ring buffer.
     */
    void (*draw_area)(int16_t x, int16_t y, int16_t w, int16_t h, const egui_color_int_t *data);

    /**
     * Wait for draw_area to complete. Used to drain pending DMA transfers.
     * NULL if draw_area is synchronous (blocking) -- no waiting needed.
     */
    void (*wait_draw_complete)(void);

    /** Signal that a full frame has been flushed. May trigger display refresh. */
    void (*flush)(void);

    /** Set display brightness (0 = off, 255 = max). NULL if not supported. */
    void (*set_brightness)(uint8_t level);

    /** Set display power state. NULL if not supported. */
    void (*set_power)(uint8_t on);

    /**
     * Set hardware rotation. NULL if HW rotation not supported;
     * core will use software rotation instead when EGUI_CONFIG_SOFTWARE_ROTATION is enabled.
     */
    void (*set_rotation)(egui_display_rotation_t rotation);

    /* ---- 2D hardware acceleration (all optional, NULL = use software) ---- */

    /** Hardware rectangle fill. NULL = software fallback. */
    void (*fill_rect)(int16_t x, int16_t y, int16_t w, int16_t h, egui_color_int_t color);

    /** Hardware block copy (blit). NULL = software fallback. */
    void (*blit)(int16_t dx, int16_t dy, int16_t w, int16_t h, const egui_color_int_t *src);

    /** Hardware alpha blend. NULL = software fallback. */
    void (*blend)(int16_t dx, int16_t dy, int16_t w, int16_t h, const egui_color_int_t *fg, egui_alpha_t alpha);

    /** Wait for VSync / TE signal before flush. NULL = no sync needed. */
    void (*wait_vsync)(void);
};

/**
 * Display driver instance. One per display.
 * Statically allocated by the port.
 */
struct egui_display_driver
{
    const egui_display_driver_ops_t *ops;

    /** Physical display dimensions (before rotation) */
    int16_t physical_width;
    int16_t physical_height;

    /** Current rotation */
    egui_display_rotation_t rotation;

    /** Current brightness (0-255) */
    uint8_t brightness;

    /** Power state: 1 = on, 0 = off */
    uint8_t power_on;

    /**
     * Non-blocking frame sync.
     * When frame_sync_enabled = 1, core skips rendering if frame_sync_ready = 0.
     * Port sets frame_sync_ready = 1 from TE/VSync ISR via egui_display_notify_vsync().
     * When frame_sync_enabled = 0, core falls back to blocking wait_vsync() if available.
     */
    uint8_t frame_sync_enabled;
    volatile uint8_t frame_sync_ready;
};

/** Register display driver with core. Called before egui_init(). */
void egui_display_driver_register(egui_display_driver_t *driver);

/** Get current display driver. Returns NULL if not registered. */
egui_display_driver_t *egui_display_driver_get(void);

/** Convenience APIs (call through registered driver) */
void egui_display_set_brightness(uint8_t level);
void egui_display_set_power(uint8_t on);
void egui_display_set_rotation(egui_display_rotation_t rotation);
egui_display_rotation_t egui_display_get_rotation(void);

/**
 * Get effective screen dimensions (after rotation).
 * Returns physical_width/height for 0/180, swapped for 90/270.
 */
int16_t egui_display_get_width(void);
int16_t egui_display_get_height(void);

/**
 * Notify that a VSync/TE event has occurred (call from ISR).
 * Sets frame_sync_ready flag so core can start the next frame without blocking.
 */
void egui_display_notify_vsync(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_DISPLAY_DRIVER_H_ */
