#include "egui_core_touch.h"

#include "egui_canvas.h"
#include "egui_core.h"

#if EGUI_CONFIG_DEBUG_TOUCH_TRACE && EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH && EGUI_CONFIG_DEBUG_TOUCH_TRACE_MAX_POINTS > 0
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

static void egui_core_touch_trace_get_point_region(egui_dim_t x, egui_dim_t y, egui_region_t *region)
{
    if (region == NULL)
    {
        return;
    }

    egui_region_init(region, x - 1, y - 1, 3, 3);
}

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

static void egui_core_touch_trace_invalidate_record(egui_core_t *core, const egui_core_touch_trace_record_t *record)
{
    if (record == NULL || egui_region_is_empty((egui_region_t *)&record->bounds))
    {
        return;
    }

    egui_core_update_region_dirty(core, (egui_region_t *)&record->bounds);
}

static void egui_core_touch_trace_clear_last(egui_core_t *core)
{
    egui_core_touch_trace_invalidate_record(core, &core->touch.trace_record);
    egui_core_touch_trace_reset_record(&core->touch.trace_record);
}

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

static void egui_core_touch_trace_begin(egui_core_t *core, egui_dim_t x, egui_dim_t y)
{
    egui_core_touch_trace_clear_last(core);
    core->touch.trace_record.active = 1;
    egui_core_touch_trace_append_point(core, &core->touch.trace_record, x, y);
}

static void egui_core_touch_trace_move(egui_core_t *core, egui_dim_t x, egui_dim_t y)
{
    if (!core->touch.trace_record.active)
    {
        egui_core_touch_trace_begin(core, x, y);
        return;
    }

    egui_core_touch_trace_append_point(core, &core->touch.trace_record, x, y);
}

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
