#include <stdio.h>
#include <assert.h>

#include "egui_view_scroll.h"
#include "font/egui_font.h"
#include "core/egui_input.h"

void egui_view_scroll_add_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;

    egui_view_group_add_child((egui_view_t *)&local->container, child);
}

void egui_view_scroll_layout_childs(egui_view_t *self)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;

    egui_view_linearlayout_layout_childs((egui_view_t *)&local->container);
}

void egui_view_scroll_start_container_scroll(egui_view_t *self, int diff_y)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll diff_y: %d\n", diff_y);
    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;
    if (diff_y == 0)
    {
        return;
    }

    // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll container region: %d, %d, %d, %d\n", container->region.location.x, container->region.location.y,
    // container->region.size.width, container->region.size.height);
    if (diff_y < 0)
    {
        // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll up\n");
        // check if container view can scroll up.
        egui_dim_t real_bottom = container->region.location.y + container->region.size.height;
        egui_dim_t bottom_limit = real_bottom - self->region.size.height;
        if (bottom_limit > 0)
        {
            diff_y = EGUI_MAX(diff_y, -bottom_limit);
            // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll up limit: %d\n", diff_y);
            egui_view_scroll_by(container, 0, diff_y);
            return;
        }
        else
        {
            egui_scroller_about_animation(&local->scroller);
        }
    }
    else
    {
        // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll down\n");
        // check if container view can scroll down.
        egui_dim_t real_top = container->region.location.y;
        if (real_top < 0)
        {
            diff_y = EGUI_MIN(diff_y, -real_top);
            // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll down limit: %d\n", diff_y);
            egui_view_scroll_by(container, 0, diff_y);
            return;
        }
        else
        {
            egui_scroller_about_animation(&local->scroller);
        }
    }
}

/**
 * Fling the scroll view
 *
 * @param velocity_y The initial velocity in the Y direction. Positive
 *                  numbers mean that the finger/curor is moving down the screen,
 *                  which means we want to scroll towards the top.
 */
void egui_view_scroll_fling(egui_view_t *self, egui_float_t velocity_y)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;

    // EGUI_LOG_DBG("egui_view_scroll_fling velocity_y: %d\n", velocity_y);

    if (velocity_y == 0)
    {
        return;
    }

    if (velocity_y < 0)
    {
        // EGUI_LOG_DBG("egui_view_scroll_fling up\n");
        // check if container view can scroll up.
        egui_dim_t real_bottom = container->region.location.y + container->region.size.height;
        egui_dim_t bottom_limit = real_bottom - self->region.size.height;
        if (bottom_limit > 0)
        {
            // EGUI_LOG_DBG("egui_view_scroll_fling up limit: %d\n", bottom_limit);
            egui_scroller_start_filing(&local->scroller, bottom_limit, velocity_y);
            return;
        }
    }
    else
    {
        // EGUI_LOG_DBG("egui_view_scroll_fling down\n");
        // check if container view can scroll down.
        egui_dim_t real_top = container->region.location.y;
        if (real_top < 0)
        {
            // EGUI_LOG_DBG("egui_view_scroll_fling down limit: %d\n", real_top);
            egui_scroller_start_filing(&local->scroller, real_top, velocity_y);
            return;
        }
    }
}

void egui_view_scroll_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    self->region.size.width = width;
    self->region.size.height = height;

    // container view width is same to parent
    // container view height will be set by container view's children.
    egui_view_set_size((egui_view_t *)&local->container, width, 0);

    egui_view_invalidate(self);
}

void egui_view_scroll_compute_scroll(egui_view_t *self)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;

    // call super compute_scroll.
    egui_view_group_compute_scroll(self);

    // compute container scroll.
    int offset = egui_scroller_compute_scroll_offset(&local->scroller);
    if (offset)
    {
        // EGUI_LOG_DBG("egui_view_scroll_compute_scroll offset: %d\n", offset);
        // egui_view_scroll_by(self, 0, offset);
        egui_view_scroll_start_container_scroll(self, offset);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_scroll_check_begin_dragged(egui_view_t *self, egui_dim_t delta)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    // EGUI_LOG_DBG("egui_view_scroll_check_begin_dragged id: %d, delta_x: %d\n", self->id, delta);
    if (!local->is_begin_dragged)
    {
        if (EGUI_ABS(delta) > local->touch_slop)
        {
            // EGUI_LOG_DBG("egui_view_scroll_check_begin_dragged begin drag\n");
            local->is_begin_dragged = 1;

            if (self->parent != NULL)
            {
                egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
            }
        }
    }
}

int egui_view_scroll_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    // EGUI_LOG_DBG("egui_view_scroll_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    if ((event->type == EGUI_MOTION_EVENT_ACTION_MOVE) && (local->is_begin_dragged))
    {
        return 1;
    }
    // call super calculate_layout.
    if (egui_view_group_on_intercept_touch_event(self, event))
    {
        return 1;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        /*
         * is_begin_dragged == false, otherwise the shortcut would have caught it. Check
         * whether the user has moved far enough from his original down touch.
         */
        egui_dim_t delta_y = event->location.y - local->last_motion_y;
        egui_view_scroll_check_begin_dragged(self, delta_y);
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        /* Remember location of down touch */
        local->last_motion_y = event->location.y;

        /*
         * If being flinged and user touches the screen, initiate drag;
         * otherwise don't.  scroller.isFinished should be false when
         * being flinged.
         */
        local->is_begin_dragged = !local->scroller.finished;
        break;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    case EGUI_MOTION_EVENT_ACTION_UP:
        local->is_begin_dragged = 0;
        break;
    default:
        break;
    }

    return local->is_begin_dragged;
}

int egui_view_scroll_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_scroll_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        /*
         * If being flinged and user touches, stop the fling. isFinished
         * will be false if being flinged.
         */
        if (egui_dlist_is_empty(&local->base.childs))
        {
            return 0;
        }
        /*
         * If being flinged and user touches, stop the fling. finished
         * will be false if being flinged.
         */
        if (!local->scroller.finished)
        {
            egui_scroller_about_animation(&local->scroller);
        }

        /* Remember location of down touch */
        local->last_motion_y = event->location.y;
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        egui_dim_t delta_y = event->location.y - local->last_motion_y;

        // if no child view, avoid parent view's scroll.
        egui_view_scroll_check_begin_dragged(self, delta_y);

        if (local->is_begin_dragged)
        {
            // Remember where the motion event started
            local->last_motion_y = event->location.y;

            egui_view_scroll_start_container_scroll(self, delta_y);
        }
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        if (local->is_begin_dragged)
        {
            // egui_scroller_start_scroll(&local->scroller, 100, 1000);
            egui_view_scroll_fling(self, egui_input_get_velocity_y());
        }
        break;
    default:
        break;
    }

    // if view clickable, return 1 to stop dispatch touch event to parent.
    return 1;
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t) = {
    .dispatch_touch_event = egui_view_group_dispatch_touch_event,
    .on_touch_event = egui_view_scroll_on_touch_event, // changed
    .on_intercept_touch_event = egui_view_scroll_on_intercept_touch_event, // changed
    .compute_scroll = egui_view_scroll_compute_scroll, // changed
    .calculate_layout = egui_view_group_calculate_layout,
    .request_layout = egui_view_group_request_layout,
    .draw = egui_view_group_draw,
    .on_attach_to_window = egui_view_group_on_attach_to_window,
    .on_draw = egui_view_on_draw,
    .on_detach_from_window = egui_view_group_on_detach_from_window,
};

void egui_view_scroll_init(egui_view_t *self)
{
    egui_view_scroll_t *local = (egui_view_scroll_t *)self;
    // call super init.
    egui_view_group_init(self);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t);

    // init local data.
    local->touch_slop = 5;
    local->last_motion_y = 0;
    local->is_begin_dragged = 0;

    egui_view_linearlayout_init((egui_view_t *)&local->container);
    egui_view_set_position((egui_view_t *)&local->container, 0, 0);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->container, 0);
    egui_view_linearlayout_set_auto_width((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_auto_height((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_orientation((egui_view_t *)&local->container, 0);
    egui_view_group_add_child(self, (egui_view_t *)&local->container);

    egui_scroller_init(&local->scroller);

    egui_view_set_view_name(self, "egui_view_scroll");
}
