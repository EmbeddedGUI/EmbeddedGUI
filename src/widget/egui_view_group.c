#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_group.h"
#include "core/egui_core.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t);

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_group_is_group_dispatch_view(egui_view_t *view)
{
    return view != NULL && view->api != NULL && view->api->dispatch_touch_event == egui_view_group_dispatch_touch_event;
}

static int egui_view_group_dispatch_child_touch_event(egui_view_t *child, egui_motion_event_t *event)
{
    if (child == NULL || event == NULL || child->api == NULL || child->api->dispatch_touch_event == NULL)
    {
        return 0;
    }

    return child->api->dispatch_touch_event(child, event);
}
#endif

static void egui_view_group_request_child_layout(egui_view_t *child)
{
    if (child == NULL || child->api == NULL || child->api->request_layout == NULL)
    {
        return;
    }

    child->api->request_layout(child);
}

static void egui_view_group_compute_child_scroll(egui_view_t *child)
{
    if (child == NULL || child->api == NULL || child->api->compute_scroll == NULL)
    {
        return;
    }

    child->api->compute_scroll(child);
}

static void egui_view_group_calculate_child_layout(egui_view_t *child)
{
    if (child == NULL || child->api == NULL || child->api->calculate_layout == NULL)
    {
        return;
    }

    child->api->calculate_layout(child);
}

static void egui_view_group_draw_child(egui_view_t *child)
{
    if (child == NULL || child->api == NULL || child->api->draw == NULL)
    {
        return;
    }

    child->api->draw(child);
}

static int egui_view_group_child_intersects_pfb(egui_view_t *child, egui_region_t *pfb_region)
{
    if (child == NULL || pfb_region == NULL || !child->is_visible || child->is_gone)
    {
        return 0;
    }

    if (egui_region_is_intersect(&child->region_screen, pfb_region))
    {
        return 1;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (child->is_focused)
    {
        egui_region_t focus_frame_region;

        egui_view_get_focus_frame_region(child, &focus_frame_region);
        if (egui_region_is_intersect(&focus_frame_region, pfb_region))
        {
            return 1;
        }
    }
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    if (child->shadow != NULL)
    {
        egui_region_t shadow_region;

        egui_shadow_get_region(child->shadow, &child->region_screen, &shadow_region);
        if (egui_region_is_intersect(&shadow_region, pfb_region))
        {
            return 1;
        }
    }
#endif

    return 0;
}

#if EGUI_CONFIG_FUNCTION_CORE_PRE_COMPUTE_SCROLL
#define EGUI_VIEW_GROUP_COMPUTE_SCROLL_HANDLER egui_view_group_compute_scroll
#else
#define EGUI_VIEW_GROUP_COMPUTE_SCROLL_HANDLER egui_view_compute_scroll
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
typedef struct egui_view_group_touch_state egui_view_group_touch_state_t;
struct egui_view_group_touch_state
{
#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
    uint8_t is_active;
    uint8_t is_disallow_intercept;
    uint8_t path_len;
    egui_view_t *path[EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX];
#else
    uint8_t is_active;
    egui_view_t *captured_view;
#endif
};

static egui_view_group_touch_state_t *egui_view_group_touch_state_get(egui_core_t *core)
{
    if (core == NULL)
    {
        return NULL;
    }

    return (egui_view_group_touch_state_t *)&core->touch.view_group_touch_state;
}

static void egui_view_group_touch_state_reset(egui_core_t *core)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);

    if (touch_state == NULL)
    {
        return;
    }

    egui_api_memset(touch_state, 0, (int)sizeof(*touch_state));
}

#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
static void egui_view_group_touch_state_set_path_entry(egui_core_t *core, uint8_t depth, egui_view_t *view)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);

    if (touch_state == NULL)
    {
        return;
    }

    if (depth >= EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX)
    {
        return;
    }

    touch_state->path[depth] = view;
    if (touch_state->path_len < depth + 1)
    {
        touch_state->path_len = depth + 1;
    }
}

static void egui_view_group_touch_state_truncate(egui_core_t *core, uint8_t path_len)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);

    if (touch_state == NULL || path_len >= touch_state->path_len)
    {
        return;
    }

    for (uint8_t i = path_len; i < touch_state->path_len && i < EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX; i++)
    {
        touch_state->path[i] = NULL;
    }
    touch_state->path_len = path_len;
}

static int egui_view_group_touch_state_contains(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);

    if (touch_state == NULL)
    {
        return 0;
    }

    for (uint8_t i = 0; i < touch_state->path_len && i < EGUI_CONFIG_TOUCH_CAPTURE_PATH_MAX; i++)
    {
        if (touch_state->path[i] == view)
        {
            return 1;
        }
    }
    return 0;
}
#else
static void egui_view_group_touch_state_set_capture(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);

    if (touch_state == NULL)
    {
        return;
    }

    touch_state->captured_view = view;
}

static int egui_view_group_touch_state_contains(egui_core_t *core, egui_view_t *view)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);
    egui_view_t *captured_view;

    if (touch_state == NULL || view == NULL)
    {
        return 0;
    }

    captured_view = touch_state->captured_view;
    while (captured_view != NULL)
    {
        if (captured_view == view)
        {
            return 1;
        }
        captured_view = EGUI_VIEW_PARENT(captured_view);
    }

    return 0;
}
#endif

void egui_view_group_touch_state_exchange(egui_core_t *core, egui_view_group_touch_state_snapshot_t *snapshot)
{
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);
    egui_view_group_touch_state_snapshot_t current_state;

    if (touch_state == NULL || snapshot == NULL)
    {
        return;
    }

    egui_api_memcpy(&current_state, touch_state, (int)sizeof(current_state));
    egui_api_memcpy(touch_state, snapshot, (int)sizeof(*touch_state));
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

static void egui_view_group_sync_child_core(egui_view_t *self, egui_view_t *child)
{
    if (self == NULL || child == NULL)
    {
        return;
    }

    EGUI_ASSERT(egui_view_get_core(self) == egui_view_get_core(child));
}

void egui_view_group_add_child(egui_view_t *self, egui_view_t *child)
{
    EGUI_LOCAL_INIT(egui_view_group_t);

    egui_view_group_sync_child_core(self, child);
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
    if (egui_view_group_touch_state_contains(egui_view_get_core(self), child))
    {
        egui_view_group_touch_state_reset(egui_view_get_core(self));
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
    egui_view_group_touch_state_reset(egui_view_get_core(self));
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

egui_view_t *egui_view_group_get_last_child(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *snode = egui_dlist_peek_tail(&local->childs);
    if (snode == NULL)
    {
        return NULL;
    }
    return EGUI_DLIST_ENTRY(snode, egui_view_t, node);
}

egui_view_t *egui_view_group_get_child_at(egui_view_t *self, int index)
{
    egui_dnode_t *p;
    int i;

    if (self == NULL || index < 0)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_group_t);
    i = 0;
    EGUI_DLIST_FOR_EACH_NODE(&local->childs, p)
    {
        if (i == index)
        {
            return EGUI_DLIST_ENTRY(p, egui_view_t, node);
        }
        i++;
    }
    return NULL;
}

int egui_view_group_get_child_index(egui_view_t *self, egui_view_t *child)
{
    egui_dnode_t *p;
    int i;

    if (self == NULL || child == NULL)
    {
        return -1;
    }
    EGUI_LOCAL_INIT(egui_view_group_t);
    i = 0;
    EGUI_DLIST_FOR_EACH_NODE(&local->childs, p)
    {
        if (EGUI_DLIST_ENTRY(p, egui_view_t, node) == child)
        {
            return i;
        }
        i++;
    }
    return -1;
}

void egui_view_group_move_child_to_index(egui_view_t *self, egui_view_t *child, int index)
{
    egui_dnode_t *p;
    int i;
    egui_view_t *successor;

    if (self == NULL || child == NULL || index < 0)
    {
        return;
    }
    EGUI_LOCAL_INIT(egui_view_group_t);

    /* Remove child from its current position. */
    egui_dlist_remove(&child->node);

    /* Walk the updated list to find the node currently at 'index'. */
    i = 0;
    successor = NULL;
    EGUI_DLIST_FOR_EACH_NODE(&local->childs, p)
    {
        if (i == index)
        {
            successor = EGUI_DLIST_ENTRY(p, egui_view_t, node);
            break;
        }
        i++;
    }

    if (successor == NULL)
    {
        /* index >= remaining count: append to end. */
        egui_dlist_append(&local->childs, &child->node);
    }
    else
    {
        /* Insert before the successor node. */
        egui_dlist_insert(&successor->node, &child->node);
    }

    egui_view_invalidate(self);
}

void egui_view_group_set_disallow_process_touch_event(egui_view_t *self, int disallow)
{
    if (self != NULL && self->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t))
    {
        EGUI_CAST_TO(egui_view_root_group_t, self)->is_disallow_process_touch_event = disallow;
    }
}

int egui_view_group_get_disallow_process_touch_event(egui_view_t *self)
{
    if (self != NULL && self->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t))
    {
        return EGUI_CAST_TO(egui_view_root_group_t, self)->is_disallow_process_touch_event;
    }
    return 0;
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
    if (self != NULL && self->api == &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t))
    {
        return EGUI_CAST_TO(egui_view_root_group_t, self)->is_disallow_process_touch_event;
    }
    return 0;
}

void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow)
{
#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(egui_view_get_core(self));

    if (touch_state == NULL)
    {
        return;
    }

    touch_state->is_disallow_intercept = disallow;
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(disallow);
#endif
}

int egui_view_group_dispatch_transformed_touch_event(egui_view_t *self, int is_canceled, egui_view_t *child, egui_motion_event_t *event)
{
    egui_motion_event_t transformed_event;

    if (event == NULL)
    {
        return 0;
    }

    egui_api_memcpy(&transformed_event, event, (int)sizeof(egui_motion_event_t));

    // change to cancel event if is_canceled is true.
    if (is_canceled)
    {
        transformed_event.type = EGUI_MOTION_EVENT_ACTION_CANCEL;
    }

    if (child != NULL)
    {
        // dispatch the touch event to the child
        return egui_view_group_dispatch_child_touch_event(child, &transformed_event);
    }

    // call super dispatch_touch_event
    return egui_view_dispatch_touch_event(self, &transformed_event);
}

int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
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

#if EGUI_CONFIG_FUNCTION_VIEW_GROUP_TOUCH_CAPTURE_PATH
static int egui_view_dispatch_touch_event_capture(egui_view_t *self, egui_motion_event_t *event, uint8_t depth);
static int egui_view_dispatch_touch_event_followup(egui_view_t *self, egui_motion_event_t *event, uint8_t depth);

static int egui_view_group_dispatch_touch_event_capture_internal(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    egui_core_t *core;
    egui_view_group_touch_state_t *touch_state;
    int is_intercepted = 0;

    if (self == NULL || event == NULL || !self->is_visible || self->is_gone)
    {
        return 0;
    }

    EGUI_LOCAL_INIT(egui_view_group_t);
    core = egui_view_get_core(self);
    touch_state = egui_view_group_touch_state_get(core);

    if (egui_view_group_is_process_touch_event_disallowed(self))
    {
        return 0;
    }

    if (touch_state == NULL)
    {
        return 0;
    }

    if (!touch_state->is_disallow_intercept)
    {
        if (self->api != NULL && self->api->on_intercept_touch_event != NULL)
        {
            is_intercepted = self->api->on_intercept_touch_event(self, event);
        }
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

            if (!egui_view_hit_test(tmp, event->location.x, event->location.y))
            {
                continue;
            }

            if (egui_view_dispatch_touch_event_capture(tmp, event, depth + 1))
            {
                egui_view_group_touch_state_set_path_entry(core, depth, self);
                return 1;
            }
        }
    }

    int is_handled = egui_view_group_dispatch_transformed_touch_event(self, false, NULL, event);
    if (is_handled)
    {
        egui_view_group_touch_state_set_path_entry(core, depth, self);
    }
    return is_handled;
}

static int egui_view_group_dispatch_touch_event_followup_internal(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    egui_core_t *core = egui_view_get_core(self);
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);
    int is_handled = 0;
    int is_intercepted = 0;
    int is_canceled;
    egui_view_t *captured_child = NULL;

    if (self == NULL || event == NULL || touch_state == NULL || egui_view_group_is_process_touch_event_disallowed(self))
    {
        return 0;
    }

    is_canceled = event->type == EGUI_MOTION_EVENT_ACTION_CANCEL;

    if (depth >= touch_state->path_len || touch_state->path[depth] != self)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    if (!is_canceled && !touch_state->is_disallow_intercept)
    {
        if (self->api != NULL && self->api->on_intercept_touch_event != NULL)
        {
            is_intercepted = self->api->on_intercept_touch_event(self, event);
        }
    }

    if (depth + 1 < touch_state->path_len)
    {
        captured_child = touch_state->path[depth + 1];
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

        egui_view_group_sync_child_core(self, captured_child);
        is_handled = egui_view_dispatch_touch_event_followup(captured_child, &cancel_event, depth + 1);
        egui_view_group_touch_state_set_path_entry(core, depth, self);
        egui_view_group_touch_state_truncate(core, depth + 1);
        return is_handled;
    }

    egui_view_group_sync_child_core(self, captured_child);
    return egui_view_dispatch_touch_event_followup(captured_child, event, depth + 1);
}

static int egui_view_dispatch_touch_event_capture(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    egui_core_t *core = egui_view_get_core(self);

    if (self == NULL || event == NULL)
    {
        return 0;
    }

    if (egui_view_group_is_group_dispatch_view(self))
    {
        return egui_view_group_dispatch_touch_event_capture_internal(self, event, depth);
    }

    int is_handled = egui_view_group_dispatch_child_touch_event(self, event);
    if (is_handled)
    {
        egui_view_group_touch_state_set_path_entry(core, depth, self);
    }
    return is_handled;
}

static int egui_view_dispatch_touch_event_followup(egui_view_t *self, egui_motion_event_t *event, uint8_t depth)
{
    if (self == NULL || event == NULL)
    {
        return 0;
    }

    if (egui_view_group_is_group_dispatch_view(self))
    {
        return egui_view_group_dispatch_touch_event_followup_internal(self, event, depth);
    }

    return egui_view_group_dispatch_child_touch_event(self, event);
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    egui_core_t *core = egui_view_get_core(self);
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);
    int is_handled = 0;

    if (self == NULL || event == NULL)
    {
        return 0;
    }

    if (touch_state == NULL)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, event->type == EGUI_MOTION_EVENT_ACTION_CANCEL, NULL, event);
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        egui_view_group_touch_state_reset(core);
        is_handled = egui_view_group_dispatch_touch_event_capture_internal(self, event, 0);
        touch_state->is_active = is_handled && touch_state->path_len > 0;
        if (!touch_state->is_active)
        {
            egui_view_group_touch_state_reset(core);
        }
        return is_handled;
    }

    if (!touch_state->is_active)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, event->type == EGUI_MOTION_EVENT_ACTION_CANCEL, NULL, event);
    }

    if (touch_state->path_len == 0 || touch_state->path[0] != self)
    {
        return 0;
    }

    is_handled = egui_view_group_dispatch_touch_event_followup_internal(self, event, 0);

    if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
    {
        egui_view_group_touch_state_reset(core);
    }

    return is_handled;
}
#else
static int egui_view_group_dispatch_touch_event_capture_simple(egui_view_t *self, egui_motion_event_t *event)
{
    egui_core_t *core;

    if (self == NULL || event == NULL || !self->is_visible || self->is_gone || egui_view_group_is_process_touch_event_disallowed(self))
    {
        return 0;
    }

    EGUI_LOCAL_INIT(egui_view_group_t);
    core = egui_view_get_core(self);

    if (!egui_dlist_is_empty(&local->childs))
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

            if (!egui_view_hit_test(tmp, event->location.x, event->location.y))
            {
                continue;
            }

            egui_view_group_sync_child_core(self, tmp);
            if (egui_view_group_is_group_dispatch_view(tmp))
            {
                if (egui_view_group_dispatch_touch_event_capture_simple(tmp, event))
                {
                    return 1;
                }
            }
            else if (egui_view_group_dispatch_child_touch_event(tmp, event))
            {
                egui_view_group_touch_state_set_capture(core, tmp);
                return 1;
            }
        }
    }

    if (egui_view_group_dispatch_transformed_touch_event(self, false, NULL, event))
    {
        egui_view_group_touch_state_set_capture(core, self);
        return 1;
    }

    return 0;
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    egui_core_t *core = egui_view_get_core(self);
    egui_view_group_touch_state_t *touch_state = egui_view_group_touch_state_get(core);
    egui_view_t *captured_view;
    int is_handled = 0;
    int is_canceled;

    if (self == NULL || event == NULL)
    {
        return 0;
    }

    is_canceled = event->type == EGUI_MOTION_EVENT_ACTION_CANCEL;

    if (touch_state == NULL)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
    {
        egui_view_group_touch_state_reset(core);
        is_handled = egui_view_group_dispatch_touch_event_capture_simple(self, event);
        touch_state->is_active = is_handled && touch_state->captured_view != NULL;
        if (!touch_state->is_active)
        {
            egui_view_group_touch_state_reset(core);
        }
        return is_handled;
    }

    if (!touch_state->is_active)
    {
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    captured_view = touch_state->captured_view;
    if (captured_view == NULL)
    {
        egui_view_group_touch_state_reset(core);
        return egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }

    if (captured_view != self)
    {
        egui_view_group_sync_child_core(self, captured_view);
    }
    is_handled = egui_view_group_dispatch_transformed_touch_event(self, is_canceled, captured_view == self ? NULL : captured_view, event);

    if (event->type == EGUI_MOTION_EVENT_ACTION_UP || is_canceled)
    {
        egui_view_group_touch_state_reset(core);
    }

    return is_handled;
}
#endif

int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
#if !EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    return egui_view_on_touch_event(self, event);
#else
    // EGUI_LOG_DBG("egui_view_group_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    int is_inside;

    if (self == NULL || event == NULL)
    {
        return 0;
    }

    EGUI_LOCAL_INIT(egui_view_group_t);
    is_inside = egui_view_hit_test(self, event->location.x, event->location.y);
    if (self->is_clickable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_UP:
        {
            int should_click = self->is_pressed && is_inside;

            egui_view_set_pressed(self, false);
            if (should_click)
            {
                egui_view_perform_click(self);
            }
            break;
        }
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
#endif
}
#else  // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(disallow);
}

void egui_view_group_touch_state_exchange(egui_core_t *core, egui_view_group_touch_state_snapshot_t *snapshot)
{
    EGUI_UNUSED(core);
    EGUI_UNUSED(snapshot);
}

int egui_view_group_dispatch_transformed_touch_event(egui_view_t *self, int is_canceled, egui_view_t *child, egui_motion_event_t *event)
{
    EGUI_UNUSED(child);
    EGUI_UNUSED(event);
    EGUI_UNUSED(is_canceled);
    EGUI_UNUSED(self);
    return 0;
}

int egui_view_group_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}

int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_UNUSED(event);
    EGUI_UNUSED(self);
    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_group_on_attach_to_window(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);
            egui_view_group_sync_child_core(self, tmp);
            egui_view_dispatch_attach_to_window(tmp);
        }
    }
}

void egui_view_group_on_detach_from_window(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

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
    egui_canvas_t *canvas;
    egui_alpha_t alpha;

    if (self == NULL)
    {
        return;
    }

    canvas = egui_view_get_canvas(self);
    if (canvas == NULL)
    {
        egui_view_draw(self);
        return;
    }

    EGUI_LOCAL_INIT(egui_view_group_t);
    egui_dnode_t *p_head;
    egui_view_t *tmp;
    alpha = egui_canvas_get_alpha(canvas);

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
            // Shadows and the common focus frame can extend beyond the widget body bounds.
            if (!egui_view_group_child_intersects_pfb(tmp, egui_canvas_get_pfb_region(canvas)))
            {
                continue;
            }

            // set canvase alpha
            egui_canvas_mix_alpha(canvas, self->alpha);
            egui_view_group_sync_child_core(self, tmp);
            egui_view_group_draw_child(tmp);
        }
    }

    // restore canvas alpha
    egui_canvas_set_alpha(canvas, alpha);
}

void egui_view_group_request_layout(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

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

            egui_view_group_sync_child_core(self, tmp);
            egui_view_group_request_child_layout(tmp);
        }
    }
}

void egui_view_group_compute_scroll(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

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

            egui_view_group_sync_child_core(self, tmp);
            egui_view_group_compute_child_scroll(tmp);
        }
    }
}

void egui_view_group_calculate_layout(egui_view_t *self)
{
    if (self == NULL)
    {
        return;
    }

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

            egui_view_group_sync_child_core(self, tmp);
            egui_view_group_calculate_child_layout(tmp);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_group_dispatch_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self == NULL || event == NULL)
    {
        return 0;
    }

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
                egui_view_group_sync_child_core(self, tmp);
                if (tmp->api != NULL && tmp->api->dispatch_key_event != NULL && tmp->api->dispatch_key_event(tmp, event))
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
    EGUI_UNUSED(self);
    egui_view_set_layer(child, EGUI_VIEW_LAYER_TOP);
}

void egui_view_group_send_child_to_back(egui_view_t *self, egui_view_t *child)
{
    EGUI_UNUSED(self);
    egui_view_set_layer(child, EGUI_VIEW_LAYER_BACKGROUND);
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_group_t) = {
        .dispatch_touch_event = egui_view_group_dispatch_touch_event,
        .on_touch_event = egui_view_group_on_touch_event,
        .on_intercept_touch_event = egui_view_group_on_intercept_touch_event,
        .compute_scroll = EGUI_VIEW_GROUP_COMPUTE_SCROLL_HANDLER,
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
        .compute_scroll = EGUI_VIEW_GROUP_COMPUTE_SCROLL_HANDLER,
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

void egui_view_group_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_group_t);
    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_group_t);

    // init childs list.
    egui_dlist_init(&local->childs);

    egui_view_set_view_name(self, "egui_view_group");
}

void egui_view_root_group_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_root_group_t);

    egui_view_group_init(self, core);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_root_group_t);
    local->is_disallow_process_touch_event = 0;

    egui_view_set_view_name(self, "egui_view_root_group");
    egui_view_set_dirty_passthrough(self, 1);
}

void egui_view_group_apply_params(egui_view_t *self, const egui_view_group_params_t *params)
{
    self->region = params->region;

    egui_view_invalidate(self);
}

void egui_view_group_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_group_params_t *params)
{
    egui_view_group_init(self, core);
    egui_view_group_apply_params(self, params);
}
