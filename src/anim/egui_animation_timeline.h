#ifndef _EGUI_ANIMATION_TIMELINE_H_
#define _EGUI_ANIMATION_TIMELINE_H_

#include "egui_animation.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY

/**
 * @brief Animation timeline entry: one animation and its start-time offset.
 */
typedef struct egui_animation_timeline_entry egui_animation_timeline_entry_t;
struct egui_animation_timeline_entry
{
    egui_animation_t *anim; /**< Pointer to the animation to control. NULL entries are skipped. */
    uint16_t start_ms;      /**< Milliseconds after timeline_start() at which this animation begins. */
};

/**
 * @brief Animation timeline that sequences multiple animations with individual start times.
 *
 * Usage:
 *   1. Call egui_animation_timeline_init(&tl).
 *   2. Call egui_animation_timeline_add(&tl, anim, start_ms) for each animation.
 *   3. Call egui_animation_timeline_start(&tl) to start all animations at once,
 *      each delayed by its start_ms offset using the egui_animation_set_delay() mechanism.
 *   4. Optionally call egui_animation_timeline_stop(&tl) to abort all pending entries.
 *
 * Each animation is independently managed: its delay is set to start_ms and
 * egui_animation_start() is called. The existing animation system handles the
 * rest without requiring a separate timer.
 */
typedef struct egui_animation_timeline egui_animation_timeline_t;
struct egui_animation_timeline
{
    egui_animation_timeline_entry_t entries[EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES];
    uint8_t count; /**< Number of valid entries (0..EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES). */
};

/** Initialize a timeline. Must be called before any other timeline operation. */
void egui_animation_timeline_init(egui_animation_timeline_t *self);

/**
 * @brief Append an animation and its start-time offset to the timeline.
 *
 * @param self      The timeline.
 * @param anim      The animation to add. Must be initialized by the caller.
 * @param start_ms  Milliseconds after egui_animation_timeline_start() at which
 *                  this animation should begin (0 = start immediately).
 *
 * Does nothing if the timeline is full (EGUI_CONFIG_ANIM_TIMELINE_MAX_ENTRIES reached).
 */
void egui_animation_timeline_add(egui_animation_timeline_t *self, egui_animation_t *anim, uint16_t start_ms);

/**
 * @brief Start all animations in the timeline.
 *
 * For each entry, sets delay = start_ms and calls egui_animation_start().
 * The animation system then fires each animation after its delay expires.
 */
void egui_animation_timeline_start(egui_animation_timeline_t *self);

/**
 * @brief Stop all animations in the timeline.
 *
 * Calls egui_animation_stop() on every non-NULL entry.
 */
void egui_animation_timeline_stop(egui_animation_timeline_t *self);

/**
 * @brief Return the total duration of the timeline in milliseconds.
 *
 * Computed as max(start_ms + anim->duration) across all entries.
 * Returns 0 when the timeline is empty.
 */
uint32_t egui_animation_timeline_get_duration(egui_animation_timeline_t *self);

#endif /* EGUI_CONFIG_FUNCTION_ANIM_DELAY */

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_TIMELINE_H_ */
