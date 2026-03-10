#include <stdlib.h>
#include <string.h>

#include "egui_view_flipdot_matrix_panel.h"

typedef struct flipdot_palette flipdot_palette_t;
struct flipdot_palette
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
    egui_color_t dot_off;
};

static const flipdot_palette_t flipdot_palette = {
        EGUI_COLOR_HEX(0x0F141A), EGUI_COLOR_HEX(0x1B232D), EGUI_COLOR_HEX(0x5C6774), EGUI_COLOR_HEX(0xF4F8FC), EGUI_COLOR_HEX(0x99A8B5),
        EGUI_COLOR_HEX(0x77CCFF), EGUI_COLOR_HEX(0xD9A15E), EGUI_COLOR_HEX(0x92D39C), EGUI_COLOR_HEX(0x04080D), EGUI_COLOR_HEX(0x27313A),
};

static const uint16_t flipdot_patterns[4][3] = {
        {0x0F3C, 0x1CF8, 0x0E70},
        {0x1E38, 0x0F1C, 0x19C6},
        {0x1BD8, 0x0E38, 0x1B0E},
        {0x0FF0, 0x1998, 0x0E3C},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_FLIPDOT_MATRIX_PANEL_MAX_STATES)
    {
        return EGUI_VIEW_FLIPDOT_MATRIX_PANEL_MAX_STATES;
    }
    return count;
}

static egui_color_t get_accent_color(const egui_view_flipdot_matrix_panel_state_t *state)
{
    if (state->accent_mode >= 2)
    {
        return flipdot_palette.safe;
    }
    if (state->accent_mode == 1)
    {
        return flipdot_palette.warm;
    }
    return flipdot_palette.accent;
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

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 30;
    main_rect->location.y = region.location.y + 54;
    main_rect->size.width = 180;
    main_rect->size.height = 172;

    left_rect->location.x = region.location.x + 2;
    left_rect->location.y = region.location.y + 92;
    left_rect->size.width = 40;
    left_rect->size.height = 104;

    right_rect->location.x = region.location.x + 198;
    right_rect->location.y = region.location.y + 92;
    right_rect->size.width = 40;
    right_rect->size.height = 104;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox = self->region_screen.location.x + self->padding.left;
    egui_dim_t oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 30;
    main_rect->location.y = oy + 54;
    main_rect->size.width = 180;
    main_rect->size.height = 172;

    left_rect->location.x = ox + 2;
    left_rect->location.y = oy + 92;
    left_rect->size.width = 40;
    left_rect->size.height = 104;

    right_rect->location.x = ox + 198;
    right_rect->location.y = oy + 92;
    right_rect->size.width = 40;
    right_rect->size.height = 104;
}

void egui_view_flipdot_matrix_panel_set_states(
        egui_view_t *self,
        const egui_view_flipdot_matrix_panel_state_t *states,
        uint8_t state_count)
{
    EGUI_LOCAL_INIT(egui_view_flipdot_matrix_panel_t);
    local->states = states;
    local->state_count = clamp_count(state_count);
    if (local->current_index >= local->state_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_flipdot_matrix_panel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_flipdot_matrix_panel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static int egui_view_flipdot_matrix_panel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_flipdot_matrix_panel_t);
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

static void draw_dot(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    draw_round_fill_safe(x, y, 4, 4, 2, color, alpha);
}

static void draw_matrix_row(egui_view_t *self, egui_dim_t x, egui_dim_t y, uint16_t pattern, egui_color_t accent_color, uint8_t focused)
{
    uint8_t i;
    egui_alpha_t on_alpha = focused ? 88 : 66;
    egui_alpha_t off_alpha = focused ? 22 : 12;

    for (i = 0; i < 12; ++i)
    {
        egui_dim_t dx = x + (i % 6) * 10;
        egui_dim_t dy = y + (i / 6) * 10;
        if ((pattern >> i) & 0x01)
        {
            draw_dot(self, dx, dy, accent_color, egui_color_alpha_mix(self->alpha, on_alpha));
        }
        else
        {
            draw_dot(self, dx, dy, flipdot_palette.dot_off, egui_color_alpha_mix(self->alpha, off_alpha));
        }
    }
}

static void draw_preview(
        egui_view_t *self,
        const egui_view_flipdot_matrix_panel_state_t *state,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t right_side,
        uint8_t pattern_index)
{
    egui_color_t accent_color = get_accent_color(state);
    egui_dim_t strip_x = right_side ? (x + w - 10) : (x + 6);

    draw_round_fill_safe(x + 1, y + 3, w, h, 8, flipdot_palette.shadow, egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(x, y, w, h, 8, flipdot_palette.panel, egui_color_alpha_mix(self->alpha, 50));
    draw_round_stroke_safe(x, y, w, h, 8, 1, flipdot_palette.border, egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(strip_x, y + 12, 4, h - 24, 2, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 10, y + 14, w - 20, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(x + 8, y + 32, w - 16, 34, 10, flipdot_palette.surface, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(x + 8, y + 32, w - 16, 34, 10, 1, flipdot_palette.border, egui_color_alpha_mix(self->alpha, 22));
    draw_matrix_row(self, x + 12, y + 38, flipdot_patterns[pattern_index][1], accent_color, 0);
    draw_round_fill_safe(x + 10, y + h - 16, w - 20, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 36));
}

static void egui_view_flipdot_matrix_panel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_flipdot_matrix_panel_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_flipdot_matrix_panel_state_t *current;
    const egui_view_flipdot_matrix_panel_state_t *left_state;
    const egui_view_flipdot_matrix_panel_state_t *right_state;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const char *status_text;
    uint8_t pattern_index;
    uint8_t row;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->states == NULL || local->state_count == 0)
    {
        return;
    }

    current = &local->states[local->current_index];
    left_state = &local->states[(local->current_index == 0) ? (local->state_count - 1) : (local->current_index - 1)];
    right_state = &local->states[(local->current_index + 1) % local->state_count];
    accent_color = get_accent_color(current);
    pattern_index = (uint8_t)(local->current_index % 4);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 66, region.location.y + 19, 108, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 12));
    egui_canvas_draw_text_in_rect(title_font, "Flipdot Matrix Panel", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap cards or board to step", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(flipdot_palette.text, flipdot_palette.muted, 30), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview(self, left_state, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0,
                 (uint8_t)((local->current_index + local->state_count - 1) % 4));
    draw_preview(self, right_state, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1,
                 (uint8_t)((local->current_index + 1) % 4));

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, flipdot_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 42));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, flipdot_palette.panel,
                         egui_color_alpha_mix(self->alpha, 70));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, flipdot_palette.border,
                           egui_color_alpha_mix(self->alpha, 44));

    pill_w = (egui_dim_t)(50 + strlen(current->status) * 5);
    if (pill_w < 88)
    {
        pill_w = 88;
    }
    pill_x = main_rect.location.x + (main_rect.size.width - pill_w) / 2;
    draw_round_fill_safe(pill_x, main_rect.location.y + 10, pill_w, 20, 10, accent_color, egui_color_alpha_mix(self->alpha, 58));
    draw_round_stroke_safe(pill_x, main_rect.location.y + 10, pill_w, 20, 10, 1, egui_rgb_mix(accent_color, flipdot_palette.text, 28),
                           egui_color_alpha_mix(self->alpha, 40));
    text_region.location.x = pill_x + 9;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = pill_w - 18;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, flipdot_palette.text, self->alpha);

    draw_round_fill_safe(main_rect.location.x + 20, main_rect.location.y + 18, pill_x - main_rect.location.x - 28, 2, 1, accent_color,
                         egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(pill_x + pill_w + 8, main_rect.location.y + 18, main_rect.location.x + main_rect.size.width - pill_x - pill_w - 28, 2, 1, accent_color,
                         egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 17, 4, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 20, main_rect.location.y + 17, 4, 4, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, 24));

    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 44, main_rect.size.width - 36, 74, 18, flipdot_palette.surface,
                         egui_color_alpha_mix(self->alpha, 30));
    draw_round_stroke_safe(main_rect.location.x + 18, main_rect.location.y + 44, main_rect.size.width - 36, 74, 18, 1, flipdot_palette.border,
                           egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(main_rect.location.x + 30, main_rect.location.y + 50, main_rect.size.width - 60, 2, 1, flipdot_palette.border,
                         egui_color_alpha_mix(self->alpha, 12));
    for (row = 0; row < 3; ++row)
    {
        egui_dim_t row_y = main_rect.location.y + 54 + row * 20;
        uint8_t focused = (uint8_t)(row == current->focus_row);
        draw_round_fill_safe(main_rect.location.x + 28, row_y - 4, main_rect.size.width - 56, 16, 8, accent_color,
                             egui_color_alpha_mix(self->alpha, focused ? 22 : 6));
        draw_round_fill_safe(main_rect.location.x + 40, row_y + 4, 8, 4, 2, accent_color,
                             egui_color_alpha_mix(self->alpha, focused ? 52 : 18));
        draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 48, row_y + 4, 8, 4, 2, accent_color,
                             egui_color_alpha_mix(self->alpha, focused ? 52 : 18));
        draw_matrix_row(self, main_rect.location.x + 56, row_y, flipdot_patterns[pattern_index][row], accent_color, focused);
    }

    text_region.location.x = main_rect.location.x + 18;
    text_region.location.y = main_rect.location.y + 124;
    text_region.size.width = main_rect.size.width - 36;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->preset, &text_region, EGUI_ALIGN_CENTER, flipdot_palette.text, self->alpha);

    text_region.location.y = main_rect.location.y + 138;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(flipdot_palette.text, flipdot_palette.muted, 20), self->alpha);

    draw_round_fill_safe(main_rect.location.x + 38, main_rect.location.y + 152, main_rect.size.width - 76, 18, 9, flipdot_palette.surface,
                         egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(main_rect.location.x + 38, main_rect.location.y + 152, main_rect.size.width - 76, 18, 9, 1, flipdot_palette.border,
                           egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(main_rect.location.x + 56, main_rect.location.y + 159, 5, 5, 2, accent_color, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(main_rect.location.x + main_rect.size.width - 61, main_rect.location.y + 159, 5, 5, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, 34));
    text_region.location.x = main_rect.location.x + 38;
    text_region.location.y = main_rect.location.y + 154;
    text_region.size.width = main_rect.size.width - 76;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(flipdot_palette.text, accent_color, 16), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev board armed";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next board armed";
    }
    else
    {
        status_text = "Matrix advanced";
    }

    draw_round_fill_safe(region.location.x + 24, region.location.y + 241, 192, 24, 12, flipdot_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 22, region.location.y + 239, 196, 26, 13, flipdot_palette.surface, egui_color_alpha_mix(self->alpha, 50));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 239, 196, 26, 13, 1, flipdot_palette.border, egui_color_alpha_mix(self->alpha, 32));
    draw_round_fill_safe(region.location.x + 36, region.location.y + 248, 7, 7, 3, accent_color, egui_color_alpha_mix(self->alpha, 80));
    draw_round_fill_safe(region.location.x + 54, region.location.y + 250, 10, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(region.location.x + 70, region.location.y + 250, 12, 2, 1, flipdot_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(region.location.x + 160, region.location.y + 250, 12, 2, 1, flipdot_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(region.location.x + 178, region.location.y + 250, 10, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(region.location.x + 198, region.location.y + 248, 7, 7, 3, accent_color, egui_color_alpha_mix(self->alpha, 60));
    text_region.location.x = region.location.x + 52;
    text_region.location.y = region.location.y + 245;
    text_region.size.width = 136;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(flipdot_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_flipdot_matrix_panel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_flipdot_matrix_panel_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_flipdot_matrix_panel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_flipdot_matrix_panel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_flipdot_matrix_panel_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_flipdot_matrix_panel_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->states = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->state_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
