#include <stdlib.h>
#include <string.h>

#include "egui_view_sample_chart.h"

#define SAMPLE_CHART_RADIUS     10
#define SAMPLE_CHART_HEADER_H   14
#define SAMPLE_CHART_PAD_X      8
#define SAMPLE_CHART_PAD_Y      6
#define SAMPLE_CHART_GRID_ALPHA 28
#define SAMPLE_CHART_LINE_ALPHA 220

static void egui_view_sample_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_sample_chart_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t chart_region;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_color_t panel_color;
    egui_color_t grid_color;
    egui_color_t line_color;
    egui_alpha_t a;

    static const uint8_t values[] = {18, 42, 28, 56, 36, 64, 40};
    const uint8_t value_count = sizeof(values) / sizeof(values[0]);
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    a = self->alpha;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    grid_color = egui_rgb_mix(local->border_color, local->muted_text_color, EGUI_ALPHA_40);
    line_color = local->accent_color;

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, SAMPLE_CHART_RADIUS, panel_color,
                                          egui_color_alpha_mix(a, EGUI_ALPHA_40));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, SAMPLE_CHART_RADIUS, 1, local->border_color,
                                     egui_color_alpha_mix(a, EGUI_ALPHA_60));

    content_x = region.location.x + SAMPLE_CHART_PAD_X;
    content_y = region.location.y + SAMPLE_CHART_PAD_Y;
    content_w = region.size.width - SAMPLE_CHART_PAD_X * 2;
    content_h = region.size.height - SAMPLE_CHART_PAD_Y * 2;
    if (content_w <= 30 || content_h <= 22)
    {
        return;
    }

    header_region.location.x = content_x;
    header_region.location.y = content_y;
    header_region.size.width = content_w;
    header_region.size.height = SAMPLE_CHART_HEADER_H;
    egui_canvas_draw_text_in_rect(local->font, "Sample Chart", &header_region, EGUI_ALIGN_LEFT, local->text_color, a);

    chart_region.location.x = content_x;
    chart_region.location.y = content_y + SAMPLE_CHART_HEADER_H + 4;
    chart_region.size.width = content_w;
    chart_region.size.height = content_h - SAMPLE_CHART_HEADER_H - 4;
    if (chart_region.size.height <= 18)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(chart_region.location.x, chart_region.location.y, chart_region.size.width, chart_region.size.height, 6,
                                          egui_rgb_mix(local->surface_color, EGUI_COLOR_BLACK, EGUI_ALPHA_30), egui_color_alpha_mix(a, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(chart_region.location.x, chart_region.location.y, chart_region.size.width, chart_region.size.height, 6, 1,
                                     local->border_color, egui_color_alpha_mix(a, EGUI_ALPHA_40));

    for (i = 1; i <= 3; i++)
    {
        egui_dim_t y = chart_region.location.y + (chart_region.size.height * i) / 4;
        egui_canvas_draw_line(chart_region.location.x + 2, y, chart_region.location.x + chart_region.size.width - 3, y, 1, grid_color,
                              egui_color_alpha_mix(a, SAMPLE_CHART_GRID_ALPHA));
    }

    for (i = 1; i <= 4; i++)
    {
        egui_dim_t x = chart_region.location.x + (chart_region.size.width * i) / 5;
        egui_canvas_draw_line(x, chart_region.location.y + 2, x, chart_region.location.y + chart_region.size.height - 3, 1, grid_color,
                              egui_color_alpha_mix(a, SAMPLE_CHART_GRID_ALPHA));
    }

    if (value_count >= 2)
    {
        egui_dim_t inner_x = chart_region.location.x + 6;
        egui_dim_t inner_y = chart_region.location.y + 6;
        egui_dim_t inner_w = chart_region.size.width - 12;
        egui_dim_t inner_h = chart_region.size.height - 12;
        egui_dim_t prev_x = inner_x;
        egui_dim_t prev_y = inner_y + inner_h - (inner_h * values[0]) / 80;

        for (i = 1; i < value_count; i++)
        {
            egui_dim_t x = inner_x + (inner_w * i) / (value_count - 1);
            egui_dim_t y = inner_y + inner_h - (inner_h * values[i]) / 80;
            egui_canvas_draw_line(prev_x, prev_y, x, y, 2, line_color, egui_color_alpha_mix(a, SAMPLE_CHART_LINE_ALPHA));
            egui_canvas_draw_circle_fill(x, y, 2, line_color, egui_color_alpha_mix(a, EGUI_ALPHA_90));
            prev_x = x;
            prev_y = y;
        }
        egui_canvas_draw_circle_fill(inner_x, inner_y + inner_h - (inner_h * values[0]) / 80, 2, line_color, egui_color_alpha_mix(a, EGUI_ALPHA_90));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_sample_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_sample_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_sample_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_sample_chart_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_sample_chart_t);
    egui_view_set_padding_all(self, 2);

    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0F172A);
    local->border_color = EGUI_COLOR_HEX(0x475569);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->accent_color = EGUI_COLOR_HEX(0x38BDF8);
}
