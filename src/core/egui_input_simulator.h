#ifndef _EGUI_INPUT_SIMULATOR_H_
#define _EGUI_INPUT_SIMULATOR_H_

/**
 * @file egui_input_simulator.h
 * @brief Input-simulation helpers used by runtime recording and screenshot tests.
 *
 * These helpers are typically
 * consumed from `egui_port_get_recording_action()`
 * to describe a deterministic sequence of clicks, drags, swipes, waits, and
 * targeted multi-display
 * interactions.
 */

#include "config/egui_config.h"
#include "egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High-level simulated interaction types consumed by the recording port.
 */
typedef enum
{
    EGUI_SIM_ACTION_NONE = 0, // terminator / no-op action
    EGUI_SIM_ACTION_CLICK,    // click or tap at one point
    EGUI_SIM_ACTION_DRAG,     // drag from one point to another over multiple steps
    EGUI_SIM_ACTION_SWIPE,    // fast drag with a fixed short step count
    EGUI_SIM_ACTION_WAIT,     // delay without generating pointer input
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    EGUI_SIM_ACTION_MULTI_DRAG, // N-finger drag gesture
#endif
} egui_sim_action_type_t;

/**
 * @brief One recorded or simulated action entry.
 *
 * Unused coordinates are ignored according to `type`. For single-display helpers, `display_id` defaults to `0`.
 */
typedef struct egui_sim_action
{
    egui_sim_action_type_t type; // enum egui_sim_action_type_t
    int x1, y1;                  // click position or drag/swipe start point
    int x2, y2;                  // drag/swipe end point
    int steps;                   // number of intermediate drag steps, `0` lets the port pick one
    int interval_ms;             // delay before this action begins
    int display_id;              // target display index, `0` selects the main/default display
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    uint8_t point_count;                             // number of valid points for `EGUI_SIM_ACTION_MULTI_DRAG`
    int point_start_x[EGUI_CONFIG_TOUCH_MAX_POINTS]; // start X coordinates for `EGUI_SIM_ACTION_MULTI_DRAG`
    int point_start_y[EGUI_CONFIG_TOUCH_MAX_POINTS]; // start Y coordinates for `EGUI_SIM_ACTION_MULTI_DRAG`
    int point_end_x[EGUI_CONFIG_TOUCH_MAX_POINTS];   // end X coordinates for `EGUI_SIM_ACTION_MULTI_DRAG`
    int point_end_y[EGUI_CONFIG_TOUCH_MAX_POINTS];   // end Y coordinates for `EGUI_SIM_ACTION_MULTI_DRAG`
#endif
} egui_sim_action_t;

/**
 * @brief Create one click or tap action for the default display.
 * @param _x Click x position
 * @param _y Click y position
 * @param _interval_ms
 * Interval before click (ms)
 */
#define EGUI_SIM_CLICK(_x, _y, _interval_ms) {EGUI_SIM_ACTION_CLICK, (_x), (_y), 0, 0, 0, (_interval_ms)}

/**
 * @brief Create one drag action for the default display.
 * @param _x1, _y1 Start position
 * @param _x2, _y2 End position
 * @param _steps Number of
 * intermediate steps (0 = auto calculate)
 * @param _interval_ms Interval before drag starts (ms)
 */
#define EGUI_SIM_DRAG(_x1, _y1, _x2, _y2, _steps, _interval_ms) {EGUI_SIM_ACTION_DRAG, (_x1), (_y1), (_x2), (_y2), (_steps), (_interval_ms)}

/**
 * @brief Create one swipe action for the default display.
 *
 * A swipe is encoded as a drag that always uses five intermediate steps.
 *
 * @param _x1,
 * _y1 Start position
 * @param _x2, _y2 End position
 * @param _interval_ms Interval before swipe (ms)
 */
#define EGUI_SIM_SWIPE(_x1, _y1, _x2, _y2, _interval_ms) {EGUI_SIM_ACTION_SWIPE, (_x1), (_y1), (_x2), (_y2), 5, (_interval_ms)}

/**
 * @brief Create one wait-only action for the default display.
 * @param _interval_ms Wait duration (ms)
 */
#define EGUI_SIM_WAIT(_interval_ms) {EGUI_SIM_ACTION_WAIT, 0, 0, 0, 0, 0, (_interval_ms)}

/**
 * @brief End marker for one action array returned by the recording hook.
 */
#define EGUI_SIM_END() {EGUI_SIM_ACTION_NONE, 0, 0, 0, 0, 0, 0}

/**
 * @brief Resolve the center point of one view in screen coordinates.
 * @param view Pointer to `egui_view_t` (or a derived type, which will be cast)
 *
 * @param p_x Pointer to store center x
 * @param p_y Pointer to store center y
 */
__EGUI_STATIC_INLINE__ void egui_sim_get_view_center(void *view, int *p_x, int *p_y)
{
    egui_view_t *v = (egui_view_t *)view;
    *p_x = v->region_screen.location.x + v->region_screen.size.width / 2;
    *p_y = v->region_screen.location.y + v->region_screen.size.height / 2;
}

/**
 * @brief Resolve a point inside one view using normalized relative coordinates.
 * @param view Pointer to `egui_view_t`
 * @param rel_x Relative x (0.0 = left, 0.5 = center, 1.0 = right)
 * @param rel_y Relative y (0.0 = top, 0.5 = center, 1.0 = bottom)
 * @param p_x Pointer to store absolute x
 * @param p_y
 * Pointer to store absolute y
 */
__EGUI_STATIC_INLINE__ void egui_sim_get_view_pos(void *view, float rel_x, float rel_y, int *p_x, int *p_y)
{
    egui_view_t *v = (egui_view_t *)view;
    *p_x = v->region_screen.location.x + (int)(v->region_screen.size.width * rel_x);
    *p_y = v->region_screen.location.y + (int)(v->region_screen.size.height * rel_y);
}

/**
 * @brief Compute the currently visible region of a view after optional clipping.
 * @param view Pointer to `egui_view_t`
 * @param clip_region Optional clip region in screen coordinates, `NULL` keeps the full view bounds
 * @param p_visible_region Pointer to store the visible region
 * @return 1 if the view
 * has a non-empty visible region, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_get_view_visible_region(void *view, const egui_region_t *clip_region, egui_region_t *p_visible_region)
{
    egui_view_t *v = (egui_view_t *)view;

    if (p_visible_region == NULL)
    {
        return 0;
    }
    egui_region_init_empty(p_visible_region);
    if (v == NULL || v->region_screen.size.width <= 0 || v->region_screen.size.height <= 0)
    {
        return 0;
    }

    *p_visible_region = v->region_screen;
    if (clip_region != NULL)
    {
        egui_region_intersect(p_visible_region, clip_region, p_visible_region);
    }
    return !egui_region_is_empty(p_visible_region);
}

/**
 * @brief Resolve the center of the visible part of one view.
 * @param view Pointer to `egui_view_t`
 * @param clip_region Optional clip region in screen coordinates
 * @param p_x Pointer to store center x
 * @param p_y Pointer to store center y
 * @return 1 if a visible clipped center could be resolved, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_get_view_clipped_center(void *view, const egui_region_t *clip_region, int *p_x, int *p_y)
{
    egui_region_t visible_region;

    if (p_x == NULL || p_y == NULL || !egui_sim_get_view_visible_region(view, clip_region, &visible_region))
    {
        return 0;
    }

    *p_x = visible_region.location.x + visible_region.size.width / 2;
    *p_y = visible_region.location.y + visible_region.size.height / 2;
    return 1;
}

/**
 * @brief Fill one action as a click on the clipped visible center of a view.
 * @param p_action Pointer to `egui_sim_action_t`
 * @param view Pointer to `egui_view_t`
 * @param clip_region Optional clip region in screen coordinates
 * @param interval_ms Interval before click
 * @return 1 if the action was set, 0 otherwise
 */
__EGUI_STATIC_INLINE__ int egui_sim_set_click_view_clipped(egui_sim_action_t *p_action, void *view, const egui_region_t *clip_region, int interval_ms)
{
    if (p_action == NULL || !egui_sim_get_view_clipped_center(view, clip_region, &p_action->x1, &p_action->y1))
    {
        return 0;
    }

    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->interval_ms = interval_ms;
    return 1;
}

/**
 * @brief View-center X expression for macro arguments and initializers.
 */
#define EGUI_SIM_VIEW_CENTER_X(_view) (((egui_view_t *)(_view))->region_screen.location.x + ((egui_view_t *)(_view))->region_screen.size.width / 2)

/** @brief View-center Y expression for macro arguments and initializers. */
#define EGUI_SIM_VIEW_CENTER_Y(_view) (((egui_view_t *)(_view))->region_screen.location.y + ((egui_view_t *)(_view))->region_screen.size.height / 2)

/**
 * @brief Fill one action as a click on the center of a view on the default display.
 * @param _p_action Pointer to `egui_sim_action_t`
 * @param _view Pointer to view
 * @param _interval_ms Interval before click (ms)
 */
#define EGUI_SIM_SET_CLICK_VIEW(_p_action, _view, _interval_ms)                                                                                                \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_CLICK;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view);                                                                                                       \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view);                                                                                                       \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Fill one action as a drag from one view center to another on the default display.
 * @param _p_action Pointer to `egui_sim_action_t`
 * @param _view_from Start view
 * @param _view_to End view
 * @param _steps Number of steps
 * @param _interval_ms Interval before drag (ms)
 */
#define EGUI_SIM_SET_DRAG_VIEW(_p_action, _view_from, _view_to, _steps, _interval_ms)                                                                          \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_DRAG;                                                                                                              \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = (_steps);                                                                                                                         \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Fill one action as a swipe between the centers of two views on the default display.
 */
#define EGUI_SIM_SET_SWIPE_VIEW(_p_action, _view_from, _view_to, _interval_ms)                                                                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_SWIPE;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = 5;                                                                                                                                \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

/**
 * @brief Request an extra screenshot on the next rendered frame.
 *
 * Call this from `egui_port_get_recording_action()` when an action sequence
 * needs a
 * snapshot at a precise frame boundary instead of relying on the
 * framework's default auto-capture timing.
 */
extern void recording_request_snapshot(void);

/**
 * @brief Fill one action as a wait-only entry on the default display.
 */
#define EGUI_SIM_SET_WAIT(_p_action, _interval_ms)                                                                                                             \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_WAIT;                                                                                                              \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
    } while (0)

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/**
 * @brief Fill one action as an N-finger drag on the default display.
 */
__EGUI_STATIC_INLINE__ void egui_sim_action_set_multi_drag(egui_sim_action_t *p_action, uint8_t point_count, const int *start_x, const int *start_y,
                                                           const int *end_x, const int *end_y, int steps, int interval_ms)
{
    if (p_action == NULL)
    {
        return;
    }

    if (point_count > EGUI_CONFIG_TOUCH_MAX_POINTS)
    {
        point_count = EGUI_CONFIG_TOUCH_MAX_POINTS;
    }

    p_action->type = EGUI_SIM_ACTION_MULTI_DRAG;
    p_action->x1 = point_count > 0 && start_x != NULL ? start_x[0] : 0;
    p_action->y1 = point_count > 0 && start_y != NULL ? start_y[0] : 0;
    p_action->x2 = point_count > 0 && end_x != NULL ? end_x[0] : p_action->x1;
    p_action->y2 = point_count > 0 && end_y != NULL ? end_y[0] : p_action->y1;
    p_action->steps = steps;
    p_action->interval_ms = interval_ms;
    p_action->display_id = 0;
    p_action->point_count = point_count;

    for (uint8_t i = 0; i < EGUI_CONFIG_TOUCH_MAX_POINTS; i++)
    {
        p_action->point_start_x[i] = 0;
        p_action->point_start_y[i] = 0;
        p_action->point_end_x[i] = 0;
        p_action->point_end_y[i] = 0;
    }

    for (uint8_t i = 0; i < point_count; i++)
    {
        p_action->point_start_x[i] = start_x != NULL ? start_x[i] : 0;
        p_action->point_start_y[i] = start_y != NULL ? start_y[i] : 0;
        p_action->point_end_x[i] = end_x != NULL ? end_x[i] : p_action->point_start_x[i];
        p_action->point_end_y[i] = end_y != NULL ? end_y[i] : p_action->point_start_y[i];
    }
}

#define EGUI_SIM_SET_MULTI_DRAG(_p_action, _point_count, _start_x, _start_y, _end_x, _end_y, _steps, _interval_ms)                                             \
    egui_sim_action_set_multi_drag((_p_action), (_point_count), (_start_x), (_start_y), (_end_x), (_end_y), (_steps), (_interval_ms))

#endif

/* ---- Multi-display action macros ---- */

/**
 * @brief Create one click or tap action for an explicit display index.
 */
#define EGUI_SIM_CLICK_DISP(_x, _y, _interval_ms, _disp) {EGUI_SIM_ACTION_CLICK, (_x), (_y), 0, 0, 0, (_interval_ms), (_disp)}

/**
 * @brief Create one drag action for an explicit display index.
 */
#define EGUI_SIM_DRAG_DISP(_x1, _y1, _x2, _y2, _steps, _interval_ms, _disp)                                                                                    \
    {EGUI_SIM_ACTION_DRAG, (_x1), (_y1), (_x2), (_y2), (_steps), (_interval_ms), (_disp)}

/**
 * @brief Create one swipe action for an explicit display index.
 */
#define EGUI_SIM_SWIPE_DISP(_x1, _y1, _x2, _y2, _interval_ms, _disp) {EGUI_SIM_ACTION_SWIPE, (_x1), (_y1), (_x2), (_y2), 5, (_interval_ms), (_disp)}

/**
 * @brief Fill one action as a centered click on a view on the selected display.
 */
#define EGUI_SIM_SET_CLICK_VIEW_DISP(_p_action, _view, _interval_ms, _disp)                                                                                    \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_CLICK;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view);                                                                                                       \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view);                                                                                                       \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
        (_p_action)->display_id = (_disp);                                                                                                                     \
    } while (0)

/**
 * @brief Fill one action as a drag between two view centers on the selected display.
 */
#define EGUI_SIM_SET_DRAG_VIEW_DISP(_p_action, _view_from, _view_to, _steps, _interval_ms, _disp)                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_DRAG;                                                                                                              \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = (_steps);                                                                                                                         \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
        (_p_action)->display_id = (_disp);                                                                                                                     \
    } while (0)

/**
 * @brief Fill one action as a swipe between two view centers on the selected display.
 */
#define EGUI_SIM_SET_SWIPE_VIEW_DISP(_p_action, _view_from, _view_to, _interval_ms, _disp)                                                                     \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        (_p_action)->type = EGUI_SIM_ACTION_SWIPE;                                                                                                             \
        (_p_action)->x1 = EGUI_SIM_VIEW_CENTER_X(_view_from);                                                                                                  \
        (_p_action)->y1 = EGUI_SIM_VIEW_CENTER_Y(_view_from);                                                                                                  \
        (_p_action)->x2 = EGUI_SIM_VIEW_CENTER_X(_view_to);                                                                                                    \
        (_p_action)->y2 = EGUI_SIM_VIEW_CENTER_Y(_view_to);                                                                                                    \
        (_p_action)->steps = 5;                                                                                                                                \
        (_p_action)->interval_ms = (_interval_ms);                                                                                                             \
        (_p_action)->display_id = (_disp);                                                                                                                     \
    } while (0)

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
/**
 * @brief Fill one action as an N-finger drag on the selected display.
 */
#define EGUI_SIM_SET_MULTI_DRAG_DISP(_p_action, _point_count, _start_x, _start_y, _end_x, _end_y, _steps, _interval_ms, _disp)                                 \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        EGUI_SIM_SET_MULTI_DRAG((_p_action), (_point_count), (_start_x), (_start_y), (_end_x), (_end_y), (_steps), (_interval_ms));                            \
        (_p_action)->display_id = (_disp);                                                                                                                     \
    } while (0)

#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INPUT_SIMULATOR_H_ */
