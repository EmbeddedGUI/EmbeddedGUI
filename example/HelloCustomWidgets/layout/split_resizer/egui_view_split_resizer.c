#include <stdlib.h>
#include <string.h>

#include "egui_view_split_resizer.h"

typedef struct split_palette split_palette_t;
struct split_palette
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

static const split_palette_t primary_palette = {
        EGUI_COLOR_HEX(0x101722), EGUI_COLOR_HEX(0x192333), EGUI_COLOR_HEX(0x415874), EGUI_COLOR_HEX(0xEDF5FF), EGUI_COLOR_HEX(0x95A9BF),
        EGUI_COLOR_HEX(0x67D7FF), EGUI_COLOR_HEX(0xF2B15E), EGUI_COLOR_HEX(0xC9A77D), EGUI_COLOR_HEX(0x9EF0FF),
};

static const split_palette_t column_palette = {
        EGUI_COLOR_HEX(0x122217), EGUI_COLOR_HEX(0x1A3225), EGUI_COLOR_HEX(0x478066), EGUI_COLOR_HEX(0xECFFF5), EGUI_COLOR_HEX(0x90B7A5),
        EGUI_COLOR_HEX(0x66E2B1), EGUI_COLOR_HEX(0xE1B35C), EGUI_COLOR_HEX(0xCAA880), EGUI_COLOR_HEX(0xA5F8D3),
};

static const split_palette_t locked_palette = {
        EGUI_COLOR_HEX(0x241913), EGUI_COLOR_HEX(0x36251D), EGUI_COLOR_HEX(0x765C47), EGUI_COLOR_HEX(0xFFF2E6), EGUI_COLOR_HEX(0xD2AF95),
        EGUI_COLOR_HEX(0xF6C688), EGUI_COLOR_HEX(0xF2B15E), EGUI_COLOR_HEX(0xC9A77D), EGUI_COLOR_HEX(0xFFDFB5),
};

static uint8_t clamp_count(uint8_t count)
{
    if (count > EGUI_VIEW_SPLIT_RESIZER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SPLIT_RESIZER_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_split_resizer_set_primary_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
    local->primary_snapshots = snapshots;
    local->primary_snapshot_count = clamp_count(snapshot_count);
    if (local->current_primary >= local->primary_snapshot_count)
    {
        local->current_primary = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_split_resizer_set_column_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
    local->column_snapshots = snapshots;
    local->column_snapshot_count = clamp_count(snapshot_count);
    if (local->current_column >= local->column_snapshot_count)
    {
        local->current_column = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_split_resizer_set_locked_snapshots(egui_view_t *self, const egui_view_split_resizer_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
    local->locked_snapshots = snapshots;
    local->locked_snapshot_count = clamp_count(snapshot_count);
    if (local->current_locked >= local->locked_snapshot_count)
    {
        local->current_locked = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_split_resizer_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

static egui_color_t get_status_color(const split_palette_t *palette, const egui_view_split_resizer_snapshot_t *snapshot)
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

static egui_dim_t get_pill_width(const egui_view_split_resizer_snapshot_t *snapshot, uint8_t compact)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact ? (egui_dim_t)(14 + len * 5) : (egui_dim_t)(16 + len * 6);
    if (compact)
    {
        if (width < 34)
        {
            width = 34;
        }
        if (width > 44)
        {
            width = 44;
        }
    }
    else
    {
        if (width < 42)
        {
            width = 42;
        }
        if (width > 60)
        {
            width = 60;
        }
    }
    return width;
}

static void draw_grip(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, uint8_t vertical, egui_color_t color)
{
    egui_dim_t dot_x;
    egui_dim_t dot_y;
    uint8_t i;

    for (i = 0; i < 3; i++)
    {
        if (vertical)
        {
            dot_x = x + w / 2 - 1;
            dot_y = y + h / 2 - 6 + i * 5;
        }
        else
        {
            dot_x = x + w / 2 - 6 + i * 5;
            dot_y = y + h / 2 - 1;
        }
        egui_canvas_draw_round_rectangle_fill(dot_x, dot_y, 3, 3, 2, color, egui_color_alpha_mix(self->alpha, 96));
    }
}

static void draw_round_fill_safe(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_color_t color,
                                 egui_alpha_t alpha)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle_fill(x, y, w, h, radius, color, alpha);
}

static void draw_round_stroke_safe(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, egui_dim_t radius, egui_dim_t stroke_width,
                                   egui_color_t color, egui_alpha_t alpha)
{
    EGUI_UNUSED(self);
    if (w <= 0 || h <= 0)
    {
        return;
    }
    egui_canvas_draw_round_rectangle(x, y, w, h, radius, stroke_width, color, alpha);
}

static void resolve_segments(egui_dim_t total, uint8_t primary_ratio, uint8_t secondary_ratio, egui_dim_t min_segment, egui_dim_t sizes[3])
{
    egui_dim_t sum;
    int max_index;

    if (total <= 0)
    {
        sizes[0] = 0;
        sizes[1] = 0;
        sizes[2] = 0;
        return;
    }

    if (min_segment < 1)
    {
        min_segment = 1;
    }

    if (total < min_segment * 3)
    {
        sizes[0] = total / 3;
        sizes[1] = total / 3;
        sizes[2] = total - sizes[0] - sizes[1];
        return;
    }

    sizes[0] = (total * primary_ratio) / 100;
    sizes[1] = (total * secondary_ratio) / 100;
    sizes[2] = total - sizes[0] - sizes[1];

    if (sizes[0] < min_segment)
    {
        sizes[0] = min_segment;
    }
    if (sizes[1] < min_segment)
    {
        sizes[1] = min_segment;
    }
    if (sizes[2] < min_segment)
    {
        sizes[2] = min_segment;
    }

    sum = sizes[0] + sizes[1] + sizes[2];
    while (sum > total)
    {
        max_index = 0;
        if (sizes[1] > sizes[max_index])
        {
            max_index = 1;
        }
        if (sizes[2] > sizes[max_index])
        {
            max_index = 2;
        }
        if (sizes[max_index] <= min_segment)
        {
            break;
        }
        sizes[max_index]--;
        sum--;
    }

    while (sum < total)
    {
        sizes[0]++;
        sum++;
    }
}

static void draw_layout_preview(egui_view_t *self, const split_palette_t *palette, const egui_view_split_resizer_snapshot_t *snapshot, egui_dim_t x,
                                egui_dim_t y, egui_dim_t w, egui_dim_t h, uint8_t compact)
{
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t separator;
    egui_dim_t panel_radius;
    egui_dim_t sizes[3];
    egui_dim_t min_segment;
    egui_dim_t primary_x;
    egui_dim_t secondary_x;
    egui_dim_t tertiary_x;
    egui_dim_t primary_y;
    egui_dim_t secondary_y;
    egui_dim_t tertiary_y;
    egui_color_t status_color;
    egui_color_t chrome_color;

    status_color = get_status_color(palette, snapshot);
    chrome_color = egui_rgb_mix(palette->panel, palette->surface, 18);
    separator = compact ? 3 : 6;
    panel_radius = compact ? 4 : 4;
    min_segment = compact ? 3 : 6;

    draw_round_fill_safe(self, x, y, w, h, compact ? 5 : 7, palette->surface, egui_color_alpha_mix(self->alpha, compact ? 34 : 30));
    draw_round_stroke_safe(self, x, y, w, h, compact ? 5 : 7, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 38 : 30));
    draw_round_fill_safe(self, x + 4, y + 4, w - 8, 2, 1, palette->text, egui_color_alpha_mix(self->alpha, compact ? 28 : 24));

    content_x = x + 5;
    content_y = y + (compact ? 6 : 8);
    content_w = w - 10;
    content_h = h - (compact ? 10 : 14);
    if (content_w <= 0 || content_h <= 0)
    {
        return;
    }

    if (snapshot->vertical_mode)
    {
        if (content_h <= separator * 2)
        {
            return;
        }
        resolve_segments(content_h - separator * 2, snapshot->primary_ratio, snapshot->secondary_ratio, min_segment, sizes);
        primary_y = content_y;
        secondary_y = primary_y + sizes[0] + separator;
        tertiary_y = secondary_y + sizes[1] + separator;

        draw_round_fill_safe(self, content_x, primary_y, content_w, sizes[0], panel_radius, status_color, egui_color_alpha_mix(self->alpha, 60));
        draw_round_fill_safe(self, content_x, secondary_y, content_w, sizes[1], panel_radius, chrome_color, egui_color_alpha_mix(self->alpha, 54));
        draw_round_fill_safe(self, content_x, tertiary_y, content_w, sizes[2], panel_radius, chrome_color, egui_color_alpha_mix(self->alpha, 32));
        draw_round_fill_safe(self, content_x + 3, primary_y + sizes[0], content_w - 6, separator, separator, palette->panel,
                             egui_color_alpha_mix(self->alpha, 88));
        draw_grip(self, content_x + 3, primary_y + sizes[0], content_w - 6, separator, 0, palette->focus);
    }
    else
    {
        if (content_w <= separator * 2)
        {
            return;
        }
        resolve_segments(content_w - separator * 2, snapshot->primary_ratio, snapshot->secondary_ratio, compact ? 8 : 12, sizes);
        primary_x = content_x;
        secondary_x = primary_x + sizes[0] + separator;
        tertiary_x = secondary_x + sizes[1] + separator;

        draw_round_fill_safe(self, primary_x, content_y, sizes[0], content_h, panel_radius, status_color, egui_color_alpha_mix(self->alpha, 60));
        draw_round_fill_safe(self, secondary_x, content_y, sizes[1], content_h, panel_radius, chrome_color, egui_color_alpha_mix(self->alpha, 54));
        draw_round_fill_safe(self, tertiary_x, content_y, sizes[2], content_h, panel_radius, chrome_color, egui_color_alpha_mix(self->alpha, 32));
        draw_round_fill_safe(self, primary_x + sizes[0], content_y + 2, separator, content_h - 4, separator, palette->panel,
                             egui_color_alpha_mix(self->alpha, compact ? 88 : 94));
        draw_grip(self, primary_x + sizes[0], content_y + 2, separator, content_h - 4, 1, palette->focus);
    }
}

static void draw_card(egui_view_t *self, const split_palette_t *palette, const egui_view_split_resizer_snapshot_t *snapshot, egui_dim_t x, egui_dim_t y,
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

    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, palette->surface, 28);
    status_color = get_status_color(palette, snapshot);
    title_color = compact ? egui_rgb_mix(palette->muted, status_color, locked ? 30 : 50) : palette->muted;
    summary_color = locked ? egui_rgb_mix(palette->text, palette->muted, 38) : (compact ? palette->text : egui_rgb_mix(palette->text, palette->muted, 10));
    footer_color = locked ? egui_rgb_mix(palette->muted, palette->border, 34)
                          : (compact ? egui_rgb_mix(palette->muted, palette->text, 16) : egui_rgb_mix(palette->muted, palette->text, 24));
    outer_padding = compact ? 12 : 15;
    pill_w = get_pill_width(snapshot, compact);
    pill_x = x + w - outer_padding - pill_w;
    title_w = pill_x - x - outer_padding - (compact ? 10 : 16);
    card_font = compact ? (const egui_font_t *)&egui_res_font_montserrat_8_4 : (const egui_font_t *)&egui_res_font_montserrat_10_4;

    egui_canvas_draw_round_rectangle_fill(x, y, w, h, 10, shell_color, egui_color_alpha_mix(self->alpha, compact ? 40 : 56));
    egui_canvas_draw_round_rectangle(x, y, w, h, 10, 1, palette->border, egui_color_alpha_mix(self->alpha, compact ? 52 : 69));

    text_region.location.x = x + outer_padding;
    text_region.location.y = y + (compact ? 9 : 10);
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(card_font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(pill_x, y + (compact ? 9 : 10), pill_w, 11, 5, status_color, egui_color_alpha_mix(self->alpha, locked ? 40 : 64));
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

    preview_x = x + outer_padding + (compact ? 1 : 5);
    preview_y = y + (compact ? 35 : 49);
    preview_w = w - outer_padding * 2 - (compact ? 8 : 16);
    preview_h = compact ? 31 : 43;
    draw_layout_preview(self, palette, snapshot, preview_x, preview_y, preview_w, preview_h, compact);

    text_region.location.x = x + outer_padding + (compact ? 2 : 4);
    text_region.location.y = preview_y + preview_h + (compact ? 5 : 10);
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

static int egui_view_split_resizer_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
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
    if (egui_region_pt_in_rect(&left_rect, event->location.x, event->location.y) && local->column_snapshot_count > 0)
    {
        local->current_column = (local->current_column + 1) % local->column_snapshot_count;
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

static void egui_view_split_resizer_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_resizer_t);
    egui_region_t region;
    egui_region_t text_region;
    egui_region_t main_rect;
    egui_region_t left_rect;
    egui_region_t right_rect;
    const egui_view_split_resizer_snapshot_t *primary;
    const egui_view_split_resizer_snapshot_t *column;
    const egui_view_split_resizer_snapshot_t *locked;
    const char *status_text;
    egui_color_t status_color;
    const egui_font_t *title_font;
    const egui_font_t *guide_font;
    const egui_font_t *status_font;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->primary_snapshots == NULL || local->column_snapshots == NULL ||
        local->locked_snapshots == NULL || local->primary_snapshot_count == 0 || local->column_snapshot_count == 0 || local->locked_snapshot_count == 0)
    {
        return;
    }

    primary = &local->primary_snapshots[local->current_primary];
    column = &local->column_snapshots[local->current_column];
    locked = &local->locked_snapshots[local->current_locked];
    title_font = (const egui_font_t *)&egui_res_font_montserrat_12_4;
    guide_font = (const egui_font_t *)&egui_res_font_montserrat_10_4;
    status_font = (const egui_font_t *)&egui_res_font_montserrat_8_4;

    text_region.location.x = region.location.x;
    text_region.location.y = region.location.y + 2;
    text_region.size.width = region.size.width;
    text_region.size.height = 18;
    egui_canvas_draw_text_in_rect(title_font, "Split Resizer", &text_region, EGUI_ALIGN_CENTER, primary_palette.accent, self->alpha);

    text_region.location.y = region.location.y + 21;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(guide_font, "Tap cards to cycle layouts", &text_region, EGUI_ALIGN_CENTER, EGUI_COLOR_HEX(0x687D95), self->alpha);

    get_zone_rects_local(self, &main_rect, &left_rect, &right_rect);
    draw_card(self, &primary_palette, primary, main_rect.location.x, main_rect.location.y, main_rect.size.width, main_rect.size.height, 0, 0);
    draw_card(self, &column_palette, column, left_rect.location.x, left_rect.location.y, left_rect.size.width, left_rect.size.height, 1, 0);
    draw_card(self, &locked_palette, locked, right_rect.location.x, right_rect.location.y, right_rect.size.width, right_rect.size.height, 1, 1);

    if (local->last_zone == 1)
    {
        status_text = "Column set ready";
        status_color = column_palette.accent;
        if (local->current_column == 1)
        {
            status_text = "Column scan live";
            status_color = column_palette.warn;
        }
        else if (local->current_column == 2)
        {
            status_text = "Column stack safe";
            status_color = column_palette.lock;
        }
    }
    else if (local->last_zone == 2)
    {
        status_text = "Grid render live";
        status_color = locked_palette.accent;
        if (local->current_locked == 1)
        {
            status_text = "Grid warn live";
            status_color = locked_palette.warn;
        }
        else if (local->current_locked == 2)
        {
            status_text = "Grid archive safe";
            status_color = locked_palette.lock;
        }
    }
    else
    {
        status_text = "Primary split ready";
        status_color = primary_palette.accent;
        if (local->current_primary == 1)
        {
            status_text = "Primary focus live";
            status_color = primary_palette.warn;
        }
        else if (local->current_primary == 2)
        {
            status_text = "Primary split safe";
            status_color = primary_palette.lock;
        }
    }

    text_region.location.y = region.location.y + 157;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(status_font, status_text, &text_region, EGUI_ALIGN_CENTER, status_color, self->alpha);

    draw_round_fill_safe(self, region.location.x + 38, region.location.y + 172, 164, 2, 1, EGUI_COLOR_HEX(0x2B4155), egui_color_alpha_mix(self->alpha, 64));
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_split_resizer_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_split_resizer_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_split_resizer_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_split_resizer_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_split_resizer_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_split_resizer_t);
    self->is_clickable = true;
    egui_view_set_padding_all(self, 0);

    local->primary_snapshots = NULL;
    local->column_snapshots = NULL;
    local->locked_snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->primary_snapshot_count = 0;
    local->column_snapshot_count = 0;
    local->locked_snapshot_count = 0;
    local->current_primary = 0;
    local->current_column = 0;
    local->current_locked = 0;
    local->last_zone = 0;
}
