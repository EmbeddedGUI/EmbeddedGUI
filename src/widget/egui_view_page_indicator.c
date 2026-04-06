#include <stdio.h>
#include <assert.h>

#include "egui_view_page_indicator.h"
#include "egui_view_icon_font.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

extern const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_page_indicator_t);

void egui_view_page_indicator_set_total_count(egui_view_t *self, uint8_t total_count)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->total_count == total_count)
    {
        return;
    }
    local->total_count = total_count;
    egui_view_invalidate_full(self);
}

void egui_view_page_indicator_set_current_index(egui_view_t *self, uint8_t current_index)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (current_index >= local->total_count)
    {
        current_index = local->total_count > 0 ? local->total_count - 1 : 0;
    }
    if (local->current_index == current_index)
    {
        return;
    }
    local->current_index = current_index;
    egui_view_invalidate_full(self);
}

void egui_view_page_indicator_set_mark_style(egui_view_t *self, egui_view_page_indicator_mark_style_t style)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->mark_style == (uint8_t)style)
    {
        return;
    }

    local->mark_style = (uint8_t)style;
    egui_view_invalidate(self);
}

void egui_view_page_indicator_set_icons(egui_view_t *self, const char *const *icons)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->icons == icons)
    {
        return;
    }

    local->icons = icons;
    egui_view_invalidate(self);
}

void egui_view_page_indicator_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_page_indicator_on_draw_dot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);

    if (local->total_count == 0)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t dot_diameter = local->dot_radius * 2;
    egui_dim_t total_width = local->total_count * dot_diameter + (local->total_count - 1) * local->dot_spacing;

    // Center dots horizontally
    egui_dim_t start_x = region.location.x + (region.size.width - total_width) / 2 + local->dot_radius;
    egui_dim_t center_y = region.location.y + region.size.height / 2;

    uint8_t i;
    for (i = 0; i < local->total_count; i++)
    {
        egui_dim_t cx = start_x + i * (dot_diameter + local->dot_spacing);
        egui_color_t color = (i == local->current_index) ? local->active_color : local->inactive_color;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        if (i == local->current_index)
        {
            egui_color_t dot_light = egui_rgb_mix(color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t dot_stops[2] = {
                    {.position = 0, .color = dot_light},
                    {.position = 255, .color = color},
            };
            egui_gradient_t dot_grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = local->alpha,
                    .stops = dot_stops,
                    .center_x = 0,
                    .center_y = 0,
                    .radius = local->dot_radius,
            };
            egui_canvas_draw_circle_fill_gradient(cx, center_y, local->dot_radius, &dot_grad);
        }
        else
        {
            egui_canvas_draw_circle_fill_hq(cx, center_y, local->dot_radius, color, local->alpha);
        }
#else
        egui_canvas_draw_circle_fill_basic(cx, center_y, local->dot_radius, color, local->alpha);
#endif
    }
}

static void egui_view_page_indicator_on_draw_icon_impl(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);
    egui_region_t region;
    const egui_font_t *icon_font;
    egui_dim_t item_size;
    egui_dim_t item_gap;
    egui_dim_t total_width;
    egui_dim_t start_x;
    uint8_t i;

    if (local->total_count == 0 || local->icons == NULL)
    {
        return;
    }

    egui_view_get_work_region(self, &region);
    icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, region.size.height, 18, 22);
    if (icon_font == NULL)
    {
        return;
    }

    item_size = region.size.height;
    item_gap = EGUI_MAX(local->dot_spacing - 2, 2);
    total_width = local->total_count * item_size + (local->total_count - 1) * item_gap;
    start_x = region.location.x + (region.size.width - total_width) / 2;

    for (i = 0; i < local->total_count; i++)
    {
        const char *icon_text = local->icons[i];
        egui_region_t icon_region = {
                {start_x + i * (item_size + item_gap), region.location.y},
                {item_size, region.size.height},
        };
        egui_color_t color = (i == local->current_index) ? local->active_color : local->inactive_color;

        if (!EGUI_VIEW_ICON_TEXT_VALID(icon_text))
        {
            continue;
        }

        egui_canvas_draw_text_in_rect(icon_font, icon_text, &icon_region, EGUI_ALIGN_CENTER, color, local->alpha);
    }
}

void egui_view_page_indicator_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);

    if (local->mark_style != EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_ICON || local->icons == NULL)
    {
        egui_view_page_indicator_on_draw_dot(self);
        return;
    }

    egui_view_page_indicator_on_draw_icon_impl(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_page_indicator_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_page_indicator_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_page_indicator_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_page_indicator_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_page_indicator_t);

    // init local data.
    local->total_count = 0;
    local->current_index = 0;
    local->dot_radius = 4;
    local->dot_spacing = 6;
    local->alpha = EGUI_ALPHA_100;
    local->active_color = EGUI_THEME_PRIMARY;
    local->inactive_color = EGUI_THEME_TRACK_BG;
    local->mark_style = EGUI_VIEW_PAGE_INDICATOR_MARK_STYLE_DOT;
    local->icons = NULL;
    local->icon_font = NULL;

    egui_view_set_view_name(self, "egui_view_page_indicator");
}

void egui_view_page_indicator_apply_params(egui_view_t *self, const egui_view_page_indicator_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_page_indicator_t);

    self->region = params->region;

    local->total_count = params->total_count;
    local->current_index = params->current_index;

    egui_view_invalidate(self);
}

void egui_view_page_indicator_init_with_params(egui_view_t *self, const egui_view_page_indicator_params_t *params)
{
    egui_view_page_indicator_init(self);
    egui_view_page_indicator_apply_params(self, params);
}
