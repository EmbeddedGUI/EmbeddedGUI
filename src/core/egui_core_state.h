#ifndef _EGUI_CORE_STATE_H_
#define _EGUI_CORE_STATE_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
typedef struct egui_core_touch_trace_record
{
    uint16_t point_count;
    uint8_t active;
    egui_region_t bounds;
    egui_location_t points[EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS];
} egui_core_touch_trace_record_t;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct egui_core_touch_state
{
    egui_input_t input;
    egui_touch_driver_t *driver;
    egui_view_group_touch_state_snapshot_t view_group_touch_state;
    uint8_t prev_pressed;
    int16_t prev_x;
    int16_t prev_y;
    egui_touch_driver_t hal_bridge_driver;
    egui_touch_driver_ops_t hal_bridge_ops;
    void *hal_bridge_hal_driver;
    uint8_t hal_touch_last_position_valid;
    uint8_t hal_touch_last_pressed;
    int16_t hal_touch_last_x;
    int16_t hal_touch_last_y;
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_trace_record_t trace_record;
#endif
} egui_core_touch_state_t;
#endif

typedef struct egui_core_asset_state
{
    const egui_theme_t *theme_current;
    const egui_i18n_locale_t *i18n_locales;
    uint16_t i18n_locale_count;
    uint16_t i18n_current_locale;
    const egui_image_file_io_t *image_file_default_io;
    const egui_image_file_decoder_t *image_file_decoders[EGUI_CONFIG_IMAGE_FILE_DECODER_MAX_COUNT];
    uint8_t image_file_decoder_count;
} egui_core_asset_state_t;

typedef struct egui_core_image_state
{
    uint16_t *image_std_shared_external_data_cache;
    uint8_t *image_std_shared_external_alpha_cache;
    uint8_t image_std_shared_external_cache_owner;
    uint32_t image_std_shared_external_cache_generation;
    uint8_t image_std_shared_external_cache_used_in_frame;
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    egui_image_std_external_row_persistent_cache_storage_t image_std_external_row_persistent_cache_storage;
#endif
#if EGUI_CONFIG_IMAGE_EXTERNAL_PERSISTENT_CACHE_MAX_BYTES > 0
    egui_image_std_external_persistent_cache_t image_std_external_persistent_cache;
#endif
#if EGUI_CONFIG_FUNCTION_IMAGE_FORMAT_RGB565
    egui_image_std_alpha_opaque_cache_t image_std_alpha_opaque_cache[EGUI_IMAGE_STD_ALPHA_OPAQUE_CACHE_SLOTS];
    uint8_t image_std_alpha_opaque_cache_next;
#endif
#if EGUI_CONFIG_FUNCTION_EXTERNAL_RESOURCE
    image_transform_external_data_row_slot_t image_transform_external_data_row_cache;
    image_transform_external_alpha_row_slot_t image_transform_external_alpha_row_cache;
#endif
    egui_image_decode_cache_storage_t image_decode_cache;
#if EGUI_CONFIG_IMAGE_CODEC_QOI_ENABLE
    egui_image_qoi_cache_t image_qoi_cache;
#endif
#if EGUI_CONFIG_IMAGE_CODEC_RLE_ENABLE
    egui_image_rle_cache_t image_rle_cache;
#endif
} egui_core_image_state_t;

typedef struct egui_core_debug_state
{
    uint8_t reserved;
#if EGUI_CONFIG_DEBUG_DIRTY_REGION_STATS
    egui_dirty_region_stats_t dirty_region_stats;
#endif
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
    egui_debug_perf_stats_t debug_perf_stats;
    egui_debug_overlay_t debug_perf_overlay;
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
    egui_debug_overlay_t debug_mem_overlay;
    size_t mem_monitor_used_size;
    size_t mem_monitor_peak_size;
#endif
} egui_core_debug_state_t;

typedef struct egui_core_text_state
{
    egui_font_std_code_lookup_cache_t font_std_code_lookup_cache;
#if EGUI_CONFIG_FONT_STD_ASCII_LOOKUP_CACHE_ENABLE
    egui_font_std_ascii_lookup_cache_t *font_std_ascii_lookup_cache;
#endif
#if EGUI_FONT_STD_BITMAP_COMPRESSED_ENABLED
    egui_font_std_external_draw_scratch_t font_std_compressed_draw_scratch;
#endif
#if EGUI_FONT_STD_COMPRESSED_GLYPH_CACHE_ENABLED
    egui_font_std_compressed_glyph_cache_t font_std_compressed_glyph_cache[EGUI_CONFIG_FONT_STD_COMPRESSED_GLYPH_CACHE_SLOTS];
    uint32_t font_std_compressed_glyph_cache_stamp;
    uint32_t font_std_compressed_glyph_cache_total_bytes;
#endif
#if EGUI_FONT_STD_DRAW_PREFIX_CACHE_ENABLED
    egui_font_std_draw_prefix_cache_t font_std_draw_prefix_cache[EGUI_CONFIG_FONT_STD_DRAW_PREFIX_CACHE_SLOTS];
    uint32_t font_std_draw_prefix_cache_stamp;
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_PREPARE_CACHE_ENABLE
    text_transform_prepare_cache_t text_transform_prepare_cache;
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_CACHE_ENABLE
    text_transform_layout_cache_t text_transform_layout_cache;
#endif
#if EGUI_CONFIG_TEXT_TRANSFORM_LAYOUT_HEAP_ENABLE
    text_transform_layout_glyph_t *text_transform_layout_glyphs;
    int text_transform_layout_capacity;
    text_transform_layout_line_t *text_transform_layout_lines;
    int text_transform_layout_line_capacity;
#endif
    text_transform_visible_tile_cache_t text_transform_visible_tile_cache;
    const egui_font_t *text_transform_axis_font;
    const void *text_transform_axis_string;
    egui_dim_t text_transform_axis_w;
    egui_dim_t text_transform_axis_h;
    const egui_font_t *text_transform_dim_font;
    const void *text_transform_dim_string;
    int16_t text_transform_dim_w;
    int16_t text_transform_dim_h;
} egui_core_text_state_t;

typedef struct egui_core_system_state
{
    egui_timer_t refresh_timer;
    egui_timer_t *timer_root;
    uint8_t is_suspended;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_focus_manager_t focus_manager;
#endif
} egui_core_system_state_t;

typedef struct egui_core_scene_state
{
    egui_dlist_t activitys;
    egui_slist_t anims;
    egui_animation_t *activity_anim_start_open;
    egui_animation_t *activity_anim_start_close;
    egui_animation_t *activity_anim_finish_open;
    egui_animation_t *activity_anim_finish_close;
    egui_activity_t *activity_open;
    egui_activity_t *activity_close;
    egui_animation_t *dialog_anim_start;
    egui_animation_t *dialog_anim_finish;
    egui_dialog_t *dialog;
    egui_toast_t *toast;
    egui_view_root_group_t root_view_group;
    egui_view_root_group_t user_root_view_group;
    egui_region_t region_dirty_arr[EGUI_CONFIG_DIRTY_AREA_COUNT];
    uint32_t dirty_epoch;
} egui_core_scene_state_t;

typedef struct egui_core_render_state
{
    egui_pfb_manager_t pfb_mgr;
    egui_display_driver_t *driver;
    egui_platform_t *platform;
    uint8_t color_16_swap;
    uint8_t software_rotation;
    egui_color_int_t *rotation_scratch;
} egui_core_render_state_t;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_STATE_H_ */
