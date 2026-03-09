#include <stdlib.h>
#include <string.h>

#include "egui_view_range_band_editor.h"

typedef struct range_palette range_palette_t;
struct range_palette
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

static const range_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x151B28), EGUI_COLOR_HEX(0x1F2B3F), EGUI_COLOR_HEX(0x4D607D), EGUI_COLOR_HEX(0xF2F7FF), EGUI_COLOR_HEX(0xA0AEC4),
        EGUI_COLOR_HEX(0x67C7FF), EGUI_COLOR_HEX(0xF0B45E), EGUI_COLOR_HEX(0x91D4A4), EGUI_COLOR_HEX(0xA9E4FF),
};

static const range_palette_t compact_palette = {
        EGUI_COLOR_HEX(0x17231B), EGUI_COLOR_HEX(0x25382D), EGUI_COLOR_HEX(0x65967E), EGUI_COLOR_HEX(0xF1FFF7), EGUI_COLOR_HEX(0xABCBB8),
        EGUI_COLOR_HEX(0x77E0B6), EGUI_COLOR_HEX(0xE8BD67), EGUI_COLOR_HEX(0xA8D9BE), EGUI_COLOR_HEX(0xB8F2D6),
};

static const range_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x271C17), EGUI_COLOR_HEX(0x3D2D25), EGUI_COLOR_HEX(0xA5826C), EGUI_COLOR_HEX(0xFFF4EB), EGUI_COLOR_HEX(0xDEBCA7),
        EGUI_COLOR_HEX(0xF3C08B), EGUI_COLOR_HEX(0xF1B868), EGUI_COLOR_HEX(0xDBC896), EGUI_COLOR_HEX(0xFFE1C0),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_RANGE_BAND_EDITOR_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_RANGE_BAND_EDITOR_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_range_band_editor_set_primary_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_range_band_editor_set_compact_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
    local->compact_snapshots = snapshots;
    local->compact_snapshot_count = clamp_count(snapshot_count);
    if (local->current_compact >= local->compact_snapshot_count)
    {
        local->current_compact = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_range_band_editor_set_locked_snapshots(
        egui_view_t *self,
        const egui_view_range_band_editor_snapshot_t *snapshots,
        uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_range_band_editor_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
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

static egui_color_t get_status_color(const range_palette_t *palette, const egui_view_range_band_editor_snapshot_t *snapshot)
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

static egui_dim_t get_pill_width(const egui_view_range_band_editor_snapshot_t *snapshot, uint8_t compact)
{
    size_t len;
    egui_dim_t width;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(18 + len * 6) : (egui_dim_t)(22 + len * 6);
    if (compact)
    {
        if (width < 46)
        {
            width = 46;
        }
        if (width > 62)
        {
            width = 62;
        }
    }
    else
    {
        if (width < 58)
        {
            width = 58;
        }
        if (width > 76)
        {
            width = 76;
        }
    }
    return width;
}

static void draw_tick(
        egui_view_t *self,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        egui_color_t color,
        egui_alpha_t alpha)
{
    draw_round_fill_safe(x, y, w, h, w / 2, color, alpha);
}

static void draw_band_preview(
        egui_view_t *self,
        const range_palette_t *palette,
        const egui_view_range_band_editor_snapshot_t *snapshot,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t compact,
        uint8_t locked)
{
    egui_dim_t lane_y;
    egui_dim_t lane_h;
    egui_dim_t lane_x;
    egui_dim_t lane_w;
    egui_dim_t start_x;
    egui_dim_t end_x;
    egui_dim_t focus_x;
    egui_dim_t tick_gap;
    egui_dim_t tick_y;
    egui_color_t status_color;
    egui_color_t lane_color;
    uint8_t i;

    if (w <= 16 || h <= 12)
    {
        return;
    }

    draw_round_fill_safe(x, y, w, h, compact ? 6 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 34 : 30));
    draw_round_stroke_safe(x, y, w, h, compact ? 6 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 48 : 42));

    lane_x = x + (compact ? 10 : 14);
    lane_w = w - (compact ? 20 : 28);
    lane_h = compact ? 8 : 10;
    lane_y = y + h / 2 - lane_h / 2 + (compact ? 1 : 0);
    lane_color = egui_rgb_mix(palette->panel, palette->surface, locked ? 22 : 10);
    status_color = get_status_color(palette, snapshot);

    draw_round_fill_safe(lane_x, lane_y, lane_w, lane_h, lane_h / 2, lane_color, egui_color_alpha_mix(self->alpha, compact ? 66 : 70));
    draw_round_stroke_safe(lane_x, lane_y, lane_w, lane_h, lane_h / 2, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 44 : 52));

    start_x = lane_x + (lane_w * snapshot->range_start) / 100;
    end_x = lane_x + (lane_w * snapshot->range_end) / 100;
    if (end_x - start_x < (compact ? 16 : 22))
    {
        end_x = start_x + (compact ? 16 : 22);
    }
    if (end_x > lane_x + lane_w)
    {
        end_x = lane_x + lane_w;
    }
    focus_x = lane_x + (lane_w * snapshot->focus_tick) / 100;

    draw_round_fill_safe(start_x, lane_y - (compact ? 3 : 4), end_x - start_x, lane_h + (compact ? 6 : 8), (lane_h + 6) / 2, status_color, egui_color_alpha_mix(self->alpha, compact ? 76 : 84));
    draw_round_stroke_safe(start_x, lane_y - (compact ? 3 : 4), end_x - start_x, lane_h + (compact ? 6 : 8), (lane_h + 6) / 2, 1, palette->focus, egui_color_alpha_mix(self->alpha, compact ? 52 : 60));

    draw_round_fill_safe(start_x - 1, lane_y - (compact ? 5 : 6), compact ? 6 : 7, lane_h + (compact ? 10 : 12), compact ? 3 : 4, palette->text, egui_color_alpha_mix(self->alpha, compact ? 60 : 68));
    draw_round_fill_safe(end_x - (compact ? 5 : 6), lane_y - (compact ? 5 : 6), compact ? 6 : 7, lane_h + (compact ? 10 : 12), compact ? 3 : 4, palette->text, egui_color_alpha_mix(self->alpha, compact ? 60 : 68));

    tick_gap = lane_w / 6;
    tick_y = lane_y + lane_h + (compact ? 7 : 9);
    for (i = 0; i < 7; i++)
    {
        egui_dim_t tx;
        egui_color_t tick_color;
        egui_alpha_t tick_alpha;

        tx = lane_x + i * tick_gap;
        tick_color = palette->border;
        tick_alpha = egui_color_alpha_mix(self->alpha, compact ? 40 : 46);
        if (abs(tx - focus_x) <= tick_gap / 2)
        {
            tick_color = status_color;
            tick_alpha = egui_color_alpha_mix(self->alpha, compact ? 78 : 84);
        }
        draw_tick(self, tx, tick_y, compact ? 2 : 3, compact ? 6 : 8, tick_color, tick_alpha);
    }

    draw_round_fill_safe(focus_x - 1, lane_y - (compact ? 8 : 10), compact ? 4 : 5, lane_h + (compact ? 16 : 20), compact ? 2 : 3, palette->focus, egui_color_alpha_mix(self->alpha, compact ? 46 : 52));
}

static void draw_card(
        egui_view_t *self,
        const range_palette_t *palette,
        const egui_view_range_band_editor_snapshot_t *snapshot,
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

    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, palette->surface, compact ? 26 : 18);
    status_color = get_status_color(palette, snapshot);
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 20 : 38) : egui_rgb_mix(palette->muted, status_color, 30);
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 42)
                           : egui_rgb_mix(palette->text, palette->muted, compact ? 10 : 12);
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 40)
                          : egui_rgb_mix(palette->muted, palette->text, compact ? 30 : 38);
    outer_padding = compact ? 11 : 16;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w;
    title_w = pill_x - x - outer_padding - (compact ? 8 : 12);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    draw_round_fill_safe(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? 40 : 60));
    draw_round_stroke_safe(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 54 : 74));

    text_region.location.x = x + outer_padding + (compact ? 4 : 2);
    text_region.location.y = y + (compact ? 11 : 12);
    text_region.size.width = title_w - (compact ? 4 : 2);
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    draw_round_fill_safe(pill_x, y + (compact ? 11 : 12), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 40 : 66));
    text_region.location.x = pill_x + 2;
    text_region.location.y = y + (compact ? 11 : 12);
    text_region.size.width = pill_w - 4;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, palette->text, self->alpha);

    text_region.location.x = x + outer_padding + (compact ? 2 : 2);
    text_region.location.y = y + (compact ? 27 : 35);
    text_region.size.width = w - outer_padding * 2 - 4;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(card_font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    preview_x = x + outer_padding + (compact ? -1 : 1);
    preview_y = y + (compact ? 43 : 58);
    preview_w = w - outer_padding * 2 - (compact ? 2 : 6);
    preview_h = compact ? 20 : 36;
    draw_band_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact, locked);

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

    left_rect->location.x = region.location.x + 11;
    left_rect->location.y = region.location.y + 189;
    left_rect->size.width = 105;
    left_rect->size.height = 87;

    right_rect->location.x = region.location.x + 124;
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

    left_rect->location.x = origin_x + 11;
    left_rect->location.y = origin_y + 189;
    left_rect->size.width = 105;
    left_rect->size.height = 87;

    right_rect->location.x = origin_x + 124;
    right_rect->location.y = origin_y + 189;
    right_rect->size.width = 105;
    right_rect->size.height = 87;
}

static int egui_view_range_band_editor_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
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

static void egui_view_range_band_editor_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_range_band_editor_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_range_band_editor_snapshot_t *primary;
    const egui_view_range_band_editor_snapshot_t *compact;
    const egui_view_range_band_editor_snapshot_t *locked;
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
    egui_canvas_draw_text_in_rect(title_font, "Range Band Editor", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 22;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to shift band scenes", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x7687A3), self->alpha);

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
            status_text = "Load set";
            status_color = compact_palette.warn;
        }
        else if (local->current_compact == 2)
        {
            status_text = "Queue safe";
            status_color = compact_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Audit ready";
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
        status_text = "Band ready";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Mix set";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Guard safe";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 167;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, egui_rgb_mix(status_color, EGUI_COLOR_WHITE, 38), self->alpha);
    draw_round_fill_safe(region.location.x + 40, region.location.y + 184, 160, 2, 1, EGUI_COLOR_HEX(0x405571), egui_color_alpha_mix(self->alpha, 56));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_range_band_editor_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_range_band_editor_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_range_band_editor_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_range_band_editor_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_range_band_editor_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_range_band_editor_t);
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
