#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_group.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct egui_view_group_touch_state egui_view_group_touch_state_t;
struct egui_view_group_touch_state
{
    uint8_t is_active;
    uint8_t is_disallow_intercept;
    uint8_t path_len;
    egui_view_t *path[EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX];
};

static egui_view_group_touch_state_t egui_view_group_touch_state;

static void egui_view_group_touch_state_reset(void)
{
    egui_api_memset(&egui_view_group_touch_state, 0, (int)sizeof(egui_view_group_touch_state));
}

static void egui_view_group_touch_state_set_path_entry(uint8_t depth, egui_view_t *view)
{
    if (depth >= EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX)
    {
        return;
    }

    egui_view_group_touch_state.path[depth] = view;
    if (egui_view_group_touch_state.path_len < depth + 1)
    {
        egui_view_group_touch_state.path_len = depth + 1;
    }
}

static void egui_view_group_touch_state_truncate(uint8_t path_len)
{
    if (path_len >= egui_view_group_touch_state.path_len)
    {
        return;
    }

    for (uint8_t i = path_len; i < egui_view_group_touch_state.path_len && i < EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX; i++)
    {
        egui_view_group_touch_state.path[i] = NULL;
    }
    egui_view_group_touch_state.path_len = path_len;
}

static int egui_view_group_touch_state_contains(egui_view_t *view)
{
    for (uint8_t i = 0; i < egui_view_group_touch_state.path_len && i < EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX; i++)
    {
        if (egui_view_group_touch_state.path[i] == view)
        {
            return 1;
        }
    }
    return 0;
}

void egui_view_group_touch_state_exchange(egui_view_group_touch_state_snapshot_t *snapshot)
{
    egui_view_group_touch_state_snapshot_t current_state;

    if (snapshot == NULL)
    {
        return;
    }

    egui_api_memcpy(&current_state, &egui_view_group_touch_state, (int)sizeof(current_state));
    egui_api_memcpy(&egui_view_group_touch_state, snapshot, (int)sizeof(egui_view_group_touch_state));
    egui_api_memcpy(snapshot, &current_state, (int)sizeof(*snapshot));
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
static int egui_view_group_layer_insert_cond(egui_dnode_t *dnode, void *data)
{
    egui_view_t *current = EGUI_DLIST_ENTRY(dnode, egui_view_t, node);
    egui_view_t *to_insert = (egui_view_t *)data;
    // Insert before the first node with a higher layer
    return (current->layer > to_insert->layer);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

void egui_view_group_add_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_view_set_parent(child, local);

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
    egui_dlist_insert_at(&local->childs, &child->node, egui_view_group_layer_insert_cond, child);
#else
    egui_dlist_append(&local->childs, &child->node);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

    if (self->is_attached_to_window)
    {
        egui_view_dispatch_attach_to_window(child);
    }
}

void egui_view_group_remove_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    if (egui_view_group_touch_state_contains(child))
    {
        egui_view_group_touch_state_reset();
    }
#endif

    if (self->is_attached_to_window)
    {
        egui_view_dispatch_detach_from_window(child);
    }

    egui_dlist_remove(&child->node);
    egui_view_set_parent(child, NULL);
}

void egui_view_group_clear_childs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_dnode_t *p_next;

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    egui_view_group_touch_state_reset();
#endif

    EGUI_DLIST_FOR_EACH_NODE_SAFE(&local->childs, p_head, p_next)
    {
        egui_view_t *tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

        if (self->is_attached_to_window)
        {
            egui_view_dispatch_detach_from_window(tmp);
        }

        egui_dlist_remove(&tmp->node);
        egui_view_set_parent(tmp, NULL);
    }

    egui_dlist_init(&local->childs);
}

int egui_view_group_get_child_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    return egui_dlist_size(&local->childs);
}

egui_view_t *egui_view_group_get_first_child(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *snode = egui_dlist_peek_head(&local->childs);
    if (snode == NULL)
    {
        return NULL;
    }
    return EGUI_DLIST_ENTRY(snode, egui_view_t, node);
}

void egui_view_group_set_disallow_process_touch_event(egui_view_t *self, int disallow)
{
    if (self->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t))
    {
        EGUI_CAST_TO(egui_view_root_group_t, self)->is_disallow_process_touch_event = disallow;
    }
}

void egui_view_group_calculate_all_child_width(egui_view_t *self, egui_dim_t *width)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *width = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_gone)
            {
                continue;
            }
            *width += tmp->region.size.width + tmp->margin.left + tmp->margin.right;
        }
    }
}

void egui_view_group_calculate_all_child_height(egui_view_t *self, egui_dim_t *height)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *height = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_gone)
            {
                continue;
            }
            *height += tmp->region.size.height + tmp->margin.top + tmp->margin.bottom;
        }
    }
}

void egui_view_group_get_max_child_width(egui_view_t *self, egui_dim_t *width)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *width = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_gone)
            {
                continue;
            }
            if (tmp->region.size.width + tmp->margin.left + tmp->margin.right > *width)
            {
                *width = tmp->region.size.width + tmp->margin.left + tmp->margin.right;
            }
        }
    }
}

void egui_view_group_get_max_child_height(egui_view_t *self, egui_dim_t *height)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *height = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_gone)
            {
                continue;
            }
            if (tmp->region.size.height + tmp->margin.top + tmp->margin.bottom > *height)
            {
                *height = tmp->region.size.height + tmp->margin.top + tmp->margin.bottom;
            }
        }
    }
}

void egui_view_group_layout_childs(egui_view_t *self, uint8_t is_orientation_horizontal, uint8_t is_auto_width, uint8_t is_auto_height, uint8_t align_type)
{
    EGUI_LOCAL_INIT(egui_view_group_t);

    // get all child's height.
    egui_dim_t total_child_width = 0;
    egui_dim_t total_child_height = 0;

    // set the view size.
    if (is_orientation_horizontal)
    {
        egui_view_group_calculate_all_child_width(self, &total_child_width);
        egui_view_group_get_max_child_height(self, &total_child_height);
    }
    else
    {
        egui_view_group_get_max_child_width(self, &total_child_width);
        egui_view_group_calculate_all_child_height(self, &total_child_height);
    }

    if (is_auto_width || is_auto_height)
    {
        egui_view_set_size(self, is_auto_width ? (total_child_width + self->padding.left + self->padding.right) : self->region.size.width,
                           is_auto_height ? (total_child_height + self->padding.top + self->padding.bottom) : self->region.size.height);
    }

    // Use content area (region minus padding) as the available space for children.
    // In egui_view_calculate_layout, children's local coordinate origin is at the padding edge,
    // so layout should compute positions within the content area dimensions.
    egui_dim_t parent_width = self->region.size.width - self->padding.left - self->padding.right;
    egui_dim_t parent_height = self->region.size.height - self->padding.top - self->padding.bottom;

    // get base position.
    egui_dim_t x, y;
    egui_common_align_get_x_y(parent_width, parent_height, total_child_width, total_child_height, align_type, &x, &y);

    egui_dnode_t *p_head;
    egui_view_t *tmp;

    egui_dim_t child_x, child_y;
    // update child position.
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_gone)
            {
                continue;
            }
            if (is_orientation_horizontal)
            {
                egui_common_align_get_x_y(total_child_width, total_child_height, tmp->region.size.width + tmp->margin.left + tmp->margin.right,
                                          tmp->region.size.height + tmp->margin.top + tmp->margin.bottom, align_type & EGUI_ALIGN_VMASK, &child_x, &child_y);

                egui_view_set_position(tmp, x + child_x + tmp->margin.left, y + child_y + tmp->margin.top);

                x += tmp->region.size.width + tmp->margin.left + tmp->margin.right;
            }
            else
            {
                egui_common_align_get_x_y(total_child_width, total_child_height, tmp->region.size.width + tmp->margin.left + tmp->margin.right,
                                          tmp->region.size.height + tmp->margin.top + tmp->margin.bottom, align_type & EGUI_ALIGN_HMASK, &child_x, &child_y);

                egui_view_set_position(tmp, x + child_x + tmp->margin.left, y + child_y + tmp->margin.top);

                y += tmp->region.size.height + tmp->margin.top + tmp->margin.bottom;
            }
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_group_is_process_touch_event_disallowed(egui_view_t *self)
{
    if (self->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t))
    {
        return EGUI_CAST_TO(egui_view_root_group_t, self)->is_disallow_process_touch_event;
    }
    return 0;
}

void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow)
{
    EGUI_UNUSED(self);
    egui_view_group_touch_state.is_disallow_intercept = disallow;
}

int egui_view_group_dispatch_transformed_touch_event(egui_view_t *self, int is_canceled, egui_view_t *child, egui_motion_event_t *event)
{
    egui_motion_event_t transformed_event;
    egui_api_memcpy(&transformed_event, event, (int)sizeof(egui_motion_event_t));

    // change to cancel event if is_canceled is true.
    if (is_canceled)
    {
        transformed_event.type = EGUI_MOTION_EVENT_ACTION_CANCEL;
    }

    if (child != NULL)
    {
        // dispatch the touch event to the child
        return child->api->dispatch_touch_event(child, &transformed_event);
    }

    // call super dispatch_touch_event
    return egui_view_dispatch_touch_event(self, &transformed_event);
}

int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // view object should not work here. just return 0.
    // EGUI_LOG_DBG("egui_view_group_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    EGUI_LOCAL_INIT(egui_view_group_t);

    // if(event->type == EGUI_MOTION_EVENT_ACTION_DOWN
    //     && isOnScrollbarThumb())
    // {
    //     return 1;
    // }

    return 0;
}

static int egui_view_dispatch_touch_event_capture(egui_view_t *self, egui_motion_event_t *event, uint8_t depth);
static int egui_view_dispatch_touch_event_followup(egui_view_t *self, egui_motion_event_t *event, uint8_t depth);

static int egui_view_group_dispatch_touch_event_capture_internal(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    int is_intercepted = 0;

    if (!self->is_visible || self->is_gone)
    {
        return 0;
    }

    if (egui_view_group_is_process_touch_event_disallowed(self))
    {
        return 0;
    }

    if (!egui_view_group_touch_state.is_disallow_intercept)
    {
        is_intercepted = self->api->on_intercept_touch_event(self, event);
    }

    if (!is_intercepted && !egui_dlist_is_empty(&local->childs))
    {
        egui_dnode_t *p_head;
        egui_view_t *tmp;

        EGUI_DLIST_FOR_EACH_NODE_REVERSE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (!tmp->is_visible || tmp->is_gone)
            {
                continue;
            }

            if (!egui_region_pt_in_rect(&tmp->region_screen, event->location.x, event->location.y))
            {
                continue;
            }

            if (egui_view_dispatch_touch_event_capture(tmp, event, depth + 1))
            {
                egui_view_group_touch_state_set_path_entry(depth, self);
                return 1;
            }
        }
    }

    int is_handled = egui_view_group_dispatch_transformed_touch_event(self, false, NULL, event);
    if (is_handled)
    {
        egui_view_group_touch_state_set_path_entry(depth, self);
    }
    return is_handled;
}

static int egui_view_group_dispatch_touch_event_followup_internal(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    int is_handled = 0;
    int is_intercepted = 0;
    int is_canceled = event->type == EGUI_MOTION_EVENT_ACTION_CANCEL;
    egui_view_t *captured_child = NULL;

    if (egui_view_group_is_process_touch_event_disallowed(self))
    {
        return 0;
    }

    if (depth >= egui_view_group_touch_state.path_len || egui_view_group_touch_state.path[depth] != self)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    if (!is_canceled && !egui_view_group_touch_state.is_disallow_intercept)
    {
        is_intercepted = self->api->on_intercept_touch_event(self, event);
    }

    if (depth + 1 < egui_view_group_touch_state.path_len)
    {
        captured_child = egui_view_group_touch_state.path[depth + 1];
    }

    if (captured_child == NULL)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    if (is_intercepted)
    {
        egui_motion_event_t cancel_event;
        egui_api_memcpy(&cancel_event, event, (int)sizeof(cancel_event));
        cancel_event.type = EGUI_MOTION_EVENT_ACTION_CANCEL;

        is_handled = egui_view_dispatch_touch_event_followup(captured_child, &cancel_event, depth + 1);
        egui_view_group_touch_state_set_path_entry(depth, self);
        egui_view_group_touch_state_truncate(depth + 1);
        return is_handled;
    }

    return egui_view_dispatch_touch_event_followup(captured_child, event, depth + 1);
}

static int egui_view_dispatch_touch_event_capture(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    if (self->api->dispatch_touch_event == egui_view_group_dispatch_touch_event)
    {
        return egui_view_group_dispatch_touch_event_capture_internal(self, event, depth);
    }

    int is_handled = self->api->dispatch_touch_event(self, event);
    if (is_handled)
    {
        egui_view_group_touch_state_set_path_entry(depth, self);
    }
    return is_handled;
}

static int egui_view_dispatch_touch_event_followup(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    if (self->api->dispatch_touch_event == egui_view_group_dispatch_touch_event)
    {
        return egui_view_group_dispatch_touch_event_followup_internal(self, event, depth);
    }

    return self->api->dispatch_touch_event(self, event);
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    int is_handled = 0;

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        egui_view_group_touch_state_reset();
        is_handled = egui_view_group_dispatch_touch_event_capture_internal(self, event, 0);
        egui_view_group_touch_state.is_active = is_handled && egui_view_group_touch_state.path_len > 0;
        if (!egui_view_group_touch_state.is_active)
        {
            egui_view_group_touch_state_reset();
        }
        return is_handled;
    }

    if (!egui_view_group_touch_state.is_active)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, event->type == EGUI_MOTION_EVENT_ACTION_CANCEL, NULL, event);
    }

    if (egui_view_group_touch_state.path_len == 0 || egui_view_group_touch_state.path[0] != self)
    {
        return 0;
    }

    is_handled = egui_view_group_dispatch_touch_event_followup_internal(self, event, 0);

    if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
    {
        egui_view_group_touch_state_reset();
    }

    return is_handled;
}

int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_group_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    EGUI_LOCAL_INIT(egui_view_group_t);
    int is_inside = egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y);
    if (self->is_clickable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_UP:
            if (self->is_pressed && is_inside)
            {
                egui_view_perform_click(self);
            }
            egui_view_set_pressed(self, false);
            break;
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            egui_view_set_pressed(self, is_inside);
            break;
        case EGUI_MOTION_EVENT_ACTION_MOVE:
            if (self->is_pressed != is_inside)
            {
                egui_view_set_pressed(self, is_inside);
            }
            break;
        case EGUI_MOTION_EVENT_ACTION_CANCEL:
            egui_view_set_pressed(self, false);
            break;
        default:
            break;
        }

        // if view clickable, return 1 to stop dispatch touch event to parent.
        return 1;
    }

    return 0;
}
#else  // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(disallow);
}

void egui_view_group_touch_state_exchange(egui_view_group_touch_state_snapshot_t *snapshot)
{
    EGUI_UNUSED(snapshot);
}

int egui_view_group_dispatch_transformed_touch_event(egui_view_t *self, int is_canceled, egui_view_t *child, egui_motion_event_t *event)
{
    return 0;
}

int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}

int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_group_on_attach_to_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            egui_view_dispatch_attach_to_window(tmp);
        }
    }
}

void egui_view_group_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            egui_view_dispatch_detach_from_window(tmp);
        }
    }
}

void egui_view_group_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;
    egui_alpha_t alpha = egui_canvas_get_alpha();

    // call super draw self.
    egui_view_draw(self);

    if (self->is_visible == false || self->is_gone == true)
    {
        return;
    }

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            // Early culling: skip children that don't intersect the current PFB tile.
            // When a child has a shadow, also check the shadow region since shadows
            // can extend beyond the widget body bounds.
            if (tmp->is_visible && !tmp->is_gone && !egui_region_is_intersect(&tmp->region_screen, egui_canvas_get_pfb_region()))
            {
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
                if (tmp->shadow != NULL)
                {
                    egui_region_t shadow_region;
                    egui_shadow_get_region(tmp->shadow, &tmp->region_screen, &shadow_region);
                    if (!egui_region_is_intersect(&shadow_region, egui_canvas_get_pfb_region()))
                    {
                        continue;
                    }
                }
                else
                {
                    continue;
                }
#else
                continue;
#endif
            }

            // set canvase alpha
            egui_canvas_mix_alpha(self->alpha);
            tmp->api->draw(tmp);
        }
    }

    // restore canvas alpha
    egui_canvas_set_alpha(alpha);
}

void egui_view_group_request_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    // call super request layout.
    egui_view_request_layout(self);

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            tmp->api->request_layout(tmp);
        }
    }
}

void egui_view_group_compute_scroll(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    // call super calculate layout.
    egui_view_compute_scroll(self);

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            tmp->api->compute_scroll(tmp);
        }
    }
}

void egui_view_group_calculate_layout(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    // call super calculate layout.
    egui_view_calculate_layout(self);

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            tmp->api->calculate_layout(tmp);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_group_dispatch_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_group_t);

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    // If there is a focused child, dispatch key event to it
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if (tmp->is_focused)
            {
                if (tmp->api->dispatch_key_event(tmp, event))
                {
                    return 1;
                }
                break;
            }
        }
    }
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

    // If no focused child consumed the event, handle it ourselves
    return egui_view_dispatch_key_event(self, event);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
void egui_view_group_reorder_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_group_t);

    // Remove from current position
    egui_dlist_remove(&child->node);

    // Re-insert at correct position based on layer
    egui_dlist_insert_at(&local->childs, &child->node, egui_view_group_layer_insert_cond, child);
}

void egui_view_group_bring_child_to_front(egui_view_t *self, egui_view_t *child)
{
    egui_view_set_layer(child, EGUI_VIEW_LAYER_TOP);
}

void egui_view_group_send_child_to_back(egui_view_t *self, egui_view_t *child)
{
    egui_view_set_layer(child, EGUI_VIEW_LAYER_BACKGROUND);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_group_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_group_on_intercept_touch_event,
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_group_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_group_on_intercept_touch_event,
        .compute_scroll = egui_view_group_compute_scroll,
        .calculate_layout = egui_view_group_calculate_layout,
        .request_layout = egui_view_group_request_layout,
        .draw = egui_view_group_draw,
        .on_attach_to_window = egui_view_group_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_group_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_group_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_group_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_group_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_group_t);

    // init childs list.
    egui_dlist_init(&local->childs);

    egui_view_set_view_name(self, "egui_view_group");
}

void egui_view_root_group_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_root_group_t);

    egui_view_group_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t);
    local->is_disallow_process_touch_event = 0;

    egui_view_set_view_name(self, "egui_view_root_group");
}

void egui_view_group_apply_params(egui_view_t *self, const egui_view_group_params_t *params)
{
    self->region = params->region;

    egui_view_invalidate(self);
}

void egui_view_group_init_with_params(egui_view_t *self, const egui_view_group_params_t *params)
{
    egui_view_group_init(self);
    egui_view_group_apply_params(self, params);
}
