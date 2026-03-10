#include <stdlib.h>
#include <string.h>

#include "egui_view_equalizer_curve_editor.h"

typedef struct eq_palette eq_palette_t;
struct eq_palette
{
    egui_color_t surface;
    egui_color_t panel;
    egui_color_t border;
    egui_color_t text;
    egui_color_t muted;
    egui_color_t accent;
    egui_color_t warm;
    egui_color_t safe;
    egui_color_t shadow;
};

static const eq_palette_t eq_palette = {
        EGUI_COLOR_HEX(0x121923), EGUI_COLOR_HEX(0x202B3A), EGUI_COLOR_HEX(0x576985), EGUI_COLOR_HEX(0xF3F8FF), EGUI_COLOR_HEX(0x98A5BC),
        EGUI_COLOR_HEX(0x79CFFF), EGUI_COLOR_HEX(0xF2B86F), EGUI_COLOR_HEX(0x97D3AF), EGUI_COLOR_HEX(0x060A10),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_EQUALIZER_CURVE_EDITOR_MAX_BANDS)
    {
        return EGUI_VIEW_EQUALIZER_CURVE_EDITOR_MAX_BANDS;
    }
    return count;
}

static egui_color_t get_accent_color(const egui_view_equalizer_curve_editor_band_t *band)
{
    if (band->accent_mode >= 2)
    {
        return eq_palette.safe;
    }
    if (band->accent_mode == 1)
    {
        return eq_palette.warm;
    }
    return eq_palette.accent;
}

static egui_dim_t clamp_round_radius(egui_dim_t w, egui_dim_t h, egui_dim_t radius)
{
    egui_dim_t max_radius_w;
    egui_dim_t max_radius_h;
    egui_dim_t max_radius;

    if (w <= 0 || h <= 0)
    {
        return 0;
    }

    max_radius_w = (w >> 1) - 1;
    max_radius_h = (h >> 1) - 1;
    max_radius = EGUI_MIN(max_radius_w, max_radius_h);
    if (max_radius < 0)
    {
        return 0;
    }

    return EGUI_MIN(radius, max_radius);
}

static void draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    radius = clamp_round_radius(w, h, radius);
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void draw_round_stroke_safe(
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        egui_dim_t radius,
        egui_dim_t stroke_width,
        egui_color_t color,
        egui_alpha_t alpha)
{
    radius = clamp_round_radius(w, h, radius);
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

void egui_view_equalizer_curve_editor_set_bands(
        egui_view_t *self,
        const egui_view_equalizer_curve_editor_band_t *bands,
        uint8_t band_count)
{
    EGUI_LOCAL_INIT(egui_view_equalizer_curve_editor_t);
    local->bands = bands;
    local->band_count = clamp_count(band_count);
    if (local->current_index >= local->band_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_equalizer_curve_editor_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_equalizer_curve_editor_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);
    main_rect->location.x = region.location.x + 26;
    main_rect->location.y = region.location.y + 60;
    main_rect->size.width = 188;
    main_rect->size.height = 156;

    left_rect->location.x = region.location.x + 4;
    left_rect->location.y = region.location.y + 84;
    left_rect->size.width = 44;
    left_rect->size.height = 112;

    right_rect->location.x = region.location.x + 192;
    right_rect->location.y = region.location.y + 84;
    right_rect->size.width = 44;
    right_rect->size.height = 112;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox;
    egui_dim_t oy;

    ox = self->region_screen.location.x + self->padding.left;
    oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 26;
    main_rect->location.y = oy + 60;
    main_rect->size.width = 188;
    main_rect->size.height = 156;

    left_rect->location.x = ox + 4;
    left_rect->location.y = oy + 84;
    left_rect->size.width = 44;
    left_rect->size.height = 112;

    right_rect->location.x = ox + 192;
    right_rect->location.y = oy + 84;
    right_rect->size.width = 44;
    right_rect->size.height = 112;
}

static int egui_view_equalizer_curve_editor_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_equalizer_curve_editor_t);
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->band_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &main_rect, &left_rect, &right_rect);
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = (local->current_index == 0) ? (local->band_count - 1) : (uint8_t)(local->current_index - 1);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->band_count);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&main_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->band_count);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void draw_preview_card(
        egui_view_t *self,
        const egui_view_equalizer_curve_editor_band_t *band,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t right_side)
{
    egui_color_t accent_color;

    accent_color = get_accent_color(band);
    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 3, w, h, 10, eq_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x, y, w, h, 10, eq_palette.panel, egui_color_alpha_mix(self->alpha, 42));
    draw_round_stroke_safe(x, y, w, h, 10, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 36));
    draw_round_fill_safe(x + 11, y + 13, w - 22, 9, 4, accent_color, egui_color_alpha_mix(self->alpha, 56));
    draw_round_fill_safe(x + 9, y + 30, w - 18, h - 48, 7, eq_palette.surface, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(x + 9, y + 30, w - 18, h - 48, 7, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(x + 13, y + 39, w - 26, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 48));
    draw_round_fill_safe(x + 13, y + 50, w - 22, 7, 3, eq_palette.panel, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(x + 16, y + 53, w - 28, 2, 1, eq_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + 13, y + 68, w - 26, 2, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(x + 20, y + 78, w - 32, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(x + 20, y + 88, w - 34, 2, 1, eq_palette.text, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(x + (right_side ? 12 : w - 14), y + 52, 2, h - 72, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x + 12, y + h - 16, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 54));
}

static void draw_curve_editor(
        egui_view_t *self,
        const egui_view_equalizer_curve_editor_band_t *band,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h)
{
    egui_color_t accent_color;
    egui_dim_t px[5];
    egui_dim_t py[5];
    uint8_t i;

    accent_color = get_accent_color(band);
    draw_round_fill_safe(x, y, w, h, 10, eq_palette.surface, egui_color_alpha_mix(self->alpha, 40));
    draw_round_stroke_safe(x, y, w, h, 10, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 28));

    for (i = 0; i < 3; i++)
    {
        egui_dim_t line_y = y + 6 + i * 8;
        draw_round_fill_safe(x + 8, line_y, w - 16, 2, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 18));
    }

    for (i = 0; i < 5; i++)
    {
        px[i] = x + 14 + i * ((w - 28) / 4);
        py[i] = y + h / 2 - band->gains[i] * 3;
    }

    draw_round_fill_safe(px[band->focus_index] - 10, y + 5, 20, h - 10, 7, accent_color, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(px[band->focus_index] - 1, y + 6, 2, h - 12, 1, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 10, y + h / 2 - 1, w - 20, 2, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 14));

    for (i = 0; i < 4; i++)
    {
        egui_dim_t line_x = px[i];
        egui_dim_t line_y = (py[i] + py[i + 1]) / 2;
        egui_dim_t line_w = px[i + 1] - px[i];
        draw_round_fill_safe(line_x, line_y - 1, line_w, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 18));
        draw_round_fill_safe(line_x, line_y, line_w, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 54));
    }

    for (i = 0; i < 5; i++)
    {
        egui_dim_t dot = (i == band->focus_index) ? 9 : 5;
        draw_round_fill_safe(px[i] - 1, y + h - 7, 2, 3, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 28));
        if (i == band->focus_index)
        {
            draw_round_fill_safe(px[i] - 9, py[i] - 9, 18, 18, 9, accent_color, egui_color_alpha_mix(self->alpha, 14));
            draw_round_stroke_safe(px[i] - 7, py[i] - 7, 14, 14, 7, 1, accent_color, egui_color_alpha_mix(self->alpha, 38));
        }
        draw_round_fill_safe(px[i] - dot / 2, py[i] - dot / 2, dot, dot, dot / 2, accent_color, egui_color_alpha_mix(self->alpha, (i == band->focus_index) ? 84 : 52));
    }
}

static void egui_view_equalizer_curve_editor_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_equalizer_curve_editor_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_equalizer_curve_editor_band_t *current;
    const egui_view_equalizer_curve_editor_band_t *left_band;
    const egui_view_equalizer_curve_editor_band_t *right_band;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const char *status_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->bands == NULL || local->band_count == 0)
    {
        return;
    }

    current = &local->bands[local->current_index];
    left_band = &local->bands[(local->current_index == 0) ? (local->band_count - 1) : (local->current_index - 1)];
    right_band = &local->bands[(local->current_index + 1) % local->band_count];
    accent_color = get_accent_color(current);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 66, region.location.y + 19, 108, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 18));
    egui_canvas_draw_text_in_rect(title_font, "EQ Curve Editor", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(body_font, "Tap sides or center to step bands", &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(eq_palette.text, eq_palette.muted, 28), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview_card(self, left_band, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_preview_card(self, right_band, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);

    draw_round_fill_safe(main_rect.location.x + 1, main_rect.location.y + 5, main_rect.size.width, main_rect.size.height, 14, eq_palette.shadow, egui_color_alpha_mix(self->alpha, 44));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, eq_palette.panel, egui_color_alpha_mix(self->alpha, 78));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 72));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 16, 44, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, 74));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 23, 28, 2, 1, eq_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(main_rect.location.x + 12, main_rect.location.y + 54, main_rect.size.width - 24, 86, 10, eq_palette.surface, egui_color_alpha_mix(self->alpha, 20));
    draw_round_stroke_safe(main_rect.location.x + 12, main_rect.location.y + 54, main_rect.size.width - 24, 86, 10, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 12));

    pill_w = (egui_dim_t)(56 + strlen(current->status) * 5);
    if (pill_w < 90)
    {
        pill_w = 90;
    }
    pill_x = main_rect.location.x + main_rect.size.width - pill_w - 15;
    draw_round_fill_safe(pill_x, main_rect.location.y + 11, pill_w, 18, 9, accent_color, egui_color_alpha_mix(self->alpha, 66));
    draw_round_stroke_safe(pill_x, main_rect.location.y + 11, pill_w, 18, 9, 1, egui_rgb_mix(accent_color, eq_palette.text, 28), egui_color_alpha_mix(self->alpha, 46));
    text_region.location.x = pill_x + 9;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = pill_w - 18;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, eq_palette.text, self->alpha);

    draw_round_fill_safe(main_rect.location.x + 24, main_rect.location.y + 34, main_rect.size.width - 78, 23, 7, eq_palette.surface,
                         egui_color_alpha_mix(self->alpha, 10));
    text_region.location.x = main_rect.location.x + 30;
    text_region.location.y = main_rect.location.y + 40;
    text_region.size.width = main_rect.size.width - 70;
    text_region.size.height = 12;
    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 41, 5, 12, 2, accent_color, egui_color_alpha_mix(self->alpha, 64));
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->preset, &text_region, EGUI_ALIGN_LEFT, eq_palette.text, self->alpha);

    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 59, main_rect.size.width - 36, 19, 6, eq_palette.panel,
                         egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(main_rect.location.x + 26, main_rect.location.y + 81, main_rect.size.width - 52, 2, 1, eq_palette.border,
                         egui_color_alpha_mix(self->alpha, 10));
    text_region.location.y = main_rect.location.y + 62;
    text_region.size.width = main_rect.size.width - 58;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(eq_palette.text, eq_palette.muted, 16), self->alpha);

    draw_curve_editor(self, current, main_rect.location.x + 16, main_rect.location.y + 88, main_rect.size.width - 32, 42);

    draw_round_fill_safe(main_rect.location.x + 48, main_rect.location.y + main_rect.size.height - 25, main_rect.size.width - 96, 18, 9, eq_palette.surface,
                         egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(main_rect.location.x + 48, main_rect.location.y + main_rect.size.height - 25, main_rect.size.width - 96, 18, 9, 1, eq_palette.border,
                           egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(main_rect.location.x + 60, main_rect.location.y + main_rect.size.height - 18, 5, 5, 2, accent_color, egui_color_alpha_mix(self->alpha, 56));
    text_region.location.x = main_rect.location.x + 32;
    text_region.location.y = main_rect.location.y + main_rect.size.height - 23;
    text_region.size.width = main_rect.size.width - 64;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(eq_palette.text, accent_color, 18), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev band ready";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next band ready";
    }
    else
    {
        status_text = "Band advanced";
    }

    draw_round_fill_safe(region.location.x + 24, region.location.y + 240, 192, 24, 12, eq_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 20, region.location.y + 236, 200, 30, 15, eq_palette.surface, egui_color_alpha_mix(self->alpha, 48));
    draw_round_stroke_safe(region.location.x + 20, region.location.y + 236, 200, 30, 15, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 32));
    draw_round_fill_safe(region.location.x + 35, region.location.y + 248, 7, 7, 3, accent_color, egui_color_alpha_mix(self->alpha, 82));
    draw_round_fill_safe(region.location.x + 52, region.location.y + 250, 12, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(region.location.x + 70, region.location.y + 250, 12, 2, 1, eq_palette.border, egui_color_alpha_mix(self->alpha, 12));
    text_region.location.x = region.location.x + 46;
    text_region.location.y = region.location.y + 244;
    text_region.size.width = 152;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(eq_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_equalizer_curve_editor_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_equalizer_curve_editor_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_equalizer_curve_editor_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_equalizer_curve_editor_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_equalizer_curve_editor_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_equalizer_curve_editor_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->bands = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->band_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
