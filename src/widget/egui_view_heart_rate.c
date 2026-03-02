#include <assert.h>
#include <string.h>

#include "egui_view_heart_rate.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

void egui_view_heart_rate_set_bpm(egui_view_t *self, uint8_t bpm)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    if (bpm != local->bpm)
    {
        local->bpm = bpm;
        egui_view_invalidate(self);
    }
}

void egui_view_heart_rate_set_animate(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    local->animate = enable;
    egui_view_invalidate(self);
}

void egui_view_heart_rate_set_heart_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    local->heart_color = color;
    egui_view_invalidate(self);
}

void egui_view_heart_rate_set_pulse_phase(egui_view_t *self, uint8_t phase)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    local->pulse_phase = phase;
    egui_view_invalidate(self);
}

void egui_view_heart_rate_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    // ECG waveform in upper area
    egui_dim_t wave_area_h = h * 3 / 5;
    egui_dim_t mid_y = y + wave_area_h / 2;
    egui_dim_t amp = EGUI_MAX(h / 9, 4);
    egui_dim_t left = x + 8;
    egui_dim_t right = x + w - 8;
    egui_dim_t span = right - left;

    if (local->animate && local->pulse_phase >= 128)
    {
        amp = amp + 2;
    }

    egui_dim_t p0x = left;
    egui_dim_t p0y = mid_y;
    egui_dim_t p1x = left + span / 5;
    egui_dim_t p1y = mid_y;
    egui_dim_t p2x = left + span * 2 / 5;
    egui_dim_t p2y = mid_y - amp;
    egui_dim_t p3x = left + span * 5 / 10;
    egui_dim_t p3y = mid_y + amp * 2;
    egui_dim_t p4x = left + span * 3 / 5;
    egui_dim_t p4y = mid_y - amp / 2;
    egui_dim_t p5x = left + span * 4 / 5;
    egui_dim_t p5y = mid_y;
    egui_dim_t p6x = right;
    egui_dim_t p6y = mid_y;

    egui_canvas_draw_line(p0x, p0y, p1x, p1y, 2, local->heart_color, EGUI_ALPHA_100);
    egui_canvas_draw_line(p1x, p1y, p2x, p2y, 2, local->heart_color, EGUI_ALPHA_100);
    egui_canvas_draw_line(p2x, p2y, p3x, p3y, 2, local->heart_color, EGUI_ALPHA_100);
    egui_canvas_draw_line(p3x, p3y, p4x, p4y, 2, local->heart_color, EGUI_ALPHA_100);
    egui_canvas_draw_line(p4x, p4y, p5x, p5y, 2, local->heart_color, EGUI_ALPHA_100);
    egui_canvas_draw_line(p5x, p5y, p6x, p6y, 2, local->heart_color, EGUI_ALPHA_100);

    // BPM text in lower area
    {
        int pos = 0;
        int remain = (int)sizeof(local->text_buffer);
        pos += egui_sprintf_int(&local->text_buffer[pos], remain - pos, local->bpm);
        pos += egui_sprintf_str(&local->text_buffer[pos], remain - pos, " BPM");
    }
    egui_region_t text_region;
    text_region.location.x = x;
    text_region.location.y = y + wave_area_h + 2;
    text_region.size.width = w;
    text_region.size.height = h - wave_area_h - 2;
    egui_canvas_draw_text_in_rect(local->font, local->text_buffer, &text_region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_heart_rate_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_heart_rate_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_heart_rate_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_heart_rate_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_heart_rate_t);

    // init local data.
    local->bpm = 0;
    local->pulse_phase = 0;
    local->animate = 0;
    local->heart_color = EGUI_THEME_DANGER;
    local->text_color = EGUI_THEME_PRIMARY_DARK;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    memset(local->text_buffer, 0, sizeof(local->text_buffer));

    egui_view_set_view_name(self, "egui_view_heart_rate");
}

void egui_view_heart_rate_apply_params(egui_view_t *self, const egui_view_heart_rate_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);

    self->region = params->region;

    local->bpm = params->bpm;

    egui_view_invalidate(self);
}

void egui_view_heart_rate_init_with_params(egui_view_t *self, const egui_view_heart_rate_params_t *params)
{
    egui_view_heart_rate_init(self);
    egui_view_heart_rate_apply_params(self, params);
}
