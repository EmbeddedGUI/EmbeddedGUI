#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_chart_pie.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

// ============== Pie Chart Drawing ==============

static void egui_view_chart_pie_draw_pie(egui_view_chart_pie_t *local, egui_region_t *region)
{
    if (local->pie_slice_count == 0 || local->pie_slices == NULL)
    {
        return;
    }

    // Calculate total value
    uint32_t total = 0;
    for (uint8_t i = 0; i < local->pie_slice_count; i++)
    {
        total += local->pie_slices[i].value;
    }
    if (total == 0)
    {
        return;
    }

    // Calculate available space considering legend
    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t font_h = egui_chart_get_font_height(font);

    egui_dim_t avail_x = region->location.x;
    egui_dim_t avail_y = region->location.y;
    egui_dim_t avail_w = region->size.width;
    egui_dim_t avail_h = region->size.height;

    if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        avail_w -= font_h * 4;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        avail_h -= font_h + 4;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        avail_y += font_h + 4;
        avail_h -= font_h + 4;
    }

    egui_dim_t center_x = avail_x + avail_w / 2;
    egui_dim_t center_y = avail_y + avail_h / 2;
    egui_dim_t radius = EGUI_MIN(avail_w, avail_h) / 2 - 2;
    if (radius <= 0)
    {
        return;
    }

    // Draw all slices as arcs with inner_r=1 to avoid center pixel artifact.
    // Then fill center pixel with first slice color.
    // Use cumulative proportion to avoid rounding error accumulation.
    int16_t start_angle = 270;
    uint32_t cumulative_value = 0;
    for (uint8_t i = 0; i < local->pie_slice_count; i++)
    {
        cumulative_value += local->pie_slices[i].value;
        int16_t end_angle;
        if (i == local->pie_slice_count - 1)
        {
            end_angle = 630; // ensure exact full circle
        }
        else
        {
            end_angle = 270 + (int16_t)((uint32_t)cumulative_value * 360 / total);
        }

        int16_t sweep = end_angle - start_angle;
        if (sweep < 1 && local->pie_slices[i].value > 0)
        {
            end_angle = start_angle + 1;
        }

        if (end_angle <= start_angle)
        {
            start_angle = end_angle;
            continue;
        }

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->pie_slices[i].color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->pie_slices[i].color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, 1, start_angle, end_angle, &grad);
        }
#else
        egui_canvas_draw_arc_fill(center_x, center_y, radius, start_angle, end_angle, local->pie_slices[i].color, EGUI_ALPHA_100);
#endif

        start_angle = end_angle;
    }

    // Fill center pixel with first slice gradient center color
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t center_color = egui_rgb_mix(local->pie_slices[0].color, EGUI_COLOR_WHITE, 80);
        egui_canvas_draw_rectangle_fill(center_x, center_y, 1, 1, center_color, EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_rectangle_fill(center_x, center_y, 1, 1, local->pie_slices[0].color, EGUI_ALPHA_100);
#endif
}

// ============== Pie Legend Drawing ==============

static void egui_view_chart_pie_draw_legend(egui_view_chart_pie_t *local, egui_region_t *region)
{
    const egui_font_t *font = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_dim_t font_h = egui_chart_get_font_height(font);
    egui_dim_t swatch_size = font_h > 2 ? font_h - 2 : 4;
    egui_dim_t item_gap = 6;
    egui_dim_t text_w = font_h * 3;

    uint8_t count = local->pie_slice_count;
    if (count == 0)
    {
        return;
    }

    // Calculate legend starting position
    egui_dim_t lx, ly;
    if (local->legend_pos == EGUI_CHART_LEGEND_BOTTOM)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + region->size.height - font_h - 2;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_TOP)
    {
        egui_dim_t total_w = count * (swatch_size + 2 + text_w + item_gap) - item_gap;
        lx = region->location.x + (region->size.width - total_w) / 2;
        if (lx < region->location.x)
        {
            lx = region->location.x;
        }
        ly = region->location.y + 1;
    }
    else if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
    {
        // For pie, legend goes to the right of the pie area
        const egui_font_t *f = local->font ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
        egui_dim_t fh = egui_chart_get_font_height(f);
        egui_dim_t pie_w = region->size.width - fh * 4;
        lx = region->location.x + pie_w + 6;
        ly = region->location.y + (region->size.height - count * (font_h + 2)) / 2;
        if (ly < region->location.y)
        {
            ly = region->location.y;
        }
    }
    else
    {
        return;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        egui_color_t color = local->pie_slices[i].color;
        const char *name = local->pie_slices[i].name;

        if (name == NULL)
        {
            continue;
        }

        // Draw color swatch
        egui_canvas_draw_rectangle_fill(lx, ly + 1, swatch_size, swatch_size, color, EGUI_ALPHA_100);

        // Draw name text
        EGUI_REGION_DEFINE(text_rect, lx + swatch_size + 2, ly, text_w, font_h);
        egui_canvas_draw_text_in_rect(font, name, &text_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->text_color, EGUI_ALPHA_100);

        // Advance position
        if (local->legend_pos == EGUI_CHART_LEGEND_RIGHT)
        {
            ly += font_h + 2;
        }
        else
        {
            lx += swatch_size + 2 + text_w + item_gap;
        }
    }
}

// ============== Main Draw ==============

void egui_view_chart_pie_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (region.size.width < 10 || region.size.height < 10)
    {
        return;
    }

    // Draw background
    egui_canvas_draw_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->bg_color, EGUI_ALPHA_100);

    // Draw pie
    egui_view_chart_pie_draw_pie(local, &region);

    // Draw legend
    if (local->legend_pos != EGUI_CHART_LEGEND_NONE)
    {
        egui_view_chart_pie_draw_legend(local, &region);
    }
}

// ============== API Table ==============

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_chart_pie_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_chart_pie_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

// ============== Init / Params ==============

void egui_view_chart_pie_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_chart_pie_t);
    // call super init
    egui_view_init(self);
    // update api
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_chart_pie_t);

    // init local data
    local->pie_slices = NULL;
    local->pie_slice_count = 0;

    // legend
    local->legend_pos = EGUI_CHART_LEGEND_NONE;

    // style
    local->bg_color = EGUI_THEME_SURFACE;
    local->text_color = EGUI_THEME_TEXT_SECONDARY;
    local->font = NULL;

    egui_view_set_view_name(self, "egui_view_chart_pie");
}

void egui_view_chart_pie_apply_params(egui_view_t *self, const egui_view_chart_pie_params_t *params)
{
    self->region = params->region;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_init_with_params(egui_view_t *self, const egui_view_chart_pie_params_t *params)
{
    egui_view_chart_pie_init(self);
    egui_view_chart_pie_apply_params(self, params);
}

// ============== Setters ==============

void egui_view_chart_pie_set_slices(egui_view_t *self, const egui_chart_pie_slice_t *slices, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->pie_slices = slices;
    local->pie_slice_count = count;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_set_legend_pos(egui_view_t *self, uint8_t pos)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    if (local->legend_pos != pos)
    {
        local->legend_pos = pos;
        egui_view_invalidate(self);
    }
}

void egui_view_chart_pie_set_colors(egui_view_t *self, egui_color_t bg, egui_color_t text)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->bg_color = bg;
    local->text_color = text;
    egui_view_invalidate(self);
}

void egui_view_chart_pie_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_chart_pie_t);
    local->font = font;
    egui_view_invalidate(self);
}
