#include <stdlib.h>
#include <string.h>

#include "egui_view_sankey_flow.h"

typedef struct sankey_palette sankey_palette_t;
struct sankey_palette
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

typedef struct sankey_profile sankey_profile_t;
struct sankey_profile
{
    uint8_t stage_a[2];
    uint8_t stage_b[2];
    uint8_t stage_c[2];
    uint8_t band_widths[4];
};

static const sankey_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x141A26), EGUI_COLOR_HEX(0x1C2537), EGUI_COLOR_HEX(0x425779), EGUI_COLOR_HEX(0xF1F7FF), EGUI_COLOR_HEX(0x99A8BE),
        EGUI_COLOR_HEX(0x63C8FF), EGUI_COLOR_HEX(0xEEB45D), EGUI_COLOR_HEX(0x9FD39A), EGUI_COLOR_HEX(0xA2E1FF),
};

static const sankey_palette_t compact_palette = {
        EGUI_COLOR_HEX(0x13211C), EGUI_COLOR_HEX(0x1C3128), EGUI_COLOR_HEX(0x538470), EGUI_COLOR_HEX(0xECFFF6), EGUI_COLOR_HEX(0x97BCA9),
        EGUI_COLOR_HEX(0x74E7BE), EGUI_COLOR_HEX(0xE7BC68), EGUI_COLOR_HEX(0xA3DCBF), EGUI_COLOR_HEX(0xB1F7D6),
};

static const sankey_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x241913), EGUI_COLOR_HEX(0x35261E), EGUI_COLOR_HEX(0x94745E), EGUI_COLOR_HEX(0xFFF3E9), EGUI_COLOR_HEX(0xD2B09C),
        EGUI_COLOR_HEX(0xF4C08D), EGUI_COLOR_HEX(0xF0C069), EGUI_COLOR_HEX(0xDCCB95), EGUI_COLOR_HEX(0xFFE3BF),
};

static const sankey_profile_t sankey_profiles[] = {
        {{18, 12}, {16, 11}, {14, 10}, {6, 5, 4, 3}},
        {{16, 10}, {11, 17}, {9, 16}, {4, 6, 3, 5}},
        {{13, 14}, {15, 12}, {16, 9}, {5, 4, 6, 3}},
        {{19, 8}, {13, 12}, {11, 10}, {6, 3, 4, 3}},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SANKEY_FLOW_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SANKEY_FLOW_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_sankey_flow_set_primary_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_sankey_flow_set_compact_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
    local->compact_snapshots = snapshots;
    local->compact_snapshot_count = clamp_count(snapshot_count);
    if (local->current_compact >= local->compact_snapshot_count)
    {
        local->current_compact = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_sankey_flow_set_locked_snapshots(egui_view_t *self, const egui_view_sankey_flow_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_sankey_flow_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
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

static void draw_round_stroke_safe(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static egui_color_t get_status_color(const sankey_palette_t *palette, const egui_view_sankey_flow_snapshot_t *snapshot)
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

static egui_dim_t get_pill_width(const egui_view_sankey_flow_snapshot_t *snapshot, uint8_t compact)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(16 + len * 5) : (egui_dim_t)(18 + len * 6);
    if (compact)
    {
        if (width < 42)
        {
            width = 42;
        }
        if (width > 54)
        {
            width = 54;
        }
    }
    else
    {
        if (width < 50)
        {
            width = 50;
        }
        if (width > 68)
        {
            width = 68;
        }
    }
    return width;
}

static void resolve_stage_rects(egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const uint8_t stage_values[2], egui_region_t rects[2], uint8_t compact)
{
    egui_dim_t top_pad;
    egui_dim_t gap;
    egui_dim_t available_h;
    egui_dim_t total;
    egui_dim_t first_h;
    egui_dim_t second_h;

    top_pad = compact ? 5 : 6;
    gap = compact ? 5 : 6;
    available_h = h - top_pad * 2 - gap;
    if (available_h <= 6)
    {
        available_h = 6;
    }
    total = stage_values[0] + stage_values[1];
    if (total == 0)
    {
        total = 1;
    }

    first_h = (available_h * stage_values[0]) / total;
    if (first_h < 6)
    {
        first_h = 6;
    }
    second_h = available_h - first_h;
    if (second_h < 6)
    {
        second_h = 6;
        first_h = available_h - second_h;
    }

    rects[0].location.x = x;
    rects[0].location.y = y + top_pad;
    rects[0].size.width = w;
    rects[0].size.height = first_h;

    rects[1].location.x = x;
    rects[1].location.y = rects[0].location.y + first_h + gap;
    rects[1].size.width = w;
    rects[1].size.height = second_h;
}

static void draw_flow_band(
        egui_view_t *self,
        egui_dim_t x1,
        egui_dim_t y1,
        egui_dim_t x2,
        egui_dim_t y2,
        egui_dim_t thickness,
        egui_color_t color)
{
    egui_dim_t min_x;
    egui_dim_t max_x;
    egui_dim_t width;
    egui_dim_t i;
    egui_dim_t step_x;
    egui_dim_t current_x;
    egui_dim_t current_y;
    egui_dim_t band_w;
    egui_dim_t band_h;

    EGUI_UNUSED(self);

    min_x = x1 < x2 ? x1 : x2;
    max_x = x1 > x2 ? x1 : x2;
    width = max_x - min_x;
    if (thickness < 2)
    {
        thickness = 2;
    }
    if (width < 1)
    {
        width = 1;
    }

    step_x = width / 7;
    if (step_x < 2)
    {
        step_x = 2;
    }

    for (i = 0; i < 11; i++)
    {
        current_x = min_x + i * step_x;
        if (current_x > max_x)
        {
            current_x = max_x;
        }
        current_y = y1 + ((y2 - y1) * i) / 10;
        band_w = step_x + 6;
        band_h = thickness;
        draw_round_fill_safe(current_x - band_w / 2, current_y - band_h / 2, band_w, band_h, band_h / 2, color, EGUI_ALPHA_100);
    }
}

static void draw_sankey_preview(
        egui_view_t *self,
        const sankey_palette_t *palette,
        const egui_view_sankey_flow_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t compact,
        uint8_t locked)
{
    const sankey_profile_t *profile;
    egui_region_t stage_a[2];
    egui_region_t stage_b[2];
    egui_region_t stage_c[2];
    egui_dim_t inset;
    egui_dim_t stage_w;
    egui_dim_t gap_x;
    egui_dim_t stage_x0;
    egui_dim_t stage_x1;
    egui_dim_t stage_x2;
    egui_dim_t band_width;
    egui_color_t base_node_color;
    egui_color_t base_band_color;
    egui_color_t focus_color;
    egui_color_t quiet_color;
    uint8_t i;

    if (w <= 8 || h <= 8)
    {
        return;
    }

    profile = &sankey_profiles[snapshot->profile % (sizeof(sankey_profiles) / sizeof(sankey_profiles[0]))];
    inset = compact ? 6 : 8;
    gap_x = compact ? 11 : 14;
    stage_w = (w - inset * 2 - gap_x * 2) / 3;
    if (stage_w < 8)
    {
        stage_w = 8;
    }
    stage_x0 = x + inset;
    stage_x1 = stage_x0 + stage_w + gap_x;
    stage_x2 = stage_x1 + stage_w + gap_x;

    base_node_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 18 : 8);
    base_band_color = egui_rgb_mix(palette->border, palette->surface, locked ? 38 : 28);
    focus_color = get_status_color(palette, snapshot);
    quiet_color = egui_rgb_mix(palette->muted, palette->panel, 26);

    draw_round_fill_safe(x, y, w, h, compact ? 6 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 34 : 32));
    draw_round_stroke_safe(x, y, w, h, compact ? 6 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 48 : 42));

    resolve_stage_rects(stage_x0, y, stage_w, h, profile->stage_a, stage_a, compact);
    resolve_stage_rects(stage_x1, y, stage_w, h, profile->stage_b, stage_b, compact);
    resolve_stage_rects(stage_x2, y, stage_w, h, profile->stage_c, stage_c, compact);

    band_width = compact ? 4 : 5;
    draw_flow_band(
            self,
            stage_a[0].location.x + stage_a[0].size.width,
            stage_a[0].location.y + stage_a[0].size.height / 2,
            stage_b[0].location.x,
            stage_b[0].location.y + stage_b[0].size.height / 2,
            band_width + profile->band_widths[0] / 2,
            snapshot->focus_stage == 0 ? focus_color : base_band_color);
    draw_flow_band(
            self,
            stage_a[1].location.x + stage_a[1].size.width,
            stage_a[1].location.y + stage_a[1].size.height / 2,
            stage_b[1].location.x,
            stage_b[1].location.y + stage_b[1].size.height / 2,
            band_width + profile->band_widths[1] / 2,
            snapshot->focus_stage == 0 ? egui_rgb_mix(focus_color, palette->border, 30) : quiet_color);
    draw_flow_band(
            self,
            stage_b[0].location.x + stage_b[0].size.width,
            stage_b[0].location.y + stage_b[0].size.height / 2,
            stage_c[0].location.x,
            stage_c[0].location.y + stage_c[0].size.height / 2,
            band_width + profile->band_widths[2] / 2,
            snapshot->focus_stage == 1 ? focus_color : base_band_color);
    draw_flow_band(
            self,
            stage_b[1].location.x + stage_b[1].size.width,
            stage_b[1].location.y + stage_b[1].size.height / 2,
            stage_c[1].location.x,
            stage_c[1].location.y + stage_c[1].size.height / 2,
            band_width + profile->band_widths[3] / 2,
            snapshot->focus_stage == 2 ? focus_color : quiet_color);

    for (i = 0; i < 2; i++)
    {
        egui_color_t node_color_a;
        egui_color_t node_color_b;
        egui_color_t node_color_c;

        node_color_a = snapshot->focus_stage == 0 ? focus_color : base_node_color;
        node_color_b = snapshot->focus_stage == 1 ? focus_color : base_node_color;
        node_color_c = snapshot->focus_stage == 2 ? focus_color : base_node_color;

        draw_round_fill_safe(
                stage_a[i].location.x,
                stage_a[i].location.y,
                stage_a[i].size.width,
                stage_a[i].size.height,
                compact ? 4 : 5,
                node_color_a,
                egui_color_alpha_mix(self->alpha, i == 0 ? 68 : 46));
        draw_round_fill_safe(
                stage_b[i].location.x,
                stage_b[i].location.y,
                stage_b[i].size.width,
                stage_b[i].size.height,
                compact ? 4 : 5,
                node_color_b,
                egui_color_alpha_mix(self->alpha, i == 0 ? 62 : 44));
        draw_round_fill_safe(
                stage_c[i].location.x,
                stage_c[i].location.y,
                stage_c[i].size.width,
                stage_c[i].size.height,
                compact ? 4 : 5,
                node_color_c,
                egui_color_alpha_mix(self->alpha, i == 0 ? 58 : 40));
    }

    draw_round_fill_safe(stage_x0 + 1, y + 4, stage_w - 2, 2, 1, palette->text, egui_color_alpha_mix(self->alpha, compact ? 30 : 36));
    draw_round_fill_safe(stage_x1 + 1, y + 4, stage_w - 2, 2, 1, palette->muted, egui_color_alpha_mix(self->alpha, compact ? 26 : 30));
    draw_round_fill_safe(stage_x2 + 1, y + 4, stage_w - 2, 2, 1, palette->muted, egui_color_alpha_mix(self->alpha, compact ? 22 : 26));
}

static void draw_card(
        egui_view_t *self,
        const sankey_palette_t *palette,
        const egui_view_sankey_flow_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t compact,
        uint8_t locked)
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
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 24 : 42) : palette->muted;
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 42)
                           : egui_rgb_mix(palette->text, palette->muted, compact ? 8 : 12);
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 30)
                          : egui_rgb_mix(palette->muted, palette->text, compact ? 24 : 28);
    outer_padding = compact ? 12 : 15;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w;
    title_w = pill_x - x - outer_padding - (compact ? 10 : 14);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? 40 : 56));
    draw_round_stroke_safe(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 54 : 68));

    text_region.location.x = x + outer_padding + 2;
    text_region.location.y = y + (compact ? 9 : 10);
    text_region.size.width = title_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    draw_round_fill_safe(pill_x, y + (compact ? 9 : 10), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 40 : 66));
    text_region.location.x = pill_x + 1;
    text_region.location.y = y + (compact ? 9 : 10);
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, palette->text, self->alpha);

    text_region.location.x = x + outer_padding + (compact ? 1 : 2);
    text_region.location.y = y + (compact ? 23 : 30);
    text_region.size.width = w - outer_padding * 2 - (compact ? 2 : 4);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    preview_x = x + outer_padding + (compact ? 0 : 3);
    preview_y = y + (compact ? 35 : 49);
    preview_w = w - outer_padding * 2 - (compact ? 5 : 12);
    preview_h = compact ? 31 : 49;
    draw_sankey_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact, locked);

    text_region.location.x = x + outer_padding + (compact ? 2 : 4);
    text_region.location.y = preview_y + preview_h + (compact ? 6 : 10);
    text_region.size.width = w - outer_padding * 2 - (compact ? 4 : 8);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, footer_color, self->alpha);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 10;
    main_rect->location.y = region.location.y + 33;
    main_rect->size.width = 220;
    main_rect->size.height = 125;

    left_rect->location.x = region.location.x + 11;
    left_rect->location.y = region.location.y + 181;
    left_rect->size.width = 102;
    left_rect->size.height = 87;

    right_rect->location.x = region.location.x + 127;
    right_rect->location.y = region.location.y + 181;
    right_rect->size.width = 102;
    right_rect->size.height = 87;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t origin_x;
    egui_dim_t origin_y;

    origin_x = self->region_screen.location.x + self->padding.left;
    origin_y = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = origin_x + 10;
    main_rect->location.y = origin_y + 33;
    main_rect->size.width = 220;
    main_rect->size.height = 125;

    left_rect->location.x = origin_x + 11;
    left_rect->location.y = origin_y + 181;
    left_rect->size.width = 102;
    left_rect->size.height = 87;

    right_rect->location.x = origin_x + 127;
    right_rect->location.y = origin_y + 181;
    right_rect->size.width = 102;
    right_rect->size.height = 87;
}

static int egui_view_sankey_flow_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
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

static void egui_view_sankey_flow_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_sankey_flow_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_sankey_flow_snapshot_t *primary;
    const egui_view_sankey_flow_snapshot_t *compact;
    const egui_view_sankey_flow_snapshot_t *locked;
    const egui_font_t *title_font;
    const egui_font_t *guide_font;
    const egui_font_t *status_font;
    const char *status_text;
    egui_color_t status_color;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->primary_snapshots == NULL || local->compact_snapshots == NULL
        || local->locked_snapshots == NULL || local->primary_snapshot_count == 0 || local->compact_snapshot_count == 0 || local->locked_snapshot_count == 0)
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
    egui_canvas_draw_text_in_rect(title_font, "Sankey Flow", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 20;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to shift flow", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x8694AD), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_card(self, &primary_palette, primary, main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 0, 0);
    draw_card(self, &compact_palette, compact, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 1, 0);
    draw_card(self, &locked_palette, locked, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);

    if (local->last_zone == 1)
    {
        status_text = "Compact route set";
        status_color = compact_palette.accent;
        if (local->current_compact == 1)
        {
            status_text = "Compact mix live";
            status_color = compact_palette.warn;
        }
        else if (local->current_compact == 2)
        {
            status_text = "Compact flow calm";
            status_color = compact_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Locked audit set";
        status_color = locked_palette.accent;
        if (local->current_locked == 1)
        {
            status_text = "Locked hold steady";
            status_color = locked_palette.warn;
        }
        else if (local->current_locked == 2)
        {
            status_text = "Locked export safe";
            status_color = locked_palette.lock;
        }
    }
    else
    {
        status_text = "Primary flow live";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Primary mix set";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Primary flow calm";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 159;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(status_color, EGUI_COLOR_WHITE, 24), self->alpha);
    draw_round_fill_safe(region.location.x + 40, region.location.y + 175, 160, 2, 1, EGUI_COLOR_HEX(0x32435A), egui_color_alpha_mix(self->alpha, 44));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_sankey_flow_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_sankey_flow_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_sankey_flow_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_sankey_flow_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_sankey_flow_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_sankey_flow_t);
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
