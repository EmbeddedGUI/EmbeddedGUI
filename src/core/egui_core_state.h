#ifndef _EGUI_CORE_STATE_H_
#define _EGUI_CORE_STATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
/** Recorded raw touch path used by the optional touch-trace debug overlay. */
typedef struct egui_core_touch_trace_record
{
    uint16_t point_count;                                             // number of valid points currently stored in `points`
    uint8_t active;                                                   // non-zero while a trace is being collected for the current gesture
    egui_region_t bounds;                                             // bounding box of the recorded gesture path
    egui_location_t points[EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS]; // sampled gesture points in screen coordinates
} egui_core_touch_trace_record_t;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Per-core touch input state, including queueing, capture snapshots, and HAL bridging. */
typedef struct egui_core_touch_state
{
    egui_input_t input;                                            // queued motion/key state owned by the touch subsystem
    egui_touch_driver_t *driver;                                   // currently registered touch driver for this core
    egui_view_group_touch_state_snapshot_t view_group_touch_state; // snapshot of capture/press state used during dispatch
    uint8_t prev_pressed;                                          // previous single-touch pressed flag for edge detection
    int16_t prev_x;                                                // previous single-touch x position
    int16_t prev_y;                                                // previous single-touch y position
    uint8_t prev_point_count;                                      // previous touch point count for edge detection
    uint8_t prev_ids[EGUI_TOUCH_DRIVER_MAX_POINTS];                // previous touch point IDs
    egui_location_t prev_locations[EGUI_TOUCH_DRIVER_MAX_POINTS];  // previous touch positions
    egui_touch_driver_t hal_bridge_driver;                         // touch driver that wraps the HAL entry points
    egui_touch_driver_ops_t hal_bridge_ops;                        // callback table exposed by the HAL bridge
    void *hal_bridge_hal_driver;                                   // opaque HAL touch-driver pointer
    uint8_t hal_touch_last_pressed;                                // last pressed flag observed through the HAL bridge
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_trace_record_t trace_record; // optional gesture path recording for debug visualization
#endif
} egui_core_touch_state_t;
#endif

/** Theme, locale, and image-decoder resources that are shared across the current core. */
typedef struct egui_core_asset_state
{
    const egui_theme_t *theme_current;      // theme currently applied to views created on this core
    const egui_i18n_locale_t *i18n_locales; // locale table registered for runtime translation lookups
    uint16_t i18n_locale_count;             // number of entries in `i18n_locales`
    uint16_t i18n_current_locale;           // active locale index inside `i18n_locales`
#if EGUI_CONFIG_FUNCTION_IMAGE_FILE
    const egui_image_file_io_t *image_file_default_io;                                              // default file I/O backend used by image-file decoders
    const egui_image_file_decoder_t *image_file_decoders[EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT]; // registered file-decoder chain
    uint8_t image_file_decoder_count;                                                               // number of valid entries in `image_file_decoders`
#endif
} egui_core_asset_state_t;

/** Image decode caches and scratch buffers reused across all image draws on one core. */
typedef struct egui_core_image_state
{
    uint16_t *image_std_shared_external_data_cache;        // shared external-resource pixel row cache for std images
    uint8_t *image_std_shared_external_alpha_cache;        // shared external-resource alpha row cache for std images
    uint8_t image_std_shared_external_cache_owner;         // owner slot id for the shared external caches
    uint32_t image_std_shared_external_cache_generation;   // generation counter used to invalidate shared cache contents
    uint8_t image_std_shared_external_cache_used_in_frame; // whether the shared external cache was touched in the current frame
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_row_persistent_cache_storage_t image_std_external_row_persistent_cache_storage; // row cache storage for external std images
#endif
#if EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES > 0
    egui_image_std_external_persistent_cache_t image_std_external_persistent_cache; // optional persistent decoded-image cache
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    egui_image_std_alpha_opaque_cache_t image_std_alpha_opaque_cache[EGUI_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS]; // lookup cache for opaque-alpha runs
    uint8_t image_std_alpha_opaque_cache_next; // round-robin replacement index for the opaque-alpha cache
#endif
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    image_transform_external_data_row_slot_t image_transform_external_data_row_cache;   // transformed external-image pixel row cache
    image_transform_external_alpha_row_slot_t image_transform_external_alpha_row_cache; // transformed external-image alpha row cache
#endif
    egui_image_decode_cache_storage_t image_decode_cache; // generic row/full-image decode scratch shared by codecs
#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_QOI
    egui_image_qoi_cache_t image_qoi_cache; // QOI decoder incremental state and checkpoints
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_CODEC_RLE
    egui_image_rle_cache_t image_rle_cache; // RLE decoder incremental state and external window cache
#endif
} egui_core_image_state_t;

/** Debug-only counters and overlays that track how one core renders. */
typedef struct egui_core_debug_state
{
    uint8_t reserved; // keeps the struct non-empty when all debug features are disabled
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    egui_dirty_region_stats_t dirty_region_stats; // aggregated dirty-region statistics for logging
#endif
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_stats_t debug_perf_stats; // rolling counters for FPS, render time, and flush time
    egui_debug_overlay_t debug_perf_overlay;  // on-screen performance monitor overlay state
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_overlay_t debug_mem_overlay; // on-screen memory monitor overlay state
    size_t mem_monitor_used_size;           // last sampled heap usage reported by the allocator
    size_t mem_monitor_peak_size;           // last sampled heap peak reported by the allocator
#endif
} egui_core_debug_state_t;

/** Text measurement, glyph lookup, and transformed-text caches shared across one core. */
typedef struct egui_core_text_state
{
#if EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
    egui_font_std_code_lookup_cache_t font_std_code_lookup_cache; // fast codepoint-to-glyph lookup cache
#endif
#if EGUI_CONFIG_FUNCTION_FONT_STD_FAST_DRAW
    egui_font_std_ascii_lookup_cache_t *font_std_ascii_lookup_cache; // optional heap-backed ASCII fast-draw lookup table
#endif
#if EGUI_FONT_STD_BITMAP_COMPRESSED_ENABLED
    egui_font_std_external_draw_scratch_t font_std_compressed_draw_scratch; // scratch buffers used while drawing compressed bitmaps
#endif
#if EGUI_FONT_STD_COMPRESSED_GLYPH_CACHE_ENABLED
    egui_font_std_compressed_glyph_cache_t
            font_std_compressed_glyph_cache[EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_SLOTS]; // cached decompressed glyph bitmaps
    uint32_t font_std_compressed_glyph_cache_stamp;                                             // monotonically increasing stamp for glyph cache replacement
    uint32_t font_std_compressed_glyph_cache_total_bytes;                                       // total heap usage currently held by the glyph cache
#endif
#if EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED
    egui_font_std_draw_prefix_cache_t font_std_draw_prefix_cache[EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS]; // cached prefix layout metrics
    uint32_t font_std_draw_prefix_cache_stamp;                                                                  // replacement stamp for prefix cache entries
#endif
#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW
    text_transform_prepare_cache_t text_transform_prepare_cache; // cache for transformed-text preprocessing
#endif
#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW
    text_transform_layout_cache_t text_transform_layout_cache; // cache for transformed-text layout results
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
    text_transform_layout_glyph_t *text_transform_layout_glyphs; // heap array holding transformed glyph layout output
    int text_transform_layout_capacity;                          // number of glyph slots allocated in `text_transform_layout_glyphs`
    text_transform_layout_line_t *text_transform_layout_lines;   // heap array holding transformed line layout output
    int text_transform_layout_line_capacity;                     // number of line slots allocated in `text_transform_layout_lines`
#endif
    text_transform_visible_tile_cache_t text_transform_visible_tile_cache; // visible-tile cache for rotated/scaled text drawing
#if EGUI_CONFIG_FUNCTION_FONT_TRANSFORM_FAST_DRAW
    const egui_font_t *text_transform_axis_font; // font used by the cached axis text metrics
    const void *text_transform_axis_string;      // string identity used by the cached axis text metrics
    egui_dim_t text_transform_axis_w;            // cached axis-aligned text width
    egui_dim_t text_transform_axis_h;            // cached axis-aligned text height
    const egui_font_t *text_transform_dim_font;  // font used by the cached transformed-dimension metrics
    const void *text_transform_dim_string;       // string identity used by the cached transformed-dimension metrics
    int16_t text_transform_dim_w;                // cached transformed text width
    int16_t text_transform_dim_h;                // cached transformed text height
#endif
} egui_core_text_state_t;

/** Core-wide timer, suspend, and focus state not tied to one scene or render pass. */
typedef struct egui_core_system_state
{
    egui_timer_t refresh_timer; /* periodic timer that drives frame refresh scheduling */
    egui_timer_t *timer_root;   /* head of the sorted timer queue managed by the core */
    uint8_t is_suspended;       /* non-zero when rendering/input work should stay paused */
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_focus_manager_t focus_manager; /* currently focused view inside this core */
#endif
#if EGUI_CONFIG_FUNCTION_ENCODER
    egui_encoder_driver_t *encoder_driver; /* registered rotary encoder driver, or NULL */
#endif
} egui_core_system_state_t;

/** Scene graph state, transition bookkeeping, and dirty regions for the active display. */
typedef struct egui_core_scene_state
{
    egui_slist_t anims; // currently running animation objects
#if EGUI_CONFIG_FUNCTION_SUPPORT_ACTIVITY
    egui_dlist_t activitys;                            // activity stack/list managed by the scene subsystem
    egui_animation_t *activity_anim_start_open;        // opening transition animation kicked off when a new activity starts
    egui_animation_t *activity_anim_start_close;       // closing transition animation kicked off for the previous activity
    egui_animation_t *activity_anim_finish_open;       // follow-up animation that finalizes an activity open transition
    egui_animation_t *activity_anim_finish_close;      // follow-up animation that finalizes an activity close transition
    egui_activity_t *activity_anim_start_open_owner;   // owner activity for `activity_anim_start_open`
    egui_activity_t *activity_anim_start_close_owner;  // owner activity for `activity_anim_start_close`
    egui_activity_t *activity_anim_finish_open_owner;  // owner activity for `activity_anim_finish_open`
    egui_activity_t *activity_anim_finish_close_owner; // owner activity for `activity_anim_finish_close`
    egui_activity_t *activity_open;                    // activity currently entering the foreground
    egui_activity_t *activity_close;                   // activity currently leaving the foreground
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_DIALOG
    egui_animation_t *dialog_anim_start;     // dialog show animation in progress
    egui_animation_t *dialog_anim_finish;    // dialog hide/finish animation in progress
    egui_dialog_t *dialog_anim_start_owner;  // owner dialog for `dialog_anim_start`
    egui_dialog_t *dialog_anim_finish_owner; // owner dialog for `dialog_anim_finish`
    egui_dialog_t *dialog;                   // dialog currently attached to the scene, if any
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOAST
    egui_toast_t *toast; // toast currently attached to the scene, if any
#endif
    egui_view_root_group_t root_view_group; // framework-owned root container at the top of the view tree
#if EGUI_CONFIG_CORE_SEPARATE_USER_ROOT_GROUP_ENABLE
    egui_view_root_group_t user_root_view_group; // user-facing root container added beneath the framework root
#endif
    egui_region_t region_dirty_arr[EGUI_CONFIG_DIRTY_AREA_COUNT]; // merged dirty rectangles waiting for refresh
    uint32_t dirty_epoch;                                         // monotonic dirty-state stamp used to detect repeated updates in one frame
} egui_core_scene_state_t;

/** Render backend state: PFB manager, bound drivers, and runtime display transforms. */
typedef struct egui_core_render_state
{
    egui_pfb_manager_t pfb_mgr;         // partial-framebuffer manager and buffer ring state
    egui_display_driver_t *driver;      // registered display driver implementation
    uint8_t color_16_swap;              // non-zero when RGB565 output bytes must be swapped before flush
    uint8_t software_rotation;          // non-zero when runtime rotation is handled in software instead of hardware
    uint8_t rotation_scratch_owned;     // non-zero when rotation_scratch was allocated by the core and must be freed by the core
    egui_color_int_t *rotation_scratch; // temporary tile buffer used while rotating PFB data in software
} egui_core_render_state_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_STATE_H_ */
