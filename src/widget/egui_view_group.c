#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_group.h"

void egui_view_group_add_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_view_set_parent(child, local);

    egui_dlist_append(&local->childs, &child->node);
}

void egui_view_group_remove_child(egui_view_t *self, egui_view_t *child)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    if(child == local->first_touch_target)
    {
        // Clear.
        local->first_touch_target = NULL;
    }
    egui_dlist_remove(&child->node);
}

void egui_view_group_clear_childs(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;

    egui_dlist_init(&local->childs);
}

int egui_view_group_get_child_count(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    return egui_dlist_size(&local->childs);
}

egui_view_t *egui_view_group_get_first_child(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *snode = egui_dlist_peek_head(&local->childs);
    if (snode == NULL)
    {
        return NULL;
    }
    return EGUI_DLIST_ENTRY(snode, egui_view_t, node);
}

void egui_view_group_set_disallow_process_touch_event(egui_view_t *self, int disallow)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    local->is_disallow_process_touch_event = disallow;
}

void egui_view_group_calculate_all_child_width(egui_view_t *self, egui_dim_t *width)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *width = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if(tmp->is_gone)
            {
                continue;
            }
            *width += tmp->region.size.width + tmp->margin.left + tmp->margin.right;
        }
    }
}

void egui_view_group_calculate_all_child_height(egui_view_t *self, egui_dim_t *height)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *height = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if(tmp->is_gone)
            {
                continue;
            }
            *height += tmp->region.size.height + tmp->margin.top + tmp->margin.bottom;
        }
    }
}

void egui_view_group_get_max_child_width(egui_view_t *self, egui_dim_t *width)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *width = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if(tmp->is_gone)
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
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    *height = 0;
    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            if(tmp->is_gone)
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
    egui_view_group_t *local = (egui_view_group_t *)self;

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
        egui_view_set_size(self, is_auto_width ? total_child_width : self->region.size.width,
                           is_auto_height ? total_child_height : self->region.size.height);
    }

    egui_dim_t parent_width = self->region.size.width;
    egui_dim_t parent_height = self->region.size.height;

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

            if(tmp->is_gone)
            {
                continue;
            }
            if (is_orientation_horizontal)
            {
                egui_common_align_get_x_y(total_child_width, total_child_height, tmp->region.size.width + tmp->margin.left + tmp->margin.right,
                                          tmp->region.size.height + tmp->margin.top + tmp->margin.bottom, align_type & EGUI_ALIGN_VMASK, &child_x,
                                          &child_y);

                egui_view_set_position(tmp, x + child_x + tmp->margin.left, y + child_y + tmp->margin.top);

                x += tmp->region.size.width + tmp->margin.left + tmp->margin.right;
            }
            else
            {
                egui_common_align_get_x_y(total_child_width, total_child_height, tmp->region.size.width + tmp->margin.left + tmp->margin.right,
                                          tmp->region.size.height + tmp->margin.top + tmp->margin.bottom, align_type & EGUI_ALIGN_HMASK, &child_x,
                                          &child_y);

                egui_view_set_position(tmp, x + child_x + tmp->margin.left, y + child_y + tmp->margin.top);

                y += tmp->region.size.height + tmp->margin.top + tmp->margin.bottom;
            }
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_group_request_disallow_intercept_touch_event(egui_view_t *self, int disallow)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    // EGUI_LOG_DBG("egui_view_group_request_disallow_intercept_touch_event id: 0x%x, old: %d, new: %d\n", self->id, local->is_disallow_intercept, disallow);
    if (local->is_disallow_intercept == disallow)
    {
        return;
    }

    local->is_disallow_intercept = disallow;

    if (self->parent != NULL)
    {
        egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, disallow);
    }
}

int egui_view_group_dispatch_transformed_touch_event(egui_view_t *self, int is_canceled, egui_view_t *child, egui_motion_event_t *event)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_motion_event_t transformed_event;
    memcpy(&transformed_event, event, sizeof(egui_motion_event_t));

    // change to cancel event if is_canceled is true.
    if (is_canceled)
    {
        transformed_event.type = EGUI_MOTION_EVENT_ACTION_CANCEL;
        local->first_touch_target = NULL;
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
    egui_view_group_t *local = (egui_view_group_t *)self;
    EGUI_UNUSED(local);

    // if(event->type == EGUI_MOTION_EVENT_ACTION_DOWN
    //     && isOnScrollbarThumb())
    // {
    //     return 1;
    // }

    return 0;
}

int egui_view_group_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    int is_handled = 0;
    int is_intercepted = 0;
    int is_already_dispatched_to_new_touch_target = 0;

    // EGUI_LOG_DBG("egui_view_group_dispatch_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    egui_view_group_t *local = (egui_view_group_t *)self;

    // disallow process touch event if is_disallow_process_touch_event is 1.
    if (local->is_disallow_process_touch_event)
    {
        return is_handled;
    }

    // Step1: Check if the view group intercepts the touch event.
    // If event type is ACTION_DOWN, or first_touch_target is set before(means the child view has already intercepted the event).
    if ((event->type == EGUI_MOTION_EVENT_ACTION_DOWN) || (local->first_touch_target != NULL))
    {
        // clear last touch target if event type is ACTION_DOWN.
        if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
        {
            local->first_touch_target = NULL;
            local->is_disallow_intercept = 0;
        }

        // EGUI_LOG_DBG("egui_view_group_dispatch_touch_event id: 0x%x, disallow_intercept: %d\n", self->id, local->is_disallow_intercept);
        if (!local->is_disallow_intercept)
        {
            is_intercepted = self->api->on_intercept_touch_event(self, event);
        }
        else
        {
            is_intercepted = 0;
        }
    }
    else
    {
        // There are no touch targets and this action is not an initial down
        // so this view group continues to intercept touches.
        is_intercepted = 1;
    }

    int is_canceled = event->type == EGUI_MOTION_EVENT_ACTION_CANCEL;

    // Step2: Dispatch the touch event to child views.
    if (!is_canceled && !is_intercepted)
    {
        egui_dnode_t *p_head;
        egui_view_t *tmp;

        if (event->type == EGUI_MOTION_EVENT_ACTION_DOWN)
        {
            // Find a child that can receive the event.
            if (!egui_dlist_is_empty(&local->childs))
            {
                // Scan children from trail to head.
                EGUI_DLIST_FOR_EACH_NODE_REVERSE(&local->childs, p_head)
                {
                    tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

                    // EGUI_LOG_DBG("found child id: 0x%x\n", tmp->id);
                    // Check the point is in the child's region.
                    if (!egui_region_pt_in_rect(&tmp->region_screen, event->location.x, event->location.y))
                    {
                        // EGUI_LOG_DBG("child not in region, id: 0x%x\n", tmp->id);
                        continue;
                    }

                    // Dispatch the event to the child.
                    if (egui_view_group_dispatch_transformed_touch_event(self, false, tmp, event))
                    {
                        // If a child can receive the event, set it as the first_touch_target.
                        local->first_touch_target = tmp;
                        is_already_dispatched_to_new_touch_target = 1;
                        break;
                    }
                }
            }
        }
    }

    // EGUI_LOG_DBG("id: 0x%x, is_intercepted: %d, is_canceled: %d, is_handled: %d, first_touch_target: 0x%x\n", self->id, is_intercepted, is_canceled, is_handled,
    //        local->first_touch_target);

    // Step3: Check first_touch_target and dispatch the event to it.
    if (local->first_touch_target == NULL)
    {
        // If the first_touch_target is not set, just call egui_view_group_dispatch_transformed_touch_event, but not set child.
        // If no child can receive the event, the egui_view_group_t will be handled by local on_touch_event.
        is_handled = egui_view_group_dispatch_transformed_touch_event(self, is_canceled, NULL, event);
    }
    else
    {
        // Dispatch to touch targets, excluding the new touch target if we already
        // dispatched to it.  Cancel touch targets if necessary.
        if (is_already_dispatched_to_new_touch_target)
        {
            is_handled = 1;
        }
        else
        {
            int cancelChild = is_intercepted;
            is_handled = egui_view_group_dispatch_transformed_touch_event(self, cancelChild, local->first_touch_target, event);
        }
    }

    return is_handled;
}

int egui_view_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_group_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    egui_view_group_t *local = (egui_view_group_t *)self;
    EGUI_UNUSED(local);
    if (self->is_clickable)
    {
        switch (event->type)
        {
        case EGUI_MOTION_EVENT_ACTION_UP:
            egui_view_perform_click(self);
            break;
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            break;
        case EGUI_MOTION_EVENT_ACTION_MOVE:
            break;
        case EGUI_MOTION_EVENT_ACTION_CANCEL:
            break;
        default:
            break;
        }

        // if view clickable, return 1 to stop dispatch touch event to parent.
        return 1;
    }

    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_group_on_attach_to_window(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            tmp->api->on_attach_to_window(tmp);
        }
    }
}

void egui_view_group_on_detach_from_window(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    egui_dnode_t *p_head;
    egui_view_t *tmp;

    if (!egui_dlist_is_empty(&local->childs))
    {
        EGUI_DLIST_FOR_EACH_NODE(&local->childs, p_head)
        {
            tmp = EGUI_DLIST_ENTRY(p_head, egui_view_t, node);

            tmp->api->on_detach_from_window(tmp);
        }
    }
}

void egui_view_group_draw(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
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
    egui_view_group_t *local = (egui_view_group_t *)self;
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
    egui_view_group_t *local = (egui_view_group_t *)self;
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
    egui_view_group_t *local = (egui_view_group_t *)self;
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

EGUI_VIEW_API_DEFINE_BASE_GROUP(egui_view_group_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

void egui_view_group_init(egui_view_t *self)
{
    egui_view_group_t *local = (egui_view_group_t *)self;
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_group_t);

    // init local data.
    local->first_touch_target = NULL;
    local->is_disallow_intercept = 0;
    local->is_disallow_process_touch_event = 0;
    // init childs list.
    egui_dlist_init(&local->childs);

    egui_view_set_view_name(self, "egui_view_group");
}
