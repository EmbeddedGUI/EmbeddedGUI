#include <stdlib.h>
#include <string.h>

#include "egui_view_jog_shuttle_wheel.h"

typedef struct shuttle_palette shuttle_palette_t;
struct shuttle_palette
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

static const shuttle_palette_t shuttle_palette = {
        EGUI_COLOR_HEX(0x111821), EGUI_COLOR_HEX(0x1C2836), EGUI_COLOR_HEX(0x55657A), EGUI_COLOR_HEX(0xF4F8FF), EGUI_COLOR_HEX(0x98A8BC),
        EGUI_COLOR_HEX(0x74C6FF), EGUI_COLOR_HEX(0xF2B46C), EGUI_COLOR_HEX(0x8FD1B0), EGUI_COLOR_HEX(0x070C12),
};

typedef struct marker_point marker_point_t;
struct marker_point
{
    egui_dim_t x;
    egui_dim_t y;
};

static const marker_point_t ring_markers[8] = {
        {76, 18}, {108, 32}, {122, 62}, {108, 96}, {76, 110}, {44, 96}, {30, 62}, {44, 32},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_JOG_SHUTTLE_WHEEL_MAX_MODES)
    {
        return EGUI_VIEW_JOG_SHUTTLE_WHEEL_MAX_MODES;
    }
    return count;
}

static egui_color_t get_accent_color(const egui_view_jog_shuttle_wheel_mode_t *mode)
{
    if (mode->accent_mode >= 2)
    {
        return shuttle_palette.safe;
    }
    if (mode->accent_mode == 1)
    {
        return shuttle_palette.warm;
    }
    return shuttle_palette.accent;
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

static void draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color,
                                   egui_alpha_t alpha)
{
    radius = clamp_round_radius(w, h, radius);
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

void egui_view_jog_shuttle_wheel_set_modes(egui_view_t *self, const egui_view_jog_shuttle_wheel_mode_t *modes, uint8_t mode_count)
{
    EGUI_LOCAL_INIT(egui_view_jog_shuttle_wheel_t);
    local->modes = modes;
    local->mode_count = clamp_count(mode_count);
    if (local->current_index >= local->mode_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_jog_shuttle_wheel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_jog_shuttle_wheel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *wheel_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);
    wheel_rect->location.x = region.location.x + 42;
    wheel_rect->location.y = region.location.y + 56;
    wheel_rect->size.width = 156;
    wheel_rect->size.height = 156;

    left_rect->location.x = region.location.x + 8;
    left_rect->location.y = region.location.y + 102;
    left_rect->size.width = 34;
    left_rect->size.height = 90;

    right_rect->location.x = region.location.x + 198;
    right_rect->location.y = region.location.y + 102;
    right_rect->size.width = 34;
    right_rect->size.height = 90;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *wheel_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox;
    egui_dim_t oy;

    ox = self->region_screen.location.x + self->padding.left;
    oy = self->region_screen.location.y + self->padding.top;

    wheel_rect->location.x = ox + 42;
    wheel_rect->location.y = oy + 56;
    wheel_rect->size.width = 156;
    wheel_rect->size.height = 156;

    left_rect->location.x = ox + 8;
    left_rect->location.y = oy + 102;
    left_rect->size.width = 34;
    left_rect->size.height = 90;

    right_rect->location.x = ox + 198;
    right_rect->location.y = oy + 102;
    right_rect->size.width = 34;
    right_rect->size.height = 90;
}

static int egui_view_jog_shuttle_wheel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_jog_shuttle_wheel_t);
    egui_region_t wheel_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->mode_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &wheel_rect, &left_rect, &right_rect);
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = (local->current_index == 0) ? (local->mode_count - 1) : (uint8_t)(local->current_index - 1);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->mode_count);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&wheel_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->mode_count);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void draw_side_preview(egui_view_t *self, const egui_view_jog_shuttle_wheel_mode_t *mode, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h,
                              uint8_t right_side)
{
    egui_color_t accent_color = get_accent_color(mode);

    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 3, w, h, 9, shuttle_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x, y, w, h, 9, shuttle_palette.panel, egui_color_alpha_mix(self->alpha, 36));
    draw_round_stroke_safe(x, y, w, h, 9, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(x + 9, y + 12, w - 18, 8, 4, accent_color, egui_color_alpha_mix(self->alpha, 50));
    draw_round_fill_safe(x + 7, y + 34, w - 14, 32, 6, shuttle_palette.surface, egui_color_alpha_mix(self->alpha, 22));
    draw_round_stroke_safe(x + 7, y + 34, w - 14, 32, 6, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + 12, y + 45, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x + 12, y + 58, w - 22, 2, 1, shuttle_palette.text, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(x + (right_side ? 9 : w - 11), y + 41, 2, 18, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x + 10, y + h - 16, w - 20, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 46));
}

static void draw_wheel(egui_view_t *self, const egui_view_jog_shuttle_wheel_mode_t *mode, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    const egui_font_t *body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    const egui_font_t *title_font = (const egui_font_t *)&egui_res_font_montserrat_12_4;
    egui_region_t text_region;
    egui_color_t accent_color = get_accent_color(mode);
    egui_dim_t i;

    draw_round_fill_safe(x + 1, y + 4, w, h, 28, shuttle_palette.shadow, egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(x, y, w, h, 28, shuttle_palette.panel, egui_color_alpha_mix(self->alpha, 58));
    draw_round_stroke_safe(x, y, w, h, 28, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 44));
    draw_round_fill_safe(x + 12, y + 12, w - 24, h - 24, 64, shuttle_palette.surface, egui_color_alpha_mix(self->alpha, 42));
    draw_round_stroke_safe(x + 12, y + 12, w - 24, h - 24, 64, 2, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(x + 32, y + 32, w - 64, h - 64, 44, shuttle_palette.panel, egui_color_alpha_mix(self->alpha, 54));
    draw_round_stroke_safe(x + 32, y + 32, w - 64, h - 64, 44, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 20));

    for (i = 0; i < 8; i++)
    {
        egui_dim_t size = (i == mode->ring_index) ? 14 : 5;
        egui_dim_t px = x + ring_markers[i].x - size / 2;
        egui_dim_t py = y + ring_markers[i].y - size / 2;
        if (i == mode->ring_index)
        {
            draw_round_fill_safe(px - 3, py - 3, size + 6, size + 6, (size + 6) / 2, accent_color, egui_color_alpha_mix(self->alpha, 14));
        }
        draw_round_fill_safe(px, py, size, size, size / 2, accent_color, egui_color_alpha_mix(self->alpha, (i == mode->ring_index) ? 84 : 38));
    }

    draw_round_fill_safe(x + 50, y + 50, 52, 52, 26, accent_color, egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(x + 48, y + 48, 56, 56, 28, 1, accent_color, egui_color_alpha_mix(self->alpha, 38));
    draw_round_fill_safe(x + 60, y + 60, 34, 34, 17, shuttle_palette.surface, egui_color_alpha_mix(self->alpha, 56));
    draw_round_stroke_safe(x + 60, y + 60, 34, 34, 17, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 24));

    text_region.location.x = x + 24;
    text_region.location.y = y + 95;
    text_region.size.width = w - 48;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(title_font, mode->preset, &text_region, EGUI_ALIGN_CENTER, shuttle_palette.text, self->alpha);

    text_region.location.y = y + 110;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, mode->summary, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(shuttle_palette.text, shuttle_palette.muted, 16),
                                  self->alpha);
}

static void egui_view_jog_shuttle_wheel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_jog_shuttle_wheel_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t wheel_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_jog_shuttle_wheel_mode_t *current;
    const egui_view_jog_shuttle_wheel_mode_t *left_mode;
    const egui_view_jog_shuttle_wheel_mode_t *right_mode;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const char *status_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->modes == NULL || local->mode_count == 0)
    {
        return;
    }

    current = &local->modes[local->current_index];
    left_mode = &local->modes[(local->current_index == 0) ? (local->mode_count - 1) : (local->current_index - 1)];
    right_mode = &local->modes[(local->current_index + 1) % local->mode_count];
    accent_color = get_accent_color(current);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 60, region.location.y + 19, 120, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));
    egui_canvas_draw_text_in_rect(title_font, "Jog Shuttle Wheel", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 28;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap edges or wheel to step modes", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(shuttle_palette.text, shuttle_palette.muted, 28), self->alpha);

    get_zone_rects_local(self, &wheel_rect, &left_rect, &right_rect);
    draw_side_preview(self, left_mode, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_side_preview(self, right_mode, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);
    draw_wheel(self, current, wheel_rect.location.x, wheel_rect.location.y, wheel_rect.size.width, wheel_rect.size.height);

    pill_w = (egui_dim_t)(52 + strlen(current->status) * 5);
    if (pill_w < 88)
    {
        pill_w = 88;
    }
    pill_x = wheel_rect.location.x + wheel_rect.size.width - pill_w - 14;
    draw_round_fill_safe(pill_x, wheel_rect.location.y + 13, pill_w, 18, 9, accent_color, egui_color_alpha_mix(self->alpha, 66));
    draw_round_stroke_safe(pill_x, wheel_rect.location.y + 13, pill_w, 18, 9, 1, egui_rgb_mix(accent_color, shuttle_palette.text, 28),
                           egui_color_alpha_mix(self->alpha, 42));
    text_region.location.x = pill_x + 9;
    text_region.location.y = wheel_rect.location.y + 14;
    text_region.size.width = pill_w - 18;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, shuttle_palette.text, self->alpha);

    draw_round_fill_safe(wheel_rect.location.x + 46, wheel_rect.location.y + wheel_rect.size.height - 28, wheel_rect.size.width - 92, 18, 9,
                         shuttle_palette.surface, egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(wheel_rect.location.x + 46, wheel_rect.location.y + wheel_rect.size.height - 28, wheel_rect.size.width - 92, 18, 9, 1,
                           shuttle_palette.border, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(wheel_rect.location.x + 58, wheel_rect.location.y + wheel_rect.size.height - 21, 5, 5, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, 52));
    text_region.location.x = wheel_rect.location.x + 30;
    text_region.location.y = wheel_rect.location.y + wheel_rect.size.height - 26;
    text_region.size.width = wheel_rect.size.width - 60;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(shuttle_palette.text, accent_color, 18),
                                  self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev cue armed";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next cue armed";
    }
    else
    {
        status_text = "Wheel advanced";
    }

    draw_round_fill_safe(region.location.x + 25, region.location.y + 239, 190, 24, 12, shuttle_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 22, region.location.y + 236, 196, 28, 14, shuttle_palette.surface, egui_color_alpha_mix(self->alpha, 46));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 236, 196, 28, 14, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(region.location.x + 36, region.location.y + 247, 7, 7, 3, accent_color, egui_color_alpha_mix(self->alpha, 84));
    draw_round_fill_safe(region.location.x + 54, region.location.y + 249, 10, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 70, region.location.y + 249, 12, 2, 1, shuttle_palette.border, egui_color_alpha_mix(self->alpha, 14));
    text_region.location.x = region.location.x + 50;
    text_region.location.y = region.location.y + 243;
    text_region.size.width = 140;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(shuttle_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_jog_shuttle_wheel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_jog_shuttle_wheel_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_jog_shuttle_wheel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_jog_shuttle_wheel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_jog_shuttle_wheel_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_jog_shuttle_wheel_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->modes = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->mode_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
