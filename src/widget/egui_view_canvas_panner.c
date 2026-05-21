#include "egui_view_canvas_panner.h"
#include "core/egui_core.h"

#include "canvas/egui_canvas.h"
#include "core/egui_core_internal.h"
#include "egui_view_arc_slider.h"
#include "egui_view_chart_axis.h"
#include "egui_view_list.h"
#include "egui_view_pattern_lock.h"
#include "egui_view_roller.h"
#include "egui_view_scroll.h"
#include "egui_view_slider.h"
#include "egui_view_textblock.h"
#include "egui_view_tileview.h"
#include "egui_view_viewpage.h"
#include "egui_view_viewpage_cache.h"
#include "egui_view_virtual_viewport.h"

/**
 * @file egui_view_canvas_panner.c
 * @brief Group-based canvas panner for oversized logical content.
 *
 * The panner keeps its children as direct
 * descendants, but temporarily shifts
 * layout into a larger logical canvas and clips drawing back to the visible
 * viewport. Touch handling also cooperates
 * with nested draggable widgets so the
 * panner only steals gestures on axes that children do not already consume.
 */

/* Minimum pointer travel before the panner turns a touch sequence into a drag. */
#define EGUI_VIEW_CANVAS_PANNER_TOUCH_SLOP 5

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_arc_slider_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_list_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_pattern_lock_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_roller_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_slider_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_textblock_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tileview_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_cache_t);
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t);
#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_axis_t);
#endif

/** Return the logical canvas width, never smaller than the viewport width. */
static egui_dim_t egui_view_canvas_panner_get_effective_canvas_width(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    return local->canvas_width > self->region.size.width ? local->canvas_width : self->region.size.width;
}

/** Return the logical canvas height, never smaller than the viewport height. */
static egui_dim_t egui_view_canvas_panner_get_effective_canvas_height(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    return local->canvas_height > self->region.size.height ? local->canvas_height : self->region.size.height;
}

/** Return the furthest valid horizontal pan offset. */
static egui_dim_t egui_view_canvas_panner_calc_max_offset_x(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    egui_dim_t canvas_width = egui_view_canvas_panner_get_effective_canvas_width(self, local);
    return canvas_width > self->region.size.width ? (egui_dim_t)(canvas_width - self->region.size.width) : 0;
}

/** Return the furthest valid vertical pan offset. */
static egui_dim_t egui_view_canvas_panner_calc_max_offset_y(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    egui_dim_t canvas_height = egui_view_canvas_panner_get_effective_canvas_height(self, local);
    return canvas_height > self->region.size.height ? (egui_dim_t)(canvas_height - self->region.size.height) : 0;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/** Check whether the logical canvas is currently scrollable on either axis. */
static uint8_t egui_view_canvas_panner_can_pan(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    return egui_view_canvas_panner_calc_max_offset_x(self, local) > 0 || egui_view_canvas_panner_calc_max_offset_y(self, local) > 0;
}

/** Detect containers that should be searched recursively during drag hit testing. */
static uint8_t egui_view_canvas_panner_is_group_like(egui_view_t *view)
{
    return view != NULL && view->api != NULL && view->api->request_layout == egui_view_group_request_layout;
}
#endif

/** Clamp the stored pan offset to the current scrollable range. */
static void egui_view_canvas_panner_clamp_offset(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    egui_dim_t max_offset_x = egui_view_canvas_panner_calc_max_offset_x(self, local);
    egui_dim_t max_offset_y = egui_view_canvas_panner_calc_max_offset_y(self, local);

    if (local->offset_x < 0)
    {
        local->offset_x = 0;
    }
    else if (local->offset_x > max_offset_x)
    {
        local->offset_x = max_offset_x;
    }

    if (local->offset_y < 0)
    {
        local->offset_y = 0;
    }
    else if (local->offset_y > max_offset_y)
    {
        local->offset_y = max_offset_y;
    }
}

/** Update the PFB scan direction so redraw prefers the newly exposed edge first. */
static void egui_view_canvas_panner_update_scan_hint(egui_view_t *self, egui_dim_t old_offset_x, egui_dim_t old_offset_y, egui_dim_t new_offset_x,
                                                     egui_dim_t new_offset_y)
{
    uint8_t reverse_x = new_offset_x > old_offset_x ? 1U : 0U;
    uint8_t reverse_y = new_offset_y > old_offset_y ? 1U : 0U;

    egui_view_set_pfb_scan_direction(self, reverse_x, reverse_y);
}

/** Request a layout pass and redraw after the visible viewport changes. */
static void egui_view_canvas_panner_request_viewport_refresh(egui_view_t *self)
{
    egui_view_request_layout(self);
    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
/* Bitmask describing which drag axes a widget or gesture can consume. */
enum
{
    EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE = 0,
    EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL = 1,
    EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL = 2,
    EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_BOTH = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL | EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL,
};

/** Return the drag axes that the panner itself can currently handle. */
static uint8_t egui_view_canvas_panner_get_self_drag_axis_mask(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    uint8_t axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;

    if (egui_view_canvas_panner_calc_max_offset_x(self, local) > 0)
    {
        axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
    }
    if (egui_view_canvas_panner_calc_max_offset_y(self, local) > 0)
    {
        axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
    }

    return axis_mask;
}

/** Report the axis consumed by a nested scroll widget, if any. */
static uint8_t egui_view_canvas_panner_get_scroll_drag_axis_mask(egui_view_t *view, egui_view_scroll_t *scroll)
{
    egui_view_t *container = EGUI_VIEW_OF(&scroll->container);

    if (container->region.size.height > view->region.size.height)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
    }

    return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
}

/** Report the axis consumed by a nested viewpage widget. */
static uint8_t egui_view_canvas_panner_get_viewpage_drag_axis_mask(egui_view_t *view, egui_view_viewpage_t *viewpage)
{
    egui_view_t *container = EGUI_VIEW_OF(&viewpage->container);

    if (container->region.size.width > view->region.size.width)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
    }

    return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
}

/** Report the axis consumed by a cached viewpage widget. */
static uint8_t egui_view_canvas_panner_get_viewpage_cache_drag_axis_mask(egui_view_t *view, egui_view_viewpage_cache_t *viewpage_cache)
{
    egui_view_t *container = EGUI_VIEW_OF(&viewpage_cache->container);

    if (viewpage_cache->total_page_cnt > 1U || container->region.size.width > view->region.size.width)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
    }

    return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
}

/** Report every axis currently reachable by a nested tileview page graph. */
static uint8_t egui_view_canvas_panner_get_tileview_drag_axis_mask(egui_view_tileview_t *tileview)
{
    uint8_t axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    uint8_t i;

    for (i = 0; i < tileview->tile_count; i++)
    {
        if (tileview->tiles[i] == NULL)
        {
            continue;
        }

        if (tileview->tile_positions[i].col != tileview->current_col)
        {
            axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
        }
        if (tileview->tile_positions[i].row != tileview->current_row)
        {
            axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
        }
        if (axis_mask == EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_BOTH)
        {
            break;
        }
    }

    return axis_mask;
}

/** Report the scroll axis consumed by a nested virtual viewport. */
static uint8_t egui_view_canvas_panner_get_virtual_viewport_drag_axis_mask(egui_view_virtual_viewport_t *virtual_viewport)
{
    if (virtual_viewport->logical_extent <= virtual_viewport->viewport_extent)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    }

    return virtual_viewport->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL ? EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL
                                                                                              : EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
}

/** Report the scroll axes consumed by a textblock with internal scrolling enabled. */
static uint8_t egui_view_canvas_panner_get_textblock_drag_axis_mask(egui_view_t *view, egui_view_textblock_t *textblock)
{
    egui_dim_t content_width = view->region.size.width - (view->padding.left + view->padding.right);
    egui_dim_t content_height = view->region.size.height - (view->padding.top + view->padding.bottom);
    uint8_t axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;

    if (!textblock->is_scroll_enabled)
    {
        return axis_mask;
    }

    if (content_width < 0)
    {
        content_width = 0;
    }
    if (content_height < 0)
    {
        content_height = 0;
    }

    if (textblock->content_width > content_width)
    {
        axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
    }
    if (textblock->content_height > content_height)
    {
        axis_mask |= EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
    }

    return axis_mask;
}

/** Map one concrete widget type to the drag axes it can reasonably claim. */
static uint8_t egui_view_canvas_panner_get_drag_axis_mask_for_view(egui_view_t *view)
{
    const egui_view_api_t *api = view != NULL ? view->api : NULL;

    if (api == NULL)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t))
    {
        return egui_view_canvas_panner_get_scroll_drag_axis_mask(view, (egui_view_scroll_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_list_t))
    {
        return egui_view_canvas_panner_get_scroll_drag_axis_mask(view, &((egui_view_list_t *)view)->base);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_t))
    {
        return egui_view_canvas_panner_get_viewpage_drag_axis_mask(view, (egui_view_viewpage_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_cache_t))
    {
        return egui_view_canvas_panner_get_viewpage_cache_drag_axis_mask(view, (egui_view_viewpage_cache_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_tileview_t))
    {
        return egui_view_canvas_panner_get_tileview_drag_axis_mask((egui_view_tileview_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t))
    {
        return egui_view_canvas_panner_get_virtual_viewport_drag_axis_mask((egui_view_virtual_viewport_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_slider_t))
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_arc_slider_t) || api == &EGUI_VIEW_API_TABLE_NAME(egui_view_pattern_lock_t))
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_BOTH;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_roller_t))
    {
        return ((egui_view_roller_t *)view)->item_count > 1U ? EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL : EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_textblock_t))
    {
        return egui_view_canvas_panner_get_textblock_drag_axis_mask(view, (egui_view_textblock_t *)view);
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_chart_axis_t))
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_BOTH;
    }
#endif

    return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
}

/** Collapse a motion vector into the dominant gesture axis. */
static uint8_t egui_view_canvas_panner_get_gesture_axis_mask(egui_dim_t delta_x, egui_dim_t delta_y)
{
    return EGUI_ABS(delta_x) > EGUI_ABS(delta_y) ? EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_HORIZONTAL : EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_VERTICAL;
}

/** Walk the hit subtree from front to back and find the first child drag axis under the touch point. */
static uint8_t egui_view_canvas_panner_resolve_hit_drag_axis_mask(egui_view_t *view, egui_dim_t screen_x, egui_dim_t screen_y)
{
    if (view == NULL || !view->is_enable || !view->is_visible || view->is_gone)
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    }

    if (!egui_region_pt_in_rect(&view->region_screen, screen_x, screen_y))
    {
        return EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    }

    if (egui_view_canvas_panner_is_group_like(view))
    {
        egui_view_group_t *group = (egui_view_group_t *)view;
        egui_dnode_t *node;

        EGUI_DLIST_FOR_EACH_NODE_REVERSE(&group->childs, node)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
            uint8_t axis_mask = egui_view_canvas_panner_resolve_hit_drag_axis_mask(child, screen_x, screen_y);

            if (axis_mask != EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE)
            {
                return axis_mask;
            }
        }
    }

    return egui_view_canvas_panner_get_drag_axis_mask_for_view(view);
}

/** Return the cached child-drag capability for the current gesture. */
static uint8_t egui_view_canvas_panner_get_child_drag_axis_mask(egui_view_canvas_panner_t *local)
{
    return local->is_drag_target_resolved ? local->child_drag_axis_mask : EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
}

/** Resolve and cache which child under the initial touch point can consume dragging. */
static uint8_t egui_view_canvas_panner_resolve_child_drag_target(egui_view_t *self, egui_view_canvas_panner_t *local)
{
    if (!local->is_drag_target_resolved)
    {
        local->child_drag_axis_mask = egui_view_canvas_panner_resolve_hit_drag_axis_mask(self, local->down_x, local->down_y);
        local->is_drag_target_resolved = 1U;
    }

    return egui_view_canvas_panner_get_child_drag_axis_mask(local);
}

/** Reset drag-tracking state at the beginning of a touch sequence. */
static void egui_view_canvas_panner_prepare_touch(egui_view_canvas_panner_t *local, const egui_motion_event_t *event)
{
    local->down_x = event->location.x;
    local->down_y = event->location.y;
    local->last_x = event->location.x;
    local->last_y = event->location.y;
    local->child_drag_axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
    local->is_drag_target_resolved = 0U;
    local->is_self_touch_owner = 0U;
    local->is_dragging = 0;
}

/** Promote the current gesture to dragging once it moves past touch slop. */
static void egui_view_canvas_panner_check_begin_dragged(egui_view_canvas_panner_t *local, const egui_motion_event_t *event)
{
    egui_dim_t delta_x = event->location.x - local->down_x;
    egui_dim_t delta_y = event->location.y - local->down_y;

    if (local->is_dragging)
    {
        return;
    }

    if (EGUI_ABS(delta_x) > local->touch_slop || EGUI_ABS(delta_y) > local->touch_slop)
    {
        local->is_dragging = 1;
    }
}

/** Apply one drag step by shifting the pan offset opposite to pointer motion. */
static void egui_view_canvas_panner_apply_drag(egui_view_t *self, egui_view_canvas_panner_t *local, const egui_motion_event_t *event)
{
    egui_dim_t delta_x = event->location.x - local->last_x;
    egui_dim_t delta_y = event->location.y - local->last_y;
    egui_dim_t old_offset_x = local->offset_x;
    egui_dim_t old_offset_y = local->offset_y;

    local->last_x = event->location.x;
    local->last_y = event->location.y;

    if (delta_x == 0 && delta_y == 0)
    {
        return;
    }

    local->offset_x = (egui_dim_t)(local->offset_x - delta_x);
    local->offset_y = (egui_dim_t)(local->offset_y - delta_y);
    egui_view_canvas_panner_clamp_offset(self, local);

    if (local->offset_x != old_offset_x || local->offset_y != old_offset_y)
    {
        egui_view_canvas_panner_update_scan_hint(self, old_offset_x, old_offset_y, local->offset_x, local->offset_y);
        egui_view_canvas_panner_request_viewport_refresh(self);
    }
}

/**
 * @brief Decide whether the panner should intercept a touch stream from its children.
 *
 * The panner only takes over when the gesture axis matches one of
 * its own
 * scrollable axes and does not overlap a draggable child under the original hit
 * position.
 */
static int egui_view_canvas_panner_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    egui_dim_t delta_x;
    egui_dim_t delta_y;
    uint8_t child_drag_axis_mask;
    uint8_t gesture_axis_mask;
    uint8_t self_drag_axis_mask;

    if (!egui_view_canvas_panner_can_pan(self, local))
    {
        return egui_view_group_on_intercept_touch_event(self, event);
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        egui_view_canvas_panner_prepare_touch(local, event);
        return 0;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->is_self_touch_owner)
        {
            return 0;
        }

        if (local->is_dragging)
        {
            return 1;
        }

        delta_x = event->location.x - local->down_x;
        delta_y = event->location.y - local->down_y;
        if (EGUI_ABS(delta_x) <= local->touch_slop && EGUI_ABS(delta_y) <= local->touch_slop)
        {
            return 0;
        }

        child_drag_axis_mask = egui_view_canvas_panner_resolve_child_drag_target(self, local);
        gesture_axis_mask = egui_view_canvas_panner_get_gesture_axis_mask(delta_x, delta_y);
        self_drag_axis_mask = egui_view_canvas_panner_get_self_drag_axis_mask(self, local);

        if ((child_drag_axis_mask & gesture_axis_mask) != 0U)
        {
            return 0;
        }
        if ((self_drag_axis_mask & gesture_axis_mask) == 0U)
        {
            return 0;
        }

        local->is_dragging = 1U;
        local->last_x = event->location.x;
        local->last_y = event->location.y;
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0U;
        local->is_self_touch_owner = 0U;
        local->is_dragging = 0U;
        egui_view_reset_pfb_scan_direction(self);
        return 0;
    default:
        return 0;
    }
}

/** Handle touch events once the panner itself owns the gesture stream. */
static int egui_view_canvas_panner_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);

    if (!egui_view_canvas_panner_can_pan(self, local))
    {
        return egui_view_group_on_touch_event(self, event);
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->is_self_touch_owner = 1U;
        egui_view_canvas_panner_prepare_touch(local, event);
        local->is_self_touch_owner = 1U;
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        egui_view_canvas_panner_check_begin_dragged(local, event);
        if (local->is_dragging)
        {
            egui_view_canvas_panner_apply_drag(self, local, event);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_PANNER_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0U;
        local->is_self_touch_owner = 0U;
        local->is_dragging = 0;
        egui_view_reset_pfb_scan_direction(self);
        return 1;
    default:
        return egui_view_group_on_touch_event(self, event);
    }
}
#endif

/** Layout children inside the larger logical canvas while preserving the viewport clip. */
static void egui_view_canvas_panner_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    egui_region_t viewport_region_screen;
    egui_dnode_t *node;

    egui_view_calculate_layout(self);
    egui_view_canvas_panner_clamp_offset(self, local);

    viewport_region_screen = self->region_screen;
    // Temporarily shift the root screen region into canvas coordinates so child layout sees the logical origin.
    self->region_screen.location.x = (egui_dim_t)(viewport_region_screen.location.x - local->offset_x);
    self->region_screen.location.y = (egui_dim_t)(viewport_region_screen.location.y - local->offset_y);
    self->region_screen.size.width = egui_view_canvas_panner_get_effective_canvas_width(self, local);
    self->region_screen.size.height = egui_view_canvas_panner_get_effective_canvas_height(self, local);

    EGUI_DLIST_FOR_EACH_NODE(&local->base.childs, node)
    {
        egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
        child->api->calculate_layout(child);
    }

    self->region_screen = viewport_region_screen;
}

/** Draw the child subtree while clipping it back to the visible viewport rectangle. */
static void egui_view_canvas_panner_draw(egui_view_t *self)
{
    egui_canvas_t *canvas = egui_view_get_canvas(self);
    egui_region_t clip_region;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip(canvas);
    const egui_region_t *active_clip = &self->region_screen;

    if (prev_clip != NULL)
    {
        egui_region_intersect(&self->region_screen, prev_clip, &clip_region);
        active_clip = &clip_region;
    }

    // Clip child drawing to the viewport even though layout happens in a larger logical canvas.
    egui_canvas_set_extra_clip(canvas, active_clip);
    egui_view_group_draw(self);

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(canvas, prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip(canvas);
    }
}

/** API table for the group-based canvas panner widget. */
const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_panner_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_canvas_panner_on_touch_event,
        .on_intercept_touch_event = egui_view_canvas_panner_on_intercept_touch_event,
#else
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_canvas_panner_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_canvas_panner_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/** Apply one parameter block that sets both viewport geometry and logical canvas size. */
void egui_view_canvas_panner_apply_params(egui_view_t *self, const egui_view_canvas_panner_params_t *params)
{
    if (params == NULL)
    {
        return;
    }

    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_set_size(self, params->region.size.width, params->region.size.height);
    egui_view_canvas_panner_set_canvas_size(self, params->canvas_width, params->canvas_height);
}

/** Convenience helper that initializes the panner before applying its parameters. */
void egui_view_canvas_panner_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_canvas_panner_params_t *params)
{
    egui_view_canvas_panner_init(self, core);
    egui_view_canvas_panner_apply_params(self, params);
}

/** Update the logical canvas size, clamp offsets, and refresh the visible viewport. */
void egui_view_canvas_panner_set_canvas_size(egui_view_t *self, egui_dim_t canvas_width, egui_dim_t canvas_height)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);

    if (canvas_width < 0)
    {
        canvas_width = 0;
    }
    if (canvas_height < 0)
    {
        canvas_height = 0;
    }

    if (local->canvas_width == canvas_width && local->canvas_height == canvas_height)
    {
        return;
    }

    local->canvas_width = canvas_width;
    local->canvas_height = canvas_height;
    egui_view_canvas_panner_clamp_offset(self, local);
    egui_view_canvas_panner_request_viewport_refresh(self);
}

/** Return the current effective canvas width after viewport fallback is applied. */
egui_dim_t egui_view_canvas_panner_get_canvas_width(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return egui_view_canvas_panner_get_effective_canvas_width(self, local);
}

/** Return the current effective canvas height after viewport fallback is applied. */
egui_dim_t egui_view_canvas_panner_get_canvas_height(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return egui_view_canvas_panner_get_effective_canvas_height(self, local);
}

/** Set the absolute pan offset, clamping it into range and refreshing if it changed. */
void egui_view_canvas_panner_set_offset(egui_view_t *self, egui_dim_t offset_x, egui_dim_t offset_y)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    egui_dim_t old_offset_x = local->offset_x;
    egui_dim_t old_offset_y = local->offset_y;

    local->offset_x = offset_x;
    local->offset_y = offset_y;
    egui_view_canvas_panner_clamp_offset(self, local);

    if (local->offset_x == old_offset_x && local->offset_y == old_offset_y)
    {
        return;
    }

    egui_view_canvas_panner_update_scan_hint(self, old_offset_x, old_offset_y, local->offset_x, local->offset_y);
    egui_view_canvas_panner_request_viewport_refresh(self);
}

/** Shift the current pan offset by a relative delta. */
void egui_view_canvas_panner_scroll_by(egui_view_t *self, egui_dim_t delta_x, egui_dim_t delta_y)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    egui_view_canvas_panner_set_offset(self, (egui_dim_t)(local->offset_x + delta_x), (egui_dim_t)(local->offset_y + delta_y));
}

/** Return the current horizontal pan offset. */
egui_dim_t egui_view_canvas_panner_get_offset_x(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return local->offset_x;
}

/** Return the current vertical pan offset. */
egui_dim_t egui_view_canvas_panner_get_offset_y(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return local->offset_y;
}

/** Return the maximum reachable horizontal pan offset. */
egui_dim_t egui_view_canvas_panner_get_max_offset_x(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return egui_view_canvas_panner_calc_max_offset_x(self, local);
}

/** Return the maximum reachable vertical pan offset. */
egui_dim_t egui_view_canvas_panner_get_max_offset_y(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_canvas_panner_t);
    return egui_view_canvas_panner_calc_max_offset_y(self, local);
}

/** Initialize the panner with zero offsets and stock drag-threshold state. */
void egui_view_canvas_panner_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_canvas_panner_t);

    egui_view_group_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_panner_t);

    // Start with a viewport-sized logical canvas until the caller expands it explicitly.
    local->canvas_width = 0;
    local->canvas_height = 0;
    local->offset_x = 0;
    local->offset_y = 0;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    local->touch_slop = EGUI_VIEW_CANVAS_PANNER_TOUCH_SLOP;
    local->down_x = 0;
    local->down_y = 0;
    local->last_x = 0;
    local->last_y = 0;
    local->child_drag_axis_mask = 0;
    local->is_drag_target_resolved = 0;
    local->is_self_touch_owner = 0;
    local->is_dragging = 0;
#endif
    egui_view_reset_pfb_scan_direction(self);

    egui_view_set_view_name(self, "egui_view_canvas_panner");
}
