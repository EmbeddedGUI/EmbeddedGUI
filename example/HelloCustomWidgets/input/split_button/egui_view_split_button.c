#include "egui_view_split_button.h"

#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_RADIUS               10
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_FILL_ALPHA           94
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_BORDER_ALPHA         60
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_CONTENT_PAD_X        10
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_CONTENT_PAD_Y        8
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_TITLE_HEIGHT         10
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_TITLE_GAP            4
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_ROW_HEIGHT           30
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_HELPER_GAP           5
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_HELPER_HEIGHT        10
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_MENU_WIDTH           28
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_RADIUS       7
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_FILL_ALPHA   96
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_BORDER_ALPHA 46
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_WIDTH          16
#define EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_HEIGHT         14

#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_RADIUS               8
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_FILL_ALPHA           92
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_BORDER_ALPHA         56
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_CONTENT_PAD_X        7
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_CONTENT_PAD_Y        6
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_TITLE_HEIGHT         9
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_TITLE_GAP            3
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_ROW_HEIGHT           22
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_MENU_WIDTH           20
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_RADIUS       5
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_FILL_ALPHA   94
#define EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_BORDER_ALPHA 42

typedef struct egui_view_split_button_metrics egui_view_split_button_metrics_t;
struct egui_view_split_button_metrics
{
    egui_region_t content_region;
    egui_region_t title_region;
    egui_region_t row_region;
    egui_region_t primary_region;
    egui_region_t menu_region;
    egui_region_t helper_region;
    uint8_t show_title;
    uint8_t show_helper;
};

static uint8_t egui_view_split_button_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SPLIT_BUTTON_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SPLIT_BUTTON_MAX_SNAPSHOTS;
    }
    return count;
}

static const egui_view_split_button_snapshot_t *egui_view_split_button_get_snapshot(egui_view_split_button_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static uint8_t egui_view_split_button_text_len(const char *text)
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

static egui_color_t egui_view_split_button_tone_color(egui_view_split_button_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_SPLIT_BUTTON_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_SPLIT_BUTTON_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_SPLIT_BUTTON_TONE_DANGER:
        return local->danger_color;
    case EGUI_VIEW_SPLIT_BUTTON_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static egui_color_t egui_view_split_button_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static uint8_t egui_view_split_button_part_is_enabled(egui_view_split_button_t *local, egui_view_t *self, const egui_view_split_button_snapshot_t *snapshot,
                                                      uint8_t part)
{
    if (snapshot == NULL || !egui_view_get_enable(self) || local->disabled_mode)
    {
        return 0;
    }

    if (part == EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY)
    {
        return snapshot->primary_enabled ? 1 : 0;
    }

    if (part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU)
    {
        return snapshot->menu_enabled ? 1 : 0;
    }

    return 0;
}

static uint8_t egui_view_split_button_resolve_default_part(egui_view_split_button_t *local, egui_view_t *self,
                                                           const egui_view_split_button_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    }

    if (egui_view_split_button_part_is_enabled(local, self, snapshot, snapshot->focus_part))
    {
        return snapshot->focus_part;
    }

    if (egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY))
    {
        return EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY;
    }

    if (egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_MENU))
    {
        return EGUI_VIEW_SPLIT_BUTTON_PART_MENU;
    }

    return EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
}

static void egui_view_split_button_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                             egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void egui_view_split_button_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t mixed_alpha = egui_color_alpha_mix(self->alpha, alpha);

    egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, mixed_alpha);
    egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, mixed_alpha);
}

static void egui_view_split_button_get_metrics(egui_view_split_button_t *local, egui_view_t *self, const egui_view_split_button_snapshot_t *snapshot,
                                               egui_view_split_button_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_CONTENT_PAD_X : EGUI_VIEW_SPLIT_BUTTON_STANDARD_CONTENT_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_CONTENT_PAD_Y : EGUI_VIEW_SPLIT_BUTTON_STANDARD_CONTENT_PAD_Y;
    egui_dim_t title_h = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_TITLE_HEIGHT : EGUI_VIEW_SPLIT_BUTTON_STANDARD_TITLE_HEIGHT;
    egui_dim_t title_gap = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_TITLE_GAP : EGUI_VIEW_SPLIT_BUTTON_STANDARD_TITLE_GAP;
    egui_dim_t row_h = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_ROW_HEIGHT : EGUI_VIEW_SPLIT_BUTTON_STANDARD_ROW_HEIGHT;
    egui_dim_t helper_gap = EGUI_VIEW_SPLIT_BUTTON_STANDARD_HELPER_GAP;
    egui_dim_t helper_h = EGUI_VIEW_SPLIT_BUTTON_STANDARD_HELPER_HEIGHT;
    egui_dim_t menu_w = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_MENU_WIDTH : EGUI_VIEW_SPLIT_BUTTON_STANDARD_MENU_WIDTH;
    egui_dim_t row_y;

    egui_view_get_work_region(self, &region);
    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    metrics->show_title = (snapshot != NULL && snapshot->title != NULL && snapshot->title[0] != '\0') ? 1 : 0;
    metrics->show_helper = (!local->compact_mode && snapshot != NULL && snapshot->helper != NULL && snapshot->helper[0] != '\0') ? 1 : 0;

    metrics->title_region.location.x = metrics->content_region.location.x;
    metrics->title_region.location.y = metrics->content_region.location.y;
    metrics->title_region.size.width = metrics->content_region.size.width;
    metrics->title_region.size.height = title_h;

    if (metrics->show_title)
    {
        row_y = metrics->content_region.location.y + title_h + title_gap;
    }
    else
    {
        row_y = metrics->content_region.location.y;
    }

    if (!metrics->show_helper)
    {
        row_y = metrics->content_region.location.y + (metrics->content_region.size.height - row_h) / 2;
    }

    metrics->row_region.location.x = metrics->content_region.location.x;
    metrics->row_region.location.y = row_y;
    metrics->row_region.size.width = metrics->content_region.size.width;
    metrics->row_region.size.height = row_h;

    metrics->primary_region.location.x = metrics->row_region.location.x;
    metrics->primary_region.location.y = metrics->row_region.location.y;
    metrics->primary_region.size.width = metrics->row_region.size.width - menu_w;
    metrics->primary_region.size.height = row_h;

    metrics->menu_region.location.x = metrics->primary_region.location.x + metrics->primary_region.size.width;
    metrics->menu_region.location.y = metrics->row_region.location.y;
    metrics->menu_region.size.width = menu_w;
    metrics->menu_region.size.height = row_h;

    metrics->helper_region.location.x = metrics->content_region.location.x;
    metrics->helper_region.location.y = metrics->row_region.location.y + row_h + helper_gap;
    metrics->helper_region.size.width = metrics->content_region.size.width;
    metrics->helper_region.size.height = helper_h;
}

static uint8_t egui_view_split_button_hit_part(egui_view_split_button_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_split_button_metrics_t metrics;
    const egui_view_split_button_snapshot_t *snapshot = egui_view_split_button_get_snapshot(local);

    egui_view_split_button_get_metrics(local, self, snapshot, &metrics);
    if (egui_region_pt_in_rect(&metrics.primary_region, x, y))
    {
        return EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY;
    }
    if (egui_region_pt_in_rect(&metrics.menu_region, x, y))
    {
        return EGUI_VIEW_SPLIT_BUTTON_PART_MENU;
    }

    return EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
}

static void egui_view_split_button_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot = egui_view_split_button_get_snapshot(local);

    if (!egui_view_split_button_part_is_enabled(local, self, snapshot, part))
    {
        return;
    }

    if (local->current_part == part)
    {
        return;
    }

    local->current_part = part;
    if (notify && local->on_part_changed)
    {
        local->on_part_changed(self, part);
    }
    egui_view_invalidate(self);
}

void egui_view_split_button_set_snapshots(egui_view_t *self, const egui_view_split_button_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_split_button_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    snapshot = egui_view_split_button_get_snapshot(local);
    local->current_part = egui_view_split_button_resolve_default_part(local, self, snapshot);
    local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_split_button_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_split_button_get_snapshot(local);
    local->current_part = egui_view_split_button_resolve_default_part(local, self, snapshot);
    local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

uint8_t egui_view_split_button_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    return local->current_snapshot;
}

void egui_view_split_button_set_current_part(egui_view_t *self, uint8_t part)
{
    egui_view_split_button_set_current_part_inner(self, part, 1);
}

uint8_t egui_view_split_button_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    return local->current_part;
}

void egui_view_split_button_set_on_part_changed_listener(egui_view_t *self, egui_view_on_split_button_part_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->on_part_changed = listener;
}

void egui_view_split_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_split_button_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_split_button_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->compact_mode = compact_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_split_button_set_disabled_mode(egui_view_t *self, uint8_t disabled_mode)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->disabled_mode = disabled_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_split_button_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                        egui_color_t danger_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->danger_color = danger_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}

static void egui_view_split_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot = egui_view_split_button_get_snapshot(local);
    egui_view_split_button_metrics_t metrics;
    egui_region_t text_region;
    egui_region_t primary_fill_region;
    egui_region_t menu_fill_region;
    egui_color_t tone_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t title_color;
    egui_color_t helper_color;
    egui_color_t row_fill;
    egui_color_t row_border;
    egui_color_t primary_fill;
    egui_color_t primary_border;
    egui_color_t primary_text;
    egui_color_t menu_fill;
    egui_color_t menu_border;
    egui_color_t menu_text;
    egui_color_t divider_color;
    egui_color_t glyph_fill;
    uint8_t primary_enabled;
    uint8_t menu_enabled;
    uint8_t show_glyph;
    egui_dim_t radius;
    egui_dim_t segment_radius;

    if (snapshot == NULL)
    {
        return;
    }

    local->current_part = egui_view_split_button_resolve_default_part(local, self, snapshot);
    egui_view_split_button_get_metrics(local, self, snapshot, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    tone_color = egui_view_split_button_tone_color(local, snapshot->tone);
    card_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 5 : 7);
    card_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 14 : 16);
    title_color = local->muted_text_color;
    helper_color = egui_rgb_mix(local->muted_text_color, tone_color, local->compact_mode ? 8 : 12);
    row_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 4 : 6);
    row_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 16 : 18);
    primary_fill = snapshot->emphasized ? tone_color
                                        : egui_rgb_mix(local->surface_color, tone_color, local->current_part == EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY ? 16 : 9);
    primary_border = egui_rgb_mix(local->border_color, tone_color, local->current_part == EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY ? 28 : 18);
    primary_text = snapshot->emphasized ? EGUI_COLOR_WHITE : (snapshot->tone == EGUI_VIEW_SPLIT_BUTTON_TONE_DANGER ? tone_color : local->text_color);
    menu_fill = egui_rgb_mix(local->surface_color, tone_color, local->current_part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU ? 14 : 7);
    menu_border = egui_rgb_mix(local->border_color, tone_color, local->current_part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU ? 28 : 16);
    menu_text = local->current_part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU ? tone_color : egui_rgb_mix(local->text_color, tone_color, 10);
    divider_color = egui_rgb_mix(local->border_color, tone_color, 18);
    glyph_fill = snapshot->emphasized ? egui_rgb_mix(tone_color, EGUI_COLOR_WHITE, 26) : egui_rgb_mix(local->surface_color, tone_color, 12);
    primary_enabled = egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY);
    menu_enabled = egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_MENU);
    radius = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_RADIUS : EGUI_VIEW_SPLIT_BUTTON_STANDARD_RADIUS;
    segment_radius = local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_RADIUS : EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_RADIUS;

    if (local->pressed_part == EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY && primary_enabled)
    {
        primary_fill = egui_rgb_mix(primary_fill, tone_color, 18);
    }
    if (local->pressed_part == EGUI_VIEW_SPLIT_BUTTON_PART_MENU && menu_enabled)
    {
        menu_fill = egui_rgb_mix(menu_fill, tone_color, 18);
    }

    if (!primary_enabled)
    {
        primary_fill = egui_rgb_mix(primary_fill, row_fill, 26);
        primary_border = egui_rgb_mix(primary_border, local->muted_text_color, 28);
        primary_text = egui_rgb_mix(primary_text, local->muted_text_color, 34);
        glyph_fill = egui_rgb_mix(glyph_fill, row_fill, 24);
    }
    if (!menu_enabled)
    {
        menu_fill = egui_rgb_mix(menu_fill, row_fill, 26);
        menu_border = egui_rgb_mix(menu_border, local->muted_text_color, 30);
        menu_text = egui_rgb_mix(menu_text, local->muted_text_color, 34);
    }

    if (!egui_view_get_enable(self) || local->disabled_mode)
    {
        card_fill = egui_view_split_button_mix_disabled(card_fill);
        card_border = egui_view_split_button_mix_disabled(card_border);
        title_color = egui_view_split_button_mix_disabled(title_color);
        helper_color = egui_view_split_button_mix_disabled(helper_color);
        row_fill = egui_view_split_button_mix_disabled(row_fill);
        row_border = egui_view_split_button_mix_disabled(row_border);
        primary_fill = egui_view_split_button_mix_disabled(primary_fill);
        primary_border = egui_view_split_button_mix_disabled(primary_border);
        primary_text = egui_view_split_button_mix_disabled(primary_text);
        menu_fill = egui_view_split_button_mix_disabled(menu_fill);
        menu_border = egui_view_split_button_mix_disabled(menu_border);
        menu_text = egui_view_split_button_mix_disabled(menu_text);
        divider_color = egui_view_split_button_mix_disabled(divider_color);
        glyph_fill = egui_view_split_button_mix_disabled(glyph_fill);
    }

    egui_canvas_draw_round_rectangle_fill(
            metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
            metrics.content_region.size.height + 4, radius, card_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_FILL_ALPHA : EGUI_VIEW_SPLIT_BUTTON_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, radius, 1, card_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_BORDER_ALPHA
                                                                                           : EGUI_VIEW_SPLIT_BUTTON_STANDARD_BORDER_ALPHA));

    if (metrics.show_title)
    {
        text_region.location.x = metrics.title_region.location.x;
        text_region.location.y = metrics.title_region.location.y;
        text_region.size.width = metrics.title_region.size.width;
        text_region.size.height = metrics.title_region.size.height;
        egui_view_split_button_draw_text(local->meta_font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.row_region.location.x, metrics.row_region.location.y, metrics.row_region.size.width,
                                          metrics.row_region.size.height, segment_radius + 1, row_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_FILL_ALPHA
                                                                                                : EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(metrics.row_region.location.x, metrics.row_region.location.y, metrics.row_region.size.width,
                                     metrics.row_region.size.height, segment_radius + 1, 1, row_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_SPLIT_BUTTON_COMPACT_SEGMENT_BORDER_ALPHA
                                                                                           : EGUI_VIEW_SPLIT_BUTTON_STANDARD_SEGMENT_BORDER_ALPHA));

    primary_fill_region.location.x = metrics.primary_region.location.x + 1;
    primary_fill_region.location.y = metrics.primary_region.location.y + 1;
    primary_fill_region.size.width = metrics.primary_region.size.width - 2;
    primary_fill_region.size.height = metrics.primary_region.size.height - 2;
    menu_fill_region.location.x = metrics.menu_region.location.x + 1;
    menu_fill_region.location.y = metrics.menu_region.location.y + 1;
    menu_fill_region.size.width = metrics.menu_region.size.width - 2;
    menu_fill_region.size.height = metrics.menu_region.size.height - 2;

    egui_canvas_draw_round_rectangle_fill(primary_fill_region.location.x, primary_fill_region.location.y, primary_fill_region.size.width,
                                          primary_fill_region.size.height, segment_radius, primary_fill, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(primary_fill_region.location.x, primary_fill_region.location.y, primary_fill_region.size.width,
                                     primary_fill_region.size.height, segment_radius, 1, primary_border, egui_color_alpha_mix(self->alpha, 30));

    egui_canvas_draw_round_rectangle_fill(menu_fill_region.location.x, menu_fill_region.location.y, menu_fill_region.size.width, menu_fill_region.size.height,
                                          segment_radius, menu_fill, egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(menu_fill_region.location.x, menu_fill_region.location.y, menu_fill_region.size.width, menu_fill_region.size.height,
                                     segment_radius, 1, menu_border, egui_color_alpha_mix(self->alpha, 26));

    egui_canvas_draw_line(metrics.menu_region.location.x, metrics.row_region.location.y + 4, metrics.menu_region.location.x,
                          metrics.row_region.location.y + metrics.row_region.size.height - 4, 1, divider_color, egui_color_alpha_mix(self->alpha, 44));

    show_glyph = (!local->compact_mode && snapshot->glyph != NULL && snapshot->glyph[0] != '\0' &&
                  primary_fill_region.size.width > (egui_dim_t)(egui_view_split_button_text_len(snapshot->label) * 5 + 28))
                         ? 1
                         : 0;
    if (show_glyph)
    {
        egui_canvas_draw_round_rectangle_fill(primary_fill_region.location.x + 5,
                                              primary_fill_region.location.y +
                                                      (primary_fill_region.size.height - EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_HEIGHT) / 2,
                                              EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_WIDTH, EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_HEIGHT, 4, glyph_fill,
                                              egui_color_alpha_mix(self->alpha, 96));

        text_region.location.x = primary_fill_region.location.x + 5;
        text_region.location.y = primary_fill_region.location.y + (primary_fill_region.size.height - EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_HEIGHT) / 2;
        text_region.size.width = EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_WIDTH;
        text_region.size.height = EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_HEIGHT;
        egui_view_split_button_draw_text(local->meta_font, self, snapshot->glyph, &text_region, EGUI_ALIGN_CENTER,
                                         snapshot->emphasized ? EGUI_COLOR_WHITE : primary_text);

        text_region.location.x = primary_fill_region.location.x + 5 + EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_WIDTH + 6;
        text_region.location.y = primary_fill_region.location.y;
        text_region.size.width = primary_fill_region.size.width - (5 + EGUI_VIEW_SPLIT_BUTTON_STANDARD_GLYPH_WIDTH + 12);
        text_region.size.height = primary_fill_region.size.height;
        egui_view_split_button_draw_text(local->font, self, snapshot->label, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, primary_text);
    }
    else
    {
        text_region.location.x = primary_fill_region.location.x + 4;
        text_region.location.y = primary_fill_region.location.y;
        text_region.size.width = primary_fill_region.size.width - 8;
        text_region.size.height = primary_fill_region.size.height;
        egui_view_split_button_draw_text(local->font, self, snapshot->label, &text_region, EGUI_ALIGN_CENTER, primary_text);
    }

    egui_view_split_button_draw_chevron(self, &menu_fill_region, menu_text, 94);

    if (metrics.show_helper)
    {
        text_region.location.x = metrics.helper_region.location.x;
        text_region.location.y = metrics.helper_region.location.y;
        text_region.size.width = metrics.helper_region.size.width;
        text_region.size.height = metrics.helper_region.size.height;
        egui_view_split_button_draw_text(local->meta_font, self, snapshot->helper, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, helper_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_split_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot = egui_view_split_button_get_snapshot(local);
    uint8_t hit_part;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->disabled_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = egui_view_split_button_hit_part(local, self, event->location.x, event->location.y);
        if (!egui_view_split_button_part_is_enabled(local, self, snapshot, hit_part))
        {
            return 0;
        }
        local->pressed_part = hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = egui_view_split_button_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part == hit_part && egui_view_split_button_part_is_enabled(local, self, snapshot, hit_part))
        {
            egui_view_split_button_set_current_part_inner(self, hit_part, 1);
        }
        local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_split_button_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_split_button_t);
    const egui_view_split_button_snapshot_t *snapshot = egui_view_split_button_get_snapshot(local);
    uint8_t current_part;
    uint8_t next_part;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->disabled_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    current_part = local->current_part;
    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_HOME:
        if (egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY))
        {
            egui_view_split_button_set_current_part_inner(self, EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY, 1);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_END:
        if (egui_view_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_SPLIT_BUTTON_PART_MENU))
        {
            egui_view_split_button_set_current_part_inner(self, EGUI_VIEW_SPLIT_BUTTON_PART_MENU, 1);
        }
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_part = current_part == EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY ? EGUI_VIEW_SPLIT_BUTTON_PART_MENU : EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY;
        if (egui_view_split_button_part_is_enabled(local, self, snapshot, next_part))
        {
            egui_view_split_button_set_current_part_inner(self, next_part, 1);
        }
        else if (egui_view_split_button_part_is_enabled(local, self, snapshot, current_part))
        {
            egui_view_split_button_set_current_part_inner(self, current_part, 1);
        }
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_split_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_split_button_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_split_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_split_button_on_key_event,
#endif
};

void egui_view_split_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_split_button_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_split_button_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_part_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1D2630);
    local->muted_text_color = EGUI_COLOR_HEX(0x6F7B89);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB87A16);
    local->danger_color = EGUI_COLOR_HEX(0xB13A35);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8795);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_part = EGUI_VIEW_SPLIT_BUTTON_PART_PRIMARY;
    local->compact_mode = 0;
    local->disabled_mode = 0;
    local->pressed_part = EGUI_VIEW_SPLIT_BUTTON_PART_NONE;

    egui_view_set_view_name(self, "egui_view_split_button");
}
