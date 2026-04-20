#ifndef _EGUI_CORE_INTERNAL_H_
#define _EGUI_CORE_INTERNAL_H_

#include "egui_core.h"
#include "egui_core_activity.h"
#include "egui_core_dialog.h"
#include "egui_core_toast.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void egui_core_add_root_view(egui_core_t *core, egui_view_t *view);
void egui_core_remove_user_root_view(egui_core_t *core, egui_view_t *view);
void egui_core_layout_childs_user_root_view(egui_core_t *core, uint8_t is_orientation_horizontal, uint8_t align_type);
void egui_core_set_pfb_scan_direction(egui_core_t *core, uint8_t reverse_x, uint8_t reverse_y);
void egui_core_reset_pfb_scan_direction(egui_core_t *core);
uint8_t egui_core_get_pfb_scan_reverse_x(egui_core_t *core);
uint8_t egui_core_get_pfb_scan_reverse_y(egui_core_t *core);
void egui_core_draw_data(egui_core_t *core, egui_region_t *p_region);
void egui_core_apply_logical_pfb_probe_shape(egui_core_t *core, egui_dim_t *pfb_width, egui_dim_t *pfb_height, uint32_t pfb_total_pixel_count);
void egui_core_render_tile_debug_decorate(egui_core_t *core, const egui_region_t *p_region_dirty, int is_debug_mode);
void egui_core_render_tile_present(egui_core_t *core, egui_region_t *region, int is_debug_mode);
uint32_t egui_core_debug_next_frame_index(egui_core_t *core);
void egui_core_debug_log_region_line(const char *tag, uint32_t frame_index, int slot, const egui_region_t *region);
void egui_core_dirty_region_stats_begin_frame(egui_core_t *core);
void egui_core_dirty_region_stats_count_tile(egui_core_t *core, const egui_region_t *region);
void egui_core_dirty_region_stats_end_frame(egui_core_t *core);
int egui_check_need_refresh(egui_core_t *core);
void egui_core_draw_view_group_pre_work(egui_core_t *core);
void egui_core_animation_polling_work(egui_core_t *core);
void egui_refresh_timer_callback(egui_timer_t *timer);
#if EGUI_DEBUG_MONITOR_SHOW
void egui_debug_overlay_init(egui_debug_overlay_t *overlay, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y);
int egui_debug_get_overlay_region(egui_core_t *core, const egui_debug_overlay_t *overlay, egui_region_t *region);
uint8_t egui_debug_overlay_set_text(egui_debug_overlay_t *overlay, const char *next_text);
void egui_debug_mark_overlay_dirty(egui_core_t *core, egui_debug_overlay_t *overlay);
void egui_debug_draw_overlays_for_current_pfb(egui_core_t *core);
void egui_debug_commit_overlay_regions(egui_core_t *core);
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
void egui_debug_perf_init_monitor(egui_core_t *core);
void egui_debug_perf_update_monitor(egui_core_t *core, uint32_t timestamp);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
void egui_debug_mem_init_monitor(egui_core_t *core);
void egui_debug_mem_update_monitor(egui_core_t *core, uint32_t timestamp);
#endif
void egui_debug_init_monitors(egui_core_t *core);
void egui_debug_update_monitors(egui_core_t *core, uint32_t timestamp);
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
void egui_debug_perf_record_work_time(egui_core_t *core, uint32_t render_time, uint32_t flush_time, uint32_t timestamp);
#endif
#endif
void egui_core_animation_append(egui_core_t *core, egui_animation_t *anim);
void egui_core_animation_remove(egui_core_t *core, egui_animation_t *anim);
void egui_core_init_display_scene(egui_core_t *core, int16_t screen_w, int16_t screen_h);
void egui_core_setup_display_start(egui_core_t *core, const egui_display_setup_t *setup);
void egui_port_notify_frame_render_complete(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_INTERNAL_H_ */
