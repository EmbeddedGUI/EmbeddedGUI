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

/** Attach one top-level view to the framework root group so it participates in layout, input, and rendering. */
void egui_core_add_root_view(egui_core_t *core, egui_view_t *view);
/** Remove one view from the user root group without touching the framework-owned root group. */
void egui_core_remove_user_root_view(egui_core_t *core, egui_view_t *view);
/** Re-layout the user root group's direct children using one axis direction and alignment mode. */
void egui_core_layout_childs_user_root_view(egui_core_t *core, uint8_t is_orientation_horizontal, uint8_t align_type);
/** Override the PFB tile scan direction for the next refresh pass. */
void egui_core_set_pfb_scan_direction(egui_core_t *core, uint8_t reverse_x, uint8_t reverse_y);
/** Restore the default left-to-right, top-to-bottom PFB tile scan order. */
void egui_core_reset_pfb_scan_direction(egui_core_t *core);
/** Return whether tile scanning is currently reversed on the X axis. */
uint8_t egui_core_get_pfb_scan_reverse_x(egui_core_t *core);
/** Return whether tile scanning is currently reversed on the Y axis. */
uint8_t egui_core_get_pfb_scan_reverse_y(egui_core_t *core);
/** Flush the current PFB tile to the display driver, applying runtime software rotation when required. */
void egui_core_draw_data(egui_core_t *core, egui_region_t *p_region);
/** Adjust one logical PFB probe tile shape so probing matches the current logical display orientation. */
void egui_core_apply_logical_pfb_probe_shape(egui_core_t *core, egui_dim_t *pfb_width, egui_dim_t *pfb_height, uint32_t pfb_total_pixel_count);
/** Draw debug outlines and overlays into the current tile before it is presented. */
void egui_core_render_tile_debug_decorate(egui_core_t *core, const egui_region_t *p_region_dirty, int is_debug_mode);
/** Present one rendered tile and optionally slow down refresh when visual debug stepping is enabled. */
void egui_core_render_tile_present(egui_core_t *core, egui_region_t *region, int is_debug_mode);
/** Return the frame index that dirty-region logs should attribute to the next refresh. */
uint32_t egui_core_debug_next_frame_index(egui_core_t *core);
/** Emit one dirty-region trace line when dirty-region detail logging is enabled. */
void egui_core_debug_log_region_line(const char *tag, uint32_t frame_index, int slot, const egui_region_t *region);
/** Begin collecting dirty-region statistics for the current frame. */
void egui_core_dirty_region_stats_begin_frame(egui_core_t *core);
/** Count one PFB tile while dirty-region statistics collection is active. */
void egui_core_dirty_region_stats_count_tile(egui_core_t *core, const egui_region_t *region);
/** Finish the current dirty-region statistics frame and print the collected summary. */
void egui_core_dirty_region_stats_end_frame(egui_core_t *core);
/** Return non-zero when at least one dirty region still needs to be refreshed. */
int egui_check_need_refresh(egui_core_t *core);
/** Recompute scroll offsets and layout for the root view tree before rendering starts. */
void egui_core_draw_view_group_pre_work(egui_core_t *core);
/** Advance all active animations using the current core timestamp. */
void egui_core_animation_polling_work(egui_core_t *core);
/** Periodic refresh timer callback that renders one frame and re-arms itself to honor the FPS limit. */
void egui_refresh_timer_callback(egui_timer_t *timer);
#if EGUI_DEBUG_MONITOR_SHOW
/** Initialize one debug overlay slot with alignment and screen offsets. */
void egui_debug_overlay_init(egui_debug_overlay_t *overlay, uint8_t align_type, egui_dim_t offset_x, egui_dim_t offset_y);
/** Resolve the on-screen bounds occupied by one debug overlay string. */
int egui_debug_get_overlay_region(egui_core_t *core, const egui_debug_overlay_t *overlay, egui_region_t *region);
/** Replace one overlay's text and report whether the visible contents changed. */
uint8_t egui_debug_overlay_set_text(egui_debug_overlay_t *overlay, const char *next_text);
/** Mark the union of the old and new overlay bounds dirty so the monitor redraws cleanly. */
void egui_debug_mark_overlay_dirty(egui_core_t *core, egui_debug_overlay_t *overlay);
/** Draw enabled debug overlays that overlap the current PFB tile. */
void egui_debug_draw_overlays_for_current_pfb(egui_core_t *core);
/** Commit overlay bounds after a frame so the next update can dirty the correct old region. */
void egui_debug_commit_overlay_regions(egui_core_t *core);
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
/** Initialize the FPS/CPU-time debug monitor. */
void egui_debug_perf_init_monitor(egui_core_t *core);
/** Refresh the FPS/CPU-time monitor text using the latest accumulated statistics. */
void egui_debug_perf_update_monitor(egui_core_t *core, uint32_t timestamp);
#endif
#if EGUI_CONFIG_DEBUG_MEM_MONITOR_SHOW
/** Initialize the memory-usage debug monitor. */
void egui_debug_mem_init_monitor(egui_core_t *core);
/** Refresh the memory-usage monitor text from the current allocator statistics. */
void egui_debug_mem_update_monitor(egui_core_t *core, uint32_t timestamp);
#endif
/** Initialize every enabled debug monitor after the display scene is ready. */
void egui_debug_init_monitors(egui_core_t *core);
/** Update every enabled debug monitor before deciding whether to refresh the frame. */
void egui_debug_update_monitors(egui_core_t *core, uint32_t timestamp);
#if EGUI_CONFIG_DEBUG_PERF_MONITOR_SHOW
/** Accumulate one frame's render and flush timing into the performance monitor window. */
void egui_debug_perf_record_work_time(egui_core_t *core, uint32_t render_time, uint32_t flush_time, uint32_t timestamp);
#endif
#endif
/** Add one animation object to the core's active animation list. */
void egui_core_animation_append(egui_core_t *core, egui_animation_t *anim);
/** Remove one animation object from the core's active animation list. */
void egui_core_animation_remove(egui_core_t *core, egui_animation_t *anim);
/** Initialize scene-level containers, transition pointers, and dirty regions for one display. */
void egui_core_init_display_scene(egui_core_t *core, int16_t screen_w, int16_t screen_h);
/** Register platform, display, touch, and generated UI hooks, then power on the screen. */
void egui_core_setup_display_start(egui_core_t *core, const egui_display_setup_t *setup);
/** Weak port hook called when one pending refresh frame starts rendering. */
void egui_port_notify_frame_render_start(void);
/** Weak port hook called after one frame has been fully rendered and presented. */
void egui_port_notify_frame_render_complete(void);
/** Weak port hook that can defer one pending refresh frame without clearing dirty regions. */
int egui_port_should_defer_refresh(egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CORE_INTERNAL_H_ */
