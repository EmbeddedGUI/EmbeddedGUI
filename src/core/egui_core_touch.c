#include "egui_core_touch.h"

#include "canvas/egui_canvas.h"
#include "egui_core.h"

/**
 * @file egui_core_touch.c
 * @brief Core-side touch routing plus optional debug trace capture and drawing helpers.
 */

/** Initialize touch-side core state, including debug trace bookkeeping when that feature is enabled. */
void egui_core_touch_init(egui_core_t *core)
{
    if (core == NULL)
    {
        return;
    }

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    core->touch.trace_record.point_count = 0;
    core->touch.trace_record.active = 0;
    egui_region_init_empty(&core->touch.trace_record.bounds);
#endif
}

/** Route one motion event into the root view tree, with optional trace recording for debug overlays. */
void egui_core_process_input_motion(egui_core_t *core, egui_motion_event_t *motion_event)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_core_touch_record_motion(core, motion_event);
#endif
    egui_view_group_dispatch_touch_event((egui_view_t *)egui_core_get_root_view(core), motion_event);
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(motion_event);
#endif
}

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
/** Clear the cached debug path so the next gesture starts from a blank record. */
static void egui_core_touch_trace_reset_record(egui_core_touch_trace_record_t *record)
{
    if (record == NULL)
    {
        return;
    }

    record->point_count = 0;
    record->active = 0;
    egui_region_init_empty(&record->bounds);
}

/** Expand one touch point into a tiny dirty rectangle so it remains visible when drawn as a point. */
static void egui_core_touch_trace_get_point_region(egui_dim_t x, egui_dim_t y, egui_region_t *region)
{
    if (region == NULL)
    {
        return;
    }

    egui_region_init(region, x - 1, y - 1, 3, 3);
}

/** Build the dirty rectangle that covers one drawn line segment in the touch trace. */
static void egui_core_touch_trace_get_segment_region(const egui_location_t *from, const egui_location_t *to, egui_region_t *region)
{
    egui_dim_t min_x;
    egui_dim_t min_y;
    egui_dim_t max_x;
    egui_dim_t max_y;

    if (from == NULL || to == NULL || region == NULL)
    {
        return;
    }

    min_x = EGUI_MIN(from->x, to->x) - 1;
    min_y = EGUI_MIN(from->y, to->y) - 1;
    max_x = EGUI_MAX(from->x, to->x) + 1;
    max_y = EGUI_MAX(from->y, to->y) + 1;
    egui_region_init(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

/** Mark the last recorded touch trace bounds dirty so the overlay can be erased or redrawn. */
static void egui_core_touch_trace_invalidate_record(egui_core_t *core, const egui_core_touch_trace_record_t *record)
{
    if (record == NULL || egui_region_is_empty((egui_region_t *)&record->bounds))
    {
        return;
    }

    egui_core_update_region_dirty(core, (egui_region_t *)&record->bounds);
}

/** Remove the previous trace from the screen before starting a new gesture path. */
static void egui_core_touch_trace_clear_last(egui_core_t *core)
{
    egui_core_touch_trace_invalidate_record(core, &core->touch.trace_record);
    egui_core_touch_trace_reset_record(&core->touch.trace_record);
}

/** Append one point to the debug trace and dirty only the incremental segment that changed. */
static void egui_core_touch_trace_append_point(egui_core_t *core, egui_core_touch_trace_record_t *record, egui_dim_t x, egui_dim_t y)
{
    egui_region_t dirty_region;
    egui_location_t point = {x, y};

    if (record == NULL)
    {
        return;
    }

    if (record->point_count > 0)
    {
        egui_location_t *last = &record->points[record->point_count - 1U];
        if (last->x == x && last->y == y)
        {
            return;
        }

        egui_core_touch_trace_get_segment_region(last, &point, &dirty_region);
        if (record->point_count < EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS)
        {
            record->points[record->point_count] = point;
            record->point_count++;
        }
        else
        {
            // Keep the newest endpoint even after the debug trace reaches its configured storage limit.
            record->points[record->point_count - 1U] = point;
        }
    }
    else
    {
        record->points[0] = point;
        record->point_count = 1;
        egui_core_touch_trace_get_point_region(x, y, &dirty_region);
    }

    if (egui_region_is_empty(&record->bounds))
    {
        egui_region_copy(&record->bounds, &dirty_region);
    }
    else
    {
        egui_region_union(&record->bounds, &dirty_region, &record->bounds);
    }

    egui_core_update_region_dirty(core, &dirty_region);
}

/** Start recording a new touch trace from the first contact point. */
static void egui_core_touch_trace_begin(egui_core_t *core, egui_dim_t x, egui_dim_t y)
{
    egui_core_touch_trace_clear_last(core);
    core->touch.trace_record.active = 1;
    egui_core_touch_trace_append_point(core, &core->touch.trace_record, x, y);
}

/** Extend the active touch trace, or start a new one if the begin event was missed. */
static void egui_core_touch_trace_move(egui_core_t *core, egui_dim_t x, egui_dim_t y)
{
    if (!core->touch.trace_record.active)
    {
        egui_core_touch_trace_begin(core, x, y);
        return;
    }

    egui_core_touch_trace_append_point(core, &core->touch.trace_record, x, y);
}

/** Finish the current touch trace while keeping the final path visible for the debug overlay. */
static void egui_core_touch_trace_end(egui_core_t *core, egui_dim_t x, egui_dim_t y)
{
    if (!core->touch.trace_record.active)
    {
        return;
    }

    egui_core_touch_trace_append_point(core, &core->touch.trace_record, x, y);
    core->touch.trace_record.active = 0;
}
#endif

/** Update the optional debug trace from one motion event. */
void egui_core_touch_record_motion(egui_core_t *core, const egui_motion_event_t *motion_event)
{
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    if (core == NULL || motion_event == NULL)
    {
        return;
    }

    switch (motion_event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        egui_core_touch_trace_begin(core, motion_event->location.x, motion_event->location.y);
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_core_touch_trace_move(core, motion_event->location.x, motion_event->location.y);
        break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        egui_core_touch_trace_end(core, motion_event->location.x, motion_event->location.y);
        break;
    default:
        break;
    }
#else
    EGUI_UNUSED(core);
    EGUI_UNUSED(motion_event);
#endif
}

/** Draw the cached touch trace onto the current canvas tile when the overlay intersects that tile. */
void egui_core_touch_draw_trace(egui_core_t *core)
{
#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
    egui_canvas_t *canvas;
    uint16_t j;
    egui_region_t *pfb_region;
    EGUI_REGION_DEFINE(region_screen, 0, 0, core->screen_width, core->screen_height);

    if (core == NULL)
    {
        return;
    }

    canvas = &core->canvas;
    pfb_region = egui_canvas_get_pfb_region(canvas);

    if (core->touch.trace_record.point_count == 0)
    {
        return;
    }

    egui_canvas_calc_work_region(canvas, &region_screen);

    // Skip tiles that do not overlap the cached trace bounds to keep the debug overlay cheap.
    if (pfb_region != NULL && !egui_region_is_intersect(&core->touch.trace_record.bounds, pfb_region))
    {
        return;
    }

    if (core->touch.trace_record.point_count == 1)
    {
        egui_canvas_draw_point(canvas, core->touch.trace_record.points[0].x, core->touch.trace_record.points[0].y, EGUI_COLOR_RED, EGUI_ALPHA_100);
        return;
    }

    for (j = 1; j < core->touch.trace_record.point_count; j++)
    {
        egui_canvas_draw_line(canvas, core->touch.trace_record.points[j - 1U].x, core->touch.trace_record.points[j - 1U].y,
                              core->touch.trace_record.points[j].x, core->touch.trace_record.points[j].y, 1, EGUI_COLOR_RED, EGUI_ALPHA_100);
    }
#else
    EGUI_UNUSED(core);
#endif
}
