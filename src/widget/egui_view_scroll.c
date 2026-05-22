#include <stdio.h>
#include <assert.h>

#include "egui_view_scroll.h"
#include "core/egui_core.h"
#include "font/egui_font.h"
#include "core/egui_input.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
static int egui_view_scroll_get_scrollbar_thumb_region(egui_view_t *self, egui_region_t *thumb_region)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = (egui_view_t *)&local->container;
    egui_dim_t content_height = container->region.size.height;
    egui_dim_t view_height = self->region.size.height;
    egui_dim_t scroll_offset;
    egui_dim_t max_scroll;
    egui_dim_t margin;
    egui_dim_t track_length;
    egui_dim_t thumb_length;
    egui_dim_t thumb_travel;
    egui_dim_t thumb_y = 0;
    egui_dim_t bar_x;
    egui_dim_t bar_y;

    if (thumb_region == NULL || !local->is_scrollbar_enabled || !self->is_visible || self->is_gone)
    {
        return 0;
    }

    if (content_height <= view_height || content_height == 0 || view_height == 0)
    {
        return 0;
    }

    scroll_offset = -container->region.location.y;
    if (scroll_offset < 0)
    {
        scroll_offset = 0;
    }
    max_scroll = content_height - view_height;
    if (scroll_offset > max_scroll)
    {
        scroll_offset = max_scroll;
    }

    margin = EGUI_THEME_SCROLLBAR_MARGIN;
    track_length = view_height - 2 * margin;
    if (track_length <= 0)
    {
        return 0;
    }

    thumb_length = (egui_dim_t)(((int32_t)track_length * view_height) / content_height);
    if (thumb_length < EGUI_THEME_SCROLLBAR_MIN_LENGTH)
    {
        thumb_length = EGUI_THEME_SCROLLBAR_MIN_LENGTH;
    }
    if (thumb_length > track_length)
    {
        thumb_length = track_length;
    }

    thumb_travel = track_length - thumb_length;
    if (max_scroll > 0 && thumb_travel > 0)
    {
        thumb_y = (egui_dim_t)(((int32_t)scroll_offset * thumb_travel) / max_scroll);
        if (thumb_y > thumb_travel)
        {
            thumb_y = thumb_travel;
        }
    }

    bar_x = self->region.size.width - EGUI_THEME_SCROLLBAR_THICKNESS - margin;
    bar_y = margin + thumb_y;
    egui_region_init(thumb_region, bar_x, bar_y, EGUI_THEME_SCROLLBAR_THICKNESS, thumb_length);

    return !egui_region_is_empty(thumb_region);
}

static void egui_view_scroll_invalidate_scrollbar_thumb_swept(egui_view_t *self, const egui_region_t *old_thumb)
{
    egui_region_t new_thumb;
    egui_region_t dirty_region;
    int has_old = old_thumb != NULL && !egui_region_is_empty((egui_region_t *)old_thumb);
    int has_new = egui_view_scroll_get_scrollbar_thumb_region(self, &new_thumb);

    if (!has_old && !has_new)
    {
        return;
    }

    if (has_old && has_new)
    {
        egui_region_union((egui_region_t *)old_thumb, &new_thumb, &dirty_region);
    }
    else if (has_old)
    {
        egui_region_copy(&dirty_region, old_thumb);
    }
    else
    {
        egui_region_copy(&dirty_region, &new_thumb);
    }

    egui_view_invalidate_region(self, &dirty_region);
}

static void egui_view_scroll_invalidate_after_container_scroll(egui_view_t *self, const egui_region_t *old_thumb)
{
    if (egui_view_get_dirty_passthrough(self))
    {
        egui_view_scroll_invalidate_scrollbar_thumb_swept(self, old_thumb);
    }
    else
    {
        egui_view_invalidate(self);
    }
}
#endif

/**
 * @file egui_view_scroll.c
 * @brief Vertical scroll view with an inner linear container and fling support.
 *
 * The outer scroll widget stays fixed as
 * the viewport. Its single internal
 * container is moved vertically to reveal different child rows, while touch
 * handling decides when to intercept drags
 * and `egui_scroller_t` drives any
 * inertial motion after release.
 */

/** Add one caller-owned child into the internal scrolling container. */
void egui_view_scroll_add_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    egui_view_group_add_child((egui_view_t *)&local->container, child);
}

/** Relayout the internal linear container after its child set changes. */
void egui_view_scroll_layout_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    egui_view_linearlayout_layout_childs((egui_view_t *)&local->container);
}

#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
/* Forward declaration — implementation follows the snap section. */
static void egui_view_scroll_notify_scroll(egui_view_t *self);
#endif

/**
 * @brief Move the inner container by one signed Y delta, clamping to valid bounds.
 *
 * Negative deltas move content upward to reveal lower items.
 * Positive deltas
 * move content downward toward the top. When a boundary is reached, the helper
 * snaps the container back into range and stops any running
 * fling animation.
 */
void egui_view_scroll_start_container_scroll(egui_view_t *self, int diff_y)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = (egui_view_t *)&local->container;

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    if (local->is_horizontal)
    {
        /* Horizontal scroll: reuse same logic but on X axis. */
        if (diff_y == 0)
        {
            return;
        }
        if (diff_y < 0)
        {
            egui_dim_t real_right = container->region.location.x + container->region.size.width;
            egui_dim_t right_limit = real_right - self->region.size.width;
            if (right_limit > 0)
            {
                diff_y = EGUI_MAX(diff_y, -right_limit);
                egui_view_scroll_by(container, diff_y, 0);
                egui_view_invalidate(self);
                return;
            }
            else
            {
                egui_dim_t content_w = container->region.size.width;
                egui_dim_t view_w = self->region.size.width;
                int max_scroll = (content_w > view_w) ? (int)(content_w - view_w) : 0;
                if (-(container->region.location.x) > max_scroll)
                {
                    egui_view_scroll_to(container, -(egui_dim_t)max_scroll, 0);
                    egui_view_invalidate(self);
                }
                egui_scroller_about_animation(&local->scroller);
            }
        }
        else
        {
            egui_dim_t real_left = container->region.location.x;
            if (real_left < 0)
            {
                diff_y = EGUI_MIN(diff_y, -real_left);
                egui_view_scroll_by(container, diff_y, 0);
                egui_view_invalidate(self);
                return;
            }
            else
            {
                if (container->region.location.x > 0)
                {
                    egui_view_scroll_to(container, 0, 0);
                    egui_view_invalidate(self);
                }
                egui_scroller_about_animation(&local->scroller);
            }
        }
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */

    // EGUI_LOG_DBG("egui_view_scroll_start_container_scroll diff_y: %d\n", diff_y);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    egui_region_t old_thumb;
    int has_old_thumb = egui_view_scroll_get_scrollbar_thumb_region(self, &old_thumb);
#endif
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
            egui_view_scroll_invalidate_after_container_scroll(self, has_old_thumb ? &old_thumb : NULL);
#else
            egui_view_invalidate(self);
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
            egui_view_scroll_notify_scroll(self);
#endif
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
                egui_view_scroll_invalidate_after_container_scroll(self, has_old_thumb ? &old_thumb : NULL);
#else
                egui_view_invalidate(self);
#endif
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
            egui_view_scroll_invalidate_after_container_scroll(self, has_old_thumb ? &old_thumb : NULL);
#else
            egui_view_invalidate(self);
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
            egui_view_scroll_notify_scroll(self);
#endif
            return;
        }
        else
        {
            // Clamp: container drifted above top boundary (container.y > 0).
            if (container->region.location.y > 0)
            {
                // Use scroll_to so region_screen and dirty regions are properly updated.
                egui_view_scroll_to(container, 0, 0);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
                egui_view_scroll_invalidate_after_container_scroll(self, has_old_thumb ? &old_thumb : NULL);
#else
                egui_view_invalidate(self);
#endif
            }
            egui_scroller_about_animation(&local->scroller);
        }
    }
}

/**
 * @brief Start inertial scrolling from the latest release velocity.
 *
 * @param velocity_y The initial velocity in the Y direction. Positive
 * numbers
 * mean the pointer moved down the screen, so content should glide
 * back toward the top.
 */
void egui_view_scroll_fling(egui_view_t *self, egui_float_t velocity_y)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = (egui_view_t *)&local->container;

    if (velocity_y == 0)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    if (local->is_horizontal)
    {
        if (velocity_y < 0)
        {
            egui_dim_t real_right = container->region.location.x + container->region.size.width;
            egui_dim_t right_limit = real_right - self->region.size.width;
            if (right_limit > 0)
            {
                egui_scroller_start_filing(&local->scroller, egui_view_get_core(self), right_limit, velocity_y);
            }
        }
        else
        {
            egui_dim_t real_left = container->region.location.x;
            if (real_left < 0)
            {
                egui_scroller_start_filing(&local->scroller, egui_view_get_core(self), real_left, velocity_y);
            }
        }
        return;
    }
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */

    if (velocity_y < 0)
    {
        egui_dim_t real_bottom = container->region.location.y + container->region.size.height;
        egui_dim_t bottom_limit = real_bottom - self->region.size.height;
        if (bottom_limit > 0)
        {
            egui_scroller_start_filing(&local->scroller, egui_view_get_core(self), bottom_limit, velocity_y);
            return;
        }
    }
    else
    {
        egui_dim_t real_top = container->region.location.y;
        if (real_top < 0)
        {
            egui_scroller_start_filing(&local->scroller, egui_view_get_core(self), real_top, velocity_y);
            return;
        }
    }
}

/** Resize the viewport and keep the inner container width locked to it. */
void egui_view_scroll_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    self->region.size.width = width;
    self->region.size.height = height;

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    if (local->is_horizontal)
    {
        /* In horizontal mode: container height is locked to viewport, width is child-driven. */
        egui_view_set_size((egui_view_t *)&local->container, 0, height);
    }
    else
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */
    {
        /* container view width is same to parent; height will be set by container view's children. */
        egui_view_set_size((egui_view_t *)&local->container, width, 0);
    }

    egui_view_invalidate(self);
}

/** Advance any active fling animation and apply its next container offset step. */
void egui_view_scroll_compute_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    // call super compute_scroll.
    egui_view_group_compute_scroll(self);

    // compute container scroll.
    int offset = egui_scroller_compute_scroll_offset(&local->scroller, egui_view_get_core(self));
    if (offset)
    {
        // EGUI_LOG_DBG("egui_view_scroll_compute_scroll offset: %d\n", offset);
        // egui_view_scroll_by(self, 0, offset);
        egui_view_scroll_start_container_scroll(self, offset);
    }
}

#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
/* Forward declaration — implementation follows after the touch event block. */
static void egui_view_scroll_snap_to_nearest(egui_view_t *self);
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Promote the current gesture to dragging once it moves past touch slop. */
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

/**
 * @brief Decide whether the scroll view should intercept the current touch stream.
 *
 * Interception starts when the user drags far enough vertically,
 * when an
 * existing fling is interrupted and should immediately become a drag, or when
 * the optional scrollbar thumb is grabbed directly.
 */
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
                // Only begin direct thumb dragging when content actually overflows the viewport.
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
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
        egui_dim_t delta_y = local->is_horizontal ? (event->location.x - local->last_motion_y) : (event->location.y - local->last_motion_y);
#else
        egui_dim_t delta_y = event->location.y - local->last_motion_y;
#endif
        egui_view_scroll_check_begin_dragged(self, delta_y);
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        /* Remember location of down touch */
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
        local->last_motion_y = local->is_horizontal ? event->location.x : event->location.y;
#else
        local->last_motion_y = event->location.y;
#endif

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

/** Handle drag scrolling, fling handoff, and optional direct scrollbar dragging. */
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
                    // Map thumb travel on the visual track back into logical content offset.
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
                        egui_region_t old_thumb;
                        int has_old_thumb = egui_view_scroll_get_scrollbar_thumb_region(self, &old_thumb);
#endif
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
                        egui_view_scroll_invalidate_after_container_scroll(self, has_old_thumb ? &old_thumb : NULL);
#else
                        egui_view_invalidate(self);
#endif
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
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
        local->last_motion_y = local->is_horizontal ? event->location.x : event->location.y;
#else
        local->last_motion_y = event->location.y;
#endif
        break;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
        egui_dim_t delta_y = local->is_horizontal ? (event->location.x - local->last_motion_y) : (event->location.y - local->last_motion_y);
#else
        egui_dim_t delta_y = event->location.y - local->last_motion_y;
#endif

        // if no child view, avoid parent view's scroll.
        egui_view_scroll_check_begin_dragged(self, delta_y);

        if (local->is_begin_dragged)
        {
            // Remember where the motion event started
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
            local->last_motion_y = local->is_horizontal ? event->location.x : event->location.y;
#else
            local->last_motion_y = event->location.y;
#endif

            egui_view_scroll_start_container_scroll(self, delta_y);
        }
    }
    break;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        if (local->is_begin_dragged)
        {
            // egui_scroller_start_scroll(&local->scroller, 100, 1000);
#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
            if (local->snap_interval_y > 0 && event->type == EGUI_MOTION_EVENT_ACTION_UP)
            {
                egui_view_scroll_snap_to_nearest(self);
            }
            else
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_SNAP */
            {
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
                egui_float_t velocity = local->is_horizontal ? egui_view_get_velocity_x(self) : egui_view_get_velocity_y(self);
                egui_view_scroll_fling(self, velocity);
#else
                egui_view_scroll_fling(self, egui_view_get_velocity_y(self));
#endif
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

#if EGUI_CONFIG_FUNCTION_SCROLL_SNAP
/**
 * @brief Animate to the nearest snap multiple of snap_interval_y.
 *
 * Replaces the fling animation with a short fixed-duration scroll that
 * lands exactly on a snap boundary.  Called from the touch-UP handler.
 */
static void egui_view_scroll_snap_to_nearest(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = (egui_view_t *)&local->container;
    egui_dim_t snap = local->snap_interval_y;
    egui_dim_t current_offset;
    egui_dim_t target;
    egui_dim_t remainder;
    egui_dim_t content_h;
    egui_dim_t view_h;
    egui_dim_t max_offset;
    egui_dim_t diff;

    if (snap <= 0)
    {
        return;
    }

    /* Current scroll position: 0 at top, increases as content scrolls up. */
    current_offset = -container->region.location.y;

    remainder = current_offset % snap;
    if (remainder * 2 < snap)
    {
        target = current_offset - remainder; /* round down to lower multiple */
    }
    else
    {
        target = current_offset + (snap - remainder); /* round up to upper multiple */
    }

    /* Clamp to valid scroll range. */
    content_h = container->region.size.height;
    view_h = self->region.size.height;
    max_offset = (content_h > view_h) ? (content_h - view_h) : 0;

    if (target < 0)
    {
        target = 0;
    }
    if (target > max_offset)
    {
        target = max_offset;
    }

    diff = target - current_offset; /* positive = scroll down (move container up) */
    if (diff != 0)
    {
        /* Use negative diff for the scroller: negative delta moves container upward. */
        egui_scroller_start_scroll(&local->scroller, egui_view_get_core(self), -diff, EGUI_CONFIG_SCROLL_SNAP_DURATION_MS);
    }
}

void egui_view_scroll_set_snap_interval_y(egui_view_t *self, egui_dim_t interval)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    local->snap_interval_y = interval;
}

egui_dim_t egui_view_scroll_get_snap_interval_y(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    return local->snap_interval_y;
}
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_SNAP */

#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
/**
 * @brief Fire the on_scroll callback with the current scroll offset if a listener is set.
 */
static void egui_view_scroll_notify_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    if (local->on_scroll == NULL)
    {
        return;
    }
    egui_view_t *container = EGUI_VIEW_OF(&local->container);
    egui_dim_t scroll_y = (egui_dim_t)(-(container->region.location.y));
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    local->on_scroll(self, scroll_y);
}

/**
 * @brief Register the scroll-position-change callback.
 */
void egui_view_scroll_set_on_scroll_listener(egui_view_t *self, egui_view_scroll_listener_t cb)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    local->on_scroll = cb;
}

egui_view_scroll_listener_t egui_view_scroll_get_on_scroll_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    return local->on_scroll;
}
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_LISTENER */

/**
 * @brief Return the current scroll offset in pixels from the top.
 */
egui_dim_t egui_view_scroll_get_scroll_y(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = EGUI_VIEW_OF(&local->container);
    egui_dim_t scroll_y = (egui_dim_t)(-(container->region.location.y));
    return scroll_y < 0 ? 0 : scroll_y;
}

/**
 * @brief Return the current scroll offset in pixels from the left.
 */
egui_dim_t egui_view_scroll_get_scroll_x(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = EGUI_VIEW_OF(&local->container);
    egui_dim_t scroll_x = (egui_dim_t)(-(container->region.location.x));
    return scroll_x < 0 ? 0 : scroll_x;
}

/**
 * @brief Scroll the viewport the minimum distance to bring @p child fully into view.
 */
void egui_view_scroll_scroll_to_child(egui_view_t *self, egui_view_t *child, uint8_t animated)
{
    egui_dim_t current_scroll_y;
    egui_dim_t viewport_h;
    egui_dim_t child_top;
    egui_dim_t child_bottom;
    egui_dim_t target_scroll_y;
    egui_dim_t content_h;
    egui_dim_t max_scroll;
    int diff;
    egui_view_t *container;

    if (self == NULL || child == NULL)
    {
        return;
    }

    EGUI_LOCAL_INIT(egui_view_scroll_t);
    container = EGUI_VIEW_OF(&local->container);

    current_scroll_y = (egui_dim_t)(-(container->region.location.y));
    if (current_scroll_y < 0)
    {
        current_scroll_y = 0;
    }
    viewport_h = self->region.size.height;
    child_top = child->region.location.y;
    child_bottom = child_top + child->region.size.height;
    target_scroll_y = current_scroll_y;

    if (child_top < (int)current_scroll_y)
    {
        /* Child is above the viewport — scroll up to reveal its top edge. */
        target_scroll_y = (egui_dim_t)child_top;
        if (child_top < 0)
        {
            target_scroll_y = 0;
        }
    }
    else if (child_bottom > (int)(current_scroll_y + viewport_h))
    {
        /* Child is below the viewport — scroll down to reveal its bottom edge. */
        target_scroll_y = (egui_dim_t)(child_bottom - viewport_h);
    }

    if (target_scroll_y == current_scroll_y)
    {
        return; /* Already fully visible. */
    }

    /* Clamp to valid scroll range. */
    content_h = container->region.size.height;
    max_scroll = (content_h > viewport_h) ? (egui_dim_t)(content_h - viewport_h) : 0;
    if (target_scroll_y > max_scroll)
    {
        target_scroll_y = max_scroll;
    }
    if (target_scroll_y < 0)
    {
        target_scroll_y = 0;
    }

    /* diff > 0  → need to scroll down (container moves up). */
    diff = (int)target_scroll_y - (int)current_scroll_y;
    if (diff == 0)
    {
        return;
    }

    if (animated)
    {
        /* Reuse the same duration as snap animations. */
        egui_scroller_start_scroll(&local->scroller, egui_view_get_core(self), -diff, EGUI_CONFIG_SCROLL_SNAP_DURATION_MS);
    }
    else
    {
        /* start_container_scroll: negative diff_y scrolls the content down (container moves up). */
        egui_view_scroll_start_container_scroll(self, -diff);
    }
}

/**
 * @brief Scroll to an absolute Y pixel offset (0 = top of content).
 */
void egui_view_scroll_to_y(egui_view_t *self, egui_dim_t y, uint8_t animated)
{
    egui_dim_t current_scroll_y;
    egui_dim_t content_h;
    egui_dim_t viewport_h;
    egui_dim_t max_scroll;
    int diff;
    egui_view_t *container;

    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    container = EGUI_VIEW_OF(&local->container);

    current_scroll_y = (egui_dim_t)(-(container->region.location.y));
    if (current_scroll_y < 0)
    {
        current_scroll_y = 0;
    }

    viewport_h = self->region.size.height;
    content_h = container->region.size.height;
    max_scroll = (content_h > viewport_h) ? (egui_dim_t)(content_h - viewport_h) : 0;
    if (y > max_scroll)
    {
        y = max_scroll;
    }
    if (y < 0)
    {
        y = 0;
    }

    diff = (int)y - (int)current_scroll_y;
    if (diff == 0)
    {
        return;
    }

    if (animated)
    {
        egui_scroller_start_scroll(&local->scroller, egui_view_get_core(self), -diff, EGUI_CONFIG_SCROLL_SNAP_DURATION_MS);
    }
    else
    {
        egui_view_scroll_start_container_scroll(self, -diff);
    }
}

/**
 * @brief Scroll to an absolute X pixel offset (0 = left of content).
 */
void egui_view_scroll_to_x(egui_view_t *self, egui_dim_t x, uint8_t animated)
{
    egui_dim_t current_scroll_x;
    egui_dim_t content_w;
    egui_dim_t viewport_w;
    egui_dim_t max_scroll;
    int diff;
    egui_view_t *container;

    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    container = EGUI_VIEW_OF(&local->container);

    current_scroll_x = (egui_dim_t)(-(container->region.location.x));
    if (current_scroll_x < 0)
    {
        current_scroll_x = 0;
    }

    viewport_w = self->region.size.width;
    content_w = container->region.size.width;
    max_scroll = (content_w > viewport_w) ? (egui_dim_t)(content_w - viewport_w) : 0;
    if (x > max_scroll)
    {
        x = max_scroll;
    }
    if (x < 0)
    {
        x = 0;
    }

    diff = (int)x - (int)current_scroll_x;
    if (diff == 0)
    {
        return;
    }

    if (animated)
    {
        egui_scroller_start_scroll(&local->scroller, egui_view_get_core(self), -diff, EGUI_CONFIG_SCROLL_SNAP_DURATION_MS);
    }
    else
    {
        egui_view_scroll_start_container_scroll(self, -diff);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
/** Enable or disable the optional right-side scrollbar overlay. */
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    if (local->is_scrollbar_enabled != enabled)
    {
        egui_region_t old_thumb;
        int has_old_thumb = egui_view_scroll_get_scrollbar_thumb_region(self, &old_thumb);
        local->is_scrollbar_enabled = enabled;
        if (egui_view_get_dirty_passthrough(self))
        {
            egui_view_scroll_invalidate_scrollbar_thumb_swept(self, has_old_thumb ? &old_thumb : NULL);
        }
        else
        {
            egui_view_invalidate(self);
        }
    }
}

/** Draw the clipped scroll content first, then the optional scrollbar overlay. */
void egui_view_scroll_draw(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    EGUI_LOCAL_INIT(egui_view_scroll_t);

    // Set canvas extra clip to this scroll view's screen region so that scrolled
    // children (items above/below the viewport) are properly clipped and not drawn
    // outside the scroll area.
    egui_canvas_set_extra_clip(canvas, &self->region_screen);

    // Draw the view group normally (self + children)
    egui_view_group_draw(self);

    egui_canvas_clear_extra_clip(canvas);

    if (!local->is_scrollbar_enabled || !self->is_visible || self->is_gone)
    {
        return;
    }

    egui_region_t thumb_region;
    if (!egui_view_scroll_get_scrollbar_thumb_region(self, &thumb_region))
    {
        return;
    }

    // Re-establish canvas state because the scrollbar is drawn in viewport coordinates, not child-content coordinates.
    egui_alpha_t alpha = egui_canvas_get_alpha(canvas);
    egui_canvas_clear_mask(canvas);
    egui_canvas_mix_alpha(canvas, self->alpha);
    egui_canvas_calc_work_region(canvas, &self->region_screen);

    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region(canvas)))
    {
        // Draw scrollbar on right side
        egui_canvas_draw_round_rectangle_fill(canvas, thumb_region.location.x, thumb_region.location.y, thumb_region.size.width, thumb_region.size.height,
                                              EGUI_THEME_SCROLLBAR_RADIUS, EGUI_THEME_SCROLLBAR_COLOR, EGUI_THEME_SCROLLBAR_ALPHA);
    }

    egui_canvas_set_alpha(canvas, alpha);
}
#else
/** Stub used when scrollbar support is compiled out. */
void egui_view_scroll_set_scrollbar_enabled(egui_view_t *self, uint8_t enabled)
{
    (void)self;
    (void)enabled;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR

/** Return non-zero when the scrollbar overlay is currently enabled. */
uint8_t egui_view_scroll_get_scrollbar_enabled(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    return local->is_scrollbar_enabled;
#else
    (void)local;
    return 0;
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_scroll_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = (egui_view_t *)&local->container;
    egui_dim_t step;
    int can_scroll;

    if (self->is_enable == false || event == NULL)
    {
        return 0;
    }

    if (event->key_code != EGUI_KEY_CODE_UP && event->key_code != EGUI_KEY_CODE_DOWN)
    {
        return egui_view_on_key_event(self, event);
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        return 1;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_UP || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
    {
        step = self->region.size.height / 4;
        if (step <= 0)
        {
            step = 20;
        }

        can_scroll = (event->key_code == EGUI_KEY_CODE_DOWN) ? ((container->region.location.y + container->region.size.height) > self->region.size.height)
                                                             : (container->region.location.y < 0);
        if (!can_scroll)
        {
            return 0;
        }

        egui_view_scroll_start_container_scroll(self, (event->key_code == EGUI_KEY_CODE_DOWN) ? -step : step);
        return 1;
    }

    return 1;
}
#endif

/** API table for the vertical scroll view widget. */
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
        .on_key_event = egui_view_scroll_on_key_event,
#endif
};

/** Initialize the scroll view, its inner linear container, and fling state. */
void egui_view_scroll_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_scroll_t);
    // call super init.
    egui_view_group_init(self, core);

    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t);

    // init local data.
    local->touch_slop = 5;
    local->last_motion_y = 0;
    local->is_begin_dragged = 0;

    egui_view_linearlayout_init((egui_view_t *)&local->container, core);
    // The inner container is the real parent for caller-added children and grows vertically with layout.
    egui_view_set_position((egui_view_t *)&local->container, 0, 0);
    egui_view_linearlayout_set_align_type((egui_view_t *)&local->container, 0);
    egui_view_linearlayout_set_auto_width((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_auto_height((egui_view_t *)&local->container, 1);
    egui_view_linearlayout_set_orientation((egui_view_t *)&local->container, 0);
    egui_view_group_add_child(self, (egui_view_t *)&local->container);

    egui_view_set_dirty_passthrough(self, 1);
    egui_view_set_dirty_passthrough((egui_view_t *)&local->container, 1);

    egui_scroller_init(&local->scroller, core);

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    local->is_horizontal = 0;
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SCROLLBAR
    local->is_scrollbar_enabled = 0;
    local->is_scrollbar_dragging = 0;
#endif
#if EGUI_CONFIG_FUNCTION_SCROLL_LISTENER
    local->on_scroll = NULL;
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_scroll");
}

/** Apply one region-only parameter block to an already initialized scroll view. */
void egui_view_scroll_apply_params(egui_view_t *self, const egui_view_scroll_params_t *params)
{
    self->region = params->region;

    egui_view_invalidate(self);
}

/** Convenience helper that initializes the scroll view before applying params. */
void egui_view_scroll_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_scroll_params_t *params)
{
    egui_view_scroll_init(self, core);
    egui_view_scroll_apply_params(self, params);
}

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
/** Switch the internal container to horizontal layout and enable X-axis touch tracking. */
void egui_view_scroll_set_horizontal(egui_view_t *self, uint8_t is_horizontal)
{
    if (self == NULL)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    local->is_horizontal = is_horizontal;
    /* Switch the inner container orientation: 1 = horizontal, 0 = vertical. */
    egui_view_linearlayout_set_orientation((egui_view_t *)&local->container, is_horizontal ? 1 : 0);
    egui_view_invalidate(self);
}

/** Return non-zero when the scroll view is in horizontal mode. */
uint8_t egui_view_scroll_get_horizontal(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    return local->is_horizontal;
}
#endif /* EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL */

/**
 * @brief Return non-zero when scroll position is at the beginning (top/left).
 */
uint8_t egui_view_scroll_is_at_top(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    if (local->is_horizontal)
    {
        return egui_view_scroll_get_scroll_x(self) == 0;
    }
#endif
    return egui_view_scroll_get_scroll_y(self) == 0;
}

/**
 * @brief Return non-zero when scroll position is at the end (bottom/right).
 */
uint8_t egui_view_scroll_is_at_bottom(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_scroll_t);
    egui_view_t *container = EGUI_VIEW_OF(&local->container);

#if EGUI_CONFIG_FUNCTION_SCROLL_HORIZONTAL
    if (local->is_horizontal)
    {
        egui_dim_t viewport_w = (egui_dim_t)self->region.size.width;
        egui_dim_t content_w = (egui_dim_t)container->region.size.width;
        egui_dim_t scroll_x = egui_view_scroll_get_scroll_x(self);
        return (scroll_x + viewport_w) >= content_w;
    }
#endif
    {
        egui_dim_t viewport_h = (egui_dim_t)self->region.size.height;
        egui_dim_t content_h = (egui_dim_t)container->region.size.height;
        egui_dim_t scroll_y = egui_view_scroll_get_scroll_y(self);
        return (scroll_y + viewport_h) >= content_h;
    }
}

void egui_view_scroll_scroll_by_y(egui_view_t *self, egui_dim_t delta_y, uint8_t animated)
{
    if (self == NULL)
    {
        return;
    }
    egui_dim_t cur = egui_view_scroll_get_scroll_y(self);
    egui_view_scroll_to_y(self, (egui_dim_t)(cur + delta_y), animated);
}

void egui_view_scroll_scroll_by_x(egui_view_t *self, egui_dim_t delta_x, uint8_t animated)
{
    if (self == NULL)
    {
        return;
    }
    egui_dim_t cur = egui_view_scroll_get_scroll_x(self);
    egui_view_scroll_to_x(self, (egui_dim_t)(cur + delta_x), animated);
}
