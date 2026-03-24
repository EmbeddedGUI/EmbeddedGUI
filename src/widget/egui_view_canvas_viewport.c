#include "egui_view_canvas_viewport.h"

#include "core/egui_canvas.h"
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

#define EGUI_VIEW_CANVAS_VIEWPORT_DEFAULT_DRAG_BAR_HEIGHT 0
#define EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_MARGIN         4
#define EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_WIDTH   40
#define EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT  3
#define EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_GAP     4
#define EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_RADIUS         6

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_viewport_t);
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

static egui_dim_t egui_view_canvas_viewport_get_max_offset_x(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    egui_dim_t viewport_width = self->region_screen.size.width;

    if (local->canvas_width <= viewport_width)
    {
        return 0;
    }

    return (egui_dim_t)(local->canvas_width - viewport_width);
}

static egui_dim_t egui_view_canvas_viewport_get_max_offset_y(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    egui_dim_t viewport_height = self->region_screen.size.height;

    if (local->canvas_height <= viewport_height)
    {
        return 0;
    }

    return (egui_dim_t)(local->canvas_height - viewport_height);
}

static void egui_view_canvas_viewport_clamp_offset(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    egui_dim_t max_offset_x = egui_view_canvas_viewport_get_max_offset_x(self, local);
    egui_dim_t max_offset_y = egui_view_canvas_viewport_get_max_offset_y(self, local);

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

static uint8_t egui_view_canvas_viewport_is_group_like(egui_view_t *view)
{
    return view != NULL && view->api != NULL && view->api->request_layout == egui_view_group_request_layout;
}

static void egui_view_canvas_viewport_mark_subtree_request_layout(egui_view_t *view)
{
    if (view == NULL)
    {
        return;
    }

    view->is_request_layout = 1;
    if (!egui_view_canvas_viewport_is_group_like(view))
    {
        return;
    }

    {
        egui_view_group_t *group = (egui_view_group_t *)view;
        egui_dnode_t *node;

        EGUI_DLIST_FOR_EACH_NODE(&group->childs, node)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
            egui_view_canvas_viewport_mark_subtree_request_layout(child);
        }
    }
}

static void egui_view_canvas_viewport_mark_content_request_layout(egui_view_canvas_viewport_t *local)
{
    egui_dnode_t *node;
    egui_view_group_t *content_layer = &local->content_layer;

    EGUI_DLIST_FOR_EACH_NODE(&content_layer->childs, node)
    {
        egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
        egui_view_canvas_viewport_mark_subtree_request_layout(child);
    }
}

static void egui_view_canvas_viewport_sync_content_layer(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    egui_view_t *content_layer = EGUI_VIEW_OF(&local->content_layer);
    egui_region_t region;
    egui_region_t region_screen;
    uint8_t changed = 0;

    region.location.x = (egui_dim_t)(-local->offset_x);
    region.location.y = (egui_dim_t)(-local->offset_y);
    region.size.width = local->canvas_width;
    region.size.height = local->canvas_height;

    region_screen.location.x = self->region_screen.location.x + region.location.x;
    region_screen.location.y = self->region_screen.location.y + region.location.y;
    region_screen.size.width = local->canvas_width;
    region_screen.size.height = local->canvas_height;

    if (!egui_region_equal(&content_layer->region, &region) || !egui_region_equal(&content_layer->region_screen, &region_screen))
    {
        changed = 1;
    }

    content_layer->region = region;
    content_layer->region_screen = region_screen;
    content_layer->is_request_layout = 0;

    if (changed)
    {
        egui_view_canvas_viewport_mark_content_request_layout(local);
    }
}

static void egui_view_canvas_viewport_layout_content_children(egui_view_canvas_viewport_t *local)
{
    egui_dnode_t *node;
    egui_view_group_t *content_layer = &local->content_layer;

    EGUI_DLIST_FOR_EACH_NODE(&content_layer->childs, node)
    {
        egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
        child->api->calculate_layout(child);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
enum
{
    EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE = 0,
    EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL = 1,
    EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL = 2,
    EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_BOTH =
            EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL | EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL,
};

static uint8_t egui_view_canvas_viewport_can_drag(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    return egui_view_canvas_viewport_get_max_offset_x(self, local) > 0 || egui_view_canvas_viewport_get_max_offset_y(self, local) > 0;
}

static uint8_t egui_view_canvas_viewport_get_self_drag_axis_mask(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    uint8_t axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;

    if (egui_view_canvas_viewport_get_max_offset_x(self, local) > 0)
    {
        axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
    }
    if (egui_view_canvas_viewport_get_max_offset_y(self, local) > 0)
    {
        axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
    }

    return axis_mask;
}

static uint8_t egui_view_canvas_viewport_get_scroll_drag_axis_mask(egui_view_t *view, egui_view_scroll_t *scroll)
{
    egui_view_t *container = EGUI_VIEW_OF(&scroll->container);

    if (container->region.size.height > view->region.size.height)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
    }

    return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
}

static uint8_t egui_view_canvas_viewport_get_viewpage_drag_axis_mask(egui_view_t *view, egui_view_viewpage_t *viewpage)
{
    egui_view_t *container = EGUI_VIEW_OF(&viewpage->container);

    if (container->region.size.width > view->region.size.width)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
    }

    return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
}

static uint8_t egui_view_canvas_viewport_get_viewpage_cache_drag_axis_mask(egui_view_t *view, egui_view_viewpage_cache_t *viewpage_cache)
{
    egui_view_t *container = EGUI_VIEW_OF(&viewpage_cache->container);

    if (viewpage_cache->total_page_cnt > 1U || container->region.size.width > view->region.size.width)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
    }

    return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
}

static uint8_t egui_view_canvas_viewport_get_tileview_drag_axis_mask(egui_view_tileview_t *tileview)
{
    uint8_t axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    uint8_t i;

    for (i = 0; i < tileview->tile_count; i++)
    {
        if (tileview->tiles[i] == NULL)
        {
            continue;
        }

        if (tileview->tile_positions[i].col != tileview->current_col)
        {
            axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
        }
        if (tileview->tile_positions[i].row != tileview->current_row)
        {
            axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
        }
        if (axis_mask == EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_BOTH)
        {
            break;
        }
    }

    return axis_mask;
}

static uint8_t egui_view_canvas_viewport_get_virtual_viewport_drag_axis_mask(egui_view_virtual_viewport_t *virtual_viewport)
{
    if (virtual_viewport->logical_extent <= virtual_viewport->viewport_extent)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    }

    return virtual_viewport->orientation == EGUI_VIEW_VIRTUAL_VIEWPORT_ORIENTATION_HORIZONTAL ? EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL
                                                                                                : EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
}

static uint8_t egui_view_canvas_viewport_get_textblock_drag_axis_mask(egui_view_t *view, egui_view_textblock_t *textblock)
{
    egui_dim_t content_width = view->region.size.width - (view->padding.left + view->padding.right);
    egui_dim_t content_height = view->region.size.height - (view->padding.top + view->padding.bottom);
    uint8_t axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;

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
        axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
    }
    if (textblock->content_height > content_height)
    {
        axis_mask |= EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
    }

    return axis_mask;
}

static uint8_t egui_view_canvas_viewport_get_drag_axis_mask_for_view(egui_view_t *view)
{
    const egui_view_api_t *api = view != NULL ? view->api : NULL;

    if (api == NULL)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_scroll_t))
    {
        return egui_view_canvas_viewport_get_scroll_drag_axis_mask(view, (egui_view_scroll_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_list_t))
    {
        return egui_view_canvas_viewport_get_scroll_drag_axis_mask(view, &((egui_view_list_t *)view)->base);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_t))
    {
        return egui_view_canvas_viewport_get_viewpage_drag_axis_mask(view, (egui_view_viewpage_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_viewpage_cache_t))
    {
        return egui_view_canvas_viewport_get_viewpage_cache_drag_axis_mask(view, (egui_view_viewpage_cache_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_tileview_t))
    {
        return egui_view_canvas_viewport_get_tileview_drag_axis_mask((egui_view_tileview_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_virtual_viewport_t))
    {
        return egui_view_canvas_viewport_get_virtual_viewport_drag_axis_mask((egui_view_virtual_viewport_t *)view);
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_slider_t))
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_arc_slider_t) || api == &EGUI_VIEW_API_TABLE_NAME(egui_view_pattern_lock_t))
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_BOTH;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_roller_t))
    {
        return ((egui_view_roller_t *)view)->item_count > 1U ? EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL
                                                              : EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    }

    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_textblock_t))
    {
        return egui_view_canvas_viewport_get_textblock_drag_axis_mask(view, (egui_view_textblock_t *)view);
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MULTI_TOUCH
    if (api == &EGUI_VIEW_API_TABLE_NAME(egui_view_chart_axis_t))
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_BOTH;
    }
#endif

    return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
}

static uint8_t egui_view_canvas_viewport_get_gesture_axis_mask(egui_dim_t delta_x, egui_dim_t delta_y)
{
    return EGUI_ABS(delta_x) > EGUI_ABS(delta_y) ? EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_HORIZONTAL : EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_VERTICAL;
}

static uint8_t egui_view_canvas_viewport_has_drag_bar(egui_view_t *self, egui_view_canvas_viewport_t *local)
{
    return local->drag_bar_height > 0 && egui_view_canvas_viewport_can_drag(self, local);
}

static uint8_t egui_view_canvas_viewport_resolve_hit_drag_axis_mask(egui_view_t *view, egui_dim_t screen_x, egui_dim_t screen_y)
{
    if (view == NULL || !view->is_enable || !view->is_visible || view->is_gone)
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    }

    if (!egui_region_pt_in_rect(&view->region_screen, screen_x, screen_y))
    {
        return EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    }

    if (egui_view_canvas_viewport_is_group_like(view))
    {
        egui_view_group_t *group = (egui_view_group_t *)view;
        egui_dnode_t *node;

        EGUI_DLIST_FOR_EACH_NODE_REVERSE(&group->childs, node)
        {
            egui_view_t *child = EGUI_DLIST_ENTRY(node, egui_view_t, node);
            uint8_t axis_mask = egui_view_canvas_viewport_resolve_hit_drag_axis_mask(child, screen_x, screen_y);

            if (axis_mask != EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE)
            {
                return axis_mask;
            }
        }
    }

    return egui_view_canvas_viewport_get_drag_axis_mask_for_view(view);
}

static uint8_t egui_view_canvas_viewport_get_child_drag_axis_mask(egui_view_canvas_viewport_t *local)
{
    return local->is_drag_target_resolved ? local->child_drag_axis_mask : EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
}

static uint8_t egui_view_canvas_viewport_resolve_child_drag_target(egui_view_canvas_viewport_t *local)
{
    if (!local->is_drag_target_resolved)
    {
        local->child_drag_axis_mask =
                egui_view_canvas_viewport_resolve_hit_drag_axis_mask(EGUI_VIEW_OF(&local->content_layer), local->down_x, local->down_y);
        local->is_child_drag_target = local->child_drag_axis_mask != EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE ? 1U : 0U;
        local->is_drag_target_resolved = 1;
    }

    return egui_view_canvas_viewport_get_child_drag_axis_mask(local);
}

static int egui_view_canvas_viewport_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_dim_t delta_x;
    egui_dim_t delta_y;
    uint8_t gesture_axis_mask;
    uint8_t child_drag_axis_mask;
    uint8_t self_drag_axis_mask;

    if (!egui_view_canvas_viewport_can_drag(self, local))
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        local->down_x = event->location.x;
        local->down_y = event->location.y;
        local->last_x = event->location.x;
        local->last_y = event->location.y;
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0;
        local->is_child_drag_target = 0;
        local->is_dragging = 0;
        return 0;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->is_dragging)
        {
            return 1;
        }

        delta_x = event->location.x - local->down_x;
        delta_y = event->location.y - local->down_y;
        if (EGUI_ABS(delta_x) > local->touch_slop || EGUI_ABS(delta_y) > local->touch_slop)
        {
            child_drag_axis_mask = egui_view_canvas_viewport_resolve_child_drag_target(local);
            gesture_axis_mask = egui_view_canvas_viewport_get_gesture_axis_mask(delta_x, delta_y);
            self_drag_axis_mask = egui_view_canvas_viewport_get_self_drag_axis_mask(self, local);

            if ((child_drag_axis_mask & gesture_axis_mask) != 0U)
            {
                return 0;
            }

            if ((self_drag_axis_mask & gesture_axis_mask) == 0U)
            {
                return 0;
            }

            local->is_dragging = 1;
            local->last_x = event->location.x;
            local->last_y = event->location.y;
            return 1;
        }
        return 0;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0;
        local->is_child_drag_target = 0;
        local->is_dragging = 0;
        return 0;
    default:
        return 0;
    }
}

static int egui_view_canvas_viewport_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_dim_t delta_x;
    egui_dim_t delta_y;
    egui_dim_t old_offset_x;
    egui_dim_t old_offset_y;

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        if (!egui_view_canvas_viewport_can_drag(self, local))
        {
            return 0;
        }

        local->down_x = event->location.x;
        local->down_y = event->location.y;
        local->last_x = event->location.x;
        local->last_y = event->location.y;
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0;
        local->is_child_drag_target = 0;
        local->is_dragging = 0;
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (!egui_view_canvas_viewport_can_drag(self, local))
        {
            return 0;
        }

        if (!local->is_dragging)
        {
            delta_x = event->location.x - local->down_x;
            delta_y = event->location.y - local->down_y;
            if (EGUI_ABS(delta_x) > local->touch_slop || EGUI_ABS(delta_y) > local->touch_slop)
            {
                local->is_dragging = 1;
            }
            else
            {
                return 1;
            }
        }

        delta_x = event->location.x - local->last_x;
        delta_y = event->location.y - local->last_y;
        local->last_x = event->location.x;
        local->last_y = event->location.y;

        old_offset_x = local->offset_x;
        old_offset_y = local->offset_y;
        local->offset_x = (egui_dim_t)(local->offset_x - delta_x);
        local->offset_y = (egui_dim_t)(local->offset_y - delta_y);
        egui_view_canvas_viewport_clamp_offset(self, local);

        if (old_offset_x != local->offset_x || old_offset_y != local->offset_y)
        {
            egui_view_canvas_viewport_mark_content_request_layout(local);
            egui_view_invalidate(self);
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->child_drag_axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
        local->is_drag_target_resolved = 0;
        local->is_child_drag_target = 0;
        local->is_dragging = 0;
        return 1;
    default:
        return 0;
    }
}
#endif

static void egui_view_canvas_viewport_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);

    egui_view_calculate_layout(self);
    egui_view_canvas_viewport_clamp_offset(self, local);
    egui_view_canvas_viewport_sync_content_layer(self, local);
    egui_view_canvas_viewport_layout_content_children(local);
}

static void egui_view_canvas_viewport_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_region_t clip_region;
    const egui_region_t *prev_clip = egui_canvas_get_extra_clip();
    const egui_region_t *active_clip = &self->region_screen;

    if (prev_clip != NULL)
    {
        egui_region_intersect(&self->region_screen, prev_clip, &clip_region);
        active_clip = &clip_region;
    }

    egui_canvas_set_extra_clip(active_clip);
    egui_view_group_draw(self);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    if (egui_view_canvas_viewport_has_drag_bar(self, local))
    {
        egui_dim_t bar_height = local->drag_bar_height;
        egui_dim_t bar_x;
        egui_dim_t bar_y;
        egui_dim_t bar_w;
        egui_dim_t bar_h;
        egui_dim_t handle_w;
        egui_dim_t handle_x;
        egui_dim_t handle_y;

        if (bar_height > self->region_screen.size.height)
        {
            bar_height = self->region_screen.size.height;
        }

        bar_x = (egui_dim_t)(self->region_screen.location.x + EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_MARGIN);
        bar_y = (egui_dim_t)(self->region_screen.location.y + self->region_screen.size.height - bar_height);
        bar_w = (egui_dim_t)(self->region_screen.size.width - EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_MARGIN * 2);
        bar_h = bar_height;
        if (bar_w > 0 && bar_h > 0)
        {
            egui_canvas_draw_round_rectangle_fill(bar_x, bar_y, bar_w, bar_h, EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_RADIUS, EGUI_COLOR_BLACK, 56);

            handle_w = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_WIDTH;
            if (handle_w > bar_w - EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_MARGIN * 2)
            {
                handle_w = bar_w - EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_MARGIN * 2;
            }

            if (handle_w > 0)
            {
                handle_x = (egui_dim_t)(bar_x + (bar_w - handle_w) / 2);
                handle_y = (egui_dim_t)(bar_y +
                                        (bar_h - (EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT * 2 + EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_GAP)) / 2);

                egui_canvas_draw_round_rectangle_fill(handle_x, handle_y, handle_w, EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT,
                                                      EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT, EGUI_COLOR_WHITE, 128);
                egui_canvas_draw_round_rectangle_fill(
                        handle_x, (egui_dim_t)(handle_y + EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT + EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_GAP),
                        handle_w, EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT, EGUI_VIEW_CANVAS_VIEWPORT_DRAG_BAR_HANDLE_HEIGHT, EGUI_COLOR_WHITE, 128);
            }
        }
    }
#endif

    if (prev_clip != NULL)
    {
        egui_canvas_set_extra_clip(prev_clip);
    }
    else
    {
        egui_canvas_clear_extra_clip();
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_viewport_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_canvas_viewport_on_touch_event,
        .on_intercept_touch_event = egui_view_canvas_viewport_on_intercept_touch_event,
#else
        .on_touch_event = NULL,
        .on_intercept_touch_event = NULL,
#endif
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_canvas_viewport_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_canvas_viewport_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_canvas_viewport_apply_params(egui_view_t *self, const egui_view_canvas_viewport_params_t *params)
{
    if (params == NULL)
    {
        return;
    }

    egui_view_set_position(self, params->region.location.x, params->region.location.y);
    egui_view_set_size(self, params->region.size.width, params->region.size.height);
    egui_view_canvas_viewport_set_canvas_size(self, params->canvas_width, params->canvas_height);
}

void egui_view_canvas_viewport_init_with_params(egui_view_t *self, const egui_view_canvas_viewport_params_t *params)
{
    egui_view_canvas_viewport_init(self);
    egui_view_canvas_viewport_apply_params(self, params);
}

void egui_view_canvas_viewport_set_canvas_size(egui_view_t *self, egui_dim_t canvas_width, egui_dim_t canvas_height)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);

    if (canvas_width <= 0)
    {
        canvas_width = 1;
    }
    if (canvas_height <= 0)
    {
        canvas_height = 1;
    }

    if (local->canvas_width == canvas_width && local->canvas_height == canvas_height)
    {
        return;
    }

    local->canvas_width = canvas_width;
    local->canvas_height = canvas_height;
    egui_view_canvas_viewport_clamp_offset(self, local);
    egui_view_canvas_viewport_mark_content_request_layout(local);
    egui_view_invalidate(self);
}

egui_dim_t egui_view_canvas_viewport_get_canvas_width(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->canvas_width;
}

egui_dim_t egui_view_canvas_viewport_get_canvas_height(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->canvas_height;
}

void egui_view_canvas_viewport_set_content_view(egui_view_t *self, egui_view_t *content_view)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_view_t *content_layer = EGUI_VIEW_OF(&local->content_layer);

    if (local->content_view == content_view)
    {
        return;
    }

    if (local->content_view != NULL && local->content_view->parent == &local->content_layer)
    {
        egui_view_group_remove_child(content_layer, local->content_view);
        egui_view_set_parent(local->content_view, NULL);
    }

    local->content_view = content_view;
    if (content_view != NULL)
    {
        egui_view_group_add_child(content_layer, content_view);
    }

    egui_view_canvas_viewport_mark_content_request_layout(local);
    egui_view_invalidate(self);
}

egui_view_t *egui_view_canvas_viewport_get_content_view(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->content_view;
}

egui_view_t *egui_view_canvas_viewport_get_content_layer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return EGUI_VIEW_OF(&local->content_layer);
}

void egui_view_canvas_viewport_set_drag_bar_height(egui_view_t *self, egui_dim_t drag_bar_height)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);

    if (drag_bar_height < 0)
    {
        drag_bar_height = 0;
    }

    if (local->drag_bar_height == drag_bar_height)
    {
        return;
    }

    local->drag_bar_height = drag_bar_height;
    egui_view_invalidate(self);
}

egui_dim_t egui_view_canvas_viewport_get_drag_bar_height(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->drag_bar_height;
}

void egui_view_canvas_viewport_set_offset(egui_view_t *self, egui_dim_t offset_x, egui_dim_t offset_y)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_dim_t old_offset_x = local->offset_x;
    egui_dim_t old_offset_y = local->offset_y;

    local->offset_x = offset_x;
    local->offset_y = offset_y;
    egui_view_canvas_viewport_clamp_offset(self, local);
    if (old_offset_x == local->offset_x && old_offset_y == local->offset_y)
    {
        return;
    }

    egui_view_canvas_viewport_mark_content_request_layout(local);
    egui_view_invalidate(self);
}

void egui_view_canvas_viewport_scroll_by(egui_view_t *self, egui_dim_t delta_x, egui_dim_t delta_y)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    egui_view_canvas_viewport_set_offset(self, (egui_dim_t)(local->offset_x + delta_x), (egui_dim_t)(local->offset_y + delta_y));
}

egui_dim_t egui_view_canvas_viewport_get_offset_x(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->offset_x;
}

egui_dim_t egui_view_canvas_viewport_get_offset_y(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_canvas_viewport_t);
    return local->offset_y;
}

void egui_view_canvas_viewport_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_canvas_viewport_t);

    egui_view_group_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_canvas_viewport_t);

    egui_view_group_init(EGUI_VIEW_OF(&local->content_layer));
    egui_view_group_add_child(self, EGUI_VIEW_OF(&local->content_layer));

    local->content_view = NULL;
    local->canvas_width = 1;
    local->canvas_height = 1;
    local->offset_x = 0;
    local->offset_y = 0;
    local->drag_bar_height = EGUI_VIEW_CANVAS_VIEWPORT_DEFAULT_DRAG_BAR_HEIGHT;
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    local->touch_slop = 5;
    local->down_x = 0;
    local->down_y = 0;
    local->last_x = 0;
    local->last_y = 0;
    local->child_drag_axis_mask = EGUI_VIEW_CANVAS_VIEWPORT_DRAG_AXIS_NONE;
    local->is_drag_target_resolved = 0;
    local->is_child_drag_target = 0;
    local->is_dragging = 0;
#endif

    egui_view_set_view_name(self, "egui_view_canvas_viewport");
}
