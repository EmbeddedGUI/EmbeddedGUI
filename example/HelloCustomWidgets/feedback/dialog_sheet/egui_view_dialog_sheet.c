#include "egui_view_dialog_sheet.h"

#define DIALOG_SHEET_STANDARD_RADIUS            12
#define DIALOG_SHEET_STANDARD_OVERLAY_ALPHA     24
#define DIALOG_SHEET_STANDARD_BORDER_ALPHA      52
#define DIALOG_SHEET_STANDARD_SHEET_TOP_GAP     14
#define DIALOG_SHEET_STANDARD_SHEET_SIDE_GAP    4
#define DIALOG_SHEET_STANDARD_SHEET_BOTTOM_GAP  4
#define DIALOG_SHEET_STANDARD_PAD_X             10
#define DIALOG_SHEET_STANDARD_HANDLE_W          28
#define DIALOG_SHEET_STANDARD_HANDLE_H          4
#define DIALOG_SHEET_STANDARD_HEADER_H          32
#define DIALOG_SHEET_STANDARD_BODY_GAP          6
#define DIALOG_SHEET_STANDARD_FOOTER_H          16
#define DIALOG_SHEET_STANDARD_ACTION_H          15
#define DIALOG_SHEET_STANDARD_ACTION_GAP        4
#define DIALOG_SHEET_STANDARD_TAG_H             10
#define DIALOG_SHEET_STANDARD_HERO_SIZE         24
#define DIALOG_SHEET_STANDARD_CLOSE_SIZE        10
#define DIALOG_SHEET_STANDARD_MIN_FOOTER_TEXT_W 24

#define DIALOG_SHEET_COMPACT_RADIUS            8
#define DIALOG_SHEET_COMPACT_OVERLAY_ALPHA     20
#define DIALOG_SHEET_COMPACT_BORDER_ALPHA      52
#define DIALOG_SHEET_COMPACT_SHEET_TOP_GAP     8
#define DIALOG_SHEET_COMPACT_SHEET_SIDE_GAP    2
#define DIALOG_SHEET_COMPACT_SHEET_BOTTOM_GAP  2
#define DIALOG_SHEET_COMPACT_PAD_X             6
#define DIALOG_SHEET_COMPACT_HANDLE_W          18
#define DIALOG_SHEET_COMPACT_HANDLE_H          3
#define DIALOG_SHEET_COMPACT_HEADER_H          22
#define DIALOG_SHEET_COMPACT_BODY_GAP          3
#define DIALOG_SHEET_COMPACT_FOOTER_H          12
#define DIALOG_SHEET_COMPACT_ACTION_H          11
#define DIALOG_SHEET_COMPACT_ACTION_GAP        4
#define DIALOG_SHEET_COMPACT_TAG_H             8
#define DIALOG_SHEET_COMPACT_HERO_SIZE         16
#define DIALOG_SHEET_COMPACT_CLOSE_SIZE        0
#define DIALOG_SHEET_COMPACT_MIN_FOOTER_TEXT_W 14

typedef struct egui_view_dialog_sheet_metrics egui_view_dialog_sheet_metrics_t;
struct egui_view_dialog_sheet_metrics
{
    egui_region_t backdrop_region;
    egui_region_t sheet_region;
    egui_region_t handle_region;
    egui_region_t header_region;
    egui_region_t hero_region;
    egui_region_t eyebrow_region;
    egui_region_t title_region;
    egui_region_t close_region;
    egui_region_t body_region;
    egui_region_t footer_region;
    egui_region_t tag_region;
    egui_region_t footer_text_region;
    egui_region_t secondary_action_region;
    egui_region_t primary_action_region;
};

static void egui_view_dialog_sheet_zero_region(egui_region_t *region)
{
    region->location.x = 0;
    region->location.y = 0;
    region->size.width = 0;
    region->size.height = 0;
}

static uint8_t egui_view_dialog_sheet_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_DIALOG_SHEET_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_DIALOG_SHEET_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_dialog_sheet_text_len(const char *text)
{
    uint8_t length = 0;

    if (text == NULL)
    {
        return 0;
    }

    while (text[length] != '\0')
    {
        length++;
    }

    return length;
}

static egui_color_t egui_view_dialog_sheet_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_dialog_sheet_tone_color(egui_view_dialog_sheet_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_DIALOG_SHEET_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_DIALOG_SHEET_TONE_ERROR:
        return local->error_color;
    case EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const char *egui_view_dialog_sheet_tone_glyph(uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_DIALOG_SHEET_TONE_SUCCESS:
        return "+";
    case EGUI_VIEW_DIALOG_SHEET_TONE_WARNING:
        return "!";
    case EGUI_VIEW_DIALOG_SHEET_TONE_ERROR:
        return "x";
    case EGUI_VIEW_DIALOG_SHEET_TONE_NEUTRAL:
        return "o";
    default:
        return "i";
    }
}

static const egui_view_dialog_sheet_snapshot_t *egui_view_dialog_sheet_get_snapshot(egui_view_dialog_sheet_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static uint8_t egui_view_dialog_sheet_has_primary(const egui_view_dialog_sheet_snapshot_t *snapshot)
{
    return snapshot != NULL && snapshot->primary_action != NULL && snapshot->primary_action[0] != '\0';
}

static uint8_t egui_view_dialog_sheet_has_secondary(const egui_view_dialog_sheet_snapshot_t *snapshot)
{
    return snapshot != NULL && snapshot->show_secondary && snapshot->secondary_action != NULL && snapshot->secondary_action[0] != '\0';
}

static uint8_t egui_view_dialog_sheet_normalize_action(const egui_view_dialog_sheet_snapshot_t *snapshot, uint8_t action_index)
{
    if (action_index == EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY && egui_view_dialog_sheet_has_secondary(snapshot))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY;
    }

    if (action_index == EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY && egui_view_dialog_sheet_has_primary(snapshot))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY;
    }

    if (egui_view_dialog_sheet_has_primary(snapshot))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY;
    }

    if (egui_view_dialog_sheet_has_secondary(snapshot))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY;
    }

    return EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
}

static egui_dim_t egui_view_dialog_sheet_pill_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t min_w = compact_mode ? 16 : 20;
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = min_w + egui_view_dialog_sheet_text_len(text) * (compact_mode ? 4 : 4);
    if (width > max_w)
    {
        width = max_w;
    }

    return width;
}

static egui_dim_t egui_view_dialog_sheet_button_width(const char *text, uint8_t compact_mode, egui_dim_t max_w)
{
    egui_dim_t min_w = compact_mode ? 22 : 26;
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = min_w + egui_view_dialog_sheet_text_len(text) * (compact_mode ? 4 : 4);
    if (width > max_w)
    {
        width = max_w;
    }

    return width;
}

static void egui_view_dialog_sheet_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                             egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0' || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_dialog_sheet_draw_button(egui_view_dialog_sheet_t *local, egui_view_t *self, const egui_region_t *region, const char *text,
                                               egui_color_t tone_color, egui_color_t border_color, egui_color_t idle_text_color, uint8_t focused,
                                               uint8_t pressed, uint8_t enabled)
{
    egui_color_t fill_color;
    egui_color_t draw_border;
    egui_color_t text_color;
    egui_alpha_t fill_alpha;
    egui_alpha_t border_alpha;
    egui_dim_t radius = local->compact_mode ? 6 : 7;

    if (region->size.width <= 0 || region->size.height <= 0 || text == NULL || text[0] == '\0')
    {
        return;
    }

    fill_color = egui_rgb_mix(local->surface_color, border_color, local->compact_mode ? 6 : 8);
    draw_border = egui_rgb_mix(border_color, tone_color, local->compact_mode ? 8 : 10);
    text_color = idle_text_color;
    fill_alpha = local->compact_mode ? 42 : 46;
    border_alpha = local->compact_mode ? 36 : 40;

    if (focused)
    {
        fill_color = egui_rgb_mix(tone_color, local->surface_color, local->compact_mode ? 18 : 14);
        draw_border = tone_color;
        text_color = local->surface_color;
        fill_alpha = local->locked_mode ? 40 : 94;
        border_alpha = local->locked_mode ? 44 : 98;
        if (local->locked_mode)
        {
            fill_color = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 8 : 10);
            text_color = tone_color;
            fill_alpha = local->compact_mode ? 22 : 26;
            border_alpha = local->compact_mode ? 30 : 34;
        }
    }

    if (pressed)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_BLACK, 12);
        draw_border = egui_rgb_mix(draw_border, EGUI_COLOR_BLACK, 8);
    }

    if (!enabled)
    {
        fill_color = egui_view_dialog_sheet_mix_disabled(fill_color);
        draw_border = egui_view_dialog_sheet_mix_disabled(draw_border);
        text_color = egui_view_dialog_sheet_mix_disabled(text_color);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, draw_border,
                                     egui_color_alpha_mix(self->alpha, border_alpha));
    egui_view_dialog_sheet_draw_text(local->meta_font, self, text, region, EGUI_ALIGN_CENTER, text_color);
}

static void egui_view_dialog_sheet_get_metrics(egui_view_dialog_sheet_t *local, egui_view_t *self, uint8_t show_secondary, uint8_t show_close,
                                               egui_view_dialog_sheet_metrics_t *metrics)
{
    const egui_view_dialog_sheet_snapshot_t *snapshot = egui_view_dialog_sheet_get_snapshot(local);
    egui_region_t region;
    egui_dim_t sheet_top_gap = local->compact_mode ? DIALOG_SHEET_COMPACT_SHEET_TOP_GAP : DIALOG_SHEET_STANDARD_SHEET_TOP_GAP;
    egui_dim_t sheet_side_gap = local->compact_mode ? DIALOG_SHEET_COMPACT_SHEET_SIDE_GAP : DIALOG_SHEET_STANDARD_SHEET_SIDE_GAP;
    egui_dim_t sheet_bottom_gap = local->compact_mode ? DIALOG_SHEET_COMPACT_SHEET_BOTTOM_GAP : DIALOG_SHEET_STANDARD_SHEET_BOTTOM_GAP;
    egui_dim_t pad_x = local->compact_mode ? DIALOG_SHEET_COMPACT_PAD_X : DIALOG_SHEET_STANDARD_PAD_X;
    egui_dim_t handle_w = local->compact_mode ? DIALOG_SHEET_COMPACT_HANDLE_W : DIALOG_SHEET_STANDARD_HANDLE_W;
    egui_dim_t handle_h = local->compact_mode ? DIALOG_SHEET_COMPACT_HANDLE_H : DIALOG_SHEET_STANDARD_HANDLE_H;
    egui_dim_t header_h = local->compact_mode ? DIALOG_SHEET_COMPACT_HEADER_H : DIALOG_SHEET_STANDARD_HEADER_H;
    egui_dim_t body_gap = local->compact_mode ? DIALOG_SHEET_COMPACT_BODY_GAP : DIALOG_SHEET_STANDARD_BODY_GAP;
    egui_dim_t footer_h = local->compact_mode ? DIALOG_SHEET_COMPACT_FOOTER_H : DIALOG_SHEET_STANDARD_FOOTER_H;
    egui_dim_t action_h = local->compact_mode ? DIALOG_SHEET_COMPACT_ACTION_H : DIALOG_SHEET_STANDARD_ACTION_H;
    egui_dim_t action_gap = local->compact_mode ? DIALOG_SHEET_COMPACT_ACTION_GAP : DIALOG_SHEET_STANDARD_ACTION_GAP;
    egui_dim_t tag_h = local->compact_mode ? DIALOG_SHEET_COMPACT_TAG_H : DIALOG_SHEET_STANDARD_TAG_H;
    egui_dim_t hero_size = local->compact_mode ? DIALOG_SHEET_COMPACT_HERO_SIZE : DIALOG_SHEET_STANDARD_HERO_SIZE;
    egui_dim_t close_size = show_close ? (local->compact_mode ? DIALOG_SHEET_COMPACT_CLOSE_SIZE : DIALOG_SHEET_STANDARD_CLOSE_SIZE) : 0;
    egui_dim_t min_footer_text_w = local->compact_mode ? DIALOG_SHEET_COMPACT_MIN_FOOTER_TEXT_W : DIALOG_SHEET_STANDARD_MIN_FOOTER_TEXT_W;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t text_x;
    egui_dim_t primary_w;
    egui_dim_t secondary_w = 0;
    egui_dim_t max_button_w;
    egui_dim_t footer_text_start;
    egui_dim_t footer_text_end;
    egui_dim_t tag_w;

    egui_view_get_work_region(self, &region);
    metrics->backdrop_region = region;
    egui_view_dialog_sheet_zero_region(&metrics->sheet_region);
    egui_view_dialog_sheet_zero_region(&metrics->handle_region);
    egui_view_dialog_sheet_zero_region(&metrics->header_region);
    egui_view_dialog_sheet_zero_region(&metrics->hero_region);
    egui_view_dialog_sheet_zero_region(&metrics->eyebrow_region);
    egui_view_dialog_sheet_zero_region(&metrics->title_region);
    egui_view_dialog_sheet_zero_region(&metrics->close_region);
    egui_view_dialog_sheet_zero_region(&metrics->body_region);
    egui_view_dialog_sheet_zero_region(&metrics->footer_region);
    egui_view_dialog_sheet_zero_region(&metrics->tag_region);
    egui_view_dialog_sheet_zero_region(&metrics->footer_text_region);
    egui_view_dialog_sheet_zero_region(&metrics->secondary_action_region);
    egui_view_dialog_sheet_zero_region(&metrics->primary_action_region);

    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    metrics->sheet_region.location.x = region.location.x + sheet_side_gap;
    metrics->sheet_region.location.y = region.location.y + sheet_top_gap;
    metrics->sheet_region.size.width = region.size.width - sheet_side_gap * 2;
    metrics->sheet_region.size.height = region.size.height - sheet_top_gap - sheet_bottom_gap;
    if (metrics->sheet_region.size.width <= 0 || metrics->sheet_region.size.height <= 0)
    {
        egui_view_dialog_sheet_zero_region(&metrics->sheet_region);
        return;
    }

    metrics->handle_region.location.x = metrics->sheet_region.location.x + (metrics->sheet_region.size.width - handle_w) / 2;
    metrics->handle_region.location.y = metrics->sheet_region.location.y + (local->compact_mode ? 4 : 6);
    metrics->handle_region.size.width = handle_w;
    metrics->handle_region.size.height = handle_h;

    content_x = metrics->sheet_region.location.x + pad_x;
    content_y = metrics->handle_region.location.y + handle_h + (local->compact_mode ? 4 : 6);
    content_w = metrics->sheet_region.size.width - pad_x * 2;

    metrics->header_region.location.x = content_x;
    metrics->header_region.location.y = content_y;
    metrics->header_region.size.width = content_w;
    metrics->header_region.size.height = header_h;

    metrics->footer_region.location.x = content_x;
    metrics->footer_region.location.y = metrics->sheet_region.location.y + metrics->sheet_region.size.height - (local->compact_mode ? 6 : 8) - footer_h;
    metrics->footer_region.size.width = content_w;
    metrics->footer_region.size.height = footer_h;

    metrics->body_region.location.x = content_x;
    metrics->body_region.location.y = metrics->header_region.location.y + metrics->header_region.size.height + body_gap;
    metrics->body_region.size.width = content_w;
    metrics->body_region.size.height = metrics->footer_region.location.y - body_gap - metrics->body_region.location.y;
    if (metrics->body_region.size.height < (local->compact_mode ? 10 : 14))
    {
        metrics->body_region.size.height = local->compact_mode ? 10 : 14;
    }

    metrics->hero_region.location.x = content_x;
    metrics->hero_region.location.y = metrics->header_region.location.y + (local->compact_mode ? 6 : 8);
    metrics->hero_region.size.width = hero_size;
    metrics->hero_region.size.height = hero_size;

    text_x = metrics->hero_region.location.x + hero_size + (local->compact_mode ? 5 : 7);
    if (show_close && close_size > 0)
    {
        metrics->close_region.location.x = content_x + content_w - close_size;
        metrics->close_region.location.y = metrics->header_region.location.y + (local->compact_mode ? 8 : 10);
        metrics->close_region.size.width = close_size;
        metrics->close_region.size.height = close_size;
    }

    metrics->eyebrow_region.location.x = text_x;
    metrics->eyebrow_region.location.y = metrics->header_region.location.y;
    metrics->eyebrow_region.size.width = content_x + content_w - text_x - (show_close && close_size > 0 ? close_size + 6 : 0);
    metrics->eyebrow_region.size.height = local->compact_mode ? 7 : 8;

    metrics->title_region.location.x = text_x;
    metrics->title_region.location.y = metrics->hero_region.location.y + (local->compact_mode ? 1 : 0);
    metrics->title_region.size.width = content_x + content_w - text_x - (show_close && close_size > 0 ? close_size + 6 : 0);
    metrics->title_region.size.height = hero_size;

    max_button_w = metrics->footer_region.size.width / (show_secondary ? 3 : 2);
    primary_w = egui_view_dialog_sheet_button_width(snapshot == NULL ? "" : snapshot->primary_action, local->compact_mode, max_button_w);
    if (show_secondary)
    {
        secondary_w = egui_view_dialog_sheet_button_width(snapshot == NULL ? "" : snapshot->secondary_action, local->compact_mode, max_button_w);
    }

    metrics->primary_action_region.size.width = primary_w;
    metrics->primary_action_region.size.height = action_h;
    metrics->primary_action_region.location.x = metrics->footer_region.location.x + metrics->footer_region.size.width - primary_w;
    metrics->primary_action_region.location.y = metrics->footer_region.location.y + (metrics->footer_region.size.height - action_h) / 2;

    if (show_secondary)
    {
        metrics->secondary_action_region.size.width = secondary_w;
        metrics->secondary_action_region.size.height = action_h;
        metrics->secondary_action_region.location.x = metrics->primary_action_region.location.x - action_gap - secondary_w;
        metrics->secondary_action_region.location.y = metrics->primary_action_region.location.y;
    }

    tag_w = egui_view_dialog_sheet_pill_width(snapshot == NULL ? "" : snapshot->tag, local->compact_mode, metrics->footer_region.size.width / 3);
    if (tag_w > 0)
    {
        metrics->tag_region.location.x = metrics->footer_region.location.x;
        metrics->tag_region.location.y = metrics->footer_region.location.y + (metrics->footer_region.size.height - tag_h) / 2;
        metrics->tag_region.size.width = tag_w;
        metrics->tag_region.size.height = tag_h;
        if (primary_w == 0 && secondary_w == 0 && (snapshot == NULL || snapshot->footer == NULL || snapshot->footer[0] == '\0'))
        {
            metrics->tag_region.location.x = metrics->footer_region.location.x + (metrics->footer_region.size.width - tag_w) / 2;
        }
    }

    footer_text_start = metrics->tag_region.size.width > 0 ? metrics->tag_region.location.x + metrics->tag_region.size.width + action_gap
                                                           : metrics->footer_region.location.x;
    footer_text_end = (show_secondary ? metrics->secondary_action_region.location.x : metrics->primary_action_region.location.x) - action_gap;
    if (footer_text_end - footer_text_start >= min_footer_text_w)
    {
        metrics->footer_text_region.location.x = footer_text_start;
        metrics->footer_text_region.location.y = metrics->footer_region.location.y;
        metrics->footer_text_region.size.width = footer_text_end - footer_text_start;
        metrics->footer_text_region.size.height = metrics->footer_region.size.height;
    }
}

static uint8_t egui_view_dialog_sheet_hit_action(egui_view_dialog_sheet_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_dialog_sheet_metrics_t metrics;
    const egui_view_dialog_sheet_snapshot_t *snapshot = egui_view_dialog_sheet_get_snapshot(local);
    uint8_t show_secondary;
    uint8_t show_close;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    }

    show_secondary = egui_view_dialog_sheet_has_secondary(snapshot);
    show_close = snapshot->show_close && !local->compact_mode && !local->locked_mode;
    egui_view_dialog_sheet_get_metrics(local, self, show_secondary, show_close, &metrics);

    if (show_secondary && egui_region_pt_in_rect(&metrics.secondary_action_region, x, y))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY;
    }

    if (egui_region_pt_in_rect(&metrics.primary_action_region, x, y))
    {
        return EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY;
    }

    return EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
}

static void egui_view_dialog_sheet_set_current_action_inner(egui_view_t *self, uint8_t action_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    const egui_view_dialog_sheet_snapshot_t *snapshot = egui_view_dialog_sheet_get_snapshot(local);
    uint8_t normalized_action = egui_view_dialog_sheet_normalize_action(snapshot, action_index);

    if (local->current_action == normalized_action)
    {
        return;
    }

    local->current_action = normalized_action;
    local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    if (notify && local->on_action_changed != NULL)
    {
        local->on_action_changed(self, normalized_action);
    }
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_snapshots(egui_view_t *self, const egui_view_dialog_sheet_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    const egui_view_dialog_sheet_snapshot_t *snapshot;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_dialog_sheet_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    snapshot = egui_view_dialog_sheet_get_snapshot(local);
    local->current_action = egui_view_dialog_sheet_normalize_action(snapshot, snapshot == NULL ? EGUI_VIEW_DIALOG_SHEET_ACTION_NONE : snapshot->focus_action);
    local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    const egui_view_dialog_sheet_snapshot_t *snapshot;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_dialog_sheet_get_snapshot(local);
    local->current_action = egui_view_dialog_sheet_normalize_action(snapshot, snapshot == NULL ? EGUI_VIEW_DIALOG_SHEET_ACTION_NONE : snapshot->focus_action);
    local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    egui_view_invalidate(self);
}

uint8_t egui_view_dialog_sheet_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    return local->current_snapshot;
}

void egui_view_dialog_sheet_set_current_action(egui_view_t *self, uint8_t action_index)
{
    egui_view_dialog_sheet_set_current_action_inner(self, action_index, 1);
}

uint8_t egui_view_dialog_sheet_get_current_action(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    return local->current_action;
}

void egui_view_dialog_sheet_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_on_action_changed_listener(egui_view_t *self, egui_view_on_dialog_sheet_action_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->on_action_changed = listener;
}

void egui_view_dialog_sheet_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->locked_mode = locked_mode ? 1 : 0;
    local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    egui_view_invalidate(self);
}

void egui_view_dialog_sheet_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t overlay_color, egui_color_t border_color,
                                        egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                        egui_color_t warning_color, egui_color_t error_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    local->surface_color = surface_color;
    local->overlay_color = overlay_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->error_color = error_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void egui_view_dialog_sheet_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    const egui_view_dialog_sheet_snapshot_t *snapshot;
    egui_view_dialog_sheet_metrics_t metrics;
    egui_color_t tone_color;
    egui_color_t overlay_fill;
    egui_color_t overlay_line;
    egui_color_t sheet_fill;
    egui_color_t sheet_border;
    egui_color_t title_color;
    egui_color_t body_color;
    egui_color_t eyebrow_color;
    egui_color_t hero_fill;
    egui_color_t hero_border;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_color;
    egui_color_t tag_fill;
    egui_color_t tag_border;
    egui_color_t tag_text;
    egui_color_t handle_color;
    egui_color_t shadow_color;
    egui_region_t preview_region;
    egui_region_t preview_line_region;
    uint8_t show_secondary;
    uint8_t show_close;
    uint8_t enabled;
    egui_dim_t radius = local->compact_mode ? DIALOG_SHEET_COMPACT_RADIUS : DIALOG_SHEET_STANDARD_RADIUS;

    snapshot = egui_view_dialog_sheet_get_snapshot(local);
    if (snapshot == NULL)
    {
        return;
    }

    show_secondary = egui_view_dialog_sheet_has_secondary(snapshot);
    show_close = snapshot->show_close && !local->compact_mode && !local->locked_mode;
    enabled = egui_view_get_enable(self) ? 1 : 0;
    egui_view_dialog_sheet_get_metrics(local, self, show_secondary, show_close, &metrics);
    if (metrics.sheet_region.size.width <= 0 || metrics.sheet_region.size.height <= 0)
    {
        return;
    }

    tone_color = egui_view_dialog_sheet_tone_color(local, snapshot->tone);
    overlay_fill = local->overlay_color;
    overlay_line = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 6 : 10);
    sheet_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 4 : 6);
    sheet_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 10 : 14);
    title_color = local->text_color;
    body_color = egui_rgb_mix(local->muted_text_color, local->text_color, local->compact_mode ? 24 : 14);
    eyebrow_color = egui_rgb_mix(local->muted_text_color, tone_color, local->compact_mode ? 22 : 28);
    hero_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 14 : 16);
    hero_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 18 : 20);
    footer_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 8 : 10);
    footer_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 12 : 16);
    footer_color = egui_rgb_mix(local->muted_text_color, tone_color, local->compact_mode ? 20 : 24);
    tag_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 16 : 20);
    tag_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 18 : 24);
    tag_text = tone_color;
    handle_color = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 10 : 14);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->border_color, 8);

    if (local->locked_mode)
    {
        tone_color = egui_rgb_mix(tone_color, local->muted_text_color, 76);
        overlay_fill = egui_rgb_mix(overlay_fill, local->surface_color, 24);
        overlay_line = egui_rgb_mix(overlay_line, local->muted_text_color, 26);
        sheet_fill = egui_rgb_mix(sheet_fill, local->surface_color, 16);
        sheet_border = egui_rgb_mix(sheet_border, local->muted_text_color, 18);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 18);
        body_color = egui_rgb_mix(body_color, local->muted_text_color, 16);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 28);
        hero_fill = egui_rgb_mix(hero_fill, local->surface_color, 24);
        hero_border = egui_rgb_mix(hero_border, local->muted_text_color, 24);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 22);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 24);
        footer_color = egui_rgb_mix(footer_color, local->muted_text_color, 18);
        tag_fill = egui_rgb_mix(tag_fill, local->surface_color, 24);
        tag_border = egui_rgb_mix(tag_border, local->muted_text_color, 26);
        tag_text = egui_rgb_mix(tag_text, local->muted_text_color, 30);
    }

    if (!enabled)
    {
        tone_color = egui_view_dialog_sheet_mix_disabled(tone_color);
        overlay_fill = egui_view_dialog_sheet_mix_disabled(overlay_fill);
        overlay_line = egui_view_dialog_sheet_mix_disabled(overlay_line);
        sheet_fill = egui_view_dialog_sheet_mix_disabled(sheet_fill);
        sheet_border = egui_view_dialog_sheet_mix_disabled(sheet_border);
        title_color = egui_view_dialog_sheet_mix_disabled(title_color);
        body_color = egui_view_dialog_sheet_mix_disabled(body_color);
        eyebrow_color = egui_view_dialog_sheet_mix_disabled(eyebrow_color);
        hero_fill = egui_view_dialog_sheet_mix_disabled(hero_fill);
        hero_border = egui_view_dialog_sheet_mix_disabled(hero_border);
        footer_fill = egui_view_dialog_sheet_mix_disabled(footer_fill);
        footer_border = egui_view_dialog_sheet_mix_disabled(footer_border);
        footer_color = egui_view_dialog_sheet_mix_disabled(footer_color);
        tag_fill = egui_view_dialog_sheet_mix_disabled(tag_fill);
        tag_border = egui_view_dialog_sheet_mix_disabled(tag_border);
        tag_text = egui_view_dialog_sheet_mix_disabled(tag_text);
        handle_color = egui_view_dialog_sheet_mix_disabled(handle_color);
        shadow_color = egui_view_dialog_sheet_mix_disabled(shadow_color);
    }

    egui_canvas_draw_round_rectangle_fill(
            metrics.backdrop_region.location.x, metrics.backdrop_region.location.y, metrics.backdrop_region.size.width, metrics.backdrop_region.size.height,
            radius, overlay_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? DIALOG_SHEET_COMPACT_OVERLAY_ALPHA : DIALOG_SHEET_STANDARD_OVERLAY_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.backdrop_region.location.x, metrics.backdrop_region.location.y, metrics.backdrop_region.size.width,
                                     metrics.backdrop_region.size.height, radius, 1, overlay_line, egui_color_alpha_mix(self->alpha, 16));

    preview_region.location.x = metrics.backdrop_region.location.x + (local->compact_mode ? 8 : 14);
    preview_region.location.y = metrics.backdrop_region.location.y + (local->compact_mode ? 6 : 10);
    preview_region.size.width = metrics.backdrop_region.size.width - (local->compact_mode ? 16 : 28);
    preview_region.size.height = metrics.sheet_region.location.y - preview_region.location.y - (local->compact_mode ? 2 : 4);
    if (!local->compact_mode && preview_region.size.height > 8)
    {
        egui_canvas_draw_round_rectangle_fill(preview_region.location.x, preview_region.location.y, preview_region.size.width, 8, 4, overlay_line,
                                              egui_color_alpha_mix(self->alpha, 10));

        preview_line_region.location.x = preview_region.location.x;
        preview_line_region.location.y = preview_region.location.y + 6;
        preview_line_region.size.width = preview_region.size.width * 3 / 5;
        preview_line_region.size.height = 4;
        egui_canvas_draw_round_rectangle_fill(preview_line_region.location.x, preview_line_region.location.y, preview_line_region.size.width,
                                              preview_line_region.size.height, preview_line_region.size.height / 2, overlay_line,
                                              egui_color_alpha_mix(self->alpha, 7));
    }

    egui_canvas_draw_round_rectangle_fill(metrics.sheet_region.location.x + 2, metrics.sheet_region.location.y + 3, metrics.sheet_region.size.width,
                                          metrics.sheet_region.size.height, radius, shadow_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 6 : 10));
    egui_canvas_draw_round_rectangle_fill(metrics.sheet_region.location.x, metrics.sheet_region.location.y, metrics.sheet_region.size.width,
                                          metrics.sheet_region.size.height, radius, sheet_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(
            metrics.sheet_region.location.x, metrics.sheet_region.location.y, metrics.sheet_region.size.width, metrics.sheet_region.size.height, radius, 1,
            sheet_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? DIALOG_SHEET_COMPACT_BORDER_ALPHA : DIALOG_SHEET_STANDARD_BORDER_ALPHA));

    egui_canvas_draw_round_rectangle_fill(metrics.handle_region.location.x, metrics.handle_region.location.y, metrics.handle_region.size.width,
                                          metrics.handle_region.size.height, metrics.handle_region.size.height / 2, handle_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 24 : (local->compact_mode ? 44 : 54)));
    egui_canvas_draw_circle_fill(metrics.hero_region.location.x + metrics.hero_region.size.width / 2,
                                 metrics.hero_region.location.y + metrics.hero_region.size.height / 2, metrics.hero_region.size.width / 2, hero_fill,
                                 egui_color_alpha_mix(self->alpha, local->locked_mode ? 56 : 96));
    if (!local->compact_mode)
    {
        egui_canvas_draw_circle(metrics.hero_region.location.x + metrics.hero_region.size.width / 2,
                                metrics.hero_region.location.y + metrics.hero_region.size.height / 2, metrics.hero_region.size.width / 2, 1, hero_border,
                                egui_color_alpha_mix(self->alpha, local->locked_mode ? 42 : 58));
    }
    egui_view_dialog_sheet_draw_text(local->meta_font, self, egui_view_dialog_sheet_tone_glyph(snapshot->tone), &metrics.hero_region, EGUI_ALIGN_CENTER,
                                     local->surface_color);

    if (!local->compact_mode)
    {
        egui_view_dialog_sheet_draw_text(local->meta_font, self, snapshot->eyebrow, &metrics.eyebrow_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                         eyebrow_color);
    }
    egui_view_dialog_sheet_draw_text(local->font, self, snapshot->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    egui_view_dialog_sheet_draw_text(local->font, self, snapshot->body, &metrics.body_region, EGUI_ALIGN_LEFT, body_color);

    if (show_close && metrics.close_region.size.width > 0)
    {
        egui_canvas_draw_line(metrics.close_region.location.x + 2, metrics.close_region.location.y + 2, metrics.close_region.location.x + 6,
                              metrics.close_region.location.y + 6, 1, body_color, egui_color_alpha_mix(self->alpha, 60));
        egui_canvas_draw_line(metrics.close_region.location.x + 6, metrics.close_region.location.y + 2, metrics.close_region.location.x + 2,
                              metrics.close_region.location.y + 6, 1, body_color, egui_color_alpha_mix(self->alpha, 60));
    }

    if (metrics.footer_text_region.size.width > 0 && snapshot->footer != NULL && snapshot->footer[0] != '\0')
    {
        egui_dim_t summary_y = metrics.footer_region.location.y + (local->compact_mode ? 2 : 1);
        egui_dim_t summary_h = metrics.footer_region.size.height - (local->compact_mode ? 4 : 2);
        egui_region_t footer_text_draw_region = metrics.footer_text_region;
        egui_dim_t summary_pad_x = local->compact_mode ? 3 : 4;

        egui_canvas_draw_round_rectangle_fill(metrics.footer_text_region.location.x, summary_y, metrics.footer_text_region.size.width, summary_h, summary_h / 2,
                                              footer_fill, egui_color_alpha_mix(self->alpha, local->compact_mode ? 20 : 24));
        egui_canvas_draw_round_rectangle(metrics.footer_text_region.location.x, summary_y, metrics.footer_text_region.size.width, summary_h, summary_h / 2, 1,
                                         footer_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 22 : 26));
        if (footer_text_draw_region.size.width > summary_pad_x * 2)
        {
            footer_text_draw_region.location.x += summary_pad_x;
            footer_text_draw_region.size.width -= summary_pad_x * 2;
        }
        egui_view_dialog_sheet_draw_text(local->meta_font, self, snapshot->footer, &footer_text_draw_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                                         footer_color);
    }

    if (metrics.tag_region.size.width > 0)
    {
        egui_canvas_draw_round_rectangle_fill(metrics.tag_region.location.x, metrics.tag_region.location.y, metrics.tag_region.size.width,
                                              metrics.tag_region.size.height, metrics.tag_region.size.height / 2, tag_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics.tag_region.location.x, metrics.tag_region.location.y, metrics.tag_region.size.width,
                                         metrics.tag_region.size.height, metrics.tag_region.size.height / 2, 1, tag_border,
                                         egui_color_alpha_mix(self->alpha, 40));
        egui_view_dialog_sheet_draw_text(local->meta_font, self, snapshot->tag, &metrics.tag_region, EGUI_ALIGN_CENTER, tag_text);
    }

    egui_view_dialog_sheet_draw_button(local, self, &metrics.primary_action_region, snapshot->primary_action, tone_color, sheet_border, title_color,
                                       local->current_action == EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
                                       local->pressed_action == EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, enabled);
    if (show_secondary)
    {
        egui_view_dialog_sheet_draw_button(local, self, &metrics.secondary_action_region, snapshot->secondary_action, tone_color, sheet_border, body_color,
                                           local->current_action == EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY,
                                           local->pressed_action == EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, enabled);
    }

    if (local->locked_mode || !enabled)
    {
        egui_canvas_draw_line(metrics.sheet_region.location.x + (local->compact_mode ? 6 : 8),
                              metrics.sheet_region.location.y + metrics.sheet_region.size.height - (local->compact_mode ? 6 : 8),
                              metrics.sheet_region.location.x + metrics.sheet_region.size.width - (local->compact_mode ? 6 : 8),
                              metrics.sheet_region.location.y + metrics.sheet_region.size.height - (local->compact_mode ? 6 : 8), 1, sheet_border,
                              egui_color_alpha_mix(self->alpha, 24));
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_dialog_sheet_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    uint8_t hit_action;

    if (!egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_action = egui_view_dialog_sheet_hit_action(local, self, event->location.x, event->location.y);
        if (hit_action == EGUI_VIEW_DIALOG_SHEET_ACTION_NONE)
        {
            return 0;
        }
        local->pressed_action = hit_action;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_action = egui_view_dialog_sheet_hit_action(local, self, event->location.x, event->location.y);
        if (local->pressed_action != EGUI_VIEW_DIALOG_SHEET_ACTION_NONE && local->pressed_action == hit_action)
        {
            egui_view_dialog_sheet_set_current_action_inner(self, hit_action, 1);
        }
        local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_action != EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_dialog_sheet_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_dialog_sheet_t);
    const egui_view_dialog_sheet_snapshot_t *snapshot = egui_view_dialog_sheet_get_snapshot(local);
    uint8_t has_secondary = egui_view_dialog_sheet_has_secondary(snapshot);

    if (!egui_view_get_enable(self) || local->locked_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (has_secondary)
        {
            egui_view_dialog_sheet_set_current_action_inner(self, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, 1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_RIGHT:
        egui_view_dialog_sheet_set_current_action_inner(self, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        if (has_secondary)
        {
            egui_view_dialog_sheet_set_current_action_inner(self,
                                                            local->current_action == EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY
                                                                    ? EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY
                                                                    : EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY,
                                                            1);
            return 1;
        }
        return 0;
    case EGUI_KEY_CODE_HOME:
        egui_view_dialog_sheet_set_current_action_inner(self, EGUI_VIEW_DIALOG_SHEET_ACTION_PRIMARY, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        if (has_secondary)
        {
            egui_view_dialog_sheet_set_current_action_inner(self, EGUI_VIEW_DIALOG_SHEET_ACTION_SECONDARY, 1);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_dialog_sheet_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_dialog_sheet_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_dialog_sheet_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_dialog_sheet_on_key_event,
#endif
};

void egui_view_dialog_sheet_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_dialog_sheet_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_dialog_sheet_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_action_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->overlay_color = EGUI_COLOR_HEX(0xEAF0F7);
    local->border_color = EGUI_COLOR_HEX(0xD7DEE7);
    local->text_color = EGUI_COLOR_HEX(0x17212B);
    local->muted_text_color = EGUI_COLOR_HEX(0x62707E);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->error_color = EGUI_COLOR_HEX(0xC93C37);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8796);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_action = EGUI_VIEW_DIALOG_SHEET_ACTION_NONE;

    egui_view_set_view_name(self, "egui_view_dialog_sheet");
}
