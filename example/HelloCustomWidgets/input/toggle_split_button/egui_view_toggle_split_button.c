#include "egui_view_toggle_split_button.h"

#define TSB_STD_RADIUS     10
#define TSB_STD_PAD_X      10
#define TSB_STD_PAD_Y      8
#define TSB_STD_TITLE_H    10
#define TSB_STD_TITLE_GAP  4
#define TSB_STD_ROW_H      32
#define TSB_STD_ROW_GAP    4
#define TSB_STD_HELPER_H   10
#define TSB_STD_HELPER_GAP 5
#define TSB_STD_MENU_W     30
#define TSB_STD_SEG_RADIUS 7
#define TSB_STD_BADGE_W    34
#define TSB_STD_BADGE_H    16
#define TSB_STD_BADGE_GAP  8
#define TSB_STD_GLYPH_W    18

#define TSB_COMPACT_RADIUS     8
#define TSB_COMPACT_PAD_X      7
#define TSB_COMPACT_PAD_Y      6
#define TSB_COMPACT_TITLE_H    9
#define TSB_COMPACT_TITLE_GAP  3
#define TSB_COMPACT_ROW_H      24
#define TSB_COMPACT_ROW_GAP    3
#define TSB_COMPACT_MENU_W     22
#define TSB_COMPACT_SEG_RADIUS 5
#define TSB_COMPACT_BADGE_W    28
#define TSB_COMPACT_BADGE_H    14
#define TSB_COMPACT_BADGE_GAP  6
#define TSB_COMPACT_GLYPH_W    14

typedef struct egui_view_toggle_split_button_metrics egui_view_toggle_split_button_metrics_t;
struct egui_view_toggle_split_button_metrics
{
    egui_region_t title_region;
    egui_region_t primary_region;
    egui_region_t badge_region;
    egui_region_t label_region;
    egui_region_t glyph_region;
    egui_region_t menu_region;
    egui_region_t helper_region;
    uint8_t show_title;
    uint8_t show_helper;
    uint8_t show_glyph;
};

static const egui_view_toggle_split_button_snapshot_t *toggle_split_button_get_snapshot(egui_view_toggle_split_button_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static uint8_t toggle_split_button_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static egui_color_t toggle_split_button_tone_color(egui_view_toggle_split_button_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_DANGER:
        return local->danger_color;
    case EGUI_VIEW_TOGGLE_SPLIT_BUTTON_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static uint8_t toggle_split_button_part_is_enabled(egui_view_toggle_split_button_t *local, egui_view_t *self,
                                                   const egui_view_toggle_split_button_snapshot_t *snapshot, uint8_t part)
{
    if (snapshot == NULL || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    if (part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY)
    {
        return snapshot->primary_enabled ? 1 : 0;
    }
    if (part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU)
    {
        return snapshot->menu_enabled ? 1 : 0;
    }
    return 0;
}

static uint8_t toggle_split_button_default_part(egui_view_toggle_split_button_t *local, egui_view_t *self,
                                                const egui_view_toggle_split_button_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    }
    if (toggle_split_button_part_is_enabled(local, self, snapshot, snapshot->focus_part))
    {
        return snapshot->focus_part;
    }
    if (toggle_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY))
    {
        return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY;
    }
    if (toggle_split_button_part_is_enabled(local, self, snapshot, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU))
    {
        return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU;
    }
    return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
}

static void toggle_split_button_notify(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);

    if (local->on_changed != NULL && local->snapshot_count > 0 && local->current_snapshot < local->snapshot_count)
    {
        local->on_changed(self, local->current_snapshot, local->checked_states[local->current_snapshot], part);
    }
}

static void toggle_split_button_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);

    if (!toggle_split_button_part_is_enabled(local, self, snapshot, part))
    {
        part = toggle_split_button_default_part(local, self, snapshot);
    }

    local->current_part = part;
    egui_view_invalidate(self);
    if (notify)
    {
        toggle_split_button_notify(self, part);
    }
}

static void toggle_split_button_get_metrics(egui_view_toggle_split_button_t *local, egui_view_t *self, const egui_view_toggle_split_button_snapshot_t *snapshot,
                                            egui_view_toggle_split_button_metrics_t *metrics)
{
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? TSB_COMPACT_PAD_X : TSB_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? TSB_COMPACT_PAD_Y : TSB_STD_PAD_Y;
    egui_dim_t title_h = local->compact_mode ? TSB_COMPACT_TITLE_H : TSB_STD_TITLE_H;
    egui_dim_t title_gap = local->compact_mode ? TSB_COMPACT_TITLE_GAP : TSB_STD_TITLE_GAP;
    egui_dim_t row_h = local->compact_mode ? TSB_COMPACT_ROW_H : TSB_STD_ROW_H;
    egui_dim_t row_gap = local->compact_mode ? TSB_COMPACT_ROW_GAP : TSB_STD_ROW_GAP;
    egui_dim_t menu_w = local->compact_mode ? TSB_COMPACT_MENU_W : TSB_STD_MENU_W;
    egui_dim_t badge_w = local->compact_mode ? TSB_COMPACT_BADGE_W : TSB_STD_BADGE_W;
    egui_dim_t badge_h = local->compact_mode ? TSB_COMPACT_BADGE_H : TSB_STD_BADGE_H;
    egui_dim_t badge_gap = local->compact_mode ? TSB_COMPACT_BADGE_GAP : TSB_STD_BADGE_GAP;
    egui_dim_t glyph_w = local->compact_mode ? TSB_COMPACT_GLYPH_W : TSB_STD_GLYPH_W;
    egui_dim_t row_y;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;

    egui_view_get_work_region(self, &work_region);
    content_x = work_region.location.x + pad_x;
    content_y = work_region.location.y + pad_y;
    content_w = work_region.size.width - pad_x * 2;
    content_h = work_region.size.height - pad_y * 2;
    metrics->show_title = snapshot != NULL && toggle_split_button_has_text(snapshot->title);
    metrics->show_helper = !local->compact_mode && snapshot != NULL && toggle_split_button_has_text(snapshot->helper);
    metrics->show_glyph = snapshot != NULL && toggle_split_button_has_text(snapshot->glyph);

    metrics->title_region.location.x = content_x;
    metrics->title_region.location.y = content_y;
    metrics->title_region.size.width = content_w;
    metrics->title_region.size.height = title_h;

    row_y = content_y;
    if (metrics->show_title)
    {
        row_y += title_h + title_gap;
    }
    if (!metrics->show_helper)
    {
        row_y = content_y + (content_h - row_h) / 2;
    }

    metrics->primary_region.location.x = content_x;
    metrics->primary_region.location.y = row_y;
    metrics->primary_region.size.width = content_w - menu_w - row_gap;
    metrics->primary_region.size.height = row_h;

    metrics->menu_region.location.x = metrics->primary_region.location.x + metrics->primary_region.size.width + row_gap;
    metrics->menu_region.location.y = row_y;
    metrics->menu_region.size.width = menu_w;
    metrics->menu_region.size.height = row_h;

    metrics->badge_region.location.x = metrics->primary_region.location.x + 5;
    metrics->badge_region.location.y = metrics->primary_region.location.y + (row_h - badge_h) / 2;
    metrics->badge_region.size.width = badge_w;
    metrics->badge_region.size.height = badge_h;

    metrics->glyph_region.location.x = metrics->primary_region.location.x + metrics->primary_region.size.width - glyph_w - 6;
    metrics->glyph_region.location.y = row_y;
    metrics->glyph_region.size.width = glyph_w;
    metrics->glyph_region.size.height = row_h;

    metrics->label_region.location.x = metrics->badge_region.location.x + badge_w + badge_gap;
    metrics->label_region.location.y = row_y;
    metrics->label_region.size.width = metrics->primary_region.size.width - (metrics->label_region.location.x - metrics->primary_region.location.x) - 8;
    if (metrics->show_glyph)
    {
        metrics->label_region.size.width -= glyph_w + 4;
    }
    metrics->label_region.size.height = row_h;

    metrics->helper_region.location.x = content_x;
    metrics->helper_region.location.y = row_y + row_h + TSB_STD_HELPER_GAP;
    metrics->helper_region.size.width = content_w;
    metrics->helper_region.size.height = TSB_STD_HELPER_H;
}

static uint8_t toggle_split_button_hit_part(egui_view_toggle_split_button_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_toggle_split_button_metrics_t metrics;
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);

    toggle_split_button_get_metrics(local, self, snapshot, &metrics);
    if (egui_region_pt_in_rect(&metrics.primary_region, x, y))
    {
        return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY;
    }
    if (egui_region_pt_in_rect(&metrics.menu_region, x, y))
    {
        return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU;
    }
    return EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
}

static void toggle_split_button_activate_part(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);

    if (part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY)
    {
        local->checked_states[local->current_snapshot] = local->checked_states[local->current_snapshot] ? 0 : 1;
        local->current_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY;
        egui_view_invalidate(self);
        toggle_split_button_notify(self, part);
        return;
    }

    if (local->snapshot_count > 0)
    {
        local->current_snapshot = (uint8_t)((local->current_snapshot + 1) % local->snapshot_count);
        local->current_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU;
        egui_view_invalidate(self);
        toggle_split_button_notify(self, part);
    }
}

void egui_view_toggle_split_button_set_snapshots(egui_view_t *self, const egui_view_toggle_split_button_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    uint8_t i;

    local->snapshots = snapshots;
    local->snapshot_count = snapshot_count > EGUI_VIEW_TOGGLE_SPLIT_BUTTON_MAX_SNAPSHOTS ? EGUI_VIEW_TOGGLE_SPLIT_BUTTON_MAX_SNAPSHOTS : snapshot_count;
    local->current_snapshot = 0;
    for (i = 0; i < EGUI_VIEW_TOGGLE_SPLIT_BUTTON_MAX_SNAPSHOTS; i++)
    {
        local->checked_states[i] = 0;
    }
    for (i = 0; i < local->snapshot_count; i++)
    {
        local->checked_states[i] = snapshots[i].checked ? 1 : 0;
    }
    local->current_part = toggle_split_button_default_part(local, self, toggle_split_button_get_snapshot(local));
    local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_toggle_split_button_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    local->current_part = toggle_split_button_default_part(local, self, toggle_split_button_get_snapshot(local));
    egui_view_invalidate(self);
}

uint8_t egui_view_toggle_split_button_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    return local->current_snapshot;
}

void egui_view_toggle_split_button_set_checked(egui_view_t *self, uint8_t checked)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);

    if (local->snapshot_count == 0)
    {
        return;
    }

    local->checked_states[local->current_snapshot] = checked ? 1 : 0;
    egui_view_invalidate(self);
}

uint8_t egui_view_toggle_split_button_get_checked(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);

    if (local->snapshot_count == 0)
    {
        return 0;
    }

    return local->checked_states[local->current_snapshot];
}

void egui_view_toggle_split_button_set_current_part(egui_view_t *self, uint8_t part)
{
    toggle_split_button_set_current_part_inner(self, part, 0);
}

uint8_t egui_view_toggle_split_button_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    return local->current_part;
}

void egui_view_toggle_split_button_set_on_changed_listener(egui_view_t *self, egui_view_on_toggle_split_button_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    local->on_changed = listener;
}

void egui_view_toggle_split_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_toggle_split_button_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_toggle_split_button_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    local->compact_mode = compact_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_toggle_split_button_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    egui_view_invalidate(self);
}

void egui_view_toggle_split_button_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                               egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                               egui_color_t danger_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
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
uint8_t egui_view_toggle_split_button_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    egui_view_toggle_split_button_metrics_t metrics;
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);

    if (region == NULL || snapshot == NULL)
    {
        return 0;
    }

    toggle_split_button_get_metrics(local, self, snapshot, &metrics);
    if (part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY)
    {
        *region = metrics.primary_region;
        return 1;
    }
    if (part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU)
    {
        *region = metrics.menu_region;
        return 1;
    }
    return 0;
}

uint8_t egui_view_toggle_split_button_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);
    uint8_t next_part;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_HOME:
        toggle_split_button_set_current_part_inner(self, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY, 1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_END:
        toggle_split_button_set_current_part_inner(self, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_part = local->current_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY ? EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU
                                                                                      : EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY;
        toggle_split_button_set_current_part_inner(self, next_part, 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (toggle_split_button_part_is_enabled(local, self, snapshot, local->current_part))
        {
            toggle_split_button_activate_part(self, local->current_part);
        }
        return 1;
    case EGUI_KEY_CODE_PLUS:
        if (local->snapshot_count > 0)
        {
            local->current_snapshot = (uint8_t)((local->current_snapshot + 1) % local->snapshot_count);
            local->current_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU;
            egui_view_invalidate(self);
            toggle_split_button_notify(self, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU);
        }
        return 1;
    case EGUI_KEY_CODE_MINUS:
        if (local->snapshot_count > 0)
        {
            local->current_snapshot = local->current_snapshot == 0 ? (uint8_t)(local->snapshot_count - 1) : (uint8_t)(local->current_snapshot - 1);
            local->current_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU;
            egui_view_invalidate(self);
            toggle_split_button_notify(self, EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU);
        }
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        toggle_split_button_set_current_part_inner(self, toggle_split_button_default_part(local, self, snapshot), 1);
        return 1;
    default:
        return 0;
    }
}

static void toggle_split_button_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                          egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!toggle_split_button_has_text(text))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void toggle_split_button_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color)
{
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, 72));
}

static void toggle_split_button_draw_chevron(egui_view_t *self, const egui_region_t *region, egui_color_t color)
{
    egui_dim_t cx = region->location.x + region->size.width / 2;
    egui_dim_t cy = region->location.y + region->size.height / 2;
    egui_alpha_t alpha = egui_color_alpha_mix(self->alpha, 92);

    egui_canvas_draw_line(cx - 3, cy - 1, cx, cy + 2, 1, color, alpha);
    egui_canvas_draw_line(cx, cy + 2, cx + 3, cy - 1, 1, color, alpha);
}

static void egui_view_toggle_split_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);
    egui_view_toggle_split_button_metrics_t metrics;
    egui_color_t tone_color;
    egui_color_t surface_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t primary_fill;
    egui_color_t primary_border;
    egui_color_t primary_text;
    egui_color_t badge_fill;
    egui_color_t badge_border;
    egui_color_t badge_text;
    egui_color_t menu_fill;
    egui_color_t menu_border;
    egui_color_t menu_text;
    egui_dim_t outer_radius = local->compact_mode ? TSB_COMPACT_RADIUS : TSB_STD_RADIUS;
    egui_dim_t segment_radius = local->compact_mode ? TSB_COMPACT_SEG_RADIUS : TSB_STD_SEG_RADIUS;
    uint8_t checked;

    if (snapshot == NULL)
    {
        return;
    }

    checked = local->checked_states[local->current_snapshot];
    tone_color = toggle_split_button_tone_color(local, snapshot->tone);
    surface_color = local->read_only_mode ? egui_rgb_mix(local->surface_color, EGUI_COLOR_HEX(0xEFF3F7), 54) : local->surface_color;
    border_color = local->read_only_mode ? egui_rgb_mix(local->border_color, EGUI_COLOR_HEX(0x9AA7B4), 50) : local->border_color;
    text_color = local->read_only_mode ? egui_rgb_mix(local->text_color, EGUI_COLOR_HEX(0x7B8895), 42) : local->text_color;
    muted_text_color = local->read_only_mode ? egui_rgb_mix(local->muted_text_color, EGUI_COLOR_HEX(0x8B98A5), 42) : local->muted_text_color;

    toggle_split_button_get_metrics(local, self, snapshot, &metrics);

    egui_canvas_draw_round_rectangle_fill(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                          self->region_screen.size.height, outer_radius, surface_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(self->region_screen.location.x, self->region_screen.location.y, self->region_screen.size.width,
                                     self->region_screen.size.height, outer_radius, 1, border_color, egui_color_alpha_mix(self->alpha, 58));

    toggle_split_button_draw_text(local->meta_font, self, snapshot->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);
    toggle_split_button_draw_text(local->meta_font, self, snapshot->helper, &metrics.helper_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_text_color);

    primary_fill = checked ? tone_color : egui_rgb_mix(surface_color, tone_color, local->compact_mode ? 16 : 12);
    primary_border = local->current_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY && !local->read_only_mode ? tone_color
                                                                                                                 : egui_rgb_mix(border_color, tone_color, 26);
    primary_text = checked ? EGUI_COLOR_HEX(0xFFFFFF) : text_color;
    if (local->pressed_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY)
    {
        primary_fill = egui_rgb_mix(primary_fill, tone_color, 26);
    }
    egui_canvas_draw_round_rectangle_fill(metrics.primary_region.location.x, metrics.primary_region.location.y, metrics.primary_region.size.width,
                                          metrics.primary_region.size.height, segment_radius, primary_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.primary_region.location.x, metrics.primary_region.location.y, metrics.primary_region.size.width,
                                     metrics.primary_region.size.height, segment_radius, 1, primary_border, egui_color_alpha_mix(self->alpha, 46));

    badge_fill = checked ? egui_rgb_mix(EGUI_COLOR_HEX(0xFFFFFF), tone_color, 14) : egui_rgb_mix(surface_color, border_color, 22);
    badge_border = checked ? egui_rgb_mix(tone_color, EGUI_COLOR_HEX(0xFFFFFF), 28) : egui_rgb_mix(border_color, tone_color, 10);
    badge_text = checked ? tone_color : muted_text_color;
    egui_canvas_draw_round_rectangle_fill(metrics.badge_region.location.x, metrics.badge_region.location.y, metrics.badge_region.size.width,
                                          metrics.badge_region.size.height, metrics.badge_region.size.height / 2, badge_fill,
                                          egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_round_rectangle(metrics.badge_region.location.x, metrics.badge_region.location.y, metrics.badge_region.size.width,
                                     metrics.badge_region.size.height, metrics.badge_region.size.height / 2, 1, badge_border,
                                     egui_color_alpha_mix(self->alpha, 68));
    toggle_split_button_draw_text(local->meta_font, self, checked ? "ON" : "OFF", &metrics.badge_region, EGUI_ALIGN_CENTER, badge_text);
    toggle_split_button_draw_text(local->font, self, snapshot->label, &metrics.label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, primary_text);
    if (metrics.show_glyph)
    {
        toggle_split_button_draw_text(local->meta_font, self, snapshot->glyph, &metrics.glyph_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER,
                                      checked ? egui_rgb_mix(EGUI_COLOR_HEX(0xFFFFFF), tone_color, 18) : muted_text_color);
    }

    menu_fill = egui_rgb_mix(surface_color, tone_color, local->compact_mode ? 8 : 6);
    menu_border =
            local->current_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU && !local->read_only_mode ? tone_color : egui_rgb_mix(border_color, tone_color, 24);
    menu_text = egui_rgb_mix(text_color, tone_color, 24);
    if (local->pressed_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU)
    {
        menu_fill = egui_rgb_mix(menu_fill, tone_color, 22);
    }
    egui_canvas_draw_round_rectangle_fill(metrics.menu_region.location.x, metrics.menu_region.location.y, metrics.menu_region.size.width,
                                          metrics.menu_region.size.height, segment_radius, menu_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.menu_region.location.x, metrics.menu_region.location.y, metrics.menu_region.size.width,
                                     metrics.menu_region.size.height, segment_radius, 1, menu_border, egui_color_alpha_mix(self->alpha, 44));
    toggle_split_button_draw_chevron(self, &metrics.menu_region, menu_text);

    if (local->current_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY && !local->read_only_mode)
    {
        toggle_split_button_draw_focus(self, &metrics.primary_region, segment_radius, tone_color);
    }
    else if (local->current_part == EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_MENU && !local->read_only_mode)
    {
        toggle_split_button_draw_focus(self, &metrics.menu_region, segment_radius, tone_color);
    }
}
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_toggle_split_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_toggle_split_button_t);
    const egui_view_toggle_split_button_snapshot_t *snapshot = toggle_split_button_get_snapshot(local);
    uint8_t hit_part;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = toggle_split_button_hit_part(local, self, event->location.x, event->location.y);
        if (!toggle_split_button_part_is_enabled(local, self, snapshot, hit_part))
        {
            return 0;
        }
        local->pressed_part = hit_part;
        toggle_split_button_set_current_part_inner(self, hit_part, 1);
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = toggle_split_button_hit_part(local, self, event->location.x, event->location.y);
        if (local->pressed_part == hit_part && toggle_split_button_part_is_enabled(local, self, snapshot, hit_part))
        {
            toggle_split_button_activate_part(self, hit_part);
        }
        local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_toggle_split_button_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    if (egui_view_toggle_split_button_handle_navigation_key(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_split_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_toggle_split_button_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_toggle_split_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_toggle_split_button_on_key_event,
#endif
};

void egui_view_toggle_split_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_toggle_split_button_t);
    uint8_t i;

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_split_button_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_changed = NULL;
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
    local->current_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_PRIMARY;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_part = EGUI_VIEW_TOGGLE_SPLIT_BUTTON_PART_NONE;
    for (i = 0; i < EGUI_VIEW_TOGGLE_SPLIT_BUTTON_MAX_SNAPSHOTS; i++)
    {
        local->checked_states[i] = 0;
    }

    egui_view_set_view_name(self, "egui_view_toggle_split_button");
}
