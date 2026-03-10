#include <stdlib.h>
#include <string.h>

#include "egui_view_scene_crossfader.h"

typedef struct crossfader_palette crossfader_palette_t;
struct crossfader_palette
{
    egui_color_t bg;
    egui_color_t panel;
    egui_color_t panel_alt;
    egui_color_t border;
    egui_color_t text;
    egui_color_t muted;
    egui_color_t cyan;
    egui_color_t amber;
    egui_color_t mint;
    egui_color_t rose;
    egui_color_t shadow;
};

static const crossfader_palette_t crossfader_palette = {
        EGUI_COLOR_HEX(0x090F15), EGUI_COLOR_HEX(0x16212B), EGUI_COLOR_HEX(0x0F171F), EGUI_COLOR_HEX(0x425667), EGUI_COLOR_HEX(0xF2F6FB),
        EGUI_COLOR_HEX(0x8EA1B4), EGUI_COLOR_HEX(0x63D0FF), EGUI_COLOR_HEX(0xDDA35E), EGUI_COLOR_HEX(0x8BE3B7), EGUI_COLOR_HEX(0xDA7A9F),
        EGUI_COLOR_HEX(0x02060B),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SCENE_CROSSFADER_MAX_STATES)
    {
        return EGUI_VIEW_SCENE_CROSSFADER_MAX_STATES;
    }
    return count;
}

static egui_color_t get_accent(const egui_view_scene_crossfader_state_t *state)
{
    if (state->accent_mode >= 3)
    {
        return crossfader_palette.rose;
    }
    if (state->accent_mode == 2)
    {
        return crossfader_palette.mint;
    }
    if (state->accent_mode == 1)
    {
        return crossfader_palette.amber;
    }
    return crossfader_palette.cyan;
}

static egui_dim_t clamp_round_radius(egui_dim_t w, egui_dim_t h, egui_dim_t radius)
{
    egui_dim_t max_radius;

    if (w <= 0 || h <= 0)
    {
        return 0;
    }
    max_radius = EGUI_MIN((w >> 1) - 1, (h >> 1) - 1);
    if (max_radius < 0)
    {
        return 0;
    }
    return EGUI_MIN(radius, max_radius);
}

static void draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, clamp_round_radius(w, h, radius), color, alpha);
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
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, clamp_round_radius(w, h, radius), stroke_width, color, alpha);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 28;
    main_rect->location.y = region.location.y + 52;
    main_rect->size.width = 184;
    main_rect->size.height = 176;

    left_rect->location.x = region.location.x + 3;
    left_rect->location.y = region.location.y + 92;
    left_rect->size.width = 34;
    left_rect->size.height = 100;

    right_rect->location.x = region.location.x + 203;
    right_rect->location.y = region.location.y + 92;
    right_rect->size.width = 34;
    right_rect->size.height = 100;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox = self->region_screen.location.x + self->padding.left;
    egui_dim_t oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 28;
    main_rect->location.y = oy + 52;
    main_rect->size.width = 184;
    main_rect->size.height = 176;

    left_rect->location.x = ox + 3;
    left_rect->location.y = oy + 92;
    left_rect->size.width = 34;
    left_rect->size.height = 100;

    right_rect->location.x = ox + 203;
    right_rect->location.y = oy + 92;
    right_rect->size.width = 34;
    right_rect->size.height = 100;
}

void egui_view_scene_crossfader_set_states(egui_view_t *self, const egui_view_scene_crossfader_state_t *states, uint8_t state_count)
{
    EGUI_LOCAL_INIT(egui_view_scene_crossfader_t);
    local->states = states;
    local->state_count = clamp_count(state_count);
    if (local->current_index >= local->state_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

static int egui_view_scene_crossfader_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_scene_crossfader_t);
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->state_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &main_rect, &left_rect, &right_rect);
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = (local->current_index == 0) ? (local->state_count - 1) : (uint8_t)(local->current_index - 1);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->state_count);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&main_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->state_count);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void draw_preview(
        egui_view_t *self,
        const egui_view_scene_crossfader_state_t *state,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t inward_edge,
        uint8_t is_right)
{
    egui_color_t accent = get_accent(state);
    egui_region_t text_region;
    egui_dim_t bar_y = y + h - 20;
    egui_dim_t level_h = 16 + (is_right ? state->right_level : state->left_level) * 4;
    egui_dim_t tag_w = w - 10;
    uint8_t active = is_right ? (state->mix_percent >= 50) : (state->mix_percent < 50);

    draw_round_fill_safe(x + 1, y + 3, w, h, 8, crossfader_palette.shadow, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(x, y, w, h, 8, crossfader_palette.panel_alt, egui_color_alpha_mix(self->alpha, 46));
    draw_round_stroke_safe(x, y, w, h, 8, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 24));
    if (inward_edge == 0)
    {
        draw_round_fill_safe(x + w - 4, y + 10, 2, h - 20, 1, accent, egui_color_alpha_mix(self->alpha, 16));
    }
    else
    {
        draw_round_fill_safe(x + 2, y + 10, 2, h - 20, 1, accent, egui_color_alpha_mix(self->alpha, 16));
    }
    draw_round_fill_safe(x + 8, y + 11, w - 16, 4, 2, accent, egui_color_alpha_mix(self->alpha, 40));
    draw_round_fill_safe(x + 10, bar_y - level_h, 6, level_h, 3, accent, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(x + 19, bar_y - 22, 5, 22, 2, accent, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 25, bar_y - 14, 3, 14, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + 8, bar_y, w - 16, 2, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(x + 9, bar_y + 5, 4, 4, 2, accent, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x + w - 13, bar_y + 5, 4, 4, 2, accent, egui_color_alpha_mix(self->alpha, 18));

    draw_round_fill_safe(x + 5, y + 18, tag_w, 12, 6, active ? accent : crossfader_palette.bg, egui_color_alpha_mix(self->alpha, active ? 20 : 24));
    draw_round_stroke_safe(x + 5, y + 18, tag_w, 12, 6, 1, active ? accent : crossfader_palette.border, egui_color_alpha_mix(self->alpha, active ? 18 : 12));
    text_region.location.x = x + 6;
    text_region.location.y = y + 20;
    text_region.size.width = w - 12;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_10_4, is_right ? state->right_tag : state->left_tag, &text_region, EGUI_ALIGN_CENTER,
                                  crossfader_palette.text, self->alpha);
}

static void egui_view_scene_crossfader_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scene_crossfader_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_scene_crossfader_state_t *current;
    const egui_view_scene_crossfader_state_t *left_state;
    const egui_view_scene_crossfader_state_t *right_state;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent;
    egui_dim_t mix_x;
    egui_dim_t mix_y;
    egui_dim_t mix_w;
    egui_dim_t handle_x;
    egui_dim_t mode_pill_w;
    egui_dim_t mode_pill_x;
    egui_dim_t footer_pill_w;
    egui_dim_t footer_pill_x;
    egui_dim_t status_pill_w;
    egui_dim_t status_pill_x;
    egui_dim_t mix_fill_w;
    const char *status_text;
    const char *lane_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->states == NULL || local->state_count == 0)
    {
        return;
    }

    current = &local->states[local->current_index];
    left_state = &local->states[(local->current_index == 0) ? (local->state_count - 1) : (local->current_index - 1)];
    right_state = &local->states[(local->current_index + 1) % local->state_count];
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    accent = get_accent(current);
    lane_text = (current->mix_percent >= 50) ? "Blend to B" : "Blend to A";
    mode_pill_w = 44 + (egui_dim_t)strlen(current->mode) * 8;
    if (mode_pill_w < 68)
    {
        mode_pill_w = 68;
    }
    if (mode_pill_w > 96)
    {
        mode_pill_w = 96;
    }
    footer_pill_w = 44 + (egui_dim_t)strlen(current->footer) * 7;
    if (footer_pill_w < 108)
    {
        footer_pill_w = 108;
    }
    if (footer_pill_w > 138)
    {
        footer_pill_w = 138;
    }

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 60, region.location.y + 19, 120, 2, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    egui_canvas_draw_text_in_rect(title_font, "Scene Crossfader", &text_region, EGUI_ALIGN_CENTER, accent, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap cards or rail", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(crossfader_palette.text, crossfader_palette.muted, 28), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview(self, left_state, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0, 0);
    draw_preview(self, right_state, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);
    draw_round_fill_safe(left_rect.location.x + left_rect.size.width - 2, left_rect.location.y + left_rect.size.height / 2, 8, 2, 1, accent,
                         egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 6, right_rect.location.y + right_rect.size.height / 2, 8, 2, 1, accent,
                         egui_color_alpha_mix(self->alpha, 10));

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, crossfader_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 40));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, crossfader_palette.panel,
                         egui_color_alpha_mix(self->alpha, 72));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, crossfader_palette.border,
                           egui_color_alpha_mix(self->alpha, 38));
    draw_round_stroke_safe(main_rect.location.x + 4, main_rect.location.y + 4, main_rect.size.width - 8, main_rect.size.height - 8, 10, 1, accent,
                           egui_color_alpha_mix(self->alpha, 10));

    mode_pill_x = main_rect.location.x + (main_rect.size.width - mode_pill_w) / 2;
    draw_round_fill_safe(mode_pill_x, main_rect.location.y + 10, mode_pill_w, 20, 10, accent, egui_color_alpha_mix(self->alpha, 56));
    draw_round_stroke_safe(mode_pill_x, main_rect.location.y + 10, mode_pill_w, 20, 10, 1, egui_rgb_mix(accent, crossfader_palette.text, 28),
                           egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(main_rect.location.x + 30, main_rect.location.y + 35, main_rect.size.width - 60, 2, 1, accent, egui_color_alpha_mix(self->alpha, 10));
    text_region.location.x = mode_pill_x + 10;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = mode_pill_w - 20;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->mode, &text_region, EGUI_ALIGN_CENTER, crossfader_palette.text, self->alpha);

    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 42, 54, 16, 8, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_stroke_safe(main_rect.location.x + 18, main_rect.location.y + 42, 54, 16, 8, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 72, main_rect.location.y + 42, 54, 16, 8, accent, egui_color_alpha_mix(self->alpha, 10));
    draw_round_stroke_safe(main_rect.location.x + main_rect.size.width - 72, main_rect.location.y + 42, 54, 16, 8, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 18));
    text_region.location.x = main_rect.location.x + 24;
    text_region.location.y = main_rect.location.y + 45;
    text_region.size.width = 42;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(body_font, current->left_tag, &text_region, EGUI_ALIGN_CENTER, crossfader_palette.text, self->alpha);
    text_region.location.x = main_rect.location.x + main_rect.size.width - 66;
    egui_canvas_draw_text_in_rect(body_font, current->right_tag, &text_region, EGUI_ALIGN_CENTER, crossfader_palette.text, self->alpha);
    draw_round_fill_safe(main_rect.location.x + 24, main_rect.location.y + 62, 28 + current->left_level, 2, 1, accent, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 24 - (28 + current->right_level), main_rect.location.y + 62, 28 + current->right_level, 2, 1,
                         accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(main_rect.location.x + 83, main_rect.location.y + 50, main_rect.size.width - 166, 2, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 12));

    mix_x = main_rect.location.x + 18;
    mix_y = main_rect.location.y + 72;
    mix_w = main_rect.size.width - 36;
    handle_x = mix_x + 10 + ((mix_w - 20) * current->mix_percent) / 100;
    mix_fill_w = handle_x - (mix_x + 10);

    draw_round_fill_safe(mix_x, mix_y, mix_w, 52, 12, crossfader_palette.bg, egui_color_alpha_mix(self->alpha, 26));
    draw_round_stroke_safe(mix_x, mix_y, mix_w, 52, 12, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(mix_x + 10, mix_y + 24, mix_w - 20, 6, 3, crossfader_palette.panel_alt, egui_color_alpha_mix(self->alpha, 44));
    draw_round_fill_safe(mix_x + 10, mix_y + 24, mix_fill_w, 6, 3, accent, egui_color_alpha_mix(self->alpha, 46));
    draw_round_fill_safe(mix_x + 8, mix_y + 8, 14, 2, 1, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(mix_x + 8, mix_y + 8, 2, 10, 1, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(mix_x + mix_w - 22, mix_y + 8, 14, 2, 1, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(mix_x + mix_w - 10, mix_y + 8, 2, 10, 1, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(handle_x - 14, mix_y + 12, 28, 30, 14, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(handle_x - 10, mix_y + 16, 20, 22, 10, accent, egui_color_alpha_mix(self->alpha, 72));
    draw_round_stroke_safe(handle_x - 10, mix_y + 16, 20, 22, 10, 1, egui_rgb_mix(accent, crossfader_palette.text, 28), egui_color_alpha_mix(self->alpha, 38));
    draw_round_fill_safe(handle_x - 16, mix_y + 20, 2, 14, 1, accent, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(handle_x + 14, mix_y + 20, 2, 14, 1, accent, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(handle_x - 4, mix_y + 22, 2, 10, 1, crossfader_palette.text, egui_color_alpha_mix(self->alpha, 40));
    draw_round_fill_safe(handle_x + 2, mix_y + 22, 2, 10, 1, crossfader_palette.text, egui_color_alpha_mix(self->alpha, 32));
    draw_round_fill_safe(mix_x + 12, mix_y + 10, 2, 30, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(mix_x + mix_w - 14, mix_y + 10, 2, 30, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(mix_x + 8, mix_y + 24, 6, 6, 3, accent, egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(mix_x + mix_w - 14, mix_y + 24, 6, 6, 3, accent, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(mix_x + (mix_w / 2) - 1, mix_y + 20, 2, 14, 1, crossfader_palette.text, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(mix_x + 22, mix_y + 38, 16 + current->curve_bias * 6, 2, 1, accent, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(mix_x + mix_w - 38 - current->curve_bias * 4, mix_y + 38, 16 + current->curve_bias * 4, 2, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    text_region.location.x = mix_x + 6;
    text_region.location.y = mix_y + 31;
    text_region.size.width = 10;
    text_region.size.height = 8;
    egui_canvas_draw_text_in_rect(body_font, "A", &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(crossfader_palette.text, accent, 20), self->alpha);
    text_region.location.x = mix_x + mix_w - 16;
    egui_canvas_draw_text_in_rect(body_font, "B", &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(crossfader_palette.text, accent, 14), self->alpha);

    text_region.location.x = mix_x + 12;
    text_region.location.y = mix_y + 6;
    text_region.size.width = mix_w - 24;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, lane_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(crossfader_palette.text, crossfader_palette.muted, 18),
                                  self->alpha);

    draw_round_fill_safe(mix_x + 12, mix_y + 42, handle_x - mix_x - 12, 2, 1, accent, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(handle_x, mix_y + 42, mix_x + mix_w - 12 - handle_x, 2, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 16));
    text_region.location.x = mix_x + 10;
    text_region.location.y = mix_y + 54;
    text_region.size.width = mix_w - 20;
    text_region.size.height = 20;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->preset, &text_region, EGUI_ALIGN_CENTER, crossfader_palette.text, self->alpha);
    draw_round_fill_safe(main_rect.location.x + 70, mix_y + 72, main_rect.size.width - 140, 2, 1, accent, egui_color_alpha_mix(self->alpha, 14));

    text_region.location.y = mix_y + 68;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(crossfader_palette.text, crossfader_palette.muted, 18), self->alpha);
    draw_round_fill_safe(main_rect.location.x + 58, mix_y + 83, 4, 4, 2, accent, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 62, mix_y + 83, 4, 4, 2, accent, egui_color_alpha_mix(self->alpha, 14));

    footer_pill_x = main_rect.location.x + (main_rect.size.width - footer_pill_w) / 2;
    draw_round_fill_safe(footer_pill_x, main_rect.location.y + 152, footer_pill_w, 18, 9, crossfader_palette.bg, egui_color_alpha_mix(self->alpha, 20));
    draw_round_stroke_safe(footer_pill_x, main_rect.location.y + 152, footer_pill_w, 18, 9, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(footer_pill_x + 16, main_rect.location.y + 159, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(footer_pill_x + 24, main_rect.location.y + 160, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(footer_pill_x + footer_pill_w - 34, main_rect.location.y + 160, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(footer_pill_x + footer_pill_w - 21, main_rect.location.y + 159, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    text_region.location.x = footer_pill_x + 12;
    text_region.location.y = main_rect.location.y + 154;
    text_region.size.width = footer_pill_w - 24;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(crossfader_palette.text, accent, 16), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev source";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next source";
    }
    else
    {
        status_text = "Crossfade step";
    }

    status_pill_w = 140 + (egui_dim_t)strlen(status_text) * 2;
    if (status_pill_w > 172)
    {
        status_pill_w = 172;
    }
    status_pill_x = region.location.x + (region.size.width - status_pill_w) / 2;
    draw_round_fill_safe(status_pill_x, region.location.y + 240, status_pill_w, 26, 13, crossfader_palette.bg, egui_color_alpha_mix(self->alpha, 48));
    draw_round_stroke_safe(status_pill_x, region.location.y + 240, status_pill_w, 26, 13, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(status_pill_x + 12, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 72));
    draw_round_fill_safe(status_pill_x + 28, region.location.y + 251, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(status_pill_x + 46, region.location.y + 251, 10, 2, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(status_pill_x + status_pill_w - 46, region.location.y + 251, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(status_pill_x + status_pill_w - 28, region.location.y + 251, 10, 2, 1, crossfader_palette.border, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(status_pill_x + status_pill_w - 19, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 60));
    text_region.location.x = status_pill_x + 34;
    text_region.location.y = region.location.y + 246;
    text_region.size.width = status_pill_w - 68;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(crossfader_palette.text, accent, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scene_crossfader_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_scene_crossfader_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_scene_crossfader_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_scene_crossfader_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_scene_crossfader_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_scene_crossfader_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->states = NULL;
    local->state_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
