#include <assert.h>
#include <string.h>

#include "egui_view_heart_rate.h"
#include "core/egui_api.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

// ---------------------------------------------------------------------------
// ECG PQRST waveform template (32 steps)
// Values are multipliers of amp_unit: +ve = up on screen, range [-5..12]
// R-peak is at index 9 (value = 12). Beat animation triggers on offset == 10.
// ---------------------------------------------------------------------------
static const int8_t s_ecg_template[32] = {
        0, 0,  0,  0,  // flat baseline (0-3)
        1, 3,  3,  1,  // P-wave hump (4-7)
        0, -2, 12, -5, // Q-dip, R-peak, S-trough (8-11)
        0, 2,  4,  4,  // T-wave rise (12-15)
        3, 2,  1,  0,  // T-wave fall (16-19)
        0, 0,  0,  0,  // flat (20-23)
        0, 0,  0,  0,  // flat (24-27)
        0, 0,  0,  0,  // flat (28-31)
};

static egui_color_t heart_rate_mix(egui_color_t back, egui_color_t fore, egui_alpha_t alpha)
{
    return egui_rgb_mix(back, fore, alpha);
}

static int16_t heart_rate_wave_value_q8(uint16_t phase_q8)
{
    uint8_t idx0 = (uint8_t)((phase_q8 >> 8) & 31u);
    uint8_t idx1 = (uint8_t)((idx0 + 1u) & 31u);
    uint8_t frac = (uint8_t)(phase_q8 & 0xFFu);

    int16_t v0 = s_ecg_template[idx0];
    int16_t v1 = s_ecg_template[idx1];

    return (int16_t)(((int32_t)v0 * (256 - frac) + (int32_t)v1 * frac) >> 8);
}

// ---------------------------------------------------------------------------
// Timer callback: advance ECG scroll, trigger heart beat animation
// ---------------------------------------------------------------------------
static void heart_rate_timer_cb(egui_timer_t *timer)
{
    egui_view_heart_rate_t *local = (egui_view_heart_rate_t *)timer->user_data;
    local->ecg_offset = (local->ecg_offset + 1) & 31;
    if (local->ecg_offset == 10)
    {
        local->beat_phase = 12;
    }
    if (local->beat_phase > 0)
    {
        local->beat_phase--;
    }
    egui_view_invalidate((egui_view_t *)local);
}

static void egui_view_heart_rate_update_timer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    uint32_t period;

    if (!local->animate || local->bpm == 0 || !self->is_attached_to_window)
    {
        egui_timer_stop_timer(&local->anim_timer);
        return;
    }

    period = 60000u / (uint32_t)local->bpm / 32u;
    if (period < 15u)
    {
        period = 15u;
    }

    egui_timer_stop_timer(&local->anim_timer);
    egui_timer_start_timer(&local->anim_timer, period, period);
}

static void egui_view_heart_rate_on_attach_to_window(egui_view_t *self)
{
    egui_view_on_attach_to_window(self);
    egui_view_heart_rate_update_timer(self);
}

static void egui_view_heart_rate_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);

    egui_timer_stop_timer(&local->anim_timer);
    egui_view_on_detach_from_window(self);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void egui_view_heart_rate_set_bpm(egui_view_t *self, uint8_t bpm)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    if (bpm == local->bpm)
    {
        return;
    }
    local->bpm = bpm;
    egui_view_heart_rate_update_timer(self);
    egui_view_invalidate(self);
}

void egui_view_heart_rate_set_animate(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    if (local->animate == enable)
    {
        return;
    }

    local->animate = enable;
    egui_view_heart_rate_update_timer(self);
    egui_view_invalidate(self);
}

void egui_view_heart_rate_set_heart_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    local->heart_color = color;
    egui_view_invalidate(self);
}

// Set initial ECG scroll phase (0-255 maps to 0-31)
void egui_view_heart_rate_set_pulse_phase(egui_view_t *self, uint8_t phase)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);
    local->ecg_offset = (uint8_t)((uint16_t)phase * 32u / 256u);
    egui_view_invalidate(self);
}

// ---------------------------------------------------------------------------
// on_draw: ECG waveform area (upper 60%) + heart icon + BPM text (lower 40%)
// ---------------------------------------------------------------------------
void egui_view_heart_rate_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_heart_rate_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t x = region.location.x;
    egui_dim_t y = region.location.y;
    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    if (w <= 4 || h <= 8)
    {
        return;
    }

    egui_color_t med_green = EGUI_COLOR_MAKE(108, 186, 255);
    egui_color_t med_green_dim = heart_rate_mix(EGUI_COLOR_BLACK, med_green, 132);
    egui_color_t med_green_soft = heart_rate_mix(EGUI_COLOR_BLACK, med_green, 70);
    egui_color_t text_main = EGUI_COLOR_MAKE(220, 236, 255);
    egui_color_t text_sub = EGUI_COLOR_MAKE(156, 206, 255);
    egui_color_t panel_bg = EGUI_COLOR_MAKE(6, 11, 20);
    egui_color_t panel_bd = heart_rate_mix(panel_bg, med_green, 92);
    egui_color_t monitor_bg = EGUI_COLOR_MAKE(7, 13, 22);

    egui_canvas_draw_rectangle_fill(x, y, w, h, panel_bg, 245);
    egui_canvas_draw_rectangle(x, y, w, h, 1, panel_bd, 230);

    egui_dim_t pad = EGUI_MAX((egui_dim_t)2, w / 28);
    egui_dim_t in_x = x + pad;
    egui_dim_t in_y = y + pad;
    egui_dim_t in_w = w - pad * 2;
    egui_dim_t in_h = h - pad * 2;

    if (in_w < 16 || in_h < 16)
    {
        return;
    }

    egui_dim_t split_gap = EGUI_MAX((egui_dim_t)2, (egui_dim_t)(in_w / 36));
    egui_dim_t right_w = EGUI_MAX((egui_dim_t)30, (egui_dim_t)(in_w * 34 / 100));
    if (right_w > in_w - 20)
    {
        right_w = in_w - 20;
    }

    egui_dim_t ecg_x = in_x;
    egui_dim_t ecg_y = in_y;
    egui_dim_t ecg_w = in_w - right_w - split_gap;
    egui_dim_t ecg_h = in_h;
    egui_dim_t info_x = ecg_x + ecg_w + split_gap;
    egui_dim_t info_y = in_y;
    egui_dim_t info_w = right_w;
    egui_dim_t info_h = in_h;

    if (ecg_w < 18 || info_w < 18)
    {
        return;
    }

    egui_canvas_draw_rectangle_fill(ecg_x, ecg_y, ecg_w, ecg_h, monitor_bg, 250);
    egui_canvas_draw_rectangle(ecg_x, ecg_y, ecg_w, ecg_h, 1, med_green_soft, 200);

    egui_canvas_draw_rectangle_fill(info_x, info_y, info_w, info_h, panel_bg, 220);
    egui_canvas_draw_rectangle(info_x, info_y, info_w, info_h, 1, med_green_soft, 180);

    for (egui_dim_t gy = ecg_y + 3; gy < ecg_y + ecg_h - 2; gy += 5)
    {
        egui_canvas_draw_hline(ecg_x + 2, gy, ecg_w - 4, med_green_soft, 34);
    }
    for (egui_dim_t gx = ecg_x + 4; gx < ecg_x + ecg_w - 2; gx += 7)
    {
        egui_canvas_draw_vline(gx, ecg_y + 2, ecg_h - 4, med_green_soft, 24);
    }

    egui_dim_t wave_x0 = ecg_x + 2;
    egui_dim_t wave_w = ecg_w - 4;
    if (wave_w < 10)
    {
        return;
    }

    egui_dim_t amp_unit = EGUI_MAX((egui_dim_t)1, (ecg_h - 8) / 17);
    egui_dim_t mid_y = ecg_y + 4 + (egui_dim_t)(12 * amp_unit);
    egui_dim_t wave_top = ecg_y + 3;
    egui_dim_t wave_bottom = ecg_y + ecg_h - 4;

    egui_dim_t sample_count = EGUI_MIN((egui_dim_t)120, wave_w);
    if (sample_count < 3)
    {
        return;
    }

    egui_dim_t points[240];
    int16_t raw_vals[120];
    int16_t smooth_vals[120];

    for (egui_dim_t i = 0; i < sample_count; i++)
    {
        uint16_t phase_q8 = (uint16_t)(((uint16_t)local->ecg_offset << 8) + (uint16_t)((uint32_t)i * (32u << 8) / (uint32_t)(sample_count - 1)));
        raw_vals[i] = heart_rate_wave_value_q8(phase_q8);
    }

    for (egui_dim_t i = 0; i < sample_count; i++)
    {
        int16_t prev = raw_vals[(i == 0) ? 0 : (i - 1)];
        int16_t curr = raw_vals[i];
        int16_t next = raw_vals[(i == sample_count - 1) ? (sample_count - 1) : (i + 1)];
        smooth_vals[i] = (int16_t)((prev + (curr << 1) + next) >> 2);
    }

    for (egui_dim_t i = 0; i < sample_count; i++)
    {
        egui_dim_t px = wave_x0 + (egui_dim_t)((int32_t)i * (wave_w - 1) / (sample_count - 1));
        egui_dim_t py = mid_y - (egui_dim_t)((int32_t)smooth_vals[i] * (int32_t)amp_unit);
        if (py < wave_top)
        {
            py = wave_top;
        }
        else if (py > wave_bottom)
        {
            py = wave_bottom;
        }

        points[i * 2] = px;
        points[i * 2 + 1] = py;
    }

    egui_canvas_draw_polyline_hq(points, (uint8_t)sample_count, 2, med_green_dim, 124);
    egui_canvas_draw_polyline_hq(points, (uint8_t)sample_count, 1, med_green, EGUI_ALPHA_100);

    egui_dim_t scan_x = wave_x0 + (egui_dim_t)(((int32_t)local->ecg_offset * (wave_w - 1)) / 31);
    uint16_t scan_phase_q8 = (uint16_t)((uint16_t)local->ecg_offset << 8);
    int16_t scan_val = heart_rate_wave_value_q8(scan_phase_q8);
    egui_dim_t scan_y = mid_y - (egui_dim_t)((int32_t)scan_val * (int32_t)amp_unit);
    if (scan_y < wave_top)
    {
        scan_y = wave_top;
    }
    else if (scan_y > wave_bottom)
    {
        scan_y = wave_bottom;
    }

    egui_canvas_draw_vline(scan_x, ecg_y + 2, ecg_h - 4, med_green, 122);
    egui_canvas_draw_circle_fill(scan_x, scan_y, 2, med_green, EGUI_ALPHA_100);

    if (info_h < 10)
    {
        return;
    }

    egui_dim_t status_r = EGUI_MAX((egui_dim_t)2, EGUI_MIN((egui_dim_t)4, (egui_dim_t)(info_w / 8)));
    egui_dim_t status_cx = info_x + status_r + 2;
    egui_dim_t status_cy = info_y + status_r + 2;
    egui_alpha_t status_alpha = (local->beat_phase > 0) ? EGUI_ALPHA_100 : 168;

    egui_canvas_draw_circle_fill(status_cx, status_cy, status_r + 2, med_green_soft, 95);
    egui_canvas_draw_circle_fill(status_cx, status_cy, status_r, med_green, status_alpha);

    egui_region_t mark_region;
    mark_region.location.x = status_cx + status_r + 2;
    mark_region.location.y = info_y;
    mark_region.size.width = EGUI_MAX((egui_dim_t)10, info_w / 2);
    mark_region.size.height = EGUI_MAX((egui_dim_t)9, info_h / 4);
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_8_4, "ECG", &mark_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_sub,
                                  EGUI_ALPHA_100);

    egui_dim_t text_x = info_x + 2;
    egui_dim_t text_w = info_w - 4;

    const egui_font_t *num_font = local->font;
    const egui_font_t *tag_font = local->font;
    if (info_h <= 22 || info_w <= 34)
    {
        num_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
        tag_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;
    }
    else if (info_h <= 28)
    {
        num_font = (const egui_font_t *)&egui_res_font_montserrat_12_4;
        tag_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;
    }

    {
        int pos = 0;
        int remain = (int)sizeof(local->text_buffer);
        uint8_t b = local->bpm;
        local->text_buffer[pos++] = (char)('0' + (b / 100) % 10);
        local->text_buffer[pos++] = (char)('0' + (b / 10) % 10);
        local->text_buffer[pos++] = (char)('0' + (b % 10));
        if (pos < remain)
        {
            local->text_buffer[pos] = '\0';
        }
    }

    egui_dim_t num_font_h = (egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(num_font);
    egui_dim_t top_h = EGUI_MAX((egui_dim_t)(num_font_h + 2), (egui_dim_t)(info_h * 58 / 100));
    if (top_h > info_h - 8)
    {
        top_h = info_h - 8;
    }
    egui_dim_t bottom_h = info_h - top_h - 1;
    if (bottom_h < 4)
    {
        bottom_h = 4;
    }

    egui_region_t num_region;
    num_region.location.x = text_x;
    num_region.location.y = info_y + EGUI_MAX((egui_dim_t)8, status_r * 2 + 4);
    num_region.size.width = text_w;
    num_region.size.height = EGUI_MAX((egui_dim_t)10, top_h - EGUI_MAX((egui_dim_t)8, status_r * 2 + 4));
    egui_canvas_draw_text_in_rect(num_font, local->text_buffer, &num_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_main, EGUI_ALPHA_100);

    egui_region_t unit_region;
    unit_region.location.x = text_x;
    unit_region.location.y = info_y + top_h + 1;
    unit_region.size.width = text_w;
    unit_region.size.height = (bottom_h > 2) ? (bottom_h - 2) : bottom_h;
    egui_canvas_draw_text_in_rect(tag_font, "RDM", &unit_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_sub, 248);

    if (text_w > 12)
    {
        egui_dim_t meter_w = EGUI_MAX((egui_dim_t)12, (egui_dim_t)(text_w - 2));
        egui_dim_t meter_x = text_x + text_w - meter_w;
        egui_dim_t meter_y = info_y + info_h - 2;
        egui_dim_t meter_fill = (egui_dim_t)((meter_w * (local->ecg_offset + 1)) / 32);
        egui_canvas_draw_hline(meter_x, meter_y - 1, meter_w, med_green_soft, 86);
        egui_canvas_draw_hline(meter_x, meter_y, meter_w, med_green_soft, 100);
        egui_canvas_draw_hline(meter_x, meter_y - 1, meter_fill, med_green, 200);
        egui_canvas_draw_hline(meter_x, meter_y, meter_fill, med_green, 220);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_heart_rate_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_heart_rate_on_attach_to_window,
        .on_draw = egui_view_heart_rate_on_draw,
        .on_detach_from_window = egui_view_heart_rate_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_heart_rate_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_heart_rate_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_heart_rate_t);

    local->bpm = 0;
    local->animate = 0;
    local->heart_color = EGUI_THEME_DANGER;
    local->text_color = EGUI_THEME_PRIMARY_DARK;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_api_memset(local->text_buffer, 0, (int)sizeof(local->text_buffer));
    local->ecg_offset = 0;
    local->beat_phase = 0;

    egui_timer_init_timer(&local->anim_timer, local, heart_rate_timer_cb);

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
