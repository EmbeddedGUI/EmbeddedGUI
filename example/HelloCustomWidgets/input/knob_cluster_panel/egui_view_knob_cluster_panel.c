#include <stdlib.h>
#include <string.h>

#include "egui_view_knob_cluster_panel.h"

typedef struct knob_palette knob_palette_t;
struct knob_palette
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

static const knob_palette_t knob_palette = {
        EGUI_COLOR_HEX(0x121922), EGUI_COLOR_HEX(0x1E2938), EGUI_COLOR_HEX(0x596B81), EGUI_COLOR_HEX(0xF3F8FF), EGUI_COLOR_HEX(0x96A7BC),
        EGUI_COLOR_HEX(0x79CAFF), EGUI_COLOR_HEX(0xF1B46D), EGUI_COLOR_HEX(0x92D2B0), EGUI_COLOR_HEX(0x050A11),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_KNOB_CLUSTER_PANEL_MAX_STATES)
    {
        return EGUI_VIEW_KNOB_CLUSTER_PANEL_MAX_STATES;
    }
    return count;
}

static egui_color_t get_accent_color(const egui_view_knob_cluster_panel_state_t *state)
{
    if (state->accent_mode >= 2)
    {
        return knob_palette.safe;
    }
    if (state->accent_mode == 1)
    {
        return knob_palette.warm;
    }
    return knob_palette.accent;
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

void egui_view_knob_cluster_panel_set_states(egui_view_t *self, const egui_view_knob_cluster_panel_state_t *states, uint8_t state_count)
{
    EGUI_LOCAL_INIT(egui_view_knob_cluster_panel_t);
    local->states = states;
    local->state_count = clamp_count(state_count);
    if (local->current_index >= local->state_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_knob_cluster_panel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_knob_cluster_panel_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 30;
    main_rect->location.y = region.location.y + 56;
    main_rect->size.width = 180;
    main_rect->size.height = 162;

    left_rect->location.x = region.location.x + 2;
    left_rect->location.y = region.location.y + 88;
    left_rect->size.width = 42;
    left_rect->size.height = 104;

    right_rect->location.x = region.location.x + 196;
    right_rect->location.y = region.location.y + 88;
    right_rect->size.width = 42;
    right_rect->size.height = 104;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox = self->region_screen.location.x + self->padding.left;
    egui_dim_t oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 30;
    main_rect->location.y = oy + 56;
    main_rect->size.width = 180;
    main_rect->size.height = 162;

    left_rect->location.x = ox + 2;
    left_rect->location.y = oy + 88;
    left_rect->size.width = 42;
    left_rect->size.height = 104;

    right_rect->location.x = ox + 196;
    right_rect->location.y = oy + 88;
    right_rect->size.width = 42;
    right_rect->size.height = 104;
}

static int egui_view_knob_cluster_panel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_knob_cluster_panel_t);
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

static void draw_side_preview(egui_view_t *self, const egui_view_knob_cluster_panel_state_t *state, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h,
                              uint8_t right_side)
{
    egui_color_t accent_color = get_accent_color(state);
    egui_dim_t edge_x = right_side ? (x + w - 14) : (x + 12);
    egui_dim_t slot_x = right_side ? (x + w - 30) : (x + 12);
    egui_dim_t strip_x = right_side ? (x + w - 11) : (x + 9);

    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 3, w, h, 9, knob_palette.shadow, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(x, y, w, h, 9, knob_palette.panel, egui_color_alpha_mix(self->alpha, 46));
    draw_round_stroke_safe(x, y, w, h, 9, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 32));
    draw_round_fill_safe(strip_x, y + 10, 4, h - 24, 2, accent_color, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(x + 10, y + 11, w - 20, 7, 4, accent_color, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(x + 9, y + 28, w - 18, 34, 14, knob_palette.surface, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(x + 9, y + 28, w - 18, 34, 14, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + 12, y + 39, w - 24, 10, 5, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(slot_x, y + 40, 10, 8, 4, accent_color, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(edge_x, y + 35, 2, 22, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(edge_x - 2, y + 67, 6, 6, 3, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x + 10, y + 71, w - 20, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, 26));
    draw_round_fill_safe(x + 10, y + h - 22, w - 20, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(x + 12, y + h - 16, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 44));
}

static void draw_knob(egui_view_t *self, egui_dim_t cx, egui_dim_t cy, egui_dim_t size, egui_color_t accent_color, uint8_t focused)
{
    egui_dim_t ring_size = focused ? size + 8 : size + 4;
    if (focused)
    {
        draw_round_fill_safe(cx - ring_size / 2, cy - ring_size / 2, ring_size, ring_size, ring_size / 2, accent_color, egui_color_alpha_mix(self->alpha, 18));
    }
    draw_round_fill_safe(cx - size / 2, cy - size / 2, size, size, size / 2, knob_palette.panel, egui_color_alpha_mix(self->alpha, 64));
    draw_round_stroke_safe(cx - size / 2, cy - size / 2, size, size, size / 2, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(cx - size / 2 + 5, cy - size / 2 + 5, size - 10, size - 10, (size - 10) / 2, 1, egui_rgb_mix(knob_palette.border, accent_color, 22),
                           egui_color_alpha_mix(self->alpha, focused ? 30 : 16));
    draw_round_fill_safe(cx - 6, cy - (focused ? 16 : 13), 12, 6, 3, accent_color, egui_color_alpha_mix(self->alpha, focused ? 78 : 42));
    draw_round_fill_safe(cx - 4, cy - 4, 8, 8, 4, knob_palette.surface, egui_color_alpha_mix(self->alpha, 54));
}

static void egui_view_knob_cluster_panel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_knob_cluster_panel_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_knob_cluster_panel_state_t *current;
    const egui_view_knob_cluster_panel_state_t *left_state;
    const egui_view_knob_cluster_panel_state_t *right_state;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t left_knob_size;
    egui_dim_t center_knob_size;
    egui_dim_t right_knob_size;
    const char *status_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->states == NULL || local->state_count == 0)
    {
        return;
    }

    current = &local->states[local->current_index];
    left_state = &local->states[(local->current_index == 0) ? (local->state_count - 1) : (local->current_index - 1)];
    right_state = &local->states[(local->current_index + 1) % local->state_count];
    accent_color = get_accent_color(current);
    left_knob_size = (current->focus_index == 0) ? 40 : 32;
    center_knob_size = (current->focus_index == 1) ? 48 : 42;
    right_knob_size = (current->focus_index == 2) ? 40 : 32;
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 66, region.location.y + 19, 108, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 14));
    egui_canvas_draw_text_in_rect(title_font, "Knob Cluster Panel", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 27;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, "Tap edges or panel to step focus", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(knob_palette.text, knob_palette.muted, 34), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_side_preview(self, left_state, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_side_preview(self, right_state, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 16, knob_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 36));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 16, knob_palette.panel,
                         egui_color_alpha_mix(self->alpha, 70));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 16, 1, knob_palette.border,
                           egui_color_alpha_mix(self->alpha, 38));

    pill_w = (egui_dim_t)(50 + strlen(current->status) * 5);
    if (pill_w < 84)
    {
        pill_w = 84;
    }
    pill_x = main_rect.location.x + (main_rect.size.width - pill_w) / 2;
    draw_round_fill_safe(pill_x, main_rect.location.y + 10, pill_w, 20, 10, accent_color, egui_color_alpha_mix(self->alpha, 64));
    draw_round_stroke_safe(pill_x, main_rect.location.y + 10, pill_w, 20, 10, 1, egui_rgb_mix(accent_color, knob_palette.text, 28),
                           egui_color_alpha_mix(self->alpha, 38));
    text_region.location.x = pill_x + 9;
    text_region.location.y = main_rect.location.y + 12;
    text_region.size.width = pill_w - 18;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, knob_palette.text, self->alpha);

    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 18, pill_x - main_rect.location.x - 26, 2, 1, accent_color,
                         egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(pill_x + pill_w + 8, main_rect.location.y + 18, main_rect.location.x + main_rect.size.width - pill_x - pill_w - 26, 2, 1, accent_color,
                         egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(main_rect.location.x + 28, main_rect.location.y + 24, pill_x - main_rect.location.x - 38, 2, 1, knob_palette.text,
                         egui_color_alpha_mix(self->alpha, 12));
    draw_round_fill_safe(pill_x + pill_w + 18, main_rect.location.y + 24, main_rect.location.x + main_rect.size.width - pill_x - pill_w - 38, 2, 1,
                         knob_palette.text, egui_color_alpha_mix(self->alpha, 12));

    draw_round_fill_safe(main_rect.location.x + 22, main_rect.location.y + 44, 136, 58, 26, knob_palette.surface, egui_color_alpha_mix(self->alpha, 36));
    draw_round_stroke_safe(main_rect.location.x + 22, main_rect.location.y + 44, 136, 58, 26, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(main_rect.location.x + 52, main_rect.location.y + 74, 76, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(main_rect.location.x + 40, main_rect.location.y + 64, 28, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(main_rect.location.x + 112, main_rect.location.y + 64, 28, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(main_rect.location.x + 51, main_rect.location.y + 70, 2, 10, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));
    draw_round_fill_safe(main_rect.location.x + 127, main_rect.location.y + 70, 2, 10, 1, accent_color, egui_color_alpha_mix(self->alpha, 16));

    draw_knob(self, main_rect.location.x + 48, main_rect.location.y + 76, left_knob_size, accent_color, current->focus_index == 0);
    draw_knob(self, main_rect.location.x + 90, main_rect.location.y + 64, center_knob_size, accent_color, current->focus_index == 1);
    draw_knob(self, main_rect.location.x + 132, main_rect.location.y + 76, right_knob_size, accent_color, current->focus_index == 2);
    draw_round_fill_safe(main_rect.location.x + 30, main_rect.location.y + 88, 20, 4, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, current->focus_index == 0 ? 32 : 14));
    draw_round_fill_safe(main_rect.location.x + 77, main_rect.location.y + 88, 26, 4, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, current->focus_index == 1 ? 32 : 14));
    draw_round_fill_safe(main_rect.location.x + 130, main_rect.location.y + 88, 20, 4, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, current->focus_index == 2 ? 32 : 14));
    draw_round_fill_safe(main_rect.location.x + 30, main_rect.location.y + 96, 18, 2, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(main_rect.location.x + 81, main_rect.location.y + 96, 18, 2, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 14));
    draw_round_fill_safe(main_rect.location.x + 132, main_rect.location.y + 96, 18, 2, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 14));

    text_region.location.x = main_rect.location.x + 22;
    text_region.location.y = main_rect.location.y + 102;
    text_region.size.width = main_rect.size.width - 44;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->preset, &text_region, EGUI_ALIGN_CENTER, knob_palette.text,
                                  self->alpha);

    text_region.location.y = main_rect.location.y + 116;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(knob_palette.text, knob_palette.muted, 18),
                                  self->alpha);

    draw_round_fill_safe(main_rect.location.x + 40, main_rect.location.y + main_rect.size.height - 30, main_rect.size.width - 80, 20, 10, knob_palette.surface,
                         egui_color_alpha_mix(self->alpha, 18));
    draw_round_stroke_safe(main_rect.location.x + 40, main_rect.location.y + main_rect.size.height - 30, main_rect.size.width - 80, 20, 10, 1,
                           knob_palette.border, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(main_rect.location.x + 59, main_rect.location.y + main_rect.size.height - 22, 5, 5, 2, accent_color,
                         egui_color_alpha_mix(self->alpha, 52));
    text_region.location.x = main_rect.location.x + 36;
    text_region.location.y = main_rect.location.y + main_rect.size.height - 26;
    text_region.size.width = main_rect.size.width - 72;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(knob_palette.text, accent_color, 18), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev knob armed";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next knob armed";
    }
    else
    {
        status_text = "Cluster advanced";
    }

    draw_round_fill_safe(region.location.x + 25, region.location.y + 239, 190, 24, 12, knob_palette.shadow, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(region.location.x + 22, region.location.y + 236, 196, 28, 14, knob_palette.surface, egui_color_alpha_mix(self->alpha, 50));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 236, 196, 28, 14, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(region.location.x + 36, region.location.y + 247, 7, 7, 3, accent_color, egui_color_alpha_mix(self->alpha, 84));
    draw_round_fill_safe(region.location.x + 54, region.location.y + 249, 10, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(region.location.x + 70, region.location.y + 249, 12, 2, 1, knob_palette.border, egui_color_alpha_mix(self->alpha, 14));
    text_region.location.x = region.location.x + 63;
    text_region.location.y = region.location.y + 244;
    text_region.size.width = 140;
    text_region.size.height = 14;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(knob_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_knob_cluster_panel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_knob_cluster_panel_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_knob_cluster_panel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_knob_cluster_panel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_knob_cluster_panel_t);
    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_knob_cluster_panel_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->states = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->state_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
