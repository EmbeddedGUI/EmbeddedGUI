#ifndef _EGUI_ENCODER_DRIVER_H_
#define _EGUI_ENCODER_DRIVER_H_

/**
 * @file egui_encoder_driver.h
 * @brief Rotary encoder driver abstraction for EmbeddedGUI.
 *
 * This module bridges a hardware rotary encoder (or software simulation)
 * to the core key-event system.  Each polling call reads a signed delta
 * (positive = CW, negative = CCW) and a push-button state, then queues
 * the corresponding key events via egui_input_add_key().
 *
 * Mapping:
 *   CW  tick        ->  KEY_RIGHT DOWN + UP  (one pair per tick)
 *   CCW tick        ->  KEY_LEFT  DOWN + UP  (one pair per tick)
 *   Button press    ->  KEY_ENTER DOWN
 *   Button release  ->  KEY_ENTER UP
 *   Button long press -> KEY_ENTER LONG_PRESS
 *
 * Enable with EGUI_CONFIG_FUNCTION_ENCODER=1.
 * Requires EGUI_CONFIG_FUNCTION_SUPPORT_KEY (auto-enabled).
 */

#include "egui_common.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_ENCODER

typedef struct egui_encoder_driver_ops egui_encoder_driver_ops_t;
typedef struct egui_encoder_driver egui_encoder_driver_t;

/**
 * Port-side callbacks that read the current encoder state.
 *
 * The read() function must be cheap (no blocking I/O); it is called once
 * per egui_polling_work() invocation.
 */
struct egui_encoder_driver_ops
{
    /**
     * Read accumulated rotation delta and button state since the last call.
     *
     * @param user_data   Opaque pointer stored in egui_encoder_driver_t.
     * @param out_delta   Signed rotation count.  Positive = CW, negative = CCW.
     * @param out_button  1 when the push-button is currently pressed, else 0.
     * @return 0 on success; non-zero signals a hardware read error.
     */
    int (*read)(void *user_data, int16_t *out_delta, uint8_t *out_button);
};

/** One encoder driver instance.  Zero-initialise before first use. */
struct egui_encoder_driver
{
    const egui_encoder_driver_ops_t *ops; /* driver callback table    */
    void *user_data;

    /* --- Internal edge-detection state (do not modify directly) --- */
    uint8_t _last_button;     /* button state from previous poll */
    uint8_t _long_press_sent; /* non-zero after LONG_PRESS was emitted */
    uint32_t _press_tick;     /* egui_timer_get_current_time() when press edge detected */
};

/** Register an encoder driver with one core.  Pass NULL to unregister. */
void egui_encoder_driver_register(egui_core_t *core, egui_encoder_driver_t *driver);

/** Return the encoder driver currently bound to a core, or NULL. */
egui_encoder_driver_t *egui_encoder_driver_get(egui_core_t *core);

/**
 * Poll the encoder driver once and queue any resulting key events.
 * Called automatically by egui_polling_work() when the feature is enabled.
 */
void egui_encoder_polling_work(egui_core_t *core);

#endif /* EGUI_CONFIG_FUNCTION_ENCODER */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ENCODER_DRIVER_H_ */
