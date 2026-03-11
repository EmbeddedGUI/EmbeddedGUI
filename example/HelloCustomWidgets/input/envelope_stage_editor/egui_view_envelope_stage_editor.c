#include <stdlib.h>
#include <string.h>

#include "egui_view_envelope_stage_editor.h"

typedef struct envelope_palette envelope_palette_t;
struct envelope_palette
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

static const envelope_palette_t envelope_palette = {
        EGUI_COLOR_HEX(0x0A0F16), EGUI_COLOR_HEX(0x18212B), EGUI_COLOR_HEX(0x10171F), EGUI_COLOR_HEX(0x445462),
        EGUI_COLOR_HEX(0xF2F7FC), EGUI_COLOR_HEX(0x8A9DAE), EGUI_COLOR_HEX(0x68CCFF), EGUI_COLOR_HEX(0xD7A45A),
        EGUI_COLOR_HEX(0x8CDCA2), EGUI_COLOR_HEX(0xD87496), EGUI_COLOR_HEX(0x02060B),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_ENVELOPE_STAGE_EDITOR_MAX_STATES)
    {
        return EGUI_VIEW_ENVELOPE_STAGE_EDITOR_MAX_STATES;
    }
    return count;
}

static egui_color_t get_accent(const egui_view_envelope_stage_editor_state_t *state)
{
    if (state->accent_mode >= 3)
    {
        return envelope_palette.rose;
    }
    if (state->accent_mode == 2)
    {
        return envelope_palette.mint;
    }
    if (state->accent_mode == 1)
    {
        return envelope_palette.amber;
    }
    return envelope_palette.cyan;
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

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 28;
    main_rect->location.y = region.location.y + 54;
    main_rect->size.width = 184;
    main_rect->size.height = 174;

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
    main_rect->location.y = oy + 54;
    main_rect->size.width = 184;
    main_rect->size.height = 174;

    left_rect->location.x = ox + 3;
    left_rect->location.y = oy + 92;
    left_rect->size.width = 34;
    left_rect->size.height = 100;

    right_rect->location.x = ox + 203;
    right_rect->location.y = oy + 92;
    right_rect->size.width = 34;
    right_rect->size.height = 100;
}

void egui_view_envelope_stage_editor_set_states(egui_view_t *self, const egui_view_envelope_stage_editor_state_t *states, uint8_t state_count)
{
    EGUI_LOCAL_INIT(egui_view_envelope_stage_editor_t);
    local->states = states;
    local->state_count = clamp_count(state_count);
    if (local->current_index >= local->state_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

static int egui_view_envelope_stage_editor_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_envelope_stage_editor_t);
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

static void draw_preview(egui_view_t *self, const egui_view_envelope_stage_editor_state_t *state, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h,
                         uint8_t inward_edge)
{
    egui_color_t accent = get_accent(state);
    uint8_t i;
    egui_dim_t base_y = y + h - 18;
    egui_dim_t col_w = 5;

    draw_round_fill_safe(x + 1, y + 3, w, h, 8, envelope_palette.shadow, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(x, y, w, h, 8, envelope_palette.panel_alt, egui_color_alpha_mix(self->alpha, 48));
    draw_round_stroke_safe(x, y, w, h, 8, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 26));
    if (inward_edge == 0)
    {
        draw_round_fill_safe(x + w - 5, y + 10, 3, h - 20, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    }
    else
    {
        draw_round_fill_safe(x + 2, y + 10, 3, h - 20, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    }
    draw_round_fill_safe(x + 8, y + 11, w - 16, 4, 2, accent, egui_color_alpha_mix(self->alpha, 50));
    draw_round_fill_safe(x + 8, y + 24, w - 16, h - 38, 6, envelope_palette.bg, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 8, base_y - 2, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 34));
    for (i = 0; i < 4; ++i)
    {
        egui_dim_t block_h = 8 + state->stage_levels[i] * 4;
        egui_dim_t block_x = x + 10 + i * 6;
        uint8_t focused = (uint8_t)(i == state->focus_stage);
        if (focused)
        {
            draw_round_fill_safe(block_x - 1, base_y - block_h - 4, col_w + 2, block_h + 8, 3, accent, egui_color_alpha_mix(self->alpha, 12));
        }
        draw_round_fill_safe(block_x, base_y - block_h, col_w, block_h, 2, accent, egui_color_alpha_mix(self->alpha, focused ? 58 : 42));
        draw_round_fill_safe(block_x + 1, base_y - block_h - 3, 3, 3, 1, accent, egui_color_alpha_mix(self->alpha, focused ? 72 : 56));
        if (i < 3)
        {
            egui_dim_t next_h = 8 + state->stage_levels[i + 1] * 4;
            egui_dim_t line_y = base_y - ((block_h + next_h) / 2);
            draw_round_fill_safe(block_x + col_w, line_y, 7, 2, 1, accent, egui_color_alpha_mix(self->alpha, 22));
        }
    }
    draw_round_fill_safe(x + w - 14, base_y - 2, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 28));
}

static void egui_view_envelope_stage_editor_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_envelope_stage_editor_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_envelope_stage_editor_state_t *current;
    const egui_view_envelope_stage_editor_state_t *left_state;
    const egui_view_envelope_stage_editor_state_t *right_state;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent;
    egui_dim_t env_x;
    egui_dim_t env_y;
    egui_dim_t env_w;
    egui_dim_t env_h;
    egui_dim_t base_y;
    egui_dim_t mode_pill_w;
    egui_dim_t mode_pill_x;
    egui_dim_t footer_pill_w;
    egui_dim_t footer_pill_x;
    egui_dim_t status_pill_w;
    egui_dim_t status_pill_x;
    egui_dim_t stage_w;
    egui_dim_t stage_center_w;
    egui_dim_t stage_mid_x[4];
    egui_dim_t stage_top_y[4];
    egui_dim_t stage_span[4];
    const char *status_text;
    const char *focus_text;
    uint8_t i;
    egui_dim_t span_total;
    egui_dim_t span_cursor;
    static const char *stage_labels[4] = {"A", "D", "S", "R"};
    static const char *stage_names[4] = {"Attack", "Decay", "Sustain", "Release"};

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
    focus_text = stage_names[current->focus_stage & 0x03];
    mode_pill_w = 42 + (egui_dim_t)strlen(current->mode) * 8;
    if (mode_pill_w < 66)
    {
        mode_pill_w = 66;
    }
    if (mode_pill_w > 92)
    {
        mode_pill_w = 92;
    }
    footer_pill_w = 42 + (egui_dim_t)strlen(current->footer) * 7;
    if (footer_pill_w < 96)
    {
        footer_pill_w = 96;
    }
    if (footer_pill_w > 136)
    {
        footer_pill_w = 136;
    }

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 60, region.location.y + 19, 120, 2, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    egui_canvas_draw_text_in_rect(title_font, "Envelope Stage Editor", &text_region, EGUI_ALIGN_CENTER, accent, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap cards or env to cycle", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(envelope_palette.text, envelope_palette.muted, 30), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview(self, left_state, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_preview(self, right_state, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);
    draw_round_fill_safe(left_rect.location.x + left_rect.size.width - 2, left_rect.location.y + left_rect.size.height / 2, 8, 2, 1, accent,
                         egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 6, right_rect.location.y + right_rect.size.height / 2, 8, 2, 1, accent,
                         egui_color_alpha_mix(self->alpha, 10));

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, envelope_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 38));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, envelope_palette.panel,
                         egui_color_alpha_mix(self->alpha, 72));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, envelope_palette.border,
                           egui_color_alpha_mix(self->alpha, 40));

    mode_pill_x = main_rect.location.x + (main_rect.size.width - mode_pill_w) / 2;
    draw_round_fill_safe(mode_pill_x, main_rect.location.y + 10, mode_pill_w, 20, 10, accent, egui_color_alpha_mix(self->alpha, 58));
    draw_round_stroke_safe(mode_pill_x, main_rect.location.y + 10, mode_pill_w, 20, 10, 1, egui_rgb_mix(accent, envelope_palette.text, 32),
                           egui_color_alpha_mix(self->alpha, 38));
    draw_round_fill_safe(main_rect.location.x + 28, main_rect.location.y + 34, main_rect.size.width - 56, 2, 1, accent, egui_color_alpha_mix(self->alpha, 10));
    text_region.location.x = mode_pill_x + 10;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = mode_pill_w - 20;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->mode, &text_region, EGUI_ALIGN_CENTER, envelope_palette.text, self->alpha);

    env_x = main_rect.location.x + 18;
    env_y = main_rect.location.y + 42;
    env_w = main_rect.size.width - 36;
    env_h = 86;
    base_y = env_y + env_h - 16;
    stage_w = 22;
    stage_center_w = env_w - 48;
    span_total = 0;
    for (i = 0; i < 4; ++i)
    {
        stage_span[i] = 12 + current->stage_levels[i] * 2;
        span_total += stage_span[i];
    }
    span_cursor = 0;

    draw_round_fill_safe(env_x, env_y, env_w, env_h, 10, envelope_palette.bg, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(env_x, env_y, env_w, env_h, 10, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(env_x + 12, env_y + 16, 2, env_h - 28, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(env_x + 10, env_y + 14, 6, 6, 3, envelope_palette.text, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(env_x + 10, env_y + 34, 6, 6, 3, envelope_palette.text, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(env_x + 10, env_y + 54, 6, 6, 3, envelope_palette.text, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(env_x + 10, base_y - 2, 6, 6, 3, envelope_palette.text, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(env_x + 8, env_y + 8, 14, 3, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(env_x + 8, env_y + 8, 3, 12, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(env_x + env_w - 22, env_y + env_h - 12, 14, 3, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(env_x + env_w - 11, env_y + env_h - 20, 3, 12, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(env_x + 10, env_y + 12, env_w - 20, 2, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(env_x + 10, env_y + 32, env_w - 20, 2, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(env_x + 10, env_y + 52, env_w - 20, 2, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(env_x + 10, base_y, env_w - 20, 2, 1, envelope_palette.text, egui_color_alpha_mix(self->alpha, 18));

    for (i = 0; i < 4; ++i)
    {
        egui_dim_t block_h = 12 + current->stage_levels[i] * 10;
        egui_dim_t center_x;
        egui_dim_t block_x;
        egui_dim_t block_y = base_y - block_h;
        egui_dim_t timing_w;
        uint8_t focused = (uint8_t)(i == current->focus_stage);

        span_cursor += stage_span[i] / 2;
        center_x = env_x + 24 + (span_cursor * stage_center_w) / EGUI_MAX(span_total, 1);
        span_cursor += stage_span[i] / 2;
        block_x = center_x - (stage_w / 2);
        stage_mid_x[i] = block_x + (stage_w / 2);
        stage_top_y[i] = block_y;

        if (focused)
        {
            draw_round_fill_safe(block_x - 6, env_y + 8, stage_w + 12, env_h - 18, 6, accent, egui_color_alpha_mix(self->alpha, 10));
        }
        draw_round_fill_safe(block_x, block_y, stage_w, block_h, 6, accent, egui_color_alpha_mix(self->alpha, focused ? 70 : 46));
        draw_round_stroke_safe(block_x, block_y, stage_w, block_h, 6, 1, egui_rgb_mix(accent, envelope_palette.text, 30),
                               egui_color_alpha_mix(self->alpha, focused ? 42 : 22));
        draw_round_fill_safe(block_x + 7, block_y + 6, stage_w - 14, 2, 1, envelope_palette.text, egui_color_alpha_mix(self->alpha, 16));
        draw_round_fill_safe(block_x + stage_w + 4, block_y + block_h - 2, 8, 3, 1, accent, egui_color_alpha_mix(self->alpha, 26));
        if (focused)
        {
            draw_round_fill_safe(stage_mid_x[i] - 6, stage_top_y[i] - 6, 12, 12, 6, accent, egui_color_alpha_mix(self->alpha, 16));
        }
        draw_round_fill_safe(stage_mid_x[i] - 4, stage_top_y[i] - 4, 8, 8, 4, accent, egui_color_alpha_mix(self->alpha, focused ? 84 : 56));
        draw_round_stroke_safe(stage_mid_x[i] - 4, stage_top_y[i] - 4, 8, 8, 4, 1, envelope_palette.text, egui_color_alpha_mix(self->alpha, focused ? 36 : 20));
        timing_w = 10 + stage_span[i] / 2;
        draw_round_fill_safe(stage_mid_x[i] - (timing_w / 2), base_y + 16, timing_w, 3, 1, accent, egui_color_alpha_mix(self->alpha, focused ? 34 : 18));
        draw_round_fill_safe(block_x - 1, base_y + 2, stage_w + 2, 12, 6, envelope_palette.bg, egui_color_alpha_mix(self->alpha, focused ? 34 : 18));
        draw_round_stroke_safe(block_x - 1, base_y + 2, stage_w + 2, 12, 6, 1, accent, egui_color_alpha_mix(self->alpha, focused ? 24 : 10));

        text_region.location.x = block_x - 2;
        text_region.location.y = base_y + 4;
        text_region.size.width = stage_w + 4;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(body_font, stage_labels[i], &text_region, EGUI_ALIGN_CENTER, focused ? envelope_palette.text : envelope_palette.muted,
                                      self->alpha);
    }

    draw_round_fill_safe(env_x + 12, base_y - 2, stage_mid_x[0] - (env_x + 12), 3, 1, accent, egui_color_alpha_mix(self->alpha, 28));
    for (i = 0; i < 3; ++i)
    {
        egui_dim_t left_x = stage_mid_x[i];
        egui_dim_t left_y = stage_top_y[i] + 3;
        egui_dim_t right_x = stage_mid_x[i + 1];
        egui_dim_t right_y = stage_top_y[i + 1] + 3;
        egui_dim_t mid_x = (left_x + right_x) / 2;

        draw_round_fill_safe(left_x, left_y, mid_x - left_x + 2, 3, 1, accent, egui_color_alpha_mix(self->alpha, 26));
        if (right_y > left_y)
        {
            draw_round_fill_safe(mid_x, left_y, 3, right_y - left_y + 3, 1, accent, egui_color_alpha_mix(self->alpha, 26));
        }
        else
        {
            draw_round_fill_safe(mid_x, right_y, 3, left_y - right_y + 3, 1, accent, egui_color_alpha_mix(self->alpha, 26));
        }
        draw_round_fill_safe(mid_x, right_y, right_x - mid_x + 3, 3, 1, accent, egui_color_alpha_mix(self->alpha, 26));
    }
    draw_round_fill_safe(stage_mid_x[3], stage_top_y[3] + 3, env_x + env_w - 18 - stage_mid_x[3], 3, 1, accent, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(env_x + env_w - 18, stage_top_y[3] + 3, 3, base_y - stage_top_y[3] - 1, 1, accent, egui_color_alpha_mix(self->alpha, 24));

    draw_round_fill_safe(env_x + 12, env_y + 16, 6, 6, 3, accent, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(env_x + env_w - 18, base_y - 2, 6, 6, 3, accent, egui_color_alpha_mix(self->alpha, 44));
    {
        egui_dim_t focus_pill_w = 18 + (egui_dim_t)strlen(focus_text) * 5;
        egui_dim_t focus_pill_x;
        if (focus_pill_w < 44)
        {
            focus_pill_w = 44;
        }
        if (focus_pill_w > 56)
        {
            focus_pill_w = 56;
        }
        focus_pill_x = stage_mid_x[current->focus_stage & 0x03] - (focus_pill_w / 2);
        if (focus_pill_x < env_x + 18)
        {
            focus_pill_x = env_x + 18;
        }
        if (focus_pill_x > env_x + env_w - focus_pill_w - 18)
        {
            focus_pill_x = env_x + env_w - focus_pill_w - 18;
        }
        draw_round_fill_safe(focus_pill_x, env_y - 10, focus_pill_w, 12, 6, accent, egui_color_alpha_mix(self->alpha, 16));
        draw_round_stroke_safe(focus_pill_x, env_y - 10, focus_pill_w, 12, 6, 1, accent, egui_color_alpha_mix(self->alpha, 18));
        text_region.location.x = focus_pill_x + 4;
        text_region.location.y = env_y - 9;
        text_region.size.width = focus_pill_w - 8;
        text_region.size.height = 8;
        egui_canvas_draw_text_in_rect(body_font, focus_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(envelope_palette.text, accent, 12), self->alpha);
    }

    text_region.location.x = main_rect.location.x + 18;
    text_region.location.y = main_rect.location.y + 128;
    text_region.size.width = main_rect.size.width - 36;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->preset, &text_region, EGUI_ALIGN_CENTER, envelope_palette.text,
                                  self->alpha);
    draw_round_fill_safe(main_rect.location.x + 72, main_rect.location.y + 141, main_rect.size.width - 144, 2, 1, accent,
                         egui_color_alpha_mix(self->alpha, 16));

    text_region.location.y = main_rect.location.y + 140;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(envelope_palette.text, envelope_palette.muted, 18),
                                  self->alpha);

    footer_pill_x = main_rect.location.x + (main_rect.size.width - footer_pill_w) / 2;
    draw_round_fill_safe(footer_pill_x, main_rect.location.y + 152, footer_pill_w, 18, 9, envelope_palette.bg, egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(footer_pill_x, main_rect.location.y + 152, footer_pill_w, 18, 9, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(footer_pill_x + 24, main_rect.location.y + 160, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(footer_pill_x + footer_pill_w - 34, main_rect.location.y + 160, 10, 2, 1, accent, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(footer_pill_x + 16, main_rect.location.y + 159, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(footer_pill_x + footer_pill_w - 21, main_rect.location.y + 159, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    text_region.location.x = footer_pill_x;
    text_region.location.y = main_rect.location.y + 154;
    text_region.size.width = footer_pill_w;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(envelope_palette.text, accent, 16), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev env armed";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next env armed";
    }
    else
    {
        status_text = "Stage advanced";
    }

    status_pill_w = 168;
    status_pill_x = region.location.x + (region.size.width - status_pill_w) / 2;
    draw_round_fill_safe(status_pill_x, region.location.y + 240, status_pill_w, 26, 13, envelope_palette.bg, egui_color_alpha_mix(self->alpha, 46));
    draw_round_stroke_safe(status_pill_x, region.location.y + 240, status_pill_w, 26, 13, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(status_pill_x + 12, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 74));
    draw_round_fill_safe(status_pill_x + 28, region.location.y + 251, 12, 2, 1, accent, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(status_pill_x + 46, region.location.y + 251, 12, 2, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(status_pill_x + status_pill_w - 58, region.location.y + 251, 12, 2, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(status_pill_x + status_pill_w - 40, region.location.y + 251, 12, 2, 1, envelope_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(status_pill_x + status_pill_w - 20, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 42));
    text_region.location.x = status_pill_x + 34;
    text_region.location.y = region.location.y + 246;
    text_region.size.width = status_pill_w - 68;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(envelope_palette.text, accent, 22), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_envelope_stage_editor_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_envelope_stage_editor_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_envelope_stage_editor_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_envelope_stage_editor_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_envelope_stage_editor_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_envelope_stage_editor_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->states = NULL;
    local->state_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
