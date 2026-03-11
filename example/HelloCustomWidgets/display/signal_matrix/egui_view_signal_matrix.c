#include <stdlib.h>
#include <string.h>

#include "egui_view_signal_matrix.h"

typedef struct signal_palette signal_palette_t;
struct signal_palette
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

static const signal_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x141B28), EGUI_COLOR_HEX(0x1E2B40), EGUI_COLOR_HEX(0x4B607C), EGUI_COLOR_HEX(0xF2F7FF), EGUI_COLOR_HEX(0x9EADC3),
        EGUI_COLOR_HEX(0x66CCFF), EGUI_COLOR_HEX(0xF0B35E), EGUI_COLOR_HEX(0x95D3A5), EGUI_COLOR_HEX(0xA9E6FF),
};

static const signal_palette_t compact_palette = {
        EGUI_COLOR_HEX(0x162119), EGUI_COLOR_HEX(0x203329), EGUI_COLOR_HEX(0x5C8B74), EGUI_COLOR_HEX(0xEEFFF6), EGUI_COLOR_HEX(0x9FBFAD),
        EGUI_COLOR_HEX(0x78E2B8), EGUI_COLOR_HEX(0xE8BE67), EGUI_COLOR_HEX(0xA8DBBF), EGUI_COLOR_HEX(0xBBF5D8),
};

static const signal_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x241A15), EGUI_COLOR_HEX(0x372821), EGUI_COLOR_HEX(0x9A7864), EGUI_COLOR_HEX(0xFFF4EB), EGUI_COLOR_HEX(0xD6B49F),
        EGUI_COLOR_HEX(0xF2C08B), EGUI_COLOR_HEX(0xF1B868), EGUI_COLOR_HEX(0xDCC998), EGUI_COLOR_HEX(0xFFE2C2),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SIGNAL_MATRIX_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SIGNAL_MATRIX_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_signal_matrix_set_primary_snapshots(egui_view_t *self, const egui_view_signal_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_signal_matrix_set_compact_snapshots(egui_view_t *self, const egui_view_signal_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
    local->compact_snapshots = snapshots;
    local->compact_snapshot_count = clamp_count(snapshot_count);
    if (local->current_compact >= local->compact_snapshot_count)
    {
        local->current_compact = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_signal_matrix_set_locked_snapshots(egui_view_t *self, const egui_view_signal_matrix_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_signal_matrix_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
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

static egui_color_t get_status_color(const signal_palette_t *palette, const egui_view_signal_matrix_snapshot_t *snapshot)
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

static egui_dim_t get_pill_width(const egui_view_signal_matrix_snapshot_t *snapshot, uint8_t compact)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(18 + len * 6) : (egui_dim_t)(22 + len * 6);
    if (compact)
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
    else
    {
        if (width < 62)
        {
            width = 62;
        }
        if (width > 80)
        {
            width = 80;
        }
    }
    return width;
}

static void draw_signal_preview(egui_view_t *self, const signal_palette_t *palette, const egui_view_signal_matrix_snapshot_t *snapshot, egui_dim_t x,
                                egui_dim_t y, egui_dim_t w, egui_dim_t h, uint8_t compact, uint8_t locked)
{
    egui_dim_t pad_x;
    egui_dim_t pad_y;
    egui_dim_t grid_x;
    egui_dim_t grid_y;
    egui_dim_t grid_w;
    egui_dim_t grid_h;
    egui_dim_t cell_gap_x;
    egui_dim_t cell_gap_y;
    egui_dim_t cell_w;
    egui_dim_t cell_h;
    egui_dim_t meter_h;
    egui_dim_t focus_x;
    egui_color_t status_color;
    uint8_t row;
    uint8_t col;

    if (w <= 18 || h <= 18)
    {
        return;
    }

    draw_round_fill_safe(x, y, w, h, compact ? 6 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 38 : 34));
    draw_round_stroke_safe(x, y, w, h, compact ? 6 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 54 : 48));

    pad_x = compact ? 5 : 8;
    pad_y = compact ? 4 : 6;
    grid_x = x + pad_x;
    grid_y = y + pad_y;
    grid_w = w - pad_x * 2;
    grid_h = h - pad_y * 2;
    cell_gap_x = compact ? 2 : 3;
    cell_gap_y = compact ? 1 : 2;
    cell_w = (grid_w - cell_gap_x * 3) / 4;
    cell_h = (grid_h - cell_gap_y * 4) / 5;
    if (cell_w < 5 || cell_h < 2)
    {
        return;
    }
    status_color = get_status_color(palette, snapshot);

    focus_x = grid_x + snapshot->focus_column * (cell_w + cell_gap_x) - 2;
    draw_round_fill_safe(focus_x, grid_y - 3, cell_w + 4, grid_h + 6, compact ? 4 : 5, palette->focus, egui_color_alpha_mix(self->alpha, compact ? 14 : 16));
    draw_round_fill_safe(focus_x + 4, grid_y + grid_h + 1, cell_w - 4, 2, 1, palette->focus, egui_color_alpha_mix(self->alpha, compact ? 72 : 76));

    for (row = 0; row < 5; row++)
    {
        meter_h = snapshot->active_rows[row];
        if (meter_h > 4)
        {
            meter_h = 4;
        }
        for (col = 0; col < 4; col++)
        {
            egui_dim_t cell_x;
            egui_dim_t cell_y;
            egui_color_t cell_color;
            egui_alpha_t cell_alpha;

            cell_x = grid_x + col * (cell_w + cell_gap_x);
            cell_y = grid_y + row * (cell_h + cell_gap_y);
            cell_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 16 : 10);
            cell_alpha = egui_color_alpha_mix(self->alpha, compact ? 52 : 58);
            if (col == snapshot->focus_column)
            {
                cell_color = egui_rgb_mix(cell_color, palette->focus, 30);
                cell_alpha = egui_color_alpha_mix(self->alpha, compact ? 64 : 68);
            }
            draw_round_fill_safe(cell_x, cell_y, cell_w, cell_h, compact ? 2 : 3, cell_color, cell_alpha);
            draw_round_stroke_safe(cell_x, cell_y, cell_w, cell_h, compact ? 2 : 3, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 30 : 34));

            if (col < meter_h)
            {
                draw_round_fill_safe(cell_x + 1, cell_y + 1, cell_w - 2, cell_h - 2, compact ? 1 : 2, status_color,
                                     egui_color_alpha_mix(self->alpha, compact ? 84 : 88));
            }
        }
    }
}

static void draw_card(egui_view_t *self, const signal_palette_t *palette, const egui_view_signal_matrix_snapshot_t *snapshot, egui_dim_t x, egui_dim_t y,
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

    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, palette->surface, compact ? (locked ? 22 : 28) : 18);
    status_color = get_status_color(palette, snapshot);
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 26 : 44) : egui_rgb_mix(palette->muted, status_color, 36);
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 42) : egui_rgb_mix(palette->text, palette->muted, compact ? 14 : 16);
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 46) : egui_rgb_mix(palette->muted, palette->text, compact ? 34 : 40);
    outer_padding = compact ? 11 : 16;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w - 1;
    title_w = pill_x - x - outer_padding - (compact ? 10 : 14);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? (locked ? 34 : 44) : 62));
    draw_round_stroke_safe(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? (locked ? 48 : 58) : 76));

    text_region.location.x = x + outer_padding + 4;
    text_region.location.y = y + (compact ? 11 : 12);
    text_region.size.width = title_w - 4;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);
    draw_round_fill_safe(x + outer_padding + 4, y + (compact ? 25 : 29), compact ? 18 : 28, 2, 1, status_color,
                         egui_color_alpha_mix(self->alpha, compact ? 56 : 62));

    draw_round_fill_safe(pill_x, y + (compact ? 11 : 12), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 40 : 66));
    draw_round_stroke_safe(pill_x, y + (compact ? 11 : 12), pill_w, 11, 5, 1, egui_rgb_mix(status_color, palette->text, locked ? 22 : 28),
                           egui_color_alpha_mix(self->alpha, locked ? 46 : 54));
    text_region.location.x = pill_x + 3;
    text_region.location.y = y + (compact ? 11 : 12);
    text_region.size.width = pill_w - 6;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, palette->text, self->alpha);

    text_region.location.x = x + outer_padding + 2;
    text_region.location.y = y + (compact ? 27 : 35);
    text_region.size.width = w - outer_padding * 2 - 4;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    preview_x = x + outer_padding;
    preview_y = y + (compact ? 42 : 56);
    preview_w = w - outer_padding * 2 - (compact ? 0 : 4);
    preview_h = compact ? 26 : 44;
    draw_signal_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact, locked);

    text_region.location.x = x + outer_padding + 2;
    text_region.location.y = preview_y + preview_h + (compact ? 7 : 8);
    text_region.size.width = w - outer_padding * 2 - 4;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, footer_color, self->alpha);
}

static void get_zone_rects_local(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_region_t region;

    egui_view_get_work_region(self, &region);

    main_rect->location.x = region.location.x + 10;
    main_rect->location.y = region.location.y + 40;
    main_rect->size.width = 220;
    main_rect->size.height = 118;

    left_rect->location.x = region.location.x + 10;
    left_rect->location.y = region.location.y + 189;
    left_rect->size.width = 105;
    left_rect->size.height = 87;

    right_rect->location.x = region.location.x + 125;
    right_rect->location.y = region.location.y + 189;
    right_rect->size.width = 105;
    right_rect->size.height = 87;
}

static void get_zone_rects_screen(egui_view_t *self, egui_region_t *main_rect, egui_region_t *left_rect, egui_region_t *right_rect)
{
    egui_dim_t origin_x;
    egui_dim_t origin_y;

    origin_x = self->region_screen.location.x + self->padding.left;
    origin_y = self->region_screen.location.y + self->padding.top;

    main_rect->location.x = origin_x + 10;
    main_rect->location.y = origin_y + 40;
    main_rect->size.width = 220;
    main_rect->size.height = 118;

    left_rect->location.x = origin_x + 10;
    left_rect->location.y = origin_y + 189;
    left_rect->size.width = 105;
    left_rect->size.height = 87;

    right_rect->location.x = origin_x + 125;
    right_rect->location.y = origin_y + 189;
    right_rect->size.width = 105;
    right_rect->size.height = 87;
}

static int egui_view_signal_matrix_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
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

static void egui_view_signal_matrix_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_signal_matrix_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_signal_matrix_snapshot_t *primary;
    const egui_view_signal_matrix_snapshot_t *compact;
    const egui_view_signal_matrix_snapshot_t *locked;
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
    text_region.location.y = region.location.y + 1;
    text_region.size.width = region.size.width;
    text_region.size.height = 20;
    egui_canvas_draw_text_in_rect(title_font, "Signal Matrix", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 23;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to cycle signal scenes", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x8093B0), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_card(self, &primary_palette, primary, main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 0, 0);
    draw_card(self, &compact_palette, compact, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 1, 0);
    draw_card(self, &locked_palette, locked, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);

    if (local->last_zone == 1)
    {
        status_text = "Mesh set";
        status_color = compact_palette.accent;
        if (local->current_compact == 1)
        {
            status_text = "Load set";
            status_color = compact_palette.warn;
        }
        else if (local->current_compact == 2)
        {
            status_text = "Mesh safe";
            status_color = compact_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Lock set";
        status_color = locked_palette.accent;
        if (local->current_locked == 1)
        {
            status_text = "Hold set";
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
        status_text = "Grid live";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Scan set";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Guard safe";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 165;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(status_color, EGUI_COLOR_WHITE, 38), self->alpha);
    draw_round_fill_safe(region.location.x + 48, region.location.y + 186, 144, 2, 1, EGUI_COLOR_HEX(0x405571), egui_color_alpha_mix(self->alpha, 50));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_signal_matrix_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_signal_matrix_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_signal_matrix_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_signal_matrix_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_signal_matrix_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_signal_matrix_t);
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
