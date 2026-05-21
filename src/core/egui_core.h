#ifndef _EGUI_CORE_H_
#define _EGUI_CORE_H_

#include "egui_common.h"
#include "egui_motion_event.h"
#include "egui_pfb_manager.h"
#include "canvas/egui_canvas.h"
#include "egui_display_driver.h"
#include "egui_touch_driver.h"
#include "egui_platform.h"
#include "egui_input.h"
#include "egui_timer.h"
#include "canvas/egui_canvas_transform_cache.h"
#include "egui_image_decode_cache.h"
#include "image/egui_image_std.h"
#include "font/egui_font_std.h"
#include "resource/egui_i18n.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "egui_focus.h"
#endif
#if EGUI_CONFIG_FUNCTION_ENCODER
#include "egui_encoder_driver.h"
#endif
#include "widget/egui_view_group.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
typedef struct egui_dirty_region_stats
{
    uint32_t frame_index;
    uint32_t frame_count;
    uint32_t current_region_count;
    uint32_t current_dirty_area;
    uint32_t current_tile_count;
    uint8_t is_collecting_frame;
    uint64_t total_dirty_area;
    uint64_t total_tile_count;
} egui_dirty_region_stats_t;
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW || EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_SHOW 1
#else
#define EGUI_DEBUG_MONITOR_SHOW 0
#endif

#if EGUI_DEBUG_MONITOR_SHOW
#define EGUI_DEBUG_MONITOR_TEXT_MAX_LEN 100
typedef struct egui_debug_overlay
{
    char text[EGUI_DEBUG_MONITOR_TEXT_MAX_LEN];
    egui_region_t region_last;
    uint8_t region_last_valid;
    uint8_t align_type;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
} egui_debug_overlay_t;
#endif

#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
typedef struct egui_debug_perf_stats
{
    uint32_t window_start_time;
    uint32_t refr_count;
    uint32_t render_count;
    uint32_t total_render_time;
    uint32_t total_flush_time;
    uint8_t initialized;
} egui_debug_perf_stats_t;
#endif

#include "egui_core_state.h"

struct egui_core
{
    egui_color_int_t *pfb;     // pointer to frame buffer
    int pfb_width;             // width of frame buffer
    int pfb_height;            // height of frame buffer
    int pfb_total_buffer_size; // size of frame buffer in bytes, for speed up.
    int screen_width;          // width of screen
    int screen_height;         // height of screen
    int color_bytes;           // bytes per pixel

    int pfb_width_count;  // width of frame buffer count in screen width
    int pfb_height_count; // height of frame buffer count in screen width
    uint8_t pfb_scan_reverse_x;
    uint8_t pfb_scan_reverse_y;

#if EGUI_CONFIG_DEBUG_VIEW_ID
    uint16_t unique_id; // unique id count
#endif

    egui_core_scene_state_t scene; // activity/dialog/root/dirty scene state

    /* ---- Fields merged from egui_display_t ---- */
    egui_canvas_t canvas;            // per-core canvas state
    egui_core_render_state_t render; // render/display owned state

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_core_touch_state_t touch; // touch/input owned state
#elif EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    egui_input_t input; // key-only queued input state
#endif

    egui_core_system_state_t system; // refresh/timer/focus owned state
    uint8_t id;                      // core instance id
    void *user_data;                 // user-defined data
    egui_core_asset_state_t asset;   // theme/i18n/image registry state
    egui_core_debug_state_t debug;   // debug/monitor owned state
    egui_core_image_state_t image;   // image/cache owned state
    egui_core_text_state_t text;     // font/text-transform owned state

    egui_region_t punch_region; // app-owned screen region excluded from PFB rendering
};

/**
 * Initialize EGUI with PFB buffer.
 *
 * @param pfb  2D PFB buffer array, declared as:
 *             EGUI_CONFIG_PFB_BUFFER_DECLARE(pfb);
 *
 * egui_init() is the primary-display convenience wrapper around
 * egui_init_display(). It automatically uses all buffers based on
 * EGUI_CONFIG_PFB_BUFFER_COUNT and the compile-time main-display macros.
 */
void egui_init(egui_core_t *core, egui_color_int_t pfb[][EGUI_CONFIG_PFB_WIDTH * EGUI_CONFIG_PFB_HEIGHT]);

/**
 * Initialize a secondary display core with explicit dimensions.
 *
 * Unlike egui_init() which uses compile-time macros, this function takes
 * screen/PFB dimensions as runtime parameters. Suitable for sub-displays
 * that may have different resolutions than the primary display.
 *
 * After calling this, the core's canvas is bound to the core instance.
 * The caller should register a display driver via egui_display_driver_register().
 *
 * @param core       Core instance to initialize (caller-allocated)
 * @param screen_w   Screen width in pixels
 * @param screen_h   Screen height in pixels
 * @param pfb_bufs   Array of PFB buffer pointers
 * @param buf_count  Number of PFB buffers (1..EGUI_PFB_BUFFER_MAX_COUNT)
 * @param pfb_w      PFB tile width
 * @param pfb_h      PFB tile height
 *
 * Runtime render flags such as RGB565 byte-swap and software rotation default to
 * the compile-time EGUI_CONFIG_* macros in this low-level helper.
 */
void egui_init_display(egui_core_t *core, int16_t screen_w, int16_t screen_h, egui_color_int_t **pfb_bufs, int buf_count, int pfb_w, int pfb_h);

/** App-provided UI bootstrap callback used by `egui_setup_display()`. */
typedef void (*egui_uicode_init_func_t)(egui_core_t *core);
/** Optional port callback that registers the touch driver for one display. */
typedef void (*egui_touch_register_func_t)(egui_core_t *core);

typedef struct egui_core_render_config
{
    uint8_t color_16_swap;              // non-zero when RGB565 output bytes must be swapped before flush
    uint8_t software_rotation;          // non-zero when runtime rotation is handled in software instead of hardware
    egui_color_int_t *rotation_scratch; // optional caller-owned scratch buffer used for software rotation
} egui_core_render_config_t;

typedef struct egui_display_setup
{
    int screen_width;                               // logical screen width in pixels
    int screen_height;                              // logical screen height in pixels
    int pfb_width;                                  // one PFB tile width
    int pfb_height;                                 // one PFB tile height
    egui_color_int_t **pfb_buffers;                 // array of PFB buffer pointers
    int pfb_buffer_count;                           // number of PFB buffers in the array
    egui_display_driver_t *display_driver;          // display driver bound to this core
    const egui_core_render_config_t *render_config; // optional per-core runtime render config; NULL uses compile-time defaults
    egui_touch_register_func_t touch_register;      // optional touch-driver registration callback
    egui_uicode_init_func_t uicode_init;            // UI bootstrap callback that creates the initial scene
    uint8_t display_id;                             // runtime display index used by multi-display ports
} egui_display_setup_t;

/**
 * Initialize a display from a single setup descriptor.
 *
 * This is the recommended startup helper once the port has already registered
 * the process-global platform callbacks and prepared the display driver,
 *
 * optional touch registration, per-core render config, and the UI bootstrap callback.
 */
void egui_setup_display(egui_core_t *core, const egui_display_setup_t *setup);

/**
 * Update one core's runtime render policy after initialization.
 *
 * Passing `NULL` reverts to the compile-time default runtime values from the
 * EGUI_CONFIG_* macros. The core is marked fully dirty so the next frame redraws
 * with the new render policy.
 */
void egui_core_set_render_config(egui_core_t *core, const egui_core_render_config_t *config);

/** Force the entire screen to be marked dirty and refreshed on the next frame. */
void egui_core_force_refresh(egui_core_t *core);
/** Return the top-most root container owned by this core. */
egui_view_group_t *egui_core_get_root_view(egui_core_t *core);
/** Check whether a new dirty region overlaps any existing dirty slot. */
int egui_core_check_region_dirty_intersect(egui_core_t *core, egui_region_t *region_dirty);
/** Merge a screen-space dirty region into the core dirty queue. */
void egui_core_update_region_dirty(egui_core_t *core, egui_region_t *region_dirty);
/** Mark the full screen as dirty. */
void egui_core_update_region_dirty_all(egui_core_t *core);
/** Clear all dirty slots after a frame completes and advance the dirty epoch. */
void egui_core_clear_region_dirty(egui_core_t *core);
/** Set one app-owned screen region that the PFB renderer must not overdraw. */
void egui_core_set_punch_region(egui_core_t *core, int16_t x, int16_t y, int16_t w, int16_t h);
/** Return the user root container used for activities, dialogs, and other app-owned trees. */
egui_view_group_t *egui_core_get_user_root_view(egui_core_t *core);
/** Add a view to the user root container. */
void egui_core_add_user_root_view(egui_view_t *view);
/** Expose the internal dirty-region array for advanced diagnostics or tooling. */
egui_region_t *egui_core_get_region_dirty_arr(egui_core_t *core);
/** Return the current dirty epoch used to detect same-frame invalidations. */
uint32_t egui_core_get_dirty_epoch(egui_core_t *core);
/** Render one dirty region by splitting it into PFB-sized tiles. */
void egui_core_draw_view_group(egui_core_t *core, egui_region_t *p_region_dirty, int is_debug_mode);
/** Optional hook for tuning logical PFB probe width on specific platforms. */
egui_dim_t egui_core_get_logical_pfb_target_width_hint(egui_core_t *core);
/** Push one motion event into the input pipeline of this core. */
void egui_core_process_input_motion(egui_core_t *core, egui_motion_event_t *motion_event);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
/** Push one key event into the input pipeline of this core. */
void egui_core_process_input_key(egui_core_t *core, egui_key_event_t *key_event);
#endif
#if EGUI_CONFIG_DEBUG_VIEW_ID
/** Allocate a per-core debug view id. */
uint16_t egui_core_get_unique_id(egui_core_t *core);
#endif
/** Run animation/layout/render work if the core has pending dirty content. */
void egui_core_refresh_screen(egui_core_t *core);
/** Stop the automatic refresh timer without changing scene state. */
void egui_core_stop_auto_refresh_screen(egui_core_t *core);
/** Return the currently active PFB buffer pointer. */
egui_color_int_t *egui_core_get_pfb_buffer_ptr(egui_core_t *core);
/** Replace the current PFB buffer and update cached size metadata. */
void egui_core_pfb_set_buffer(egui_core_t *core, egui_color_int_t *pfb, uint16_t width, uint16_t height);
/** Stop automatic refresh work for a powered-down display. */
void egui_core_power_off(egui_core_t *core);
/** Restart automatic refresh work for an active display. */
void egui_core_power_on(egui_core_t *core);
/** Update runtime screen size and resize the built-in root views. */
void egui_core_set_screen_size(egui_core_t *core, int16_t width, int16_t height);
/** Suspend rendering and timers while keeping the scene tree intact. */
void egui_core_suspend(egui_core_t *core);
/** Resume rendering, mark everything dirty, and restart refresh timing. */
void egui_core_resume(egui_core_t *core);
/** Return non-zero when the core is suspended. */
int egui_core_is_suspended(egui_core_t *core);

/** Notify the PFB manager that one asynchronous flush finished. */
void egui_pfb_notify_flush_complete(egui_core_t *core);
/** Acquire exclusive access to the display bus for a PFB submission sequence. */
void egui_pfb_bus_acquire(egui_core_t *core);
/** Release the display bus previously acquired by egui_pfb_bus_acquire(). */
void egui_pfb_bus_release(egui_core_t *core);
/** Fill the whole physical screen with black using the current PFB configuration. */
void egui_core_clear_screen(egui_core_t *core);
/** Turn off the display and suspend UI refresh work. */
void egui_screen_off(egui_core_t *core);
/** Turn on the display, clear stale pixels, and redraw the current scene. */
void egui_screen_on(egui_core_t *core);

/** Render all pending dirty regions immediately in polling mode. */
void egui_polling_refresh_display(egui_core_t *core);
/** Return non-zero when at least one dirty region still needs rendering. */
int egui_check_need_refresh(egui_core_t *core);
/** Run per-frame pre-work such as scroll updates and layout calculation. */
void egui_core_draw_view_group_pre_work(egui_core_t *core);
/** Poll timers and input devices once. Call this from the main loop in polling ports. */
void egui_polling_work(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_H_ */
