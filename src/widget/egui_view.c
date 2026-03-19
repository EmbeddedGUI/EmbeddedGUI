#include <stdio.h>
#include <assert.h>

#include "egui_view.h"
#include "core/egui_core.h"
#include "core/egui_api.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
#include "shadow/egui_shadow.h"
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
#include "core/egui_focus.h"
#endif

int egui_view_is_visible(egui_view_t *self)
{
    if ((!self->is_visible) || (self->is_gone))
    {
        return 0;
    }
    egui_view_t *p = (egui_view_t *)self->parent;
    while (p)
    {
        if ((!p->is_visible) || (p->is_gone))
        {
            return 0;
        }

        p = (egui_view_t *)p->parent;
    }

    return 1;
}

void egui_view_invalidate(egui_view_t *self)
{
    if (egui_view_is_visible(self))
    {
        self->api->request_layout(self);
    }
}

void egui_view_invalidate_region(egui_view_t *self, const egui_region_t *dirty_region)
{
    if (dirty_region == NULL || !egui_view_is_visible(self))
    {
        return;
    }

    EGUI_REGION_DEFINE(screen_region, self->region_screen.location.x + dirty_region->location.x, self->region_screen.location.y + dirty_region->location.y,
                       dirty_region->size.width, dirty_region->size.height);
    egui_region_intersect(&screen_region, &self->region_screen, &screen_region);

    if (!egui_region_is_empty(&screen_region))
    {
        egui_core_update_region_dirty(&screen_region);
    }
}

void egui_view_invalidate_sub_region(egui_view_t *self, const egui_sub_region_table_t *table, uint16_t index)
{
    if (table == NULL || table->regions == NULL || index >= table->count)
    {
        return;
    }

    egui_view_invalidate_region(self, &table->regions[index].region);
}

void egui_view_set_background(egui_view_t *self, egui_background_t *background)
{
    if (background == self->background)
    {
        return;
    }
    self->background = background;
    egui_view_invalidate(self);
}

void egui_view_draw_background(egui_view_t *self)
{
    if (self->background)
    {
        self->background->api->draw(self->background, self);
    }
}

void egui_view_set_parent(egui_view_t *self, egui_view_group_t *parent)
{
    self->parent = parent;
}

void egui_view_set_alpha(egui_view_t *self, egui_alpha_t alpha)
{
    if (alpha != self->alpha)
    {
        self->alpha = alpha;

        egui_view_invalidate(self);
    }
}

// TODO: need get raw pos static.
void egui_view_get_raw_pos(egui_view_t *self, egui_location_t *location)
{
    location->x += self->region.location.x;
    location->y += self->region.location.y;

    // recursion implement
    // if(self->parent)
    // {
    //     egui_view_get_raw_pos(self->parent, location);
    // }

    // implement without recursion
    egui_view_t *p = (egui_view_t *)self->parent;
    while (p)
    {
        location->x += p->region.location.x;
        location->y += p->region.location.y;

        p = (egui_view_t *)p->parent;
    }
}

void egui_view_layout(egui_view_t *self, egui_region_t *region)
{
    // update dirty region
    egui_core_update_region_dirty(&self->region_screen);

    // update region
    egui_region_copy(&self->region, region);

    // EGUI_LOG_DBG("region_dirty new: %d %d %d %d\n", self->region_dirty.location.x, self->region_dirty.location.y, self->region_dirty.size.width,
    // self->region_dirty.size.height);

    egui_view_invalidate(self);
}

void egui_view_scroll_to(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;
    egui_region_copy(&region, &self->region);

    region.location.x = x;
    region.location.y = y;

    egui_view_layout(self, &region);
}

void egui_view_scroll_by(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_region_t region;
    egui_region_copy(&region, &self->region);

    region.location.x += x;
    region.location.y += y;

    egui_view_layout(self, &region);
}

void egui_view_get_work_region(egui_view_t *self, egui_region_t *region)
{
    region->location.x = self->padding.left;
    region->location.y = self->padding.top;
    region->size.width = self->region.size.width - (self->padding.left + self->padding.right);
    region->size.height = self->region.size.height - (self->padding.top + self->padding.bottom);
}

void egui_view_copy_api(egui_view_t *self, egui_view_api_t *api)
{
    *api = *self->api;
    self->api = api;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
void egui_view_override_api_on_touch(egui_view_t *self, egui_view_api_t *api, egui_view_on_touch_listener_t listener)
{
    egui_view_copy_api(self, api);
    api->on_touch = listener;
}
#endif

void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->on_click_listener = listener;
    self->is_clickable = true;
    egui_view_invalidate(self);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
}

egui_view_on_click_listener_t egui_view_get_on_click_listener(egui_view_t *self)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    return self->on_click_listener;
#else
    EGUI_UNUSED(self);
    return NULL;
#endif
}

void egui_view_set_enable(egui_view_t *self, int is_enable)
{
    self->is_enable = is_enable;

    egui_view_invalidate(self);
}

int egui_view_get_enable(egui_view_t *self)
{
    return self->is_enable;
}

void egui_view_set_clickable(egui_view_t *self, int is_clickable)
{
    self->is_clickable = is_clickable;
}

int egui_view_get_clickable(egui_view_t *self)
{
    return self->is_clickable;
}

void egui_view_set_position(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    self->region.location.x = x;
    self->region.location.y = y;

    egui_view_invalidate(self);
}

void egui_view_set_size(egui_view_t *self, egui_dim_t width, egui_dim_t height)
{
    self->region.size.width = width;
    self->region.size.height = height;

    egui_view_invalidate(self);
}

void egui_view_set_pressed(egui_view_t *self, int is_pressed)
{
    self->is_pressed = is_pressed;

    egui_view_invalidate(self);
}

int egui_view_get_pressed(egui_view_t *self)
{
    return self->is_pressed;
}

void egui_view_set_visible(egui_view_t *self, int is_visible)
{
    if (is_visible == self->is_visible)
    {
        return;
    }
    // avoid self change to invisible.
    egui_view_invalidate(self);

    self->is_visible = is_visible;

    // avoid self change to invisible.
    egui_view_invalidate(self);
}

int egui_view_get_visible(egui_view_t *self)
{
    return self->is_visible;
}

void egui_view_set_gone(egui_view_t *self, int is_gone)
{
    if (is_gone == self->is_gone)
    {
        return;
    }

    // avoid self change to invisible.
    egui_view_invalidate(self);

    self->is_gone = is_gone;

    // avoid self change to invisible.
    egui_view_invalidate(self);
}

int egui_view_get_gone(egui_view_t *self)
{
    return self->is_gone;
}

void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                           egui_dim_margin_padding_t bottom)
{
    self->padding.left = left;
    self->padding.right = right;
    self->padding.top = top;
    self->padding.bottom = bottom;

    egui_view_invalidate(self);
}

void egui_view_set_padding_all(egui_view_t *self, egui_dim_margin_padding_t padding)
{
    egui_view_set_padding(self, padding, padding, padding, padding);
}

void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top,
                          egui_dim_margin_padding_t bottom)
{
    self->margin.left = left;
    self->margin.right = right;
    self->margin.top = top;
    self->margin.bottom = bottom;

    egui_view_invalidate(self);
}

void egui_view_set_margin_all(egui_view_t *self, egui_dim_margin_padding_t margin)
{
    egui_view_set_margin(self, margin, margin, margin, margin);
}

void egui_view_set_shadow(egui_view_t *self, const egui_shadow_t *shadow)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    self->shadow = shadow;
    egui_view_invalidate(self);
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(shadow);
#endif
}

void egui_view_set_view_name(egui_view_t *self, const char *name)
{
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    self->name = name;
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // view object should not work here. just return 0.
    // EGUI_LOG_DBG("egui_view_on_intercept_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

    return 0;
}

int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_dispatch_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

    if (self->is_enable && self->api->on_touch != NULL && self->api->on_touch(self, event))
    {
        return 1;
    }

    return self->api->on_touch_event(self, event);
}

int egui_view_perform_click(egui_view_t *self)
{
    int is_handled = 0;
    egui_view_on_click_listener_t listener = self->on_click_listener;

    if (self->api->perform_click != NULL)
    {
        is_handled = self->api->perform_click(self);
    }

    if (listener != NULL)
    {
        listener(self);
        is_handled = 1;
    }

    return is_handled;
}

int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));
    int is_inside = egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y);

    if (self->is_enable == false)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP || event->type == EGUI_MOTION_EVENT_ACTION_CANCEL)
        {
            egui_view_set_pressed(self, false);
        }
        // A disabled view that is clickable still consumes the touch
        // events, it just doesn't respond to them.
        return (self->is_clickable);
    }

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
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
            if (is_inside && self->is_focusable)
            {
                egui_view_request_focus(self);
            }
            else if (is_inside && !self->is_no_focus_clear)
            {
                // Clear focus when a non-focusable widget is touched
                // (e.g. dismiss on-screen keyboard when tapping other controls).
                // Skip if is_no_focus_clear is set (e.g. keyboard keys must not dismiss the keyboard).
                egui_focus_manager_clear_focus();
            }
#endif
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
#else
int egui_view_on_intercept_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}

int egui_view_dispatch_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}

int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    return 0;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

void egui_view_on_attach_to_window(egui_view_t *self)
{
    // EGUI_LOG_DBG("on_attach_to_window %d\n", self->id);
}

void egui_view_on_draw(egui_view_t *self)
{
    // EGUI_LOG_DBG("on_draw %d\n", self->id);
}

void egui_view_on_detach_from_window(egui_view_t *self)
{
    // EGUI_LOG_DBG("on_detach_from_window %d\n", self->id);
}

void egui_view_draw(egui_view_t *self)
{
    egui_alpha_t alpha = egui_canvas_get_alpha();

#if EGUI_CONFIG_DEBUG_CLASS_NAME
    egui_activity_t *activity = egui_core_activity_get_by_view(self);
    if (activity)
    {
#if EGUI_CONFIG_DEBUG_VIEW_ID
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s, activity: %s\n", self->id, self->is_visible, self->name, activity->name);
#else
        EGUI_LOG_DBG("draw view visible: %d, name: %s, activity: %s\n", self->is_visible, self->name, activity->name);
#endif
    }
    else
    {
#if EGUI_CONFIG_DEBUG_VIEW_ID
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s\n", self->id, self->is_visible, self->name);
#else
        EGUI_LOG_DBG("draw view visible: %d, name: %s\n", self->is_visible, self->name);
#endif
    }
#endif

    if (self->is_visible == false || self->is_gone == true)
    {
        return;
    }

    // clear canvas mask
    egui_canvas_clear_mask();
    // set canvase alpha
    egui_canvas_mix_alpha(self->alpha);

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    // draw shadow with expanded work region (shadow extends beyond view bounds)
    if (self->shadow != NULL)
    {
        egui_region_t shadow_region;
        egui_shadow_get_region(self->shadow, &self->region_screen, &shadow_region);
        egui_canvas_calc_work_region(&shadow_region);
        if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
        {
            egui_shadow_draw(self->shadow, &self->region_screen);
        }
    }
#endif

    // For fast drawing, we only draw the region that is intersected with the canvas.
    egui_canvas_calc_work_region(&self->region_screen);

    if (!egui_region_is_empty(egui_canvas_get_base_view_work_region()))
    {
        // draw background
        egui_view_draw_background(self);
        // call on_draw
        self->api->on_draw(self);
    }

    // restore canvas alpha
    egui_canvas_set_alpha(alpha);
}

void egui_view_request_layout(egui_view_t *self)
{
    self->is_request_layout = true;
}

void egui_view_compute_scroll(egui_view_t *self)
{
    // work in child process.
}

void egui_view_calculate_layout(egui_view_t *self)
{
    // recaculate layout raw pos
    if (self->is_request_layout)
    {
        self->is_request_layout = false;

        egui_region_t *p_raw_region = &self->region_screen;
        egui_view_t *p_parent = (egui_view_t *)self->parent;
        // get parent raw pos, the parent raw pos is already calculated.
        if (p_parent)
        {
            egui_region_intersect_with_size(&self->region, p_parent->region_screen.size.width - (p_parent->padding.left + p_parent->padding.right),
                                            p_parent->region_screen.size.height - (p_parent->padding.top + p_parent->padding.bottom), p_raw_region);
            p_raw_region->location.x += p_parent->region_screen.location.x + p_parent->padding.left;
            p_raw_region->location.y += p_parent->region_screen.location.y + p_parent->padding.top;

            // EGUI_LOG_DBG("id: 0x%x\n", self->id);
            // EGUI_LOG_DBG("parent, raw_region: %d %d %d %d\n", p_parent->region_screen.location.x, p_parent->region_screen.location.y,
            // p_parent->region_screen.size.width, p_parent->region_screen.size.height); EGUI_LOG_DBG("self, region: %d %d %d %d\n", self->region.location.x,
            // self->region.location.y, self->region.size.width, self->region.size.height); EGUI_LOG_DBG("self, raw_region: %d %d %d %d\n",
            // p_raw_region->location.x, p_raw_region->location.y, p_raw_region->size.width, p_raw_region->size.height);
        }
        else
        {
            egui_region_copy(p_raw_region, &self->region);

            // EGUI_LOG_DBG("no parent, raw_region: %d %d %d %d\n", p_raw_region->location.x, p_raw_region->location.y, p_raw_region->size.width,
            // p_raw_region->size.height);
        }

        // update dirty region
        egui_core_update_region_dirty(p_raw_region);
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
        if (self->shadow != NULL)
        {
            egui_region_t shadow_region;
            egui_shadow_get_region(self->shadow, p_raw_region, &shadow_region);
            egui_core_update_region_dirty(&shadow_region);
        }
#endif
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
int egui_view_dispatch_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self->is_enable && self->api->on_key != NULL && self->api->on_key(self, event))
    {
        return 1;
    }

    return self->api->on_key_event(self, event);
}

void egui_view_override_api_on_key(egui_view_t *self, egui_view_api_t *api, egui_view_on_key_listener_t listener)
{
    egui_view_copy_api(self, api);
    api->on_key = listener;
}

int egui_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (self->is_enable == false)
    {
        return 0;
    }

    // If clickable, ENTER key triggers click
    if (self->is_clickable)
    {
        if (event->type == EGUI_KEY_EVENT_ACTION_UP && event->key_code == EGUI_KEY_CODE_ENTER)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
            egui_view_perform_click(self);
#endif
            return 1;
        }
        if (event->type == EGUI_KEY_EVENT_ACTION_DOWN && event->key_code == EGUI_KEY_CODE_ENTER)
        {
            return 1; // consume the down event
        }
    }

    return 0;
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_KEY

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
void egui_view_set_focusable(egui_view_t *self, int is_focusable)
{
    self->is_focusable = is_focusable;
}

void egui_view_override_api_on_focus_changed(egui_view_t *self, egui_view_api_t *api, egui_view_on_focus_change_listener_t listener)
{
    egui_view_copy_api(self, api);
    api->on_focus_changed = listener;
}

int egui_view_get_focusable(egui_view_t *self)
{
    return self->is_focusable;
}

void egui_view_request_focus(egui_view_t *self)
{
    if (self->is_focusable && self->is_enable && self->is_visible && !self->is_gone)
    {
        egui_focus_manager_set_focus(self);
    }
}

void egui_view_clear_focus(egui_view_t *self)
{
    if (self->is_focused)
    {
        egui_focus_manager_clear_focus();
    }
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
void egui_view_set_layer(egui_view_t *self, uint8_t layer)
{
    if (self->layer == layer)
    {
        return;
    }

    self->layer = layer;

    if (self->parent != NULL)
    {
        egui_view_group_reorder_child((egui_view_t *)self->parent, self);
    }

    egui_view_invalidate(self);
}

uint8_t egui_view_get_layer(egui_view_t *self)
{
    return self->layer;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch = NULL,
        .perform_click = NULL,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .on_key = NULL,
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        .on_focus_changed = NULL,
#endif
};

void egui_view_init(egui_view_t *self)
{
#if EGUI_CONFIG_DEBUG_VIEW_ID
    self->id = egui_core_get_unique_id();
#endif
    self->parent = NULL; // set parent later
    self->node.next = NULL;

    self->region.location.x = 0;
    self->region.location.y = 0;
    self->region.size.width = 0;
    self->region.size.height = 0;

    self->padding.left = 0;
    self->padding.right = 0;
    self->padding.top = 0;
    self->padding.bottom = 0;

    self->margin.left = 0;
    self->margin.right = 0;
    self->margin.top = 0;
    self->margin.bottom = 0;

    self->is_enable = true;

    self->is_clickable = false;
    self->is_pressed = false;
    self->is_visible = true;
    self->is_gone = false;
    self->is_request_layout = true;

    self->background = NULL;
#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
    self->shadow = NULL;
#endif
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->on_click_listener = NULL;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = false;
    self->is_focused = false;
    self->is_no_focus_clear = 0;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS

#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
    self->layer = EGUI_VIEW_LAYER_DEFAULT;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_LAYER

    self->alpha = EGUI_ALPHA_100;

    // init api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_t);

    egui_view_set_view_name(self, "egui_view");
}
