#include <stdio.h>
#include <assert.h>

#include "egui_view_viewpage.h"
#include "font/egui_font.h"
#include "core/egui_input.h"

void egui_view_viewpage_add_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    egui_view_group_add_child((egui_view_t *)&local->container, child);
}

void egui_view_viewpage_remove_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    egui_view_group_remove_child((egui_view_t *)&local->container, child);
}

void egui_view_viewpage_layout_childs(egui_view_t *self)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    egui_view_linearlayout_layout_childs((egui_view_t *)&local->container);
}

void egui_view_viewpage_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    self->region.size.width = width;
    self->region.size.height = height;

    // container view width will be set by container view's children.
    // container view height is same to parent
    egui_view_set_size((egui_view_t *)&local->container, 0, height);

    egui_view_invalidate(self);
}

void egui_view_viewpage_start_container_scroll(egui_view_t *self, int diff_x)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll diff_x: %d\n", diff_x);
    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;
    if (diff_x == 0)
    {
        return;
    }

    // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll container region: %d, %d, %d, %d\n", container->region.location.x, container->region.location.x,
    // container->region.size.width, container->region.size.height);
    if (diff_x < 0)
    {
        // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll up\n");
        // check if container view can scroll up.
        egui_dim_t real_right = container->region.location.x + container->region.size.width;
        egui_dim_t right_limit = real_right - self->region.size.width;
        if (right_limit > 0)
        {
            diff_x = EGUI_MAX(diff_x, -right_limit);
            // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll up limit: %d\n", diff_x);
            egui_view_scroll_by(container, diff_x, 0);
            return;
        }
        else
        {
            egui_scroller_about_animation(&local->scroller);
        }
    }
    else
    {
        // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll down\n");
        // check if container view can scroll down.
        egui_dim_t real_left = container->region.location.x;
        if (real_left < 0)
        {
            diff_x = EGUI_MIN(diff_x, -real_left);
            // EGUI_LOG_DBG("egui_view_viewpage_start_container_scroll down limit: %d\n", diff_x);
            egui_view_scroll_by(container, diff_x, 0);
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
 * @param velocity_x The initial velocity in the Y direction. Positive
 *                  numbers mean that the finger/curor is moving down the screen,
 *                  which means we want to scroll towards the top.
 */
void egui_view_viewpage_fling(egui_view_t *self, egui_float_t velocity_x)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;

    // EGUI_LOG_DBG("egui_view_viewpage_fling velocity_x: %d\n", velocity_x);

    if (velocity_x == 0)
    {
        return;
    }

    if (velocity_x < 0)
    {
        // EGUI_LOG_DBG("egui_view_viewpage_fling up\n");
        // check if container view can scroll up.
        egui_dim_t real_right = container->region.location.x + container->region.size.width;
        egui_dim_t right_limit = real_right - self->region.size.width;
        if (right_limit > 0)
        {
            // EGUI_LOG_DBG("egui_view_viewpage_fling up limit: %d\n", right_limit);
            egui_scroller_start_filing(&local->scroller, right_limit, velocity_x);
            return;
        }
    }
    else
    {
        // EGUI_LOG_DBG("egui_view_viewpage_fling down\n");
        // check if container view can scroll down.
        egui_dim_t real_left = container->region.location.x;
        if (real_left < 0)
        {
            // EGUI_LOG_DBG("egui_view_viewpage_fling down limit: %d\n", real_left);
            egui_scroller_start_filing(&local->scroller, real_left, velocity_x);
            return;
        }
    }
}

void egui_view_viewpage_scroll_to_page(egui_view_t *self, int page_index)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // EGUI_LOG_DBG("egui_view_viewpage_scroll_to_page page_index: %d\n", page_index);

    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;
    int container_count = egui_view_group_get_child_count(container);
    if (page_index < 0 || page_index >= container_count)
    {
        page_index = container_count - 1;
    }
    egui_dim_t target_x = -page_index * self->region.size.width;
    egui_dim_t diff_x = target_x - container->region.location.x;

    // EGUI_LOG_DBG("egui_view_viewpage_scroll_to_page target_x: %d, location: %d, diff_x: %d\n", target_x, container->region.location.x, diff_x);
    // EGUI_LOG_DBG("egui_view_viewpage_scroll_to_page target_x: %d, location: %d, diff_x: %d\n", target_x, EGUI_ABS(container->region.location.x), EGUI_ABS(diff_x));

    local->current_page_index = page_index;

    // egui_view_viewpage_start_container_scroll(self, diff_x);
    egui_scroller_start_scroll(&local->scroller, diff_x, EGUI_ABS(diff_x) * 1.5);
}

void egui_view_viewpage_slow_scroll_to_page(egui_view_t *self)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    // get container view.
    egui_view_t *container = (egui_view_t *)&local->container;

    // EGUI_LOG_DBG("egui_view_viewpage_scroll_to_page location: %d, width: %d\n", container->region.location.x, self->region.size.width);
    // EGUI_LOG_DBG("egui_view_viewpage_slow_scroll_to_page location: %d, width: %d\n", EGUI_ABS(container->region.location.x), self->region.size.width);

    uint8_t page_index_next = (EGUI_ABS(container->region.location.x) + (self->region.size.width >> 1)) / self->region.size.width;
    egui_view_viewpage_scroll_to_page(self, page_index_next);
}

void egui_view_viewpage_set_current_page(egui_view_t *self, int page_index)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    egui_view_t *container = (egui_view_t *)&local->container;
    int container_count = egui_view_group_get_child_count(container);

    if (page_index < 0 || page_index >= container_count)
    {
        page_index = container_count - 1;
    }

    egui_dim_t target_x = -page_index * self->region.size.width;

    container->region.location.x = target_x;

    local->current_page_index = page_index;

    egui_view_invalidate(self);
}

void egui_view_viewpage_compute_scroll(egui_view_t *self)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

    // call super compute_scroll.
    egui_view_group_compute_scroll(self);

    // compute container scroll.
    int offset = egui_scroller_compute_scroll_offset(&local->scroller);
    if (offset)
    {
        // EGUI_LOG_DBG("egui_view_viewpage_compute_scroll offset: %d\n", offset);
        // egui_view_viewpage_by(self, 0, offset);
        egui_view_viewpage_start_container_scroll(self, offset);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_viewpage_check_begin_dragged(egui_view_t *self, egui_dim_t delta)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // EGUI_LOG_DBG("egui_view_viewpage_check_begin_dragged id: %d, delta_x: %d\n", self->id, delta);
    if (!local->is_begin_dragged)
    {
        if (EGUI_ABS(delta) > local->touch_slop)
        {
            // EGUI_LOG_DBG("egui_view_viewpage_check_begin_dragged begin drag\n");
            local->is_begin_dragged = 1;

            if (self->parent != NULL)
            {
                egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
            }
        }
    }
}

int egui_view_viewpage_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // EGUI_LOG_DBG("egui_view_viewpage_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
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
        egui_dim_t delta_x = event->location.x - local->last_motion_x;
        egui_view_viewpage_check_begin_dragged(self, delta_x);
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        /* Remember location of down touch */
        local->last_motion_x = event->location.x;

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

int egui_view_viewpage_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_viewpage_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;

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
        local->last_motion_x = event->location.x;
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        egui_dim_t delta_x = event->location.x - local->last_motion_x;
        // if no child view, avoid parent view's scroll.
        egui_view_viewpage_check_begin_dragged(self, delta_x);

        if (local->is_begin_dragged)
        {
            // Remember where the motion event started
            local->last_motion_x = event->location.x;

            egui_view_viewpage_start_container_scroll(self, delta_x);
        }
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        if (local->is_begin_dragged)
        {
            // egui_scroller_start_scroll(&local->scroller, 100, 1000);
            egui_float_t velocity_x = egui_input_get_velocity_x();
            // EGUI_LOG_DBG("egui_view_viewpage_on_touch_event velocity_x: %d\n", velocity_x);
            int container_count = egui_view_group_get_child_count((egui_view_t *)&local->container);
            if (velocity_x > (EGUI_FLOAT_VALUE(0.05f)) && local->current_page_index > 0)
            {
                egui_view_viewpage_scroll_to_page(self, local->current_page_index - 1);
            }
            else if (velocity_x < (EGUI_FLOAT_VALUE(-0.05f)) && local->current_page_index < container_count)
            {
                egui_view_viewpage_scroll_to_page(self, local->current_page_index + 1);
            }
            else
            {
                egui_view_viewpage_slow_scroll_to_page(self);
            }
        }
        break;
    default:
        break;
    }

    // if view clickable, return 1 to stop dispatch touch event to parent.
    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

EGUI_VIEW_API_DEFINE_BASE_GROUP(egui_view_viewpage_t, NULL, egui_view_viewpage_on_touch_event, egui_view_viewpage_on_intercept_touch_event,
                                egui_view_viewpage_compute_scroll, NULL, NULL, NULL, NULL, NULL, NULL);

void egui_view_viewpage_init(egui_view_t *self)
{
    egui_view_viewpage_t *local = (egui_view_viewpage_t *)self;
    // call super init.
    egui_view_group_init(self);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_t);

    // init local data.
    local->touch_slop = 5;
    local->last_motion_x = 0;
    local->is_begin_dragged = 0;

    egui_view_linearlayout_init((egui_view_t *)&local->container);
    egui_view_set_position((egui_view_t *)&local->container, 0, 0);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->container, 0);
    egui_view_linearlayout_set_auto_width((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_auto_height((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_orientation((egui_view_t *)&local->container, 1);
    egui_view_group_add_child(self, (egui_view_t *)&local->container);

    egui_scroller_init(&local->scroller);

    egui_view_set_view_name(self, "egui_view_viewpage");
}
