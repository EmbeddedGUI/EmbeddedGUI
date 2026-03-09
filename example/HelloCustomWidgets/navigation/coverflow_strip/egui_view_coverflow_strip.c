#include <stdlib.h>
#include <string.h>

#include "egui_view_coverflow_strip.h"

typedef struct coverflow_palette coverflow_palette_t;
struct coverflow_palette
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

static const coverflow_palette_t coverflow_palette = {
        EGUI_COLOR_HEX(0x121826), EGUI_COLOR_HEX(0x1F2940), EGUI_COLOR_HEX(0x51627F), EGUI_COLOR_HEX(0xF4F7FF), EGUI_COLOR_HEX(0x9AA9C4),
        EGUI_COLOR_HEX(0x73C7FF), EGUI_COLOR_HEX(0xF2B76D), EGUI_COLOR_HEX(0x95D4AF), EGUI_COLOR_HEX(0x060A12),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_COVERFLOW_STRIP_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_COVERFLOW_STRIP_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_coverflow_strip_set_snapshots(
        egui_view_t *self,
        const egui_view_coverflow_strip_snapshot_t *snapshots,
        uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_coverflow_strip_t);
    local->snapshots = snapshots;
    local->snapshot_count = clamp_count(snapshot_count);
    if (local->current_index >= local->snapshot_count)
    {
        local->current_index = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_coverflow_strip_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_coverflow_strip_t);
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

static egui_color_t get_accent_color(const egui_view_coverflow_strip_snapshot_t *snapshot)
{
    if (snapshot->accent_mode >= 2)
    {
        return coverflow_palette.safe;
    }
    if (snapshot->accent_mode == 1)
    {
        return coverflow_palette.warn;
    }
    return coverflow_palette.accent;
}

static uint8_t get_left_index(const egui_view_coverflow_strip_t *local)
{
    if (local->snapshot_count == 0)
    {
        return 0;
    }
    if (local->current_index == 0)
    {
        return local->snapshot_count - 1;
    }
    return local->current_index - 1;
}

static uint8_t get_right_index(const egui_view_coverflow_strip_t *local)
{
    if (local->snapshot_count == 0)
    {
        return 0;
    }
    return (uint8_t)((local->current_index + 1) % local->snapshot_count);
}

static void draw_side_card(
        egui_view_t *self,
        const egui_view_coverflow_strip_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t right_side)
{
    egui_region_t text_region;
    egui_color_t accent_color;
    egui_color_t shell_color;
    const egui_font_t *mini_font;
    accent_color = get_accent_color(snapshot);
    shell_color = egui_rgb_mix(coverflow_palette.panel, coverflow_palette.surface, right_side ? 18 : 24);
    mini_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;

    draw_round_fill_safe(x + (right_side ? 0 : 2), y + 4, w, h, 10, coverflow_palette.shadow, egui_color_alpha_mix(self->alpha, 42));
    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, 56));
    draw_round_stroke_safe(x, y, w, h, 10, 1, coverflow_palette.border, egui_color_alpha_mix(self->alpha, 54));
    draw_round_fill_safe(x + 11, y + 13, w - 22, 10, 5, accent_color, egui_color_alpha_mix(self->alpha, 62));
    draw_round_stroke_safe(x + 11, y + 13, w - 22, 10, 5, 1, egui_rgb_mix(accent_color, coverflow_palette.text, 28), egui_color_alpha_mix(self->alpha, 52));

    text_region.location.x = x + 14;
    text_region.location.y = y + 13;
    text_region.size.width = w - 28;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(mini_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, coverflow_palette.text, self->alpha);

    draw_round_fill_safe(x + 16, y + 48, w - 32, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 62));

    draw_round_fill_safe(x + 14, y + h - 18, w - 28, 8, 4, coverflow_palette.surface, egui_color_alpha_mix(self->alpha, 44));
    draw_round_fill_safe(x + 20, y + h - 15, w - 40, 2, 1, accent_color, egui_color_alpha_mix(self->alpha, 72));
}

static void draw_center_card(egui_view_t *self, const egui_view_coverflow_strip_snapshot_t *snapshot, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h)
{
    egui_region_t text_region;
    egui_color_t accent_color;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    const egui_font_t *title_font;
    const egui_font_t *body_font;

    accent_color = get_accent_color(snapshot);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_12_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    pill_w = (egui_dim_t)(46 + strlen(snapshot->status) * 4);
    if (pill_w < 62)
    {
        pill_w = 62;
    }
    if (pill_w > 74)
    {
        pill_w = 74;
    }
    pill_x = x + w - pill_w - 14;

    draw_round_fill_safe(x + 2, y + 5, w, h, 14, coverflow_palette.shadow, egui_color_alpha_mix(self->alpha, 50));
    draw_round_fill_safe(x, y, w, h, 14, coverflow_palette.panel, egui_color_alpha_mix(self->alpha, 78));
    draw_round_stroke_safe(x, y, w, h, 14, 1, coverflow_palette.border, egui_color_alpha_mix(self->alpha, 76));
    draw_round_fill_safe(x + 16, y + 16, 42, 3, 1, accent_color, egui_color_alpha_mix(self->alpha, 78));

    draw_round_fill_safe(pill_x, y + 14, pill_w, 12, 6, accent_color, egui_color_alpha_mix(self->alpha, 68));
    draw_round_stroke_safe(
            pill_x,
            y + 14,
            pill_w,
            12,
            6,
            1,
            egui_rgb_mix(accent_color, coverflow_palette.text, 28),
            egui_color_alpha_mix(self->alpha, 56));
    text_region.location.x = pill_x + 4;
    text_region.location.y = y + 14;
    text_region.size.width = pill_w - 8;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, coverflow_palette.text, self->alpha);

    text_region.location.x = x + 18;
    text_region.location.y = y + 36;
    text_region.size.width = w - 36;
    text_region.size.height = 16;
    egui_canvas_draw_text_in_rect(title_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, coverflow_palette.text, self->alpha);

    text_region.location.y = y + 57;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, snapshot->summary, &text_region, EGUI_ALIGN_LEFT, egui_rgb_mix(coverflow_palette.text, coverflow_palette.muted, 24), self->alpha);

    draw_round_fill_safe(x + 16, y + 82, w - 32, 30, 9, coverflow_palette.surface, egui_color_alpha_mix(self->alpha, 42));
    draw_round_stroke_safe(x + 16, y + 82, w - 32, 30, 9, 1, coverflow_palette.border, egui_color_alpha_mix(self->alpha, 36));
    draw_round_fill_safe(x + 26, y + 95, w - 52, 4, 2, accent_color, egui_color_alpha_mix(self->alpha, 78));
    draw_round_fill_safe(x + 28, y + 104, w - 76, 3, 1, coverflow_palette.text, egui_color_alpha_mix(self->alpha, 56));

    text_region.location.y = y + h - 24;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(coverflow_palette.text, accent_color, 28), self->alpha);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *left_rect, egui_region_t *center_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);

    left_rect->location.x = region.location.x + 10;
    left_rect->location.y = region.location.y + 84;
    left_rect->size.width = 60;
    left_rect->size.height = 114;

    center_rect->location.x = region.location.x + 50;
    center_rect->location.y = region.location.y + 62;
    center_rect->size.width = 140;
    center_rect->size.height = 154;

    right_rect->location.x = region.location.x + 170;
    right_rect->location.y = region.location.y + 84;
    right_rect->size.width = 60;
    right_rect->size.height = 114;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *left_rect, egui_region_t *center_rect, egui_region_t *right_rect)
{
    egui_dim_t origin_x;
    egui_dim_t origin_y;

    origin_x = self->region_screen.location.x + self->padding.left;
    origin_y = self->region_screen.location.y + self->padding.top;

    left_rect->location.x = origin_x + 10;
    left_rect->location.y = origin_y + 84;
    left_rect->size.width = 60;
    left_rect->size.height = 114;

    center_rect->location.x = origin_x + 50;
    center_rect->location.y = origin_y + 62;
    center_rect->size.width = 140;
    center_rect->size.height = 154;

    right_rect->location.x = origin_x + 170;
    right_rect->location.y = origin_y + 84;
    right_rect->size.width = 60;
    right_rect->size.height = 114;
}

static int egui_view_coverflow_strip_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_coverflow_strip_t);
    egui_region_t left_rect;
    egui_region_t center_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP || local->snapshot_count == 0)
    {
        return 1;
    }

    get_zone_rects_screen(self, &left_rect, &center_rect, &right_rect);

    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y))
    {
        local->current_index = get_left_index(local);
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y))
    {
        local->current_index = get_right_index(local);
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&center_rect, event->location.x, event->location.y))
    {
        local->current_index = get_right_index(local);
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void egui_view_coverflow_strip_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_coverflow_strip_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t left_rect;
    egui_region_t center_rect;
    egui_region_t right_rect;
    const egui_view_coverflow_strip_snapshot_t *current;
    const egui_view_coverflow_strip_snapshot_t *left_snapshot;
    const egui_view_coverflow_strip_snapshot_t *right_snapshot;
    const egui_font_t *title_font;
    const egui_font_t *guide_font;
    const egui_font_t *body_font;
    egui_color_t accent_color;
    const char *status_text;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    current = &local->snapshots[local->current_index];
    left_snapshot = &local->snapshots[get_left_index(local)];
    right_snapshot = &local->snapshots[get_right_index(local)];
    accent_color = get_accent_color(current);
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    guide_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    body_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 1;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    egui_canvas_draw_text_in_rect(title_font, "Coverflow Strip", &text_region, EGUI_ALIGN_CENTER, accent_color, self->alpha);

    text_region.location.y = region.location.y + 24;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap side cards to rotate scenes", &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(coverflow_palette.text, coverflow_palette.muted, 32), self->alpha);

    get_zone_rects_local(self, &left_rect, &center_rect, &right_rect);
    draw_side_card(self, left_snapshot, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 0);
    draw_side_card(self, right_snapshot, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1);
    draw_center_card(self, current, center_rect.location.x, center_rect.location.y, center_rect.size.width, center_rect.size.height);

    for (i = 0; i < local->snapshot_count && i < 5; i++)
    {
        egui_dim_t dot_x;
        egui_color_t dot_color;
        egui_alpha_t dot_alpha;

        dot_x = region.location.x + 86 + i * 16;
        dot_color = (i == local->current_index) ? accent_color : coverflow_palette.border;
        dot_alpha = egui_color_alpha_mix(self->alpha, (i == local->current_index) ? 82 : 54);
        draw_round_fill_safe(dot_x, region.location.y + 225, (i == local->current_index) ? 12 : 6, 6, 3, dot_color, dot_alpha);
    }

    if (local->last_zone == 0)
    {
        status_text = "Left card active";
    }
    else if (local->last_zone == 2)
    {
        status_text = "Right card active";
    }
    else
    {
        status_text = "Center card next";
    }

    draw_round_fill_safe(region.location.x + 28, region.location.y + 240, 184, 26, 13, coverflow_palette.surface, egui_color_alpha_mix(self->alpha, 44));
    draw_round_stroke_safe(region.location.x + 28, region.location.y + 240, 184, 26, 13, 1, coverflow_palette.border, egui_color_alpha_mix(self->alpha, 36));
    text_region.location.x = region.location.x + 40;
    text_region.location.y = region.location.y + 247;
    text_region.size.width = 164;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(body_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(coverflow_palette.text, accent_color, 22), self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_coverflow_strip_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_coverflow_strip_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_coverflow_strip_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_coverflow_strip_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_coverflow_strip_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_coverflow_strip_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->snapshot_count = 0;
    local->current_index = 0;
    local->last_zone = 1;
}
