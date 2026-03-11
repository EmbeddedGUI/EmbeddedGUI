#include <stdlib.h>
#include <string.h>

#include "egui_view_priority_matrix.h"

typedef struct priority_palette priority_palette_t;
struct priority_palette
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

typedef struct priority_marker priority_marker_t;
struct priority_marker
{
    uint8_t x;
    uint8_t y;
    uint8_t size;
    uint8_t tone;
};

static const priority_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x151825), EGUI_COLOR_HEX(0x1E2638), EGUI_COLOR_HEX(0x425170), EGUI_COLOR_HEX(0xF3F6FF), EGUI_COLOR_HEX(0x9BA9C0),
        EGUI_COLOR_HEX(0x7B86FF), EGUI_COLOR_HEX(0xF0B45D), EGUI_COLOR_HEX(0x8DCE98), EGUI_COLOR_HEX(0xA7AEFF),
};

static const priority_palette_t compact_palette = {
        EGUI_COLOR_HEX(0x142019), EGUI_COLOR_HEX(0x1C3025), EGUI_COLOR_HEX(0x446751), EGUI_COLOR_HEX(0xEEFFF4), EGUI_COLOR_HEX(0x98BAA6),
        EGUI_COLOR_HEX(0x73D7A1), EGUI_COLOR_HEX(0xE8BE66), EGUI_COLOR_HEX(0x8ED4AB), EGUI_COLOR_HEX(0x9BF3C1),
};

static const priority_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x241A17), EGUI_COLOR_HEX(0x342721), EGUI_COLOR_HEX(0x896C5B), EGUI_COLOR_HEX(0xFFF3EA), EGUI_COLOR_HEX(0xD0B09E),
        EGUI_COLOR_HEX(0xF2B384), EGUI_COLOR_HEX(0xF1C161), EGUI_COLOR_HEX(0xD8CA90), EGUI_COLOR_HEX(0xFFDDB6),
};

static const priority_marker_t marker_patterns[][6] = {
        {{18, 18, 7, 0}, {29, 30, 5, 0}, {40, 23, 4, 1}, {71, 26, 4, 2}, {58, 68, 5, 2}, {78, 76, 4, 3}},
        {{22, 24, 4, 2}, {63, 16, 7, 0}, {78, 24, 5, 0}, {69, 39, 4, 1}, {36, 72, 4, 2}, {78, 69, 5, 3}},
        {{24, 22, 4, 2}, {74, 28, 4, 2}, {22, 62, 7, 0}, {37, 76, 5, 0}, {49, 60, 4, 1}, {77, 74, 4, 3}},
        {{19, 18, 5, 0}, {70, 20, 5, 1}, {28, 68, 5, 2}, {75, 70, 5, 3}, {48, 42, 4, 1}, {57, 54, 4, 2}},
        {{20, 22, 4, 2}, {62, 28, 4, 2}, {77, 36, 5, 1}, {55, 63, 4, 2}, {71, 76, 7, 0}, {33, 72, 4, 3}},
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_PRIORITY_MATRIX_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_PRIORITY_MATRIX_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_priority_matrix_set_primary_snapshots(egui_view_t *self, const egui_view_priority_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_priority_matrix_set_compact_snapshots(egui_view_t *self, const egui_view_priority_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
    local->compact_snapshots = snapshots;
    local->compact_snapshot_count = clamp_count(snapshot_count);
    if (local->current_compact >= local->compact_snapshot_count)
    {
        local->current_compact = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_priority_matrix_set_locked_snapshots(egui_view_t *self, const egui_view_priority_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_priority_matrix_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static egui_color_t get_status_color(const priority_palette_t *palette, const egui_view_priority_matrix_snapshot_t *snapshot)
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

static egui_dim_t get_pill_width(const egui_view_priority_matrix_snapshot_t *snapshot, uint8_t compact)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(16 + len * 5) : (egui_dim_t)(18 + len * 6);
    if (compact)
    {
        if (width < 40)
        {
            width = 40;
        }
        if (width > 50)
        {
            width = 50;
        }
    }
    else
    {
        if (width < 48)
        {
            width = 48;
        }
        if (width > 64)
        {
            width = 64;
        }
    }
    return width;
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

static uint8_t get_quadrant_from_percent(uint8_t x, uint8_t y)
{
    if (x < 50 && y < 50)
    {
        return 0;
    }
    if (x >= 50 && y < 50)
    {
        return 1;
    }
    if (x < 50)
    {
        return 2;
    }
    return 3;
}

static void draw_marker(egui_view_t *self, const priority_palette_t *palette, const egui_view_priority_matrix_snapshot_t *snapshot, egui_dim_t board_x,
                        egui_dim_t board_y, egui_dim_t board_w, egui_dim_t board_h, const priority_marker_t *marker, uint8_t compact)
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t size;
    egui_color_t color;
    uint8_t marker_quadrant;

    x = board_x + (egui_dim_t)((board_w * marker->x) / 100);
    y = board_y + (egui_dim_t)((board_h * marker->y) / 100);
    size = compact ? (egui_dim_t)(marker->size - 1) : (egui_dim_t)marker->size;
    if (size < 3)
    {
        size = 3;
    }

    marker_quadrant = get_quadrant_from_percent(marker->x, marker->y);
    color = egui_rgb_mix(palette->text, palette->muted, 18 + marker->tone * 16);
    if (marker_quadrant == snapshot->focus_quadrant)
    {
        color = get_status_color(palette, snapshot);
    }
    else if (marker->tone == 3)
    {
        color = egui_rgb_mix(palette->muted, palette->border, 28);
    }

    draw_round_fill_safe(x - size / 2, y - size / 2, size, size, size / 2, color, egui_color_alpha_mix(self->alpha, compact ? 84 : 92));
}

static void draw_quadrant_badges(egui_view_t *self, const priority_palette_t *palette, const egui_view_priority_matrix_snapshot_t *snapshot, egui_dim_t board_x,
                                 egui_dim_t board_y, egui_dim_t board_w, egui_dim_t board_h, uint8_t compact)
{
    egui_dim_t badge_w;
    egui_dim_t badge_h;
    egui_dim_t gap;
    egui_dim_t base_y;
    egui_dim_t x;
    uint8_t i;

    gap = compact ? 4 : 5;
    badge_h = compact ? 4 : 6;
    badge_w = (board_w - gap * 3) / 4;
    base_y = board_y + board_h + (compact ? 5 : 7);
    if (badge_w <= 0)
    {
        return;
    }

    for (i = 0; i < 4; i++)
    {
        egui_color_t color;

        color = egui_rgb_mix(palette->border, palette->panel, 24);
        if (i == snapshot->focus_quadrant)
        {
            color = get_status_color(palette, snapshot);
        }
        else if (snapshot->label_mode == i)
        {
            color = egui_rgb_mix(get_status_color(palette, snapshot), palette->border, 42);
        }

        x = board_x + i * (badge_w + gap);
        draw_round_fill_safe(x, base_y, badge_w, badge_h, 2, color, egui_color_alpha_mix(self->alpha, compact ? 74 : 82));
    }
}

static void draw_priority_preview(egui_view_t *self, const priority_palette_t *palette, const egui_view_priority_matrix_snapshot_t *snapshot, egui_dim_t x,
                                  egui_dim_t y, egui_dim_t w, egui_dim_t h, uint8_t compact, uint8_t locked)
{
    egui_dim_t chrome_x;
    egui_dim_t chrome_y;
    egui_dim_t chrome_w;
    egui_dim_t chrome_h;
    egui_dim_t board_x;
    egui_dim_t board_y;
    egui_dim_t board_w;
    egui_dim_t board_h;
    egui_dim_t half_w;
    egui_dim_t half_h;
    egui_dim_t focus_x;
    egui_dim_t focus_y;
    egui_dim_t focus_w;
    egui_dim_t focus_h;
    egui_dim_t quad_gap;
    egui_color_t status_color;
    egui_color_t calm_color;
    egui_color_t quad_color;
    egui_dim_t center_x;
    egui_dim_t center_y;
    uint8_t pattern_index;
    uint8_t i;

    if (w <= 0 || h <= 0)
    {
        return;
    }

    status_color = get_status_color(palette, snapshot);
    calm_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 42 : 26);
    quad_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 52 : 40);
    chrome_x = x;
    chrome_y = y;
    chrome_w = w;
    chrome_h = h;
    board_x = chrome_x + (compact ? 7 : 9);
    board_y = chrome_y + (compact ? 7 : 8);
    board_w = chrome_w - (compact ? 14 : 18);
    board_h = chrome_h - (compact ? 19 : 24);
    if (board_w <= 8 || board_h <= 8)
    {
        return;
    }

    half_w = board_w / 2;
    half_h = board_h / 2;
    focus_w = board_w - half_w;
    focus_h = board_h - half_h;
    focus_x = board_x + ((snapshot->focus_quadrant & 1) ? half_w : 0);
    focus_y = board_y + ((snapshot->focus_quadrant >= 2) ? half_h : 0);
    if ((snapshot->focus_quadrant & 1) == 0)
    {
        focus_w = half_w;
    }
    if (snapshot->focus_quadrant < 2)
    {
        focus_h = half_h;
    }
    quad_gap = compact ? 3 : 4;

    draw_round_fill_safe(chrome_x, chrome_y, chrome_w, chrome_h, compact ? 6 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 34 : 30));
    draw_round_stroke_safe(chrome_x, chrome_y, chrome_w, chrome_h, compact ? 6 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 42 : 36));
    draw_round_fill_safe(board_x, board_y, board_w, board_h, compact ? 5 : 6, calm_color, egui_color_alpha_mix(self->alpha, compact ? 44 : 52));
    draw_round_fill_safe(board_x + 2, board_y + 2, half_w - quad_gap, half_h - quad_gap, compact ? 4 : 5, quad_color, egui_color_alpha_mix(self->alpha, 28));
    draw_round_fill_safe(board_x + half_w + quad_gap - 2, board_y + 2, board_w - half_w - quad_gap, half_h - quad_gap, compact ? 4 : 5, quad_color,
                         egui_color_alpha_mix(self->alpha, 24));
    draw_round_fill_safe(board_x + 2, board_y + half_h + quad_gap - 2, half_w - quad_gap, board_h - half_h - quad_gap, compact ? 4 : 5, quad_color,
                         egui_color_alpha_mix(self->alpha, 20));
    draw_round_fill_safe(board_x + half_w + quad_gap - 2, board_y + half_h + quad_gap - 2, board_w - half_w - quad_gap, board_h - half_h - quad_gap,
                         compact ? 4 : 5, quad_color, egui_color_alpha_mix(self->alpha, 18));
    draw_round_fill_safe(focus_x, focus_y, focus_w, focus_h, compact ? 5 : 6, status_color, egui_color_alpha_mix(self->alpha, locked ? 28 : 48));
    draw_round_stroke_safe(focus_x, focus_y, focus_w, focus_h, compact ? 5 : 6, 1, palette->focus, egui_color_alpha_mix(self->alpha, compact ? 68 : 84));

    center_x = board_x + half_w;
    center_y = board_y + half_h;
    draw_round_fill_safe(center_x - 1, board_y + 4, 2, board_h - 8, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 58 : 64));
    draw_round_fill_safe(board_x + 4, center_y - 1, board_w - 8, 2, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 58 : 64));
    draw_round_fill_safe(board_x + 6, board_y + 6, 5, 3, 2, snapshot->focus_quadrant == 0 ? status_color : palette->muted,
                         egui_color_alpha_mix(self->alpha, 70));
    draw_round_fill_safe(board_x + board_w - 11, board_y + 6, 5, 3, 2, snapshot->focus_quadrant == 1 ? status_color : palette->muted,
                         egui_color_alpha_mix(self->alpha, 66));
    draw_round_fill_safe(board_x + 6, board_y + board_h - 9, 5, 3, 2, snapshot->focus_quadrant == 2 ? status_color : palette->muted,
                         egui_color_alpha_mix(self->alpha, 62));
    draw_round_fill_safe(board_x + board_w - 11, board_y + board_h - 9, 5, 3, 2, snapshot->focus_quadrant == 3 ? status_color : palette->muted,
                         egui_color_alpha_mix(self->alpha, 58));
    draw_round_fill_safe(board_x + 5, board_y + 5, 6, 2, 1, palette->text, egui_color_alpha_mix(self->alpha, compact ? 30 : 36));
    draw_round_fill_safe(board_x + board_w - 11, board_y + board_h - 7, 6, 2, 1, palette->muted, egui_color_alpha_mix(self->alpha, compact ? 22 : 28));

    pattern_index = snapshot->marker_pattern;
    if (pattern_index >= (sizeof(marker_patterns) / sizeof(marker_patterns[0])))
    {
        pattern_index = 0;
    }
    for (i = 0; i < 6; i++)
    {
        draw_marker(self, palette, snapshot, board_x, board_y, board_w, board_h, &marker_patterns[pattern_index][i], compact);
    }

    draw_round_fill_safe(center_x - 3, center_y - 3, 6, 6, 3, palette->text, egui_color_alpha_mix(self->alpha, compact ? 68 : 82));
    draw_quadrant_badges(self, palette, snapshot, board_x, board_y, board_w, board_h, compact);
}

static void draw_card(egui_view_t *self, const priority_palette_t *palette, const egui_view_priority_matrix_snapshot_t *snapshot, egui_dim_t x, egui_dim_t y,
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

    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, palette->surface, compact ? 30 : 22);
    status_color = get_status_color(palette, snapshot);
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 26 : 44) : palette->muted;
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 42) : (compact ? palette->text : egui_rgb_mix(palette->text, palette->muted, 12));
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 34) : egui_rgb_mix(palette->muted, palette->text, compact ? 20 : 22);
    outer_padding = compact ? 12 : 15;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w;
    title_w = pill_x - x - outer_padding - (compact ? 10 : 14);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? 42 : 56));
    draw_round_stroke_safe(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 56 : 68));

    text_region.location.x = x + outer_padding + 1;
    text_region.location.y = y + (compact ? 9 : 10);
    text_region.size.width = title_w - 1;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    draw_round_fill_safe(pill_x, y + (compact ? 9 : 10), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 38 : 64));
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

    preview_x = x + outer_padding + (compact ? 1 : 3);
    preview_y = y + (compact ? 35 : 49);
    preview_w = w - outer_padding * 2 - (compact ? 7 : 12);
    preview_h = compact ? 30 : 49;
    draw_priority_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact, locked);

    text_region.location.x = x + outer_padding + (compact ? 2 : 4);
    text_region.location.y = preview_y + preview_h + (compact ? 7 : 11);
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

static int egui_view_priority_matrix_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
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

static void egui_view_priority_matrix_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_priority_matrix_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_priority_matrix_snapshot_t *primary;
    const egui_view_priority_matrix_snapshot_t *compact;
    const egui_view_priority_matrix_snapshot_t *locked;
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
    title_font = (const egui_font_t *)&egui_res_font_montserrat_14_4;
    guide_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    status_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 1;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    egui_canvas_draw_text_in_rect(title_font, "Priority Matrix", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 21;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to shift focus", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x7D8AA3), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_card(self, &primary_palette, primary, main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 0, 0);
    draw_card(self, &compact_palette, compact, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 1, 0);
    draw_card(self, &locked_palette, locked, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);

    if (local->last_zone == 1)
    {
        status_text = "Compact scan set";
        status_color = compact_palette.accent;
        if (local->current_compact == 1)
        {
            status_text = "Compact load live";
            status_color = compact_palette.warn;
        }
        else if (local->current_compact == 2)
        {
            status_text = "Compact lane calm";
            status_color = compact_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Locked review set";
        status_color = locked_palette.accent;
        if (local->current_locked == 1)
        {
            status_text = "Locked hold steady";
            status_color = locked_palette.warn;
        }
        else if (local->current_locked == 2)
        {
            status_text = "Locked archive safe";
            status_color = locked_palette.lock;
        }
    }
    else
    {
        status_text = "Priority focus now";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Priority plan set";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Priority handoff calm";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 159;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(status_color, EGUI_COLOR_WHITE, 18), self->alpha);
    draw_round_fill_safe(region.location.x + 40, region.location.y + 175, 160, 2, 1, EGUI_COLOR_HEX(0x32435A), egui_color_alpha_mix(self->alpha, 52));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_priority_matrix_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_priority_matrix_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_priority_matrix_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_priority_matrix_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_priority_matrix_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_priority_matrix_t);
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
