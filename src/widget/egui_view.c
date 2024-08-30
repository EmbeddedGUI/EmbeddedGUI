#include <stdio.h>
#include <assert.h>

#include "egui_view.h"
#include "core/egui_core.h"
#include "core/egui_api.h"

void egui_view_invalidate(egui_view_t *self)
{
    self->api->request_layout(self);
}

void egui_view_set_background(egui_view_t *self, egui_background_t *background)
{
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

void egui_view_set_on_click_listener(egui_view_t *self, egui_view_on_click_listener_t listener)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->on_click_listener = listener;

    self->is_clickable = true;

    egui_view_invalidate(self);
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
}

void egui_view_set_on_touch_listener(egui_view_t *self, egui_view_on_touch_listener_t listener)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->on_touch_listener = listener;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
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
    self->is_visible = is_visible;

    egui_view_invalidate(self);
}

int egui_view_get_visible(egui_view_t *self)
{
    return self->is_visible;
}

void egui_view_set_gone(egui_view_t *self, int is_gone)
{
    self->is_gone = is_gone;

    egui_view_invalidate(self);
}

int egui_view_get_gone(egui_view_t *self)
{
    return self->is_gone;
}

void egui_view_set_padding(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top, egui_dim_margin_padding_t bottom)
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

void egui_view_set_margin(egui_view_t *self, egui_dim_margin_padding_t left, egui_dim_margin_padding_t right, egui_dim_margin_padding_t top, egui_dim_margin_padding_t bottom)
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

    if (self->is_enable && self->on_touch_listener != NULL && self->on_touch_listener(self, event))
    {
        return 1;
    }

    return self->api->on_touch_event(self, event);
}

int egui_view_perform_click(egui_view_t *self)
{
    if (self->on_click_listener != NULL)
    {
        self->on_click_listener(self);

        return 1;
    }

    return 0;
}

int egui_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    // EGUI_LOG_DBG("egui_view_on_touch_event id: 0x%x, %s\n", self->id, egui_motion_event_string(event->type));

    if (self->is_enable == false)
    {
        if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
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
            egui_view_set_pressed(self, false);
            egui_view_perform_click(self);
            break;
        case EGUI_MOTION_EVENT_ACTION_DOWN:
            egui_view_set_pressed(self, true);
            break;
        case EGUI_MOTION_EVENT_ACTION_MOVE:
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
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s, activity: %s\n", self->id, self->is_visible, self->name, activity->name);
    }
    else
    {
        EGUI_LOG_DBG("draw view id: %02d, visible: %d, name: %s\n", self->id, self->is_visible, self->name);
    }
#endif

    if (self->is_visible == false || self->is_gone == true)
    {
        return;
    }

    // clear canvas mask
    egui_canvas_clear_mask();
    // For fast drawing, we only draw the region that is intersected with the canvas.
    egui_canvas_calc_work_region(&self->region_screen);
    // set canvase alpha
    egui_canvas_mix_alpha(self->alpha);

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
    }
}

EGUI_VIEW_API_DEFINE(egui_view_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

void egui_view_init(egui_view_t *self)
{
    self->id = egui_core_get_unique_id();
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
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
    self->on_touch_listener = NULL;
    self->on_click_listener = NULL;
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

    self->alpha = EGUI_ALPHA_100;

    // init api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_t);

    egui_view_set_view_name(self, "egui_view");
}
