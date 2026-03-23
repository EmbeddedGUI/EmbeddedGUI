#include <stdio.h>
#include <assert.h>

#include "egui_view_scroll.h"
#include "font/egui_font.h"
#include "core/egui_input.h"

void egui_view_scroll_add_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    egui_view_group_add_child((egui_view_t *)&local->container, child);
}

void egui_view_scroll_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    egui_view_linearlayout_layout_childs((egui_view_t *)&local->container);
}

void egui_view_scroll_start_container_scroll(egui_view_t *self, int diff_y)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
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
            egui_view_invalidate(self);
            return;
        }
        else
        {
            // Clamp: content already at or past bottom boundary.
            egui_dim_t content_h = container->region.size.height;
            egui_dim_t view_h = self->region.size.height;
            int max_scroll = (content_h > view_h) ? (int)(content_h - view_h) : 0;
            if (-(container->region.location.y) > max_scroll)
            {
                // Use scroll_to so region_screen and dirty regions are properly updated.
                egui_view_scroll_to(container, 0, -(egui_dim_t)max_scroll);
            }
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
            egui_view_invalidate(self);
            return;
        }
        else
        {
            // Clamp: container drifted above top boundary (container.y > 0).
            if (container->region.location.y > 0)
            {
                // Use scroll_to so region_screen and dirty regions are properly updated.
                egui_view_scroll_to(container, 0, 0);
            }
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
    EGUI_LOCAL_INIT(egui_view_scroll_t);
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
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    self->region.size.width = width;
    self->region.size.height = height;

    // container view width is same to parent
    // container view height will be set by container view's children.
    egui_view_set_size((egui_view_t *)&local->container, width, 0);

    egui_view_invalidate(self);
}

void egui_view_scroll_compute_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

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
    EGUI_LOCAL_INIT(egui_view_scroll_t);
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
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    // EGUI_LOG_DBG("egui_view_scroll_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    if (local->is_scrollbar_enabled)
    {
        if (local->is_scrollbar_dragging)
        {
            return 1;
        }
        if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
        {
            egui_dim_t local_x = event->location.x - self->region_screen.location.x;
            egui_dim_t bar_area_start = self->region.size.width - EGUI_THEME_SCROLLBAR_TOUCH_WIDTH;
            if (local_x >= bar_area_start)
            {
                egui_view_t *container = (egui_view_t *)&local->container;
                if (container->region.size.height > self->region.size.height)
                {
                    local->is_scrollbar_dragging = 1;
                    local->is_begin_dragged = 0;
                    egui_scroller_about_animation(&local->scroller);
                    if (self->parent != NULL)
                    {
                        egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
                    }
                    return 1;
                }
            }
        }
    }
#endif

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
         * whether the user has moved far enough from his
         * original down touch.
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

         * * being flinged.
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
    EGUI_LOCAL_INIT(egui_view_scroll_t);

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    if (local->is_scrollbar_dragging)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_DOWN:
        case EGUI_MOTION_EVENT_ACTION_MOVE:
        {
            egui_view_t *container = (egui_view_t *)&local->container;
            egui_dim_t content_height = container->region.size.height;
            egui_dim_t view_height = self->region.size.height;
            if (content_height > view_height)
            {
                egui_dim_t local_y = event->location.y - self->region_screen.location.y;
                egui_dim_t margin = EGUI_THEME_SCROLLBAR_MARGIN;
                egui_dim_t track_length = view_height - 2 * margin;
                if (track_length > 0)
                {
                    egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_height) / content_height);
                    if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
                    {
                        thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
                    }
                    if (thumb_length > track_length)
                    {
                        thumb_length = track_length;
                    }

                    egui_dim_t thumb_travel = track_length - thumb_length;
                    if (thumb_travel > 0)
                    {
                        egui_dim_t thumb_pos = local_y - margin - thumb_length / 2;
                        if (thumb_pos < 0)
                        {
                            thumb_pos = 0;
                        }
                        if (thumb_pos > thumb_travel)
                        {
                            thumb_pos = thumb_travel;
                        }

                        egui_dim_t max_scroll = content_height - view_height;
                        egui_dim_t target_offset = (egui_dim_t)(((int32_t)thumb_pos * max_scroll) / thumb_travel);

                        egui_view_scroll_to(container, 0, -target_offset);
                        egui_view_invalidate(self);
                    }
                }
            }
            break;
        }
        case EGUI_MOTION_EVENT_ACTION_UP:
        case EGUI_MOTION_EVENT_ACTION_CANCEL:
            local->is_scrollbar_dragging = 0;
            break;
        default:
            break;
        }
        return 1;
    }
#endif

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
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    if (local->is_scrollbar_enabled != enabled)
    {
        local->is_scrollbar_enabled = enabled;
        egui_view_invalidate(self);
    }
}

void egui_view_scroll_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    // Set canvas extra clip to this scroll view's screen region so that scrolled
    // children (items above/below the viewport) are properly clipped and not drawn
    // outside the scroll area.
    egui_canvas_set_extra_clip(&self->region_screen);

    // Draw the view group normally (self + children)
    egui_view_group_draw(self);

    egui_canvas_clear_extra_clip();

    if (!local->is_scrollbar_enabled || !self->is_visible || self->is_gone)
    {
        return;
    }

    egui_view_t *container = (egui_view_t *)&local->container;
    egui_dim_t content_height = container->region.size.height;
    egui_dim_t view_height = self->region.size.height;

    // Only draw scrollbar if content is taller than view
    if (content_height <= view_height || content_height == 0 || view_height == 0)
    {
        return;
    }

    // Calculate scroll offset (container.y is negative when scrolled down)
    egui_dim_t scroll_offset = -container->region.location.y;
    if (scroll_offset < 0)
    {
        scroll_offset = 0;
    }
    egui_dim_t max_scroll = content_height - view_height;
    if (scroll_offset > max_scroll)
    {
        scroll_offset = max_scroll;
    }

    // Calculate thumb dimensions
    egui_dim_t margin = EGUI_THEME_SCROLLBAR_MARGIN;
    egui_dim_t track_length = view_height - 2 * margin;
    if (track_length <= 0)
    {
        return;
    }

    egui_dim_t thumb_length = (egui_dim_t)(((int32_t)track_length * view_height) / content_height);
    if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
    {
        thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
    }
    if (thumb_length > track_length)
    {
        thumb_length = track_length;
    }

    // Calculate thumb position
    egui_dim_t thumb_travel = track_length - thumb_length;
    egui_dim_t thumb_y = 0;
    if (max_scroll > 0 && thumb_travel > 0)
    {
        thumb_y = (egui_dim_t)(((int32_t)scroll_offset * thumb_travel) / max_scroll);
        if (thumb_y > thumb_travel)
        {
            thumb_y = thumb_travel;
        }
    }

    // Re-establish canvas work region for scrollbar drawing
    egui_alpha_t alpha = egui_canvas_get_alpha();
    egui_canvas_clear_mask();
    egui_canvas_mix_alpha(self->alpha);
    egui_canvas_calc_work_region(&self->region_screen);

    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
    {
        // Draw scrollbar on right side
        egui_dim_t bar_x = self->region.size.width - EGUI_THEME_SCROLLBAR_THICKNESS - margin;
        egui_dim_t bar_y = margin + thumb_y;

        egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, EGUI_THEME_SCROLLBAR_THICKNESS, thumb_length, EGUI_THEME_SCROLLBAR_RADIUS,
                                              EGUI_THEME_SCROLLBAR_COLOR, EGUI_THEME_SCROLLBAR_ALPHA);
    }

    egui_canvas_set_alpha(alpha);
}
#else
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled)
{
    (void)self;
    (void)enabled;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t) = {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_scroll_on_touch_event,                     // changed
        .on_intercept_touch_event = egui_view_scroll_on_intercept_touch_event, // changed
#else                                                                          // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = NULL,           // changed
        .on_intercept_touch_event = NULL, // changed
#endif                                                                         // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .compute_scroll = egui_view_scroll_compute_scroll,                     // changed
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
        .draw = egui_view_scroll_draw,
#else
        .draw = egui_view_group_draw,
#endif
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_scroll_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_scroll_t);
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    local->is_scrollbar_enabled = 0;
    local->is_scrollbar_dragging = 0;
#endif

    egui_view_set_view_name(self, "egui_view_scroll");
}

void egui_view_scroll_apply_params(egui_view_t *self, const egui_view_scroll_params_t *params)
{
    self->region = params->region;

    egui_view_invalidate(self);
}

void egui_view_scroll_init_with_params(egui_view_t *self, const egui_view_scroll_params_t *params)
{
    egui_view_scroll_init(self);
    egui_view_scroll_apply_params(self, params);
}
