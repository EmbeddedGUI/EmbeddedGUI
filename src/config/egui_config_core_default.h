#ifndef _EGUI_CONFIG_CORE_DEFAULT_H_
#define _EGUI_CONFIG_CORE_DEFAULT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Subject / observer ---- */

/**
 * Subject-Observer data binding.
 * When disabled, no data is added to existing structs.
 */
#ifndef EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER
#define EGUI_CONFIG_FUNCTION_SUBJECT_OBSERVER 0
#endif

/**
 * Maximum number of egui_observer_t pointers one egui_subject_t can hold.
 * Observers are stored in a fixed array inside egui_subject_t.
 */
#ifndef EGUI_CONFIG_SUBJECT_MAX_OBSERVERS
#define EGUI_CONFIG_SUBJECT_MAX_OBSERVERS 4
#endif

/* ---- Event listeners ---- */

/**
 * Lightweight event listener support.
 * When enabled, each view gains a small fixed listener table.
 */
#ifndef EGUI_CONFIG_FUNCTION_EVENT_LITE
#define EGUI_CONFIG_FUNCTION_EVENT_LITE 0
#endif

/**
 * Maximum event listeners that can be registered on one view.
 */
#ifndef EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW
#define EGUI_CONFIG_EVENT_MAX_LISTENERS_PER_VIEW 4
#endif

/* ---- Focus group ---- */

/**
 * Explicit focus group helper for encoder/key UIs that want a small caller-owned
 * focus ring instead of full-tree traversal.
 */
#ifndef EGUI_CONFIG_FUNCTION_FOCUS_GROUP
#define EGUI_CONFIG_FUNCTION_FOCUS_GROUP 0
#endif

/**
 * Maximum number of views in one focus group.
 */
#ifndef EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS
#define EGUI_CONFIG_FOCUS_GROUP_MAX_VIEWS 8
#endif

/* ---- Rotary encoder ---- */

/**
 * Rotary encoder driver support.
 * When enabled, egui_encoder_polling_work() translates encoder ticks and button
 * presses into key events.
 */
#ifndef EGUI_CONFIG_FUNCTION_ENCODER
#define EGUI_CONFIG_FUNCTION_ENCODER 0
#endif

/**
 * Encoder long-press threshold in milliseconds.
 */
#ifndef EGUI_CONFIG_ENCODER_PRESS_LONG_MS
#define EGUI_CONFIG_ENCODER_PRESS_LONG_MS 500
#endif

/* Encoder requires key support. */
#if EGUI_CONFIG_FUNCTION_ENCODER
#undef EGUI_CONFIG_FUNCTION_SUPPORT_KEY
#define EGUI_CONFIG_FUNCTION_SUPPORT_KEY 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CONFIG_CORE_DEFAULT_H_ */
