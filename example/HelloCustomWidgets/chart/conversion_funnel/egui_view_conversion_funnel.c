#include <stdlib.h>
#include <string.h>

#include "egui_view_conversion_funnel.h"

typedef struct funnel_palette funnel_palette_t;
struct funnel_palette
{
    egui_color_t surface;
    egui_color_t panel;
    egui_color_t border;
    egui_color_t text;
    egui_color_t muted;
    egui_color_t accent;
    egui_color_t warn;
    egui_color_t lock;
    egui_color_t focus;
};

typedef struct funnel_profile funnel_profile_t;
struct funnel_profile
{
    uint8_t widths[4];
};

static const funnel_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x151A26), EGUI_COLOR_HEX(0x1D2637), EGUI_COLOR_HEX(0x445879), EGUI_COLOR_HEX(0xF2F7FF), EGUI_COLOR_HEX(0x9BA8BE),
        EGUI_COLOR_HEX(0x64CBFF), EGUI_COLOR_HEX(0xEDB15E), EGUI_COLOR_HEX(0x9DD39C), EGUI_COLOR_HEX(0xA6E4FF),
};

static const funnel_palette_t compact_palette = {
        EGUI_COLOR_HEX(0x16221C), EGUI_COLOR_HEX(0x22362C), EGUI_COLOR_HEX(0x5D8F79), EGUI_COLOR_HEX(0xEEFFF6), EGUI_COLOR_HEX(0xA3C6B2),
        EGUI_COLOR_HEX(0x76E5BD), EGUI_COLOR_HEX(0xE6BC67), EGUI_COLOR_HEX(0xA4DCBF), EGUI_COLOR_HEX(0xB6F6D8),
};

static const funnel_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x261C17), EGUI_COLOR_HEX(0x3C2C24), EGUI_COLOR_HEX(0x9E7B66), EGUI_COLOR_HEX(0xFFF4EB), EGUI_COLOR_HEX(0xDDB9A5),
        EGUI_COLOR_HEX(0xF4BF8B), EGUI_COLOR_HEX(0xF1C06A), EGUI_COLOR_HEX(0xDDCB97), EGUI_COLOR_HEX(0xFFE2C0),
};

static const funnel_profile_t funnel_profiles[] = {
        {{92, 74, 52, 34}},
        {{88, 66, 46, 26}},
        {{86, 70, 50, 30}},
        {{90, 62, 40, 22}},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_CONVERSION_FUNNEL_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_CONVERSION_FUNNEL_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_conversion_funnel_set_primary_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_conversion_funnel_set_compact_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
    local->compact_snapshots = snapshots;
    local->compact_snapshot_count = clamp_count(snapshot_count);
    if (local->current_compact >= local->compact_snapshot_count)
    {
        local->current_compact = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_conversion_funnel_set_locked_snapshots(egui_view_t *self, const egui_view_conversion_funnel_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_conversion_funnel_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
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

static void draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color,
                                   egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static egui_color_t get_status_color(const funnel_palette_t *palette, const egui_view_conversion_funnel_snapshot_t *snapshot)
{
    if (snapshot->accent_mode >= 2)
    {
        return palette->lock;
    }
    if (snapshot->accent_mode == 1)
    {
        return palette->warn;
    }
    return palette->accent;
}

static egui_dim_t get_pill_width(const egui_view_conversion_funnel_snapshot_t *snapshot, uint8_t compact)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(18 + len * 6) : (egui_dim_t)(22 + len * 6);
    if (compact)
    {
        if (width < 46)
        {
            width = 46;
        }
        if (width > 60)
        {
            width = 60;
        }
    }
    else
    {
        if (width < 54)
        {
            width = 54;
        }
        if (width > 74)
        {
            width = 74;
        }
    }
    return width;
}

static void draw_funnel_preview(egui_view_t *self, const funnel_palette_t *palette, const egui_view_conversion_funnel_snapshot_t *snapshot, egui_dim_t x,
                                egui_dim_t y, egui_dim_t w, egui_dim_t h, uint8_t compact, uint8_t locked)
{
    const funnel_profile_t *profile;
    egui_dim_t lane_h;
    egui_dim_t gap_h;
    egui_dim_t current_y;
    egui_dim_t layer_w;
    egui_dim_t layer_x;
    egui_color_t base_color;
    egui_color_t status_color;
    uint8_t i;

    if (w <= 12 || h <= 12)
    {
        return;
    }

    profile = &funnel_profiles[snapshot->profile % (sizeof(funnel_profiles) / sizeof(funnel_profiles[0]))];
    gap_h = compact ? 4 : 5;
    lane_h = (h - gap_h * 3) / 4;
    if (lane_h < 5)
    {
        lane_h = 5;
    }
    base_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 18 : 10);
    status_color = get_status_color(palette, snapshot);

    draw_round_fill_safe(x, y, w, h, compact ? 6 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 34 : 32));
    draw_round_stroke_safe(x, y, w, h, compact ? 6 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 48 : 42));

    current_y = y;
    for (i = 0; i < 4; i++)
    {
        egui_color_t layer_color;
        egui_alpha_t layer_alpha;

        layer_w = (w * profile->widths[i]) / 100;
        if (layer_w < 12)
        {
            layer_w = 12;
        }
        layer_x = x + (w - layer_w) / 2;
        layer_color = i == snapshot->focus_stage ? status_color : base_color;
        layer_alpha = i == snapshot->focus_stage ? egui_color_alpha_mix(self->alpha, compact ? 84 : 90) : egui_color_alpha_mix(self->alpha, compact ? 48 : 56);

        draw_round_fill_safe(layer_x, current_y, layer_w, lane_h, compact ? 4 : 5, layer_color, layer_alpha);
        draw_round_stroke_safe(layer_x, current_y, layer_w, lane_h, compact ? 4 : 5, 1, palette->focus,
                               egui_color_alpha_mix(self->alpha, i == snapshot->focus_stage ? 62 : 18));

        if (i < 3)
        {
            egui_dim_t next_w;
            egui_dim_t next_x;
            egui_dim_t bridge_w;
            egui_dim_t bridge_x;

            next_w = (w * profile->widths[i + 1]) / 100;
            if (next_w < 12)
            {
                next_w = 12;
            }
            next_x = x + (w - next_w) / 2;
            bridge_w = layer_w > next_w ? next_w + 8 : layer_w + 8;
            bridge_x = x + (w - bridge_w) / 2;
            draw_round_fill_safe(bridge_x, current_y + lane_h, bridge_w, gap_h, gap_h / 2,
                                 i == snapshot->focus_stage ? status_color : egui_rgb_mix(palette->border, palette->surface, locked ? 38 : 30),
                                 egui_color_alpha_mix(self->alpha, compact ? 66 : 74));
            draw_round_fill_safe(next_x + 1, current_y + lane_h + gap_h / 2, next_w - 2, 2, 1, palette->text,
                                 egui_color_alpha_mix(self->alpha, compact ? 18 : 22));
        }

        current_y += lane_h + gap_h;
    }
}

static void draw_card(egui_view_t *self, const funnel_palette_t *palette, const egui_view_conversion_funnel_snapshot_t *snapshot, egui_dim_t x, egui_dim_t y,
                      egui_dim_t w, egui_dim_t h, uint8_t compact, uint8_t locked)
{
    egui_region_t text_region;
    egui_color_t shell_color;
    egui_color_t status_color;
    egui_color_t title_color;
    egui_color_t summary_color;
    egui_color_t footer_color;
    egui_dim_t outer_padding;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t preview_x;
    egui_dim_t preview_y;
    egui_dim_t preview_w;
    egui_dim_t preview_h;
    const egui_font_t *card_font;

    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, palette->surface, compact ? 28 : 20);
    status_color = get_status_color(palette, snapshot);
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 24 : 42) : egui_rgb_mix(palette->muted, status_color, 26);
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 42) : egui_rgb_mix(palette->text, palette->muted, compact ? 8 : 12);
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 36) : egui_rgb_mix(palette->muted, palette->text, compact ? 28 : 34);
    outer_padding = compact ? 11 : 16;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w;
    title_w = pill_x - x - outer_padding - (compact ? 10 : 14);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? 40 : 58));
    draw_round_stroke_safe(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 54 : 72));

    text_region.location.x = x + outer_padding + (compact ? 3 : 2);
    text_region.location.y = y + (compact ? 10 : 11);
    text_region.size.width = title_w - (compact ? 3 : 2);
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    draw_round_fill_safe(pill_x, y + (compact ? 10 : 11), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 40 : 66));
    text_region.location.x = pill_x + 2;
    text_region.location.y = y + (compact ? 10 : 11);
    text_region.size.width = pill_w - 4;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, palette->text, self->alpha);

    text_region.location.x = x + outer_padding + (compact ? 1 : 2);
    text_region.location.y = y + (compact ? 25 : 33);
    text_region.size.width = w - outer_padding * 2 - (compact ? 2 : 4);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    preview_x = x + outer_padding + (compact ? -1 : 2);
    preview_y = y + (compact ? 39 : 53);
    preview_w = w - outer_padding * 2 - (compact ? 5 : 10);
    preview_h = compact ? 26 : 43;
    draw_funnel_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact, locked);

    text_region.location.x = x + outer_padding + (compact ? 2 : 4);
    text_region.location.y = preview_y + preview_h + (compact ? 7 : 9);
    text_region.size.width = w - outer_padding * 2 - (compact ? 4 : 8);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, footer_color, self->alpha);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 10;
    main_rect->location.y = region.location.y + 38;
    main_rect->size.width = 220;
    main_rect->size.height = 120;

    left_rect->location.x = region.location.x + 11;
    left_rect->location.y = region.location.y + 186;
    left_rect->size.width = 104;
    left_rect->size.height = 87;

    right_rect->location.x = region.location.x + 125;
    right_rect->location.y = region.location.y + 186;
    right_rect->size.width = 104;
    right_rect->size.height = 87;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t origin_x;
    egui_dim_t origin_y;

    origin_x = self->region_screen.location.x + self->padding.left;
    origin_y = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = origin_x + 10;
    main_rect->location.y = origin_y + 38;
    main_rect->size.width = 220;
    main_rect->size.height = 120;

    left_rect->location.x = origin_x + 11;
    left_rect->location.y = origin_y + 186;
    left_rect->size.width = 104;
    left_rect->size.height = 87;

    right_rect->location.x = origin_x + 125;
    right_rect->location.y = origin_y + 186;
    right_rect->size.width = 104;
    right_rect->size.height = 87;
}

static int egui_view_conversion_funnel_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;

    if (event->type != EGUI_MOTION_EVENT_ACTION_UP)
    {
        return 1;
    }

    get_zone_rects_screen(self, &main_rect, &left_rect, &right_rect);

    if (egui_region_pt_in_rect(&main_rect, event->location.x, event->location.y) && local->primary_snapshot_count > 0)
    {
        local->current_primary = (local->current_primary + 1) % local->primary_snapshot_count;
        local->last_zone = 0;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y) && local->compact_snapshot_count > 0)
    {
        local->current_compact = (local->current_compact + 1) % local->compact_snapshot_count;
        local->last_zone = 1;
        egui_view_invalidate(self);
        return 1;
    }
    if (egui_region_pt_in_rect(&right_rect, event->location.x, event->location.y) && local->locked_snapshot_count > 0)
    {
        local->current_locked = (local->current_locked + 1) % local->locked_snapshot_count;
        local->last_zone = 2;
        egui_view_invalidate(self);
        return 1;
    }
    return 1;
}

static void egui_view_conversion_funnel_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_conversion_funnel_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_conversion_funnel_snapshot_t *primary;
    const egui_view_conversion_funnel_snapshot_t *compact;
    const egui_view_conversion_funnel_snapshot_t *locked;
    const egui_font_t *title_font;
    const egui_font_t *guide_font;
    const egui_font_t *status_font;
    const char *status_text;
    egui_color_t status_color;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->primary_snapshots == NULL || local->compact_snapshots == NULL ||
        local->locked_snapshots == NULL || local->primary_snapshot_count == 0 || local->compact_snapshot_count == 0 || local->locked_snapshot_count == 0)
    {
        return;
    }

    primary = &local->primary_snapshots[local->current_primary];
    compact = &local->compact_snapshots[local->current_compact];
    locked = &local->locked_snapshots[local->current_locked];
    title_font = (const egui_font_t *)&egui_res_font_montserrat_16_4;
    guide_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    status_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    egui_canvas_draw_text_in_rect(title_font, "Conversion Funnel", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 21;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to shift funnel", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x7D8CA7), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_card(self, &primary_palette, primary, main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 0, 0);
    draw_card(self, &compact_palette, compact, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 1, 0);
    draw_card(self, &locked_palette, locked, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);

    if (local->last_zone == 1)
    {
        status_text = "Queue ready";
        status_color = compact_palette.accent;
        if (local->current_compact == 1)
        {
            status_text = "Load live";
            status_color = compact_palette.warn;
        }
        else if (local->current_compact == 2)
        {
            status_text = "Queue calm";
            status_color = compact_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Audit ready";
        status_color = locked_palette.accent;
        if (local->current_locked == 1)
        {
            status_text = "Hold steady";
            status_color = locked_palette.warn;
        }
        else if (local->current_locked == 2)
        {
            status_text = "Sync safe";
            status_color = locked_palette.lock;
        }
    }
    else
    {
        status_text = "Primary ready";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Mix active";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Seal locked";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 164;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(status_color, EGUI_COLOR_WHITE, 34), self->alpha);
    draw_round_fill_safe(region.location.x + 40, region.location.y + 181, 160, 2, 1, EGUI_COLOR_HEX(0x3B4E69), egui_color_alpha_mix(self->alpha, 52));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_conversion_funnel_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_conversion_funnel_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_conversion_funnel_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_conversion_funnel_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_conversion_funnel_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_conversion_funnel_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->primary_snapshots = NULL;
    local->compact_snapshots = NULL;
    local->locked_snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->primary_snapshot_count = 0;
    local->compact_snapshot_count = 0;
    local->locked_snapshot_count = 0;
    local->current_primary = 0;
    local->current_compact = 0;
    local->current_locked = 0;
    local->last_zone = 0;
}
