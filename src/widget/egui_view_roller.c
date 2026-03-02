#include <stdio.h>
#include <assert.h>

#include "egui_view_roller.h"
#include "core/egui_canvas_gradient.h"
#include "widget/egui_view_group.h"
#include "resource/egui_resource.h"

void egui_view_roller_set_on_selected_listener(egui_view_t *self, egui_view_on_roller_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);
    local->on_selected = listener;
}

void egui_view_roller_set_items(egui_view_t *self, const char **items, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);
    local->items = items;
    local->item_count = count;
    if (local->current_index >= count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_roller_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);
    if (index >= local->item_count)
    {
        return;
    }
    if (index != local->current_index)
    {
        local->current_index = index;
        local->scroll_offset = 0;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_roller_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);
    return local->current_index;
}

void egui_view_roller_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);

    if (local->items == NULL || local->item_count == 0 || local->font == NULL)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t item_height = region.size.height / local->visible_count;
    if (item_height <= 0)
    {
        return;
    }

    // Draw highlight background for center row
    egui_dim_t center_row_index = local->visible_count / 2;
    egui_dim_t highlight_y = region.location.y + center_row_index * item_height;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->highlight_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->highlight_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_rectangle_fill_gradient(region.location.x, highlight_y, region.size.width, item_height, &grad);
    }
#else
    egui_canvas_draw_rectangle_fill(region.location.x, highlight_y, region.size.width, item_height, local->highlight_color, EGUI_ALPHA_100);
#endif

    // Draw visible items
    int16_t half = local->visible_count / 2;
    for (int16_t row = 0; row < local->visible_count; row++)
    {
        int16_t item_index = (int16_t)local->current_index - half + row;

        if (item_index < 0 || item_index >= local->item_count)
        {
            continue;
        }

        egui_dim_t text_y = region.location.y + row * item_height + local->scroll_offset;
        egui_region_t row_rect;
        row_rect.location.x = region.location.x;
        row_rect.location.y = text_y;
        row_rect.size.width = region.size.width;
        row_rect.size.height = item_height;

        egui_color_t color = (item_index == local->current_index) ? local->selected_text_color : local->text_color;

        egui_canvas_draw_text_in_rect(local->font, local->items[item_index], &row_rect, EGUI_ALIGN_CENTER, color, EGUI_ALPHA_100);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_roller_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);

    if (self->is_enable == false || local->items == NULL || local->item_count == 0)
    {
        return 0;
    }

    egui_dim_t item_height = self->region.size.height / local->visible_count;
    if (item_height <= 0)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        local->last_touch_y = event->location.y;
        local->is_dragging = 1;
        local->scroll_offset = 0;

        // Request parent to not intercept touch events
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (local->is_dragging)
        {
            egui_dim_t diff = event->location.y - local->last_touch_y;
            local->scroll_offset += diff;
            local->last_touch_y = event->location.y;

            // Check if scrolled enough to change index
            if (local->scroll_offset >= item_height)
            {
                // Scrolled down -> previous item
                if (local->current_index > 0)
                {
                    local->current_index--;
                }
                local->scroll_offset -= item_height;
            }
            else if (local->scroll_offset <= -item_height)
            {
                // Scrolled up -> next item
                if (local->current_index < local->item_count - 1)
                {
                    local->current_index++;
                }
                local->scroll_offset += item_height;
            }

            egui_view_invalidate(self);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        local->is_dragging = 0;
        local->scroll_offset = 0;
        egui_view_invalidate(self);

        if (local->on_selected)
        {
            local->on_selected(self, local->current_index);
        }
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_roller_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_roller_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_roller_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_roller_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_roller_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_roller_t);

    // init local data.
    local->on_selected = NULL;
    local->items = NULL;
    local->item_count = 0;
    local->current_index = 0;
    local->visible_count = 3;
    local->scroll_offset = 0;
    local->last_touch_y = 0;
    local->is_dragging = 0;
    local->text_color = EGUI_THEME_TEXT_SECONDARY;
    local->selected_text_color = EGUI_THEME_TEXT_PRIMARY;
    local->highlight_color = EGUI_THEME_TRACK_BG;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    egui_view_set_view_name(self, "egui_view_roller");
}

void egui_view_roller_apply_params(egui_view_t *self, const egui_view_roller_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_roller_t);

    self->region = params->region;

    local->items = params->items;
    local->item_count = params->item_count;
    local->current_index = params->current_index;

    egui_view_invalidate(self);
}

void egui_view_roller_init_with_params(egui_view_t *self, const egui_view_roller_params_t *params)
{
    egui_view_roller_init(self);
    egui_view_roller_apply_params(self, params);
}
