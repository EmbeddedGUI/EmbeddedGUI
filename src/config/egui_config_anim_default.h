#ifndef _EGUI_CONFIG_ANIM_DEFAULT_H_
#define _EGUI_CONFIG_ANIM_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Animation feature switches ---- */

/**
 * Animation delay.
 * When enabled, egui_animation_t gains a delay_ms field.
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_DELAY
#define EGUI_CONFIG_FUNCTION_ANIM_DELAY 0
#endif

/**
 * Animation on-complete callback.
 * When enabled, egui_animation_t gains on_complete_cb + on_complete_user_data.
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB
#define EGUI_CONFIG_FUNCTION_ANIM_COMPLETE_CB 0
#endif

/**
 * Animation pause/resume.
 * When enabled, egui_animation_t gains is_paused + pause_elapsed fields.
 */
#ifndef EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME
#define EGUI_CONFIG_FUNCTION_ANIM_PAUSE_RESUME 0
#endif

/**
 * Animation timeline entry capacity.
 * Used by egui_animation_timeline_t when EGUI_CONFIG_FUNCTION_ANIM_DELAY is enabled.
 */
#ifndef EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES
#define EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES 8
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_ANIM_DEFAULT_H_ */
