#include <assert.h>

#include "egui_view_number_picker.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

static const egui_font_t *egui_view_number_picker_get_icon_font(egui_view_number_picker_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    if (area_size <= 18)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= 22)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
}

static void egui_view_number_picker_get_zone_region(egui_view_t *self, int8_t zone, egui_region_t *zone_region)
{
    egui_region_t region;
    egui_dim_t third_h;

    egui_view_get_work_region(self, &region);
    third_h = region.size.height / 3;

    zone_region->location.x = region.location.x;
    zone_region->size.width = region.size.width;

    if (zone > 0)
    {
        zone_region->location.y = region.location.y;
        zone_region->size.height = third_h;
    }
    else if (zone < 0)
    {
        zone_region->location.y = region.location.y + region.size.height - third_h;
        zone_region->size.height = third_h;
    }
    else
    {
        zone_region->location.y = region.location.y + third_h;
        zone_region->size.height = region.size.height - third_h * 2;
    }
}

static void egui_view_number_picker_local_region_to_screen(egui_view_t *self, const egui_region_t *local_region, egui_region_t *screen_region)
{
    screen_region->location.x = self->region_screen.location.x + local_region->location.x;
    screen_region->location.y = self->region_screen.location.y + local_region->location.y;
    screen_region->size.width = local_region->size.width;
    screen_region->size.height = local_region->size.height;
}

static void egui_view_number_picker_invalidate_zone(egui_view_t *self, int8_t zone)
{
    egui_region_t zone_region;

    egui_view_number_picker_get_zone_region(self, zone, &zone_region);
    egui_view_invalidate_region(self, &zone_region);
}

void egui_view_number_picker_set_on_value_changed_listener(egui_view_t *self, egui_view_on_number_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    local->on_value_changed = listener;
}

void egui_view_number_picker_set_value(egui_view_t *self, int16_t value)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    if (value < local->min_value)
    {
        value = local->min_value;
    }
    if (value > local->max_value)
    {
        value = local->max_value;
    }
    if (value != local->value)
    {
        local->value = value;
        if (local->on_value_changed)
        {
            local->on_value_changed(self, value);
        }
        egui_view_number_picker_invalidate_zone(self, 0);
    }
}

int16_t egui_view_number_picker_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    return local->value;
}

void egui_view_number_picker_set_range(egui_view_t *self, int16_t min_value, int16_t max_value)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    local->min_value = min_value;
    local->max_value = max_value;
    // Clamp current value
    if (local->value < min_value)
    {
        local->value = min_value;
    }
    if (local->value > max_value)
    {
        local->value = max_value;
    }
    egui_view_invalidate(self);
}

void egui_view_number_picker_set_step(egui_view_t *self, int16_t step)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    local->step = step;
}

void egui_view_number_picker_set_button_icons(egui_view_t *self, const char *icon_inc, const char *icon_dec)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);

    if (local->icon_inc == icon_inc && local->icon_dec == icon_dec)
    {
        return;
    }

    local->icon_inc = icon_inc;
    local->icon_dec = icon_dec;
    egui_view_invalidate(self);
}

void egui_view_number_picker_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_number_picker_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);

    egui_region_t top_rect;
    egui_region_t mid_rect;
    egui_region_t bottom_rect;
    egui_region_t screen_rect;
    egui_dim_t w;
    egui_dim_t third_h;

    egui_view_number_picker_get_zone_region(self, 1, &top_rect);
    egui_view_number_picker_get_zone_region(self, 0, &mid_rect);
    egui_view_number_picker_get_zone_region(self, -1, &bottom_rect);

    w = top_rect.size.width;
    third_h = top_rect.size.height;

    // Horizontal divider lines between sections (subtle, semi-transparent)
    {
        egui_dim_t margin = 8;
        egui_dim_t div1_y = mid_rect.location.y;
        egui_dim_t div2_y = bottom_rect.location.y;

        egui_canvas_draw_line(top_rect.location.x + margin, div1_y, top_rect.location.x + w - margin, div1_y, 1, local->button_color, 60);
        egui_canvas_draw_line(top_rect.location.x + margin, div2_y, top_rect.location.x + w - margin, div2_y, 1, local->button_color, 60);
    }

    // Top 1/3: up arrow icon
    {
        egui_view_number_picker_local_region_to_screen(self, &top_rect, &screen_rect);
        if (egui_canvas_is_region_active(&screen_rect))
        {
            const egui_font_t *icon_font = egui_view_number_picker_get_icon_font(local, EGUI_MIN(w, third_h) - 4);

            if (self->is_pressed && local->pressed_zone == 1)
            {
                egui_canvas_draw_fillrect(top_rect.location.x, top_rect.location.y, top_rect.size.width, top_rect.size.height, EGUI_COLOR_MAKE(255, 255, 255),
                                          30);
            }
            egui_canvas_draw_text_in_rect(icon_font, local->icon_inc, &top_rect, EGUI_ALIGN_CENTER, local->button_color, local->alpha);
        }
    }

    // Middle 1/3: number text (between div1 and div2, using actual boundaries)
    {
        egui_view_number_picker_local_region_to_screen(self, &mid_rect, &screen_rect);
        if (local->font != NULL && egui_canvas_is_region_active(&screen_rect))
        {
            egui_sprintf_int(local->text_buf, sizeof(local->text_buf), local->value);
            egui_canvas_draw_text_in_rect(local->font, local->text_buf, &mid_rect, EGUI_ALIGN_CENTER, local->text_color, local->alpha);
        }
    }

    // Bottom 1/3: down arrow icon
    {
        egui_view_number_picker_local_region_to_screen(self, &bottom_rect, &screen_rect);
        if (egui_canvas_is_region_active(&screen_rect))
        {
            const egui_font_t *icon_font = egui_view_number_picker_get_icon_font(local, EGUI_MIN(w, third_h) - 4);

            if (self->is_pressed && local->pressed_zone == -1)
            {
                egui_canvas_draw_fillrect(bottom_rect.location.x, bottom_rect.location.y, bottom_rect.size.width, bottom_rect.size.height,
                                          EGUI_COLOR_MAKE(255, 255, 255), 30);
            }
            egui_canvas_draw_text_in_rect(icon_font, local->icon_dec, &bottom_rect, EGUI_ALIGN_CENTER, local->button_color, local->alpha);
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_number_picker_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);
    egui_dim_t local_y = event->location.y - self->region_screen.location.y;
    egui_dim_t third_h = self->region.size.height / 3;
    int is_inside = egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y);
    int8_t hit_zone = 0;

    if (is_inside)
    {
        if (local_y < third_h)
        {
            hit_zone = 1;
        }
        else if (local_y >= third_h * 2)
        {
            hit_zone = -1;
        }
    }

    if (self->is_enable == false)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        egui_region_t zone_region;
        const egui_region_t *dirty_ptr = NULL;

        local->pressed_zone = hit_zone;
        if (hit_zone != 0)
        {
            egui_view_number_picker_get_zone_region(self, hit_zone, &zone_region);
            dirty_ptr = &zone_region;
        }
        egui_view_set_pressed_with_region(self, hit_zone != 0, dirty_ptr);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        int should_press = local->pressed_zone != 0 && hit_zone == local->pressed_zone;
        if (self->is_pressed != should_press)
        {
            egui_region_t zone_region;

            egui_view_number_picker_get_zone_region(self, local->pressed_zone, &zone_region);
            egui_view_set_pressed_with_region(self, should_press, &zone_region);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int8_t pressed_zone = local->pressed_zone;
        egui_region_t zone_region;
        const egui_region_t *dirty_ptr = NULL;

        if (pressed_zone != 0)
        {
            egui_view_number_picker_get_zone_region(self, pressed_zone, &zone_region);
            dirty_ptr = &zone_region;
        }

        egui_view_set_pressed_with_region(self, false, dirty_ptr);
        local->pressed_zone = 0;

        if (pressed_zone != hit_zone)
        {
            break;
        }

        if (hit_zone == 1)
        {
            // Top area: increment
            int16_t new_val = local->value + local->step;
            if (new_val > local->max_value)
            {
                new_val = local->max_value;
            }
            egui_view_number_picker_set_value(self, new_val);
        }
        else if (hit_zone == -1)
        {
            // Bottom area: decrement
            int16_t new_val = local->value - local->step;
            if (new_val < local->min_value)
            {
                new_val = local->min_value;
            }
            egui_view_number_picker_set_value(self, new_val);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        int8_t pressed_zone = local->pressed_zone;
        egui_region_t zone_region;
        const egui_region_t *dirty_ptr = NULL;

        if (pressed_zone != 0)
        {
            egui_view_number_picker_get_zone_region(self, pressed_zone, &zone_region);
            dirty_ptr = &zone_region;
        }

        egui_view_set_pressed_with_region(self, false, dirty_ptr);
        local->pressed_zone = 0;
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_number_picker_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_number_picker_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_number_picker_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_number_picker_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_number_picker_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_number_picker_t);

    // init local data.
    local->on_value_changed = NULL;
    local->value = 0;
    local->min_value = 0;
    local->max_value = 100;
    local->step = 1;
    local->alpha = EGUI_ALPHA_100;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->button_color = EGUI_THEME_PRIMARY;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_inc = EGUI_ICON_MS_KEYBOARD_ARROW_UP;
    local->icon_dec = EGUI_ICON_MS_KEYBOARD_ARROW_DOWN;
    local->icon_font = NULL;
    local->pressed_zone = 0;

    egui_view_set_view_name(self, "egui_view_number_picker");
}

void egui_view_number_picker_apply_params(egui_view_t *self, const egui_view_number_picker_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_number_picker_t);

    self->region = params->region;

    local->value = params->value;
    local->min_value = params->min_value;
    local->max_value = params->max_value;

    egui_view_invalidate(self);
}

void egui_view_number_picker_init_with_params(egui_view_t *self, const egui_view_number_picker_params_t *params)
{
    egui_view_number_picker_init(self);
    egui_view_number_picker_apply_params(self, params);
}
