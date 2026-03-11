#include <stdlib.h>
#include <string.h>

#include "egui_view_piano_roll_editor.h"

typedef struct piano_roll_palette piano_roll_palette_t;
struct piano_roll_palette
{
    egui_color_t bg;
    egui_color_t panel;
    egui_color_t panel_alt;
    egui_color_t border;
    egui_color_t text;
    egui_color_t muted;
    egui_color_t cyan;
    egui_color_t amber;
    egui_color_t lime;
    egui_color_t rose;
    egui_color_t shadow;
};

static const piano_roll_palette_t piano_roll_palette = {
        EGUI_COLOR_HEX(0x0A0E14), EGUI_COLOR_HEX(0x16202A), EGUI_COLOR_HEX(0x111821), EGUI_COLOR_HEX(0x415264),
        EGUI_COLOR_HEX(0xEEF5FF), EGUI_COLOR_HEX(0x8EA2B6), EGUI_COLOR_HEX(0x6BC9FF), EGUI_COLOR_HEX(0xDBA75A),
        EGUI_COLOR_HEX(0x87D88A), EGUI_COLOR_HEX(0xD86E8E), EGUI_COLOR_HEX(0x02050A),
};

static const uint8_t piano_roll_blocks[4][5][3] = {
        {{0, 0, 2}, {1, 2, 2}, {2, 4, 1}, {3, 1, 3}, {4, 5, 1}},
        {{0, 1, 3}, {1, 4, 1}, {2, 2, 2}, {3, 5, 1}, {4, 0, 2}},
        {{0, 3, 2}, {1, 0, 2}, {2, 5, 1}, {3, 2, 2}, {4, 4, 2}},
        {{0, 2, 2}, {1, 5, 1}, {2, 1, 3}, {3, 4, 1}, {4, 0, 2}},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_PIANO_ROLL_EDITOR_MAX_STATES)
    {
        return EGUI_VIEW_PIANO_ROLL_EDITOR_MAX_STATES;
    }
    return count;
}

static egui_color_t get_accent(const egui_view_piano_roll_editor_state_t *state)
{
    if (state->accent_mode >= 3)
    {
        return piano_roll_palette.rose;
    }
    if (state->accent_mode == 2)
    {
        return piano_roll_palette.lime;
    }
    if (state->accent_mode == 1)
    {
        return piano_roll_palette.amber;
    }
    return piano_roll_palette.cyan;
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

    main_rect->location.x = region.location.x + 26;
    main_rect->location.y = region.location.y + 52;
    main_rect->size.width = 188;
    main_rect->size.height = 178;

    left_rect->location.x = region.location.x + 1;
    left_rect->location.y = region.location.y + 90;
    left_rect->size.width = 36;
    left_rect->size.height = 102;

    right_rect->location.x = region.location.x + 203;
    right_rect->location.y = region.location.y + 90;
    right_rect->size.width = 36;
    right_rect->size.height = 102;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox = self->region_screen.location.x + self->padding.left;
    egui_dim_t oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 26;
    main_rect->location.y = oy + 52;
    main_rect->size.width = 188;
    main_rect->size.height = 178;

    left_rect->location.x = ox + 1;
    left_rect->location.y = oy + 90;
    left_rect->size.width = 36;
    left_rect->size.height = 102;

    right_rect->location.x = ox + 203;
    right_rect->location.y = oy + 90;
    right_rect->size.width = 36;
    right_rect->size.height = 102;
}

void egui_view_piano_roll_editor_set_states(egui_view_t *self, const egui_view_piano_roll_editor_state_t *states, uint8_t state_count)
{
    EGUI_LOCAL_INIT(egui_view_piano_roll_editor_t);
    local->states = states;
    local->state_count = clamp_count(state_count);
    if (local->current_index >= local->state_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

static int egui_view_piano_roll_editor_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_piano_roll_editor_t);
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

static void draw_note_block(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_color_t accent, uint8_t focused)
{
    draw_round_fill_safe(x + 1, y + 1, w, h, 5, piano_roll_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x, y, w, h, 5, accent, egui_color_alpha_mix(self->alpha, focused ? 70 : 48));
    draw_round_stroke_safe(x, y, w, h, 5, 1, egui_rgb_mix(accent, piano_roll_palette.text, 32), egui_color_alpha_mix(self->alpha, focused ? 42 : 24));
    draw_round_fill_safe(x + 3, y + 2, 4, h - 4, 2, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, focused ? 18 : 10));
    draw_round_fill_safe(x + 6, y + 4, w - 12, 2, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, focused ? 20 : 10));
}

static void draw_preview(egui_view_t *self, const egui_view_piano_roll_editor_state_t *state, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h,
                         uint8_t pattern_index)
{
    egui_color_t accent = get_accent(state);
    uint8_t i;

    draw_round_fill_safe(x + 1, y + 3, w, h, 8, piano_roll_palette.shadow, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(x, y, w, h, 8, piano_roll_palette.panel_alt, egui_color_alpha_mix(self->alpha, 48));
    draw_round_stroke_safe(x, y, w, h, 8, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(x + 8, y + 10, w - 16, 5, 2, accent, egui_color_alpha_mix(self->alpha, 50));
    draw_round_fill_safe(x + 7, y + 24, 8, h - 40, 4, piano_roll_palette.bg, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(x + 17, y + 24, w - 24, h - 40, 6, piano_roll_palette.bg, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 14, y + 30, 2, h - 52, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 10));
    draw_round_fill_safe(x + 23, y + 30, 2, h - 52, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 8));
    draw_round_fill_safe(x + 29, y + 30, 2, h - 52, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 8));
    draw_round_fill_safe(x + 18, y + 42, w - 24, 2, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 8));
    draw_round_fill_safe(x + 18, y + 54, w - 24, 2, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 8));
    for (i = 0; i < 3; ++i)
    {
        egui_dim_t note_x = x + 19 + i * 4;
        egui_dim_t note_y = y + 32 + ((piano_roll_blocks[pattern_index][i][0] * 10) % 26);
        draw_round_fill_safe(note_x, note_y, 9, 4, 2, accent, egui_color_alpha_mix(self->alpha, 48));
    }
}

static void egui_view_piano_roll_editor_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_piano_roll_editor_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_piano_roll_editor_state_t *current;
    const egui_view_piano_roll_editor_state_t *left_state;
    const egui_view_piano_roll_editor_state_t *right_state;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent;
    egui_dim_t grid_x;
    egui_dim_t grid_y;
    egui_dim_t grid_w;
    egui_dim_t grid_h;
    egui_dim_t loop_x;
    egui_dim_t loop_w;
    egui_dim_t col_w;
    egui_dim_t row_h;
    const char *status_text;
    uint8_t i;
    uint8_t pattern_index;

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
    pattern_index = (uint8_t)(local->current_index % 4);

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 66, region.location.y + 19, 108, 2, 1, accent, egui_color_alpha_mix(self->alpha, 12));
    egui_canvas_draw_text_in_rect(title_font, "Piano Roll Editor", &text_region, EGUI_ALIGN_CENTER, accent, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap lanes or cards to cycle", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(piano_roll_palette.text, piano_roll_palette.muted, 30), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview(self, left_state, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height,
                 (uint8_t)((local->current_index + local->state_count - 1) % 4));
    draw_preview(self, right_state, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height,
                 (uint8_t)((local->current_index + 1) % 4));

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, piano_roll_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 42));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, piano_roll_palette.panel,
                         egui_color_alpha_mix(self->alpha, 72));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, piano_roll_palette.border,
                           egui_color_alpha_mix(self->alpha, 40));

    draw_round_fill_safe(main_rect.location.x + 52, main_rect.location.y + 10, 84, 20, 10, accent, egui_color_alpha_mix(self->alpha, 58));
    draw_round_stroke_safe(main_rect.location.x + 52, main_rect.location.y + 10, 84, 20, 10, 1, egui_rgb_mix(accent, piano_roll_palette.text, 32),
                           egui_color_alpha_mix(self->alpha, 42));
    text_region.location.x = main_rect.location.x + 63;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = 62;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->mode, &text_region, EGUI_ALIGN_CENTER, piano_roll_palette.text, self->alpha);

    text_region.location.x = main_rect.location.x + 18;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = 34;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, "REC", &text_region, EGUI_ALIGN_CENTER, piano_roll_palette.muted, self->alpha);
    draw_round_fill_safe(main_rect.location.x + 24, main_rect.location.y + 26, 22, 2, 1, accent, egui_color_alpha_mix(self->alpha, 14));

    text_region.location.x = main_rect.location.x + 138;
    text_region.size.width = 34;
    egui_canvas_draw_text_in_rect(body_font, "BAR", &text_region, EGUI_ALIGN_CENTER, piano_roll_palette.muted, self->alpha);
    draw_round_fill_safe(main_rect.location.x + 144, main_rect.location.y + 26, 22, 2, 1, accent, egui_color_alpha_mix(self->alpha, 14));

    grid_x = main_rect.location.x + 18;
    grid_y = main_rect.location.y + 40;
    grid_w = main_rect.size.width - 36;
    grid_h = 82;
    col_w = 22;
    row_h = 16;

    draw_round_fill_safe(grid_x, grid_y, grid_w, grid_h, 10, piano_roll_palette.bg, egui_color_alpha_mix(self->alpha, 30));
    draw_round_stroke_safe(grid_x, grid_y, grid_w, grid_h, 10, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(grid_x + 6, grid_y + 10, 24, grid_h - 20, 5, piano_roll_palette.panel_alt, egui_color_alpha_mix(self->alpha, 42));
    draw_round_stroke_safe(grid_x + 6, grid_y + 10, 24, grid_h - 20, 5, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(grid_x + 34, grid_y + 10, grid_w - 42, grid_h - 20, 8, piano_roll_palette.panel_alt, egui_color_alpha_mix(self->alpha, 20));

    for (i = 0; i < 5; ++i)
    {
        egui_dim_t key_y = grid_y + 14 + i * row_h;
        egui_alpha_t key_alpha = (i == current->focus_row) ? 34 : 18;
        egui_color_t key_color = (i == 1 || i == 4) ? piano_roll_palette.text : accent;
        if (i == current->focus_row)
        {
            draw_round_fill_safe(grid_x + 36, key_y + 1, grid_w - 50, 10, 4, accent, egui_color_alpha_mix(self->alpha, 10));
            draw_round_stroke_safe(grid_x + 8, key_y - 1, 20, 12, 4, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 16));
        }
        draw_round_fill_safe(grid_x + 9, key_y, 18, 10, 4, key_color, egui_color_alpha_mix(self->alpha, key_alpha));
        draw_round_fill_safe(grid_x + 23, key_y + 2, 3, 6, 1, piano_roll_palette.bg, egui_color_alpha_mix(self->alpha, 24));
        draw_round_fill_safe(grid_x + 36, key_y + 4, grid_w - 50, 2, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 12));
    }
    for (i = 0; i < 6; ++i)
    {
        egui_dim_t line_x = grid_x + 40 + i * col_w;
        if ((i % 2) == 0)
        {
            draw_round_fill_safe(line_x + 2, grid_y + 12, col_w - 2, grid_h - 24, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 3));
        }
        draw_round_fill_safe(line_x, grid_y + 12, 2, grid_h - 24, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 12));
    }

    loop_x = grid_x + 40 + current->loop_start * col_w;
    loop_w = current->loop_width * col_w;
    draw_round_fill_safe(loop_x, grid_y + 12, loop_w, grid_h - 24, 4, accent, egui_color_alpha_mix(self->alpha, 8));
    draw_round_fill_safe(loop_x, grid_y + 6, loop_w, 6, 3, accent, egui_color_alpha_mix(self->alpha, 42));
    draw_round_fill_safe(loop_x, grid_y + 4, 3, 10, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(loop_x + loop_w - 3, grid_y + 4, 3, 10, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 18));

    for (i = 0; i < 5; ++i)
    {
        uint8_t row = piano_roll_blocks[pattern_index][i][0];
        uint8_t col = piano_roll_blocks[pattern_index][i][1];
        uint8_t span = piano_roll_blocks[pattern_index][i][2];
        egui_dim_t note_x = grid_x + 41 + col * col_w;
        egui_dim_t note_y = grid_y + 13 + row * row_h;
        uint8_t focused = (uint8_t)(row == current->focus_row);

        draw_note_block(self, note_x, note_y, span * col_w - 4, 10, accent, focused);
    }

    draw_round_fill_safe(grid_x + 41 + current->play_col * col_w, grid_y + 8, 3, grid_h - 16, 1, piano_roll_palette.text,
                         egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(grid_x + 38 + current->play_col * col_w, grid_y + 6, 9, 3, 1, piano_roll_palette.text, egui_color_alpha_mix(self->alpha, 36));
    draw_round_fill_safe(grid_x + 38 + current->play_col * col_w, grid_y + grid_h - 11, 9, 3, 1, piano_roll_palette.text,
                         egui_color_alpha_mix(self->alpha, 24));

    text_region.location.x = main_rect.location.x + 18;
    text_region.location.y = main_rect.location.y + 127;
    text_region.size.width = main_rect.size.width - 36;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->clip, &text_region, EGUI_ALIGN_CENTER, piano_roll_palette.text,
                                  self->alpha);

    text_region.location.y = main_rect.location.y + 139;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(piano_roll_palette.text, piano_roll_palette.muted, 18), self->alpha);
    draw_round_fill_safe(main_rect.location.x + 58, main_rect.location.y + 151, main_rect.size.width - 116, 2, 1, piano_roll_palette.border,
                         egui_color_alpha_mix(self->alpha, 10));

    draw_round_fill_safe(main_rect.location.x + 30, main_rect.location.y + 155, main_rect.size.width - 60, 18, 9, piano_roll_palette.bg,
                         egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(main_rect.location.x + 30, main_rect.location.y + 155, main_rect.size.width - 60, 18, 9, 1, piano_roll_palette.border,
                           egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(main_rect.location.x + 50, main_rect.location.y + 162, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 55, main_rect.location.y + 162, 5, 5, 2, accent, egui_color_alpha_mix(self->alpha, 54));
    text_region.location.x = main_rect.location.x + 30;
    text_region.location.y = main_rect.location.y + 157;
    text_region.size.width = main_rect.size.width - 60;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(piano_roll_palette.text, accent, 16), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev clip queued";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next clip queued";
    }
    else
    {
        status_text = "Playhead advanced";
    }

    draw_round_fill_safe(region.location.x + 22, region.location.y + 239, 196, 28, 14, piano_roll_palette.bg, egui_color_alpha_mix(self->alpha, 46));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 239, 196, 28, 14, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(region.location.x + 34, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 74));
    draw_round_fill_safe(region.location.x + 50, region.location.y + 252, 12, 2, 1, accent, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(region.location.x + 68, region.location.y + 252, 12, 2, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(region.location.x + 142, region.location.y + 252, 12, 2, 1, accent, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 160, region.location.y + 252, 12, 2, 1, piano_roll_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(region.location.x + 184, region.location.y + 249, 7, 7, 3, accent, egui_color_alpha_mix(self->alpha, 66));
    text_region.location.x = region.location.x + 50;
    text_region.location.y = region.location.y + 247;
    text_region.size.width = 134;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(piano_roll_palette.text, accent, 22), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_piano_roll_editor_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_piano_roll_editor_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_piano_roll_editor_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_piano_roll_editor_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_piano_roll_editor_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_piano_roll_editor_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->states = NULL;
    local->state_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
