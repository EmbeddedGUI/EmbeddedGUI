#include <stdlib.h>
#include "egui_view_sample_widget.h"

#define SAMPLE_WIDGET_RADIUS 6

static void egui_view_sample_widget_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);
    egui_region_t region;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, SAMPLE_WIDGET_RADIUS,
                                          EGUI_COLOR_HEX(0x0F172A), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));

    // Draw border
    if (local->border_width > 0)
    {
        egui_dim_t x = region.location.x;
        egui_dim_t y = region.location.y;
        egui_dim_t w = region.size.width;
        egui_dim_t h = region.size.height;

        egui_canvas_draw_rectangle(x, y, w, h, local->border_width, local->border_color, EGUI_ALPHA_100);
    }

    // Draw base label
    egui_view_label_on_draw(self);
}

void egui_view_sample_widget_set_border(egui_view_t *self, egui_color_t color, uint8_t width)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);
    local->border_color = color;
    local->border_width = width;
    egui_view_invalidate(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_sample_widget_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_sample_widget_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_sample_widget_init(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_sample_widget_t);

    // Init base label
    egui_view_label_init(self);
    egui_view_label_set_font(self, (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color(self, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
    egui_view_set_padding_all(self, 4);

    // Override draw API
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_sample_widget_t);

    // Default border
    local->border_color = EGUI_COLOR_WHITE;
    local->border_width = 2;
}
