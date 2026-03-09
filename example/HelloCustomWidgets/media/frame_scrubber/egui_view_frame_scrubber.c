#include <stdlib.h>
#include <string.h>

#include "egui_view_frame_scrubber.h"

typedef struct scrubber_palette scrubber_palette_t;
struct scrubber_palette
{
    egui_color_t surface;
    egui_color_t panel;
    egui_color_t border;
    egui_color_t text;
    egui_color_t muted;
    egui_color_t accent;
    egui_color_t warn;
    egui_color_t safe;
    egui_color_t shadow;
};

static const scrubber_palette_t scrubber_palette = {
        EGUI_COLOR_HEX(0x141A24), EGUI_COLOR_HEX(0x1F2B3B), EGUI_COLOR_HEX(0x56667F), EGUI_COLOR_HEX(0xF3F8FF), EGUI_COLOR_HEX(0x9BA9BF),
        EGUI_COLOR_HEX(0x6CCBFF), EGUI_COLOR_HEX(0xF0B86B), EGUI_COLOR_HEX(0x96D6B0), EGUI_COLOR_HEX(0x060910),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_FRAME_SCRUBBER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_FRAME_SCRUBBER_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_frame_scrubber_set_snapshots(
        egui_view_t *self,
        const egui_view_frame_scrubber_snapshot_t *snapshots,
        uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_frame_scrubber_t);
    local->snapshots = snapshots;
    local->snapshot_count = clamp_count(snapshot_count);
    if (local->current_index >= local->snapshot_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_frame_scrubber_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_frame_scrubber_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static void draw_round_fill_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
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
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static egui_color_t get_accent_color(const egui_view_frame_scrubber_snapshot_t *snapshot)
{
    if (snapshot->accent_mode >= 2)
    {
        return scrubber_palette.safe;
    }
    if (snapshot->accent_mode == 1)
    {
        return scrubber_palette.warn;
    }
    return scrubber_palette.accent;
}

static void draw_thumbnail_strip(
        egui_view_t *self,
        const egui_view_frame_scrubber_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h)
{
    egui_dim_t thumb_w;
    egui_dim_t gap;
    egui_dim_t thumb_x;
    uint8_t i;
    egui_color_t accent_color;

    accent_color = get_accent_color(snapshot);
    gap = 5;
    thumb_w = (w - gap * 5) / 6;

    draw_round_fill_safe(x, y, w, h, 10, scrubber_palette.surface, egui_color_alpha_mix(self->alpha, 38));
    draw_round_stroke_safe(x, y, w, h, 10, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 36));

    for (i = 0; i < 6; i++)
    {
        egui_dim_t local_x;
        egui_color_t frame_color;
        egui_alpha_t alpha;
        egui_dim_t inner_w;

        local_x = x + 4 + i * (thumb_w + gap);
        frame_color = egui_rgb_mix(scrubber_palette.panel, scrubber_palette.surface, (i == snapshot->playhead_index) ? 4 : 24);
        alpha = egui_color_alpha_mix(self->alpha, (i == snapshot->playhead_index) ? 76 : 46);
        draw_round_fill_safe(local_x, y + 4, thumb_w, h - 8, 6, frame_color, alpha);
        draw_round_stroke_safe(local_x, y + 4, thumb_w, h - 8, 6, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, (i == snapshot->playhead_index) ? 46 : 26));
        if (i == snapshot->playhead_index)
        {
            draw_round_stroke_safe(local_x - 1, y + 3, thumb_w + 2, h - 6, 7, 1, accent_color, egui_color_alpha_mix(self->alpha, 34));
        }
        inner_w = thumb_w - ((i == snapshot->playhead_index) ? 8 : 10);
        draw_round_fill_safe(local_x + 4, y + 10, inner_w, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, (i == snapshot->playhead_index) ? 84 : 30));
        draw_round_fill_safe(local_x + 4, y + 18, thumb_w - 12, 2, 1, scrubber_palette.text, egui_color_alpha_mix(self->alpha, (i == snapshot->playhead_index) ? 40 : 22));
        draw_round_fill_safe(local_x + 6, y + 24, thumb_w - 16, 2, 1, scrubber_palette.muted, egui_color_alpha_mix(self->alpha, (i == snapshot->marker_index) ? 38 : 18));
    }

    thumb_x = x + 4 + snapshot->playhead_index * (thumb_w + gap);
    draw_round_fill_safe(thumb_x + thumb_w / 2 - 1, y + 4, 2, h - 8, 1, accent_color, egui_color_alpha_mix(self->alpha, 88));
    draw_round_fill_safe(thumb_x + thumb_w / 2 - 4, y + 1, 8, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 84));
    draw_round_fill_safe(thumb_x + thumb_w / 2 - 3, y + h - 10, 6, 6, 3, accent_color, egui_color_alpha_mix(self->alpha, 72));
    draw_round_fill_safe(x + 6 + snapshot->marker_index * (thumb_w + gap), y + h - 9, thumb_w - 4, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 68));
}

static void draw_preview_card(
        egui_view_t *self,
        const egui_view_frame_scrubber_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t right_side)
{
    egui_color_t accent_color;

    accent_color = get_accent_color(snapshot);
    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 3, w, h, 10, scrubber_palette.shadow, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(x, y, w, h, 10, scrubber_palette.panel, egui_color_alpha_mix(self->alpha, 50));
    draw_round_stroke_safe(x, y, w, h, 10, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 52));
    draw_round_fill_safe(x + 10, y + 12, w - 20, 10, 5, accent_color, egui_color_alpha_mix(self->alpha, 56));
    draw_round_fill_safe(x + 9, y + 29, w - 18, h - 48, 7, scrubber_palette.surface, egui_color_alpha_mix(self->alpha, 28));
    draw_round_stroke_safe(x + 9, y + 29, w - 18, h - 48, 7, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(x + 12, y + 32, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 56));
    draw_round_fill_safe(x + 14, y + 40, w - 28, 2, 1, scrubber_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(x + (right_side ? 12 : w - 14), y + 48, 2, h - 66, 1, accent_color, egui_color_alpha_mix(self->alpha, 34));
    draw_round_fill_safe(x + 12, y + h - 16, w - 24, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 56));
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;
    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 28;
    main_rect->location.y = region.location.y + 61;
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
    main_rect->location.y = oy + 61;
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

static int egui_view_frame_scrubber_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_frame_scrubber_t);
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->snapshot_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &main_rect, &left_rect, &right_rect);
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = (local->current_index == 0) ? (local->snapshot_count - 1) : (uint8_t)(local->current_index - 1);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->snapshot_count);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&main_rect, event->location.x, event->location.y))
    {
        local->current_index = (uint8_t)((local->current_index + 1) % local->snapshot_count);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void egui_view_frame_scrubber_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_frame_scrubber_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_frame_scrubber_snapshot_t *current;
    const egui_view_frame_scrubber_snapshot_t *left_snapshot;
    const egui_view_frame_scrubber_snapshot_t *right_snapshot;
    const egui_font_t *title_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const char *status_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    current = &local->snapshots[local->current_index];
    left_snapshot = &local->snapshots[(local->current_index == 0) ? (local->snapshot_count - 1) : (local->current_index - 1)];
    right_snapshot = &local->snapshots[(local->current_index + 1) % local->snapshot_count];
    accent_color = get_accent_color(current);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    draw_round_fill_safe(region.location.x + 80, region.location.y + 18, 80, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 24));
    egui_canvas_draw_text_in_rect(title_font, "Frame Scrubber", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 25;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(body_font, "Tap sides or center to scrub frames", &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(scrubber_palette.text, scrubber_palette.muted, 24), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_preview_card(self, left_snapshot, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_preview_card(self, right_snapshot, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);

    draw_round_fill_safe(main_rect.location.x + 2, main_rect.location.y + 4, main_rect.size.width, main_rect.size.height, 14, scrubber_palette.shadow, egui_color_alpha_mix(self->alpha, 42));
    draw_round_fill_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, scrubber_palette.panel, egui_color_alpha_mix(self->alpha, 78));
    draw_round_stroke_safe(main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 14, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 74));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 16, 36, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, 78));
    draw_round_fill_safe(main_rect.location.x + 16, main_rect.location.y + 23, 26, 2, 1, scrubber_palette.text, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(main_rect.location.x + 12, main_rect.location.y + 52, main_rect.size.width - 24, 78, 10, scrubber_palette.surface, egui_color_alpha_mix(self->alpha, 22));
    draw_round_stroke_safe(main_rect.location.x + 12, main_rect.location.y + 52, main_rect.size.width - 24, 78, 10, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 14));

    pill_w = (egui_dim_t)(44 + strlen(current->status) * 5);
    if (pill_w < 68)
    {
        pill_w = 68;
    }
    pill_x = main_rect.location.x + main_rect.size.width - pill_w - 14;
    draw_round_fill_safe(pill_x, main_rect.location.y + 14, pill_w, 13, 6, accent_color, egui_color_alpha_mix(self->alpha, 68));
    draw_round_stroke_safe(pill_x, main_rect.location.y + 14, pill_w, 13, 6, 1, egui_rgb_mix(accent_color, scrubber_palette.text, 28), egui_color_alpha_mix(self->alpha, 52));
    text_region.location.x = pill_x + 5;
    text_region.location.y = main_rect.location.y + 14;
    text_region.size.width = pill_w - 10;
    text_region.size.height = 13;
    egui_canvas_draw_text_in_rect(body_font, current->status, &text_region, EGUI_ALIGN_CENTER, scrubber_palette.text, self->alpha);

    text_region.location.x = main_rect.location.x + 20;
    text_region.location.y = main_rect.location.y + 38;
    text_region.size.width = main_rect.size.width - 40;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_12_4, current->title, &text_region, EGUI_ALIGN_LEFT, scrubber_palette.text, self->alpha);

    text_region.location.y = main_rect.location.y + 60;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->summary, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(scrubber_palette.text, scrubber_palette.muted, 24), self->alpha);

    draw_thumbnail_strip(self, current, main_rect.location.x + 16, main_rect.location.y + 84, main_rect.size.width - 32, 34);

    text_region.location.x = main_rect.location.x + 24;
    text_region.location.y = main_rect.location.y + main_rect.size.height - 22;
    text_region.size.width = main_rect.size.width - 48;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, current->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(scrubber_palette.text, accent_color, 20), self->alpha);

    if (local->last_zone == 0)
    {
        status_text = "Left preview ready";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Right preview ready";
    }
    else
    {
        status_text = "Playhead advanced";
    }

    draw_round_fill_safe(region.location.x + 22, region.location.y + 236, 196, 26, 13, scrubber_palette.surface, egui_color_alpha_mix(self->alpha, 44));
    draw_round_stroke_safe(region.location.x + 22, region.location.y + 236, 196, 26, 13, 1, scrubber_palette.border, egui_color_alpha_mix(self->alpha, 32));
    draw_round_fill_safe(region.location.x + 36, region.location.y + 246, 6, 6, 3, accent_color, egui_color_alpha_mix(self->alpha, 82));
    text_region.location.x = region.location.x + 46;
    text_region.location.y = region.location.y + 243;
    text_region.size.width = 156;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect((const egui_font_t *)&egui_res_font_montserrat_8_4, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(scrubber_palette.text, accent_color, 20), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_frame_scrubber_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_frame_scrubber_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_frame_scrubber_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_frame_scrubber_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_frame_scrubber_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_frame_scrubber_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->snapshot_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
