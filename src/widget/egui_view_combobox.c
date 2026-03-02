#include <stdio.h>
#include <assert.h>

#include "egui_view_combobox.h"
#include "widget/egui_view_group.h"
#include "resource/egui_resource.h"
#include "core/egui_core.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_combobox_set_on_selected_listener(egui_view_t *self, egui_view_on_combobox_selected_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->on_selected = listener;
}

void egui_view_combobox_set_items(egui_view_t *self, const char **items, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->items = items;
    local->item_count = count;
    if (local->current_index >= count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_combobox_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (index >= local->item_count)
    {
        return;
    }
    if (index != local->current_index)
    {
        local->current_index = index;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_combobox_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->current_index;
}

const char *egui_view_combobox_get_current_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->items == NULL || local->item_count == 0)
    {
        return NULL;
    }
    return local->items[local->current_index];
}

void egui_view_combobox_set_max_visible_items(egui_view_t *self, uint8_t max_items)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (max_items < 1)
    {
        max_items = 1;
    }
    local->max_visible_items = max_items;
    egui_view_invalidate(self);
}

void egui_view_combobox_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_combobox_expand(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (!local->is_expanded && local->item_count > 0)
    {
        local->is_expanded = 1;
#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
        egui_view_set_layer(self, EGUI_VIEW_LAYER_TOP);
#endif
        uint8_t visible_count = local->item_count;
        if (visible_count > local->max_visible_items)
        {
            visible_count = local->max_visible_items;
        }
        egui_dim_t expanded_height = local->collapsed_height + visible_count * local->item_height;
        // Mark old region dirty before changing size
        egui_core_update_region_dirty(&self->region_screen);
        self->region.size.height = expanded_height;
        self->region_screen.size.height = expanded_height;
        // Mark new region dirty as well
        egui_core_update_region_dirty(&self->region_screen);
        egui_view_invalidate(self);
    }
}

void egui_view_combobox_collapse(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    if (local->is_expanded)
    {
        local->is_expanded = 0;
#if EGUI_CONFIG_FUNCTION_SUPPORT_LAYER
        egui_view_set_layer(self, EGUI_VIEW_LAYER_DEFAULT);
#endif
        // Mark old (expanded) region dirty before changing size
        egui_core_update_region_dirty(&self->region_screen);
        self->region.size.height = local->collapsed_height;
        self->region_screen.size.height = local->collapsed_height;
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_combobox_is_expanded(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);
    return local->is_expanded;
}

static void egui_view_combobox_draw_arrow(egui_dim_t x, egui_dim_t y, egui_dim_t size, uint8_t is_down, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t half = size / 2;
    if (is_down)
    {
        egui_canvas_draw_triangle_fill(x, y, x + size, y, x + half, y + half, color, alpha);
    }
    else
    {
        egui_canvas_draw_triangle_fill(x + half, y, x, y + half, x + size, y + half, color, alpha);
    }
}

void egui_view_combobox_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (local->font == NULL)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t header_height = local->collapsed_height;
    egui_dim_t padding = 4;
    egui_dim_t arrow_size = 8;
    egui_dim_t border_radius = EGUI_THEME_RADIUS_SM;

    if (local->is_expanded)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->bg_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->bg_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, local->bg_color,
                                              local->alpha);
#endif
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, border_radius, 1, local->border_color,
                                         local->alpha);
    }
    else
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->bg_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->bg_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, header_height, border_radius, &grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, header_height, border_radius, local->bg_color,
                                              local->alpha);
#endif
        egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, header_height, border_radius, 1, local->border_color,
                                         local->alpha);
    }

    egui_region_t text_rect;
    text_rect.location.x = region.location.x + padding;
    text_rect.location.y = region.location.y;
    text_rect.size.width = region.size.width - padding * 2 - arrow_size - padding;
    text_rect.size.height = header_height;

    if (local->items != NULL && local->item_count > 0)
    {
        egui_canvas_draw_text_in_rect(local->font, local->items[local->current_index], &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color,
                                      local->alpha);
    }

    egui_dim_t arrow_x = region.location.x + region.size.width - arrow_size - padding;
    egui_dim_t arrow_y = region.location.y + (header_height - arrow_size / 2) / 2;
    egui_view_combobox_draw_arrow(arrow_x, arrow_y, arrow_size, !local->is_expanded, local->arrow_color, local->alpha);

    if (local->is_expanded && local->items != NULL)
    {
        egui_dim_t item_y = region.location.y + header_height;
        uint8_t visible_count = local->item_count;
        if (visible_count > local->max_visible_items)
        {
            visible_count = local->max_visible_items;
        }

        for (uint8_t i = 0; i < visible_count; i++)
        {
            egui_region_t item_rect;
            item_rect.location.x = region.location.x + padding;
            item_rect.location.y = item_y;
            item_rect.size.width = region.size.width - padding * 2;
            item_rect.size.height = local->item_height;

            if (i == local->current_index)
            {
                egui_canvas_draw_rectangle_fill(region.location.x + 1, item_y, region.size.width - 2, local->item_height, local->highlight_color, local->alpha);
            }

            egui_canvas_draw_text_in_rect(local->font, local->items[i], &item_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, local->alpha);

            item_y += local->item_height;
        }
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_combobox_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    if (self->is_enable == false || local->items == NULL || local->item_count == 0)
    {
        return 0;
    }

    if (event->type == EGUI_MOTION_EVENT_ACTION_UP)
    {
        egui_dim_t local_y = event->location.y - self->region_screen.location.y;

        if (local->is_expanded)
        {
            if (local_y < local->collapsed_height)
            {
                egui_view_combobox_collapse(self);
            }
            else
            {
                egui_dim_t item_y = local_y - local->collapsed_height;
                uint8_t clicked_index = item_y / local->item_height;

                uint8_t visible_count = local->item_count;
                if (visible_count > local->max_visible_items)
                {
                    visible_count = local->max_visible_items;
                }

                if (clicked_index < visible_count)
                {
                    local->current_index = clicked_index;
                    egui_view_combobox_collapse(self);

                    if (local->on_selected)
                    {
                        local->on_selected(self, local->current_index);
                    }
                }
            }
        }
        else
        {
            egui_view_combobox_expand(self);
        }
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_combobox_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_combobox_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_combobox_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_combobox_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_combobox_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_combobox_t);

    // init local data.
    local->on_selected = NULL;
    local->items = NULL;
    local->item_count = 0;
    local->current_index = 0;
    local->is_expanded = 0;
    local->max_visible_items = 5;

    local->alpha = EGUI_ALPHA_100;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->bg_color = EGUI_THEME_SURFACE;
    local->border_color = EGUI_THEME_BORDER;
    local->highlight_color = EGUI_THEME_PRIMARY;
    local->arrow_color = EGUI_THEME_TEXT_SECONDARY;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;

    local->collapsed_height = 30;
    local->item_height = 25;

    egui_view_set_view_name(self, "egui_view_combobox");
}

void egui_view_combobox_apply_params(egui_view_t *self, const egui_view_combobox_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_combobox_t);

    self->region = params->region;
    local->collapsed_height = params->region.size.height;

    local->items = params->items;
    local->item_count = params->item_count;
    local->current_index = params->current_index;

    egui_view_invalidate(self);
}

void egui_view_combobox_init_with_params(egui_view_t *self, const egui_view_combobox_params_t *params)
{
    egui_view_combobox_init(self);
    egui_view_combobox_apply_params(self, params);
}
