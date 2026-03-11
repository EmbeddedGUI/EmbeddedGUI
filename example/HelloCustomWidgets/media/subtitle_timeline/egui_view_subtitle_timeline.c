#include <stdlib.h>
#include <string.h>

#include "egui_view_subtitle_timeline.h"

typedef struct subtitle_palette subtitle_palette_t;
struct subtitle_palette
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

static const subtitle_palette_t subtitle_palette = {
        EGUI_COLOR_HEX(0x131822), EGUI_COLOR_HEX(0x202938), EGUI_COLOR_HEX(0x5B6A82), EGUI_COLOR_HEX(0xF5F8FF), EGUI_COLOR_HEX(0x95A2B8),
        EGUI_COLOR_HEX(0x78D0FF), EGUI_COLOR_HEX(0xF2B772), EGUI_COLOR_HEX(0x98D5AD), EGUI_COLOR_HEX(0x05080F),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SUBTITLE_TIMELINE_MAX_CUES)
    {
        return EGUI_VIEW_SUBTITLE_TIMELINE_MAX_CUES;
    }
    return count;
}

static egui_color_t get_accent_color(const egui_view_subtitle_timeline_cue_t *cue)
{
    if (cue->emphasis >= 2)
    {
        return subtitle_palette.safe;
    }
    if (cue->emphasis == 1)
    {
        return subtitle_palette.warm;
    }
    return subtitle_palette.accent;
}

static void draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color,
                                   egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

void egui_view_subtitle_timeline_set_cues(egui_view_t *self, const egui_view_subtitle_timeline_cue_t *cues, uint8_t cue_count)
{
    EGUI_LOCAL_INIT(egui_view_subtitle_timeline_t);
    local->cues = cues;
    local->cue_count = clamp_count(cue_count);
    if (local->current_index >= local->cue_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_subtitle_timeline_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_subtitle_timeline_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);
    main_rect->location.x = region.location.x + 28;
    main_rect->location.y = region.location.y + 62;
    main_rect->size.width = 184;
    main_rect->size.height = 154;

    left_rect->location.x = region.location.x + 10;
    left_rect->location.y = region.location.y + 86;
    left_rect->size.width = 48;
    left_rect->size.height = 112;

    right_rect->location.x = region.location.x + 182;
    right_rect->location.y = region.location.y + 86;
    right_rect->size.width = 48;
    right_rect->size.height = 112;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t ox;
    egui_dim_t oy;

    ox = self->region_screen.location.x + self->padding.left;
    oy = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = ox + 28;
    main_rect->location.y = oy + 62;
    main_rect->size.width = 184;
    main_rect->size.height = 154;

    left_rect->location.x = ox + 10;
    left_rect->location.y = oy + 86;
    left_rect->size.width = 48;
    left_rect->size.height = 112;

    right_rect->location.x = ox + 182;
    right_rect->location.y = oy + 86;
    right_rect->size.width = 48;
    right_rect->size.height = 112;
}

static int egui_view_subtitle_timeline_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_subtitle_timeline_t);
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->cue_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &main_rect, &left_rect, &right_rect);
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = (local->current_index == 0) ? (local->cue_count - 1) : (uint8_t)(local->current_index - 1);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->cue_count);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&main_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->cue_count);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void draw_preview_card(egui_view_t *self, const egui_view_subtitle_timeline_cue_t *cue, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h,
                              uint8_t right_side)
{
    egui_color_t accent_color;

    accent_color = get_accent_color(cue);
    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 3, w, h, 10, subtitle_palette.shadow, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(x, y, w, h, 10, subtitle_palette.panel, egui_color_alpha_mix(self->alpha, 52));
    draw_round_stroke_safe(x, y, w, h, 10, 1, subtitle_palette.border, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(x + 11, y + 12, w - 22, 10, 5, accent_color, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(x + 9, y + 29, w - 18, h - 48, 7, subtitle_palette.surface, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(x + 9, y + 29, w - 18, h - 48, 7, 1, subtitle_palette.border, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(x + 12, y + 36, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 48));
    draw_round_fill_safe(x + 13, y + 45, w - 26, 2, 1, subtitle_palette.text, egui_color_alpha_mix(self->alpha, 22));
    draw_round_fill_safe(x + 13, y + 54, w - 22, 6, 3, subtitle_palette.panel, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(x + 16, y + 56, w - 28, 2, 1, subtitle_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + (right_side ? 12 : w - 14), y + 52, 2, h - 72, 1, accent_color, egui_color_alpha_mix(self->alpha, 30));
    draw_round_fill_safe(x + 12, y + h - 16, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 54));
}

static void draw_timeline_strip(egui_view_t *self, const egui_view_subtitle_timeline_cue_t *cue, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_dim_t gap;
    egui_dim_t seg_x;
    egui_dim_t widths[5];
    uint8_t i;
    egui_color_t accent_color;

    accent_color = get_accent_color(cue);
    gap = 5;
    widths[0] = 24;
    widths[1] = 30;
    widths[2] = 22;
    widths[3] = 28;
    widths[4] = (egui_dim_t)(w - 8 - gap * 4 - widths[0] - widths[1] - widths[2] - widths[3]);

    draw_round_fill_safe(x, y, w, h, 10, subtitle_palette.surface, egui_color_alpha_mix(self->alpha, 40));
    draw_round_stroke_safe(x, y, w, h, 10, 1, subtitle_palette.border, egui_color_alpha_mix(self->alpha, 28));

    seg_x = x + 4;
    for (i = 0; i < 5; i++)
    {
        egui_color_t seg_color;
        egui_alpha_t alpha;
        egui_dim_t local_w;

        local_w = widths[i] - ((i == cue->active_index) ? 0 : 2);
        seg_color = egui_rgb_mix(subtitle_palette.panel, subtitle_palette.surface, (i == cue->active_index) ? 4 : 22);
        alpha = egui_color_alpha_mix(self->alpha, (i == cue->active_index) ? 76 : 44);
        draw_round_fill_safe(seg_x, y + 5, local_w, h - 10, 6, seg_color, alpha);
        draw_round_stroke_safe(seg_x, y + 5, local_w, h - 10, 6, 1, subtitle_palette.border,
                               egui_color_alpha_mix(self->alpha, (i == cue->active_index) ? 42 : 22));
        if (i == cue->active_index)
        {
            draw_round_stroke_safe(seg_x - 1, y + 4, local_w + 2, h - 8, 7, 1, accent_color, egui_color_alpha_mix(self->alpha, 28));
            draw_round_fill_safe(seg_x + local_w / 2 - 3, y + 2, 6, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 76));
            draw_round_fill_safe(seg_x + local_w / 2 - 3, y + h - 9, 6, 5, 2, accent_color, egui_color_alpha_mix(self->alpha, 64));
        }
        draw_round_fill_safe(seg_x + 4, y + 11, local_w - 8, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, (i == cue->active_index) ? 82 : 26));
        draw_round_fill_safe(seg_x + 4, y + 19, local_w - 10, 2, 1, subtitle_palette.text,
                             egui_color_alpha_mix(self->alpha, (i == cue->active_index) ? 34 : 16));
        seg_x += widths[i] + gap;
        if (i < 4)
        {
            draw_round_fill_safe(seg_x - 3, y + h / 2 - 1, 2, 2, 1, subtitle_palette.border, egui_color_alpha_mix(self->alpha, 20));
        }
    }
}

static void egui_view_subtitle_timeline_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_subtitle_timeline_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_subtitle_timeline_cue_t *current;
    const egui_view_subtitle_timeline_cue_t *left_cue;
    const egui_view_subtitle_timeline_cue_t *right_cue;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const char *status_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->cues == NULL || local->cue_count == 0)
    {
        return;
    }

    current = &local->cues[local->current_index];
    left_cue = &local->cues[(local->current_index == 0) ? (local->cue_count - 1) : (local->current_index - 1)];
    right_cue = &local->cues[(local->current_index + 1) % local->cue_count];
    accent_color = get_accent_color(current);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 84, region.location.y + 18, 72, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 18));
    egui_canvas_draw_text_in_rect(title_font, "Subtitle Timeline", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 25;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(body_font, "Tap sides or center to step subtitle cues", &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(subtitle_palette.text, subtitle_palette.muted, 28), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview_card(self, left_cue, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_preview_card(self, right_cue, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, subtitle_palette.shadow,
                         egui_color_alpha_mix(self->alpha, 38));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, subtitle_palette.panel,
                         egui_color_alpha_mix(self->alpha, 78));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, subtitle_palette.border,
                           egui_color_alpha_mix(self->alpha, 72));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 16, 38, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, 74));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 23, 28, 2, 1, subtitle_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(main_rect.location.x + 12, main_rect.location.y + 54, main_rect.size.width - 24, 76, 10, subtitle_palette.surface,
                         egui_color_alpha_mix(self->alpha, 20));
    draw_round_stroke_safe(main_rect.location.x + 12, main_rect.location.y + 54, main_rect.size.width - 24, 76, 10, 1, subtitle_palette.border,
                           egui_color_alpha_mix(self->alpha, 12));

    pill_w = (egui_dim_t)(46 + strlen(current->status) * 5);
    if (pill_w < 72)
    {
        pill_w = 72;
    }
    pill_x = main_rect.location.x + main_rect.size.width - pill_w - 14;
    draw_round_fill_safe(pill_x, main_rect.location.y + 14, pill_w, 13, 6, accent_color, egui_color_alpha_mix(self->alpha, 66));
    draw_round_stroke_safe(pill_x, main_rect.location.y + 14, pill_w, 13, 6, 1, egui_rgb_mix(accent_color, subtitle_palette.text, 28),
                           egui_color_alpha_mix(self->alpha, 46));
    text_region.location.x = pill_x + 5;
    text_region.location.y = main_rect.location.y + 14;
    text_region.size.width = pill_w - 10;
    text_region.size.height = 13;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, subtitle_palette.text, self->alpha);

    text_region.location.x = main_rect.location.x + 20;
    text_region.location.y = main_rect.location.y + 38;
    text_region.size.width = main_rect.size.width - 40;
    text_region.size.height = 12;
    draw_round_fill_safe(main_rect.location.x + 18, main_rect.location.y + 43, 3, 8, 1, accent_color, egui_color_alpha_mix(self->alpha, 62));
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->speaker, &text_region, EGUI_ALIGN_LEFT, subtitle_palette.text,
                                  self->alpha);

    text_region.location.y = main_rect.location.y + 62;
    text_region.size.height = 22;
    egui_canvas_draw_text_in_rect(body_font, current->line, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(subtitle_palette.text, subtitle_palette.muted, 16),
                                  self->alpha);

    draw_timeline_strip(self, current, main_rect.location.x + 16, main_rect.location.y + 92, main_rect.size.width - 32, 32);

    text_region.location.x = main_rect.location.x + 24;
    text_region.location.y = main_rect.location.y + main_rect.size.height - 23;
    text_region.size.width = main_rect.size.width - 56;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(subtitle_palette.text, accent_color, 18),
                                  self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Prev cue ready";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Next cue ready";
    }
    else
    {
        status_text = "Cue advanced";
    }

    draw_round_fill_safe(region.location.x + 22, region.location.y + 236, 196, 26, 13, subtitle_palette.surface, egui_color_alpha_mix(self->alpha, 44));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 236, 196, 26, 13, 1, subtitle_palette.border, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(region.location.x + 37, region.location.y + 247, 5, 5, 2, accent_color, egui_color_alpha_mix(self->alpha, 80));
    text_region.location.x = region.location.x + 44;
    text_region.location.y = region.location.y + 243;
    text_region.size.width = 158;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_8_4, status_text, &text_region, EGUI_ALIGN_CENTER,
                                  egui_rgb_mix(subtitle_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_subtitle_timeline_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_subtitle_timeline_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_subtitle_timeline_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_subtitle_timeline_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_subtitle_timeline_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_subtitle_timeline_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->cues = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->cue_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
