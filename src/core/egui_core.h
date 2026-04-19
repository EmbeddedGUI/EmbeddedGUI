#ifndef _EGUI_CORE_H_
#define _EGUI_CORE_H_

#include "egui_common.h"
#include "egui_motion_event.h"
#include "egui_pfb_manager.h"
#include "egui_canvas.h"
#include "egui_display_driver.h"
#include "egui_touch_driver.h"
#include "egui_platform.h"
#include "egui_input.h"
#include "egui_timer.h"
#include "egui_canvas_transform_cache.h"
#include "egui_image_decode_cache.h"
#include "image/egui_image_std.h"
#include "font/egui_font_std.h"
#include "resource/egui_i18n.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "egui_focus.h"
#endif
#include "widget/egui_view_group.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "app/egui_dialog.h"
#include "app/egui_toast.h"

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

typedef struct egui_image_file_io egui_image_file_io_t;
typedef struct egui_image_file_decoder egui_image_file_decoder_t;

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
#endif

    egui_core_system_state_t system; // refresh/timer/focus owned state
    uint8_t id;                      // core instance id
    void *user_data;                 // user-defined data
    egui_core_asset_state_t asset;   // theme/i18n/image registry state
    egui_core_debug_state_t debug;   // debug/monitor owned state
    egui_core_image_state_t image;   // image/cache owned state
    egui_core_text_state_t text;     // font/text-transform owned state
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
 */
void egui_init_display(egui_core_t *core, int16_t screen_w, int16_t screen_h, egui_color_int_t **pfb_bufs, int buf_count, int pfb_w, int pfb_h);

typedef void (*egui_uicode_init_func_t)(egui_core_t *core);
typedef void (*egui_touch_register_func_t)(egui_core_t *core);

typedef struct egui_display_setup
{
    int screen_width;
    int screen_height;
    int pfb_width;
    int pfb_height;
    egui_color_int_t **pfb_buffers;
    int pfb_buffer_count;
    egui_display_driver_t *display_driver;
    egui_platform_t *platform;
    egui_touch_register_func_t touch_register;
    egui_uicode_init_func_t uicode_init;
    uint8_t display_id;
} egui_display_setup_t;

void egui_setup_display(egui_core_t *core, const egui_display_setup_t *setup);

void egui_core_force_refresh(egui_core_t *core);
egui_view_group_t *egui_core_get_root_view(egui_core_t *core);
int egui_core_check_region_dirty_intersect(egui_core_t *core, egui_region_t *region_dirty);
void egui_core_update_region_dirty(egui_core_t *core, egui_region_t *region_dirty);
void egui_core_update_region_dirty_all(egui_core_t *core);
void egui_core_clear_region_dirty(egui_core_t *core);
egui_view_group_t *egui_core_get_user_root_view(egui_core_t *core);
void egui_core_add_user_root_view(egui_view_t *view);
egui_region_t *egui_core_get_region_dirty_arr(egui_core_t *core);
uint32_t egui_core_get_dirty_epoch(egui_core_t *core);
void egui_core_draw_view_group(egui_core_t *core, egui_region_t *p_region_dirty, int is_debug_mode);
egui_dim_t egui_core_get_logical_pfb_target_width_hint(egui_core_t *core);
void egui_core_process_input_motion(egui_core_t *core, egui_motion_event_t *motion_event);
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
void egui_core_process_input_key(egui_core_t *core, egui_key_event_t *key_event);
#endif
#if EGUI_CONFIG_DEBUG_VIEW_ID
uint16_t egui_core_get_unique_id(egui_core_t *core);
#endif
void egui_core_refresh_screen(egui_core_t *core);
void egui_core_stop_auto_refresh_screen(egui_core_t *core);
egui_color_int_t *egui_core_get_pfb_buffer_ptr(egui_core_t *core);
void egui_core_pfb_set_buffer(egui_core_t *core, egui_color_int_t *pfb, uint16_t width, uint16_t height);
void egui_core_power_off(egui_core_t *core);
void egui_core_power_on(egui_core_t *core);
void egui_core_set_screen_size(egui_core_t *core, int16_t width, int16_t height);
void egui_core_suspend(egui_core_t *core);
void egui_core_resume(egui_core_t *core);
int egui_core_is_suspended(egui_core_t *core);

void egui_pfb_notify_flush_complete(egui_core_t *core);
void egui_pfb_bus_acquire(egui_core_t *core);
void egui_pfb_bus_release(egui_core_t *core);
void egui_core_clear_screen(egui_core_t *core);
void egui_screen_off(egui_core_t *core);
void egui_screen_on(egui_core_t *core);

void egui_polling_refresh_display(egui_core_t *core);
int egui_check_need_refresh(egui_core_t *core);
void egui_core_draw_view_group_pre_work(egui_core_t *core);
void egui_polling_work(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_H_ */
