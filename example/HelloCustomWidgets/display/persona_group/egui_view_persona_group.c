#include "egui_view_persona_group.h"

#define EGUI_VIEW_PERSONA_GROUP_STANDARD_PAD_X         8
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_PAD_Y         7
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_RADIUS        10
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_AVATAR_SIZE   20
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_AVATAR_STEP   15
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_HEADER_HEIGHT 8
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_TITLE_HEIGHT  11
#define EGUI_VIEW_PERSONA_GROUP_STANDARD_FOOTER_HEIGHT 11

#define EGUI_VIEW_PERSONA_GROUP_COMPACT_PAD_X         6
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_PAD_Y         5
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_RADIUS        8
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_AVATAR_SIZE   15
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_AVATAR_STEP   12
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_TITLE_HEIGHT  9
#define EGUI_VIEW_PERSONA_GROUP_COMPACT_FOOTER_HEIGHT 10

typedef struct egui_view_persona_group_metrics egui_view_persona_group_metrics_t;
struct egui_view_persona_group_metrics
{
    egui_region_t content_region;
    egui_region_t avatar_regions[EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS];
    egui_region_t overflow_region;
    egui_region_t name_region;
    egui_region_t role_region;
    egui_region_t footer_region;
    egui_region_t eyebrow_region;
    egui_region_t title_region;
    uint8_t bubble_count;
};

static const egui_color_t egui_view_persona_group_avatar_palette[] = {
        EGUI_COLOR_HEX(0x2E63DA),
        EGUI_COLOR_HEX(0x178454),
        EGUI_COLOR_HEX(0xB67619),
        EGUI_COLOR_HEX(0x7B8794),
};

static uint8_t egui_view_persona_group_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_PERSONA_GROUP_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_persona_group_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS)
    {
        return EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_persona_group_text_len(const char *text)
{
    uint8_t len = 0;

    if (text == NULL)
    {
        return 0;
    }

    while (text[len] != '\0')
    {
        len++;
    }

    return len;
}

static egui_color_t egui_view_persona_group_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 66);
}

static egui_color_t egui_view_persona_group_presence_color(egui_view_persona_group_t *local, uint8_t presence)
{
    switch (presence)
    {
    case EGUI_VIEW_PERSONA_GROUP_PRESENCE_BUSY:
        return local->warning_color;
    case EGUI_VIEW_PERSONA_GROUP_PRESENCE_AWAY:
        return egui_rgb_mix(local->warning_color, local->accent_color, 38);
    case EGUI_VIEW_PERSONA_GROUP_PRESENCE_IDLE:
        return local->neutral_color;
    default:
        return local->success_color;
    }
}

static const egui_view_persona_group_snapshot_t *egui_view_persona_group_get_snapshot(egui_view_persona_group_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

static const egui_view_persona_group_item_t *egui_view_persona_group_get_item(egui_view_persona_group_t *local)
{
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0)
    {
        return NULL;
    }
    if (local->current_index >= snapshot->item_count)
    {
        return NULL;
    }

    return &snapshot->items[local->current_index];
}

static void egui_view_persona_group_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                              egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_persona_group_footer_width(const char *text, uint8_t compact_mode, egui_dim_t max_width)
{
    egui_dim_t width = (compact_mode ? 18 : 24) + egui_view_persona_group_text_len(text) * (compact_mode ? 4 : 5);

    if (width > max_width)
    {
        width = max_width;
    }

    return width;
}

static void egui_view_persona_group_notify_change(egui_view_t *self, egui_view_persona_group_t *local)
{
    if (local->on_focus_changed)
    {
        local->on_focus_changed(self, local->current_snapshot, local->current_index);
    }
}

static void egui_view_persona_group_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify);
static void egui_view_persona_group_get_metrics(egui_view_persona_group_t *local, egui_view_t *self, egui_view_persona_group_metrics_t *metrics)
{
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_PAD_X : EGUI_VIEW_PERSONA_GROUP_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_PAD_Y : EGUI_VIEW_PERSONA_GROUP_STANDARD_PAD_Y;
    egui_dim_t avatar_size = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_AVATAR_SIZE : EGUI_VIEW_PERSONA_GROUP_STANDARD_AVATAR_SIZE;
    egui_dim_t avatar_step = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_AVATAR_STEP : EGUI_VIEW_PERSONA_GROUP_STANDARD_AVATAR_STEP;
    egui_dim_t bubble_count;
    egui_dim_t row_width;
    egui_dim_t start_x;
    egui_dim_t row_y;
    egui_dim_t footer_h = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_FOOTER_HEIGHT : EGUI_VIEW_PERSONA_GROUP_STANDARD_FOOTER_HEIGHT;
    egui_dim_t overflow_size = avatar_size - (local->compact_mode ? 1 : 2);
    uint8_t item_count = 0;
    uint8_t i;

    egui_view_get_work_region(self, &work_region);
    metrics->content_region.location.x = work_region.location.x + pad_x;
    metrics->content_region.location.y = work_region.location.y + pad_y;
    metrics->content_region.size.width = work_region.size.width - pad_x * 2;
    metrics->content_region.size.height = work_region.size.height - pad_y * 2;

    for (i = 0; i < EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS; i++)
    {
        metrics->avatar_regions[i].location.x = 0;
        metrics->avatar_regions[i].location.y = 0;
        metrics->avatar_regions[i].size.width = 0;
        metrics->avatar_regions[i].size.height = 0;
    }

    metrics->overflow_region.location.x = 0;
    metrics->overflow_region.location.y = 0;
    metrics->overflow_region.size.width = 0;
    metrics->overflow_region.size.height = 0;

    if (snapshot != NULL)
    {
        item_count = egui_view_persona_group_clamp_item_count(snapshot->item_count);
    }

    metrics->bubble_count = item_count;
    if (snapshot != NULL && snapshot->overflow_count > 0)
    {
        metrics->bubble_count++;
    }

    bubble_count = metrics->bubble_count > 0 ? metrics->bubble_count : 1;
    row_width = avatar_size + (bubble_count - 1) * avatar_step;
    if (row_width > metrics->content_region.size.width)
    {
        row_width = metrics->content_region.size.width;
    }
    start_x = metrics->content_region.location.x + (metrics->content_region.size.width - row_width) / 2;
    row_y = metrics->content_region.location.y + (local->compact_mode ? 19 : 29);

    for (i = 0; i < item_count; i++)
    {
        metrics->avatar_regions[i].location.x = start_x + i * avatar_step;
        metrics->avatar_regions[i].location.y = row_y;
        metrics->avatar_regions[i].size.width = avatar_size;
        metrics->avatar_regions[i].size.height = avatar_size;
    }

    if (snapshot != NULL && snapshot->overflow_count > 0)
    {
        metrics->overflow_region.location.x = start_x + item_count * avatar_step + (avatar_size - overflow_size) / 2;
        metrics->overflow_region.location.y = row_y + (avatar_size - overflow_size) / 2;
        metrics->overflow_region.size.width = overflow_size;
        metrics->overflow_region.size.height = overflow_size;
    }

    metrics->eyebrow_region.location.x = metrics->content_region.location.x + 2;
    metrics->eyebrow_region.location.y = metrics->content_region.location.y + 1;
    metrics->eyebrow_region.size.width = metrics->content_region.size.width - 4;
    metrics->eyebrow_region.size.height = EGUI_VIEW_PERSONA_GROUP_STANDARD_HEADER_HEIGHT;

    metrics->title_region.location.x = metrics->content_region.location.x + 2;
    metrics->title_region.location.y = metrics->content_region.location.y + (local->compact_mode ? 4 : 12);
    metrics->title_region.size.width = metrics->content_region.size.width - 4;
    metrics->title_region.size.height = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_TITLE_HEIGHT : EGUI_VIEW_PERSONA_GROUP_STANDARD_TITLE_HEIGHT;

    metrics->name_region.location.x = metrics->content_region.location.x + 4;
    metrics->name_region.location.y = row_y + avatar_size + (local->compact_mode ? 5 : 7);
    metrics->name_region.size.width = metrics->content_region.size.width - 8;
    metrics->name_region.size.height = local->compact_mode ? 9 : 10;

    metrics->role_region.location.x = metrics->content_region.location.x + 4;
    metrics->role_region.location.y = metrics->name_region.location.y + metrics->name_region.size.height + 1;
    metrics->role_region.size.width = metrics->content_region.size.width - 8;
    metrics->role_region.size.height = local->compact_mode ? 0 : 8;

    metrics->footer_region.size.height = footer_h;
    metrics->footer_region.location.y = metrics->content_region.location.y + metrics->content_region.size.height - footer_h;
    metrics->footer_region.location.x = metrics->content_region.location.x + 4;
    metrics->footer_region.size.width = metrics->content_region.size.width - 8;
}

static uint8_t egui_view_persona_group_hit_index(egui_view_persona_group_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);
    egui_view_persona_group_metrics_t metrics;
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
    }

    item_count = egui_view_persona_group_clamp_item_count(snapshot->item_count);
    egui_view_persona_group_get_metrics(local, self, &metrics);

    for (i = 0; i < item_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.avatar_regions[i], x, y))
        {
            return i;
        }
    }

    return EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
}

static void egui_view_persona_group_set_current_snapshot_inner(egui_view_t *self, uint8_t snapshot_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    const egui_view_persona_group_snapshot_t *snapshot;

    if (local->snapshots == NULL || local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_persona_group_get_snapshot(local);
    if (snapshot != NULL)
    {
        local->current_index = snapshot->focus_index;
        if (local->current_index >= snapshot->item_count)
        {
            local->current_index = 0;
        }
    }
    local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
    if (notify)
    {
        egui_view_persona_group_notify_change(self, local);
    }
    egui_view_invalidate(self);
}

static void egui_view_persona_group_set_current_index_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);

    if (snapshot == NULL || snapshot->items == NULL || snapshot->item_count == 0 || item_index >= snapshot->item_count)
    {
        return;
    }
    if (local->current_index == item_index)
    {
        return;
    }

    local->current_index = item_index;
    if (notify)
    {
        egui_view_persona_group_notify_change(self, local);
    }
    egui_view_invalidate(self);
}

void egui_view_persona_group_set_snapshots(egui_view_t *self, const egui_view_persona_group_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    const egui_view_persona_group_snapshot_t *snapshot;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_persona_group_clamp_snapshot_count(snapshot_count);
    local->current_snapshot = 0;
    local->current_index = 0;
    local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;

    snapshot = egui_view_persona_group_get_snapshot(local);
    if (snapshot != NULL && snapshot->item_count > 0)
    {
        local->current_index = snapshot->focus_index;
        if (local->current_index >= snapshot->item_count)
        {
            local->current_index = 0;
        }
    }

    egui_view_invalidate(self);
}

void egui_view_persona_group_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    egui_view_persona_group_set_current_snapshot_inner(self, snapshot_index, 1);
}

uint8_t egui_view_persona_group_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    return local->current_snapshot;
}

void egui_view_persona_group_set_current_index(egui_view_t *self, uint8_t item_index)
{
    egui_view_persona_group_set_current_index_inner(self, item_index, 1);
}

uint8_t egui_view_persona_group_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    return local->current_index;
}

void egui_view_persona_group_set_on_focus_changed_listener(egui_view_t *self, egui_view_on_persona_group_focus_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    local->on_focus_changed = listener;
}

void egui_view_persona_group_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_persona_group_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_persona_group_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_persona_group_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_persona_group_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                         egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                         egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->section_color = section_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->neutral_color = neutral_color;
    egui_view_invalidate(self);
}
static void egui_view_persona_group_draw_avatar(egui_view_t *self, egui_view_persona_group_t *local, const egui_view_persona_group_item_t *item,
                                                const egui_region_t *region, uint8_t selected, uint8_t pressed)
{
    egui_region_t text_region;
    egui_color_t fill_color = egui_view_persona_group_avatar_palette[item->tone % 4];
    egui_color_t ring_color = selected ? local->accent_color : egui_rgb_mix(local->border_color, fill_color, 20);
    egui_color_t text_color = EGUI_COLOR_HEX(0xFFFFFF);
    egui_color_t presence_color = egui_view_persona_group_presence_color(local, item->presence);
    egui_dim_t center_x = region->location.x + region->size.width / 2;
    egui_dim_t center_y = region->location.y + region->size.height / 2;
    egui_dim_t radius = region->size.width / 2;

    if (item->emphasized)
    {
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 8);
    }
    if (pressed)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_BLACK, 18);
    }
    if (local->read_only_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->section_color, 28);
        ring_color = egui_rgb_mix(ring_color, local->muted_text_color, 24);
        text_color = egui_rgb_mix(text_color, local->muted_text_color, 30);
        presence_color = egui_rgb_mix(presence_color, local->muted_text_color, 24);
    }
    if (!egui_view_get_enable(self))
    {
        fill_color = egui_view_persona_group_mix_disabled(fill_color);
        ring_color = egui_view_persona_group_mix_disabled(ring_color);
        text_color = egui_view_persona_group_mix_disabled(text_color);
        presence_color = egui_view_persona_group_mix_disabled(presence_color);
    }

    if (selected)
    {
        egui_canvas_draw_circle_basic(center_x, center_y, radius + 3, 1, ring_color, egui_color_alpha_mix(self->alpha, 90));
    }

    egui_canvas_draw_circle_fill_basic(center_x, center_y, radius, fill_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_circle_basic(center_x, center_y, radius, 1, egui_rgb_mix(fill_color, local->surface_color, 12), egui_color_alpha_mix(self->alpha, 44));

    text_region.location.x = region->location.x;
    text_region.location.y = region->location.y;
    text_region.size.width = region->size.width;
    text_region.size.height = region->size.height;
    egui_view_persona_group_draw_text(local->meta_font, self, item->initials, &text_region, EGUI_ALIGN_CENTER, text_color);

    egui_canvas_draw_circle_fill_basic(region->location.x + region->size.width - 4, region->location.y + region->size.height - 4, 3, EGUI_COLOR_HEX(0xFFFFFF),
                                       egui_color_alpha_mix(self->alpha, 100));
    egui_canvas_draw_circle_fill_basic(region->location.x + region->size.width - 4, region->location.y + region->size.height - 4, 2, presence_color,
                                       egui_color_alpha_mix(self->alpha, 100));
}

static void egui_view_persona_group_draw_overflow(egui_view_t *self, egui_view_persona_group_t *local, const egui_region_t *region, uint8_t overflow_count)
{
    egui_region_t text_region;
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, local->section_color, 18);
    egui_color_t border_color = egui_rgb_mix(local->border_color, local->section_color, 18);
    egui_color_t text_color = local->muted_text_color;
    char text[4];
    egui_dim_t center_x = region->location.x + region->size.width / 2;
    egui_dim_t center_y = region->location.y + region->size.height / 2;
    egui_dim_t radius = region->size.width / 2;

    text[0] = '+';
    if (overflow_count > 9)
    {
        overflow_count = 9;
    }
    text[1] = (char)('0' + overflow_count);
    text[2] = '\0';

    if (local->read_only_mode)
    {
        fill_color = egui_rgb_mix(fill_color, local->surface_color, 18);
        border_color = egui_rgb_mix(border_color, local->muted_text_color, 20);
        text_color = egui_rgb_mix(text_color, local->muted_text_color, 22);
    }
    if (!egui_view_get_enable(self))
    {
        fill_color = egui_view_persona_group_mix_disabled(fill_color);
        border_color = egui_view_persona_group_mix_disabled(border_color);
        text_color = egui_view_persona_group_mix_disabled(text_color);
    }

    egui_canvas_draw_circle_fill_basic(center_x, center_y, radius, fill_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_circle_basic(center_x, center_y, radius, 1, border_color, egui_color_alpha_mix(self->alpha, 44));

    text_region.location.x = region->location.x;
    text_region.location.y = region->location.y;
    text_region.size.width = region->size.width;
    text_region.size.height = region->size.height;
    egui_view_persona_group_draw_text(local->meta_font, self, text, &text_region, EGUI_ALIGN_CENTER, text_color);
}

static void egui_view_persona_group_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);
    const egui_view_persona_group_item_t *item = egui_view_persona_group_get_item(local);
    egui_view_persona_group_metrics_t metrics;
    egui_region_t text_region;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t eyebrow_color;
    egui_color_t title_color;
    egui_color_t role_color;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_dim_t footer_w;
    egui_dim_t card_radius = local->compact_mode ? EGUI_VIEW_PERSONA_GROUP_COMPACT_RADIUS : EGUI_VIEW_PERSONA_GROUP_STANDARD_RADIUS;
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL || item == NULL)
    {
        return;
    }

    egui_view_persona_group_get_metrics(local, self, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    item_count = egui_view_persona_group_clamp_item_count(snapshot->item_count);
    card_fill = egui_rgb_mix(local->surface_color, local->section_color, local->compact_mode ? 5 : 8);
    card_border = egui_rgb_mix(local->border_color, local->section_color, local->compact_mode ? 14 : 16);
    eyebrow_color = egui_rgb_mix(local->accent_color, local->muted_text_color, 18);
    title_color = local->text_color;
    role_color = local->muted_text_color;
    footer_fill = egui_rgb_mix(local->surface_color, local->accent_color, local->compact_mode ? 6 : 8);
    footer_border = egui_rgb_mix(local->border_color, local->accent_color, local->compact_mode ? 12 : 14);
    footer_text = local->accent_color;

    if (local->read_only_mode)
    {
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 20);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 18);
        eyebrow_color = egui_rgb_mix(eyebrow_color, local->muted_text_color, 22);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 12);
        role_color = egui_rgb_mix(role_color, local->muted_text_color, 16);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 18);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 18);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 24);
    }
    if (!egui_view_get_enable(self))
    {
        card_fill = egui_view_persona_group_mix_disabled(card_fill);
        card_border = egui_view_persona_group_mix_disabled(card_border);
        eyebrow_color = egui_view_persona_group_mix_disabled(eyebrow_color);
        title_color = egui_view_persona_group_mix_disabled(title_color);
        role_color = egui_view_persona_group_mix_disabled(role_color);
        footer_fill = egui_view_persona_group_mix_disabled(footer_fill);
        footer_border = egui_view_persona_group_mix_disabled(footer_border);
        footer_text = egui_view_persona_group_mix_disabled(footer_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                          metrics.content_region.size.height + 4, card_radius, card_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, card_radius, 1, card_border, egui_color_alpha_mix(self->alpha, 56));

    if (!local->compact_mode)
    {
        egui_view_persona_group_draw_text(local->meta_font, self, snapshot->eyebrow, &metrics.eyebrow_region, EGUI_ALIGN_CENTER, eyebrow_color);
    }
    egui_view_persona_group_draw_text(local->font, self, snapshot->title, &metrics.title_region, EGUI_ALIGN_CENTER, title_color);

    if (snapshot->overflow_count > 0)
    {
        egui_view_persona_group_draw_overflow(self, local, &metrics.overflow_region, snapshot->overflow_count);
    }

    for (i = 0; i < item_count; i++)
    {
        if (i == local->current_index)
        {
            continue;
        }
        egui_view_persona_group_draw_avatar(self, local, &snapshot->items[i], &metrics.avatar_regions[i], 0, i == local->pressed_index);
    }
    if (local->current_index < item_count)
    {
        egui_view_persona_group_draw_avatar(self, local, &snapshot->items[local->current_index], &metrics.avatar_regions[local->current_index], 1,
                                            local->current_index == local->pressed_index);
    }

    egui_view_persona_group_draw_text(local->font, self, item->name, &metrics.name_region, EGUI_ALIGN_CENTER, title_color);

    if (!local->compact_mode)
    {
        egui_view_persona_group_draw_text(local->meta_font, self, item->role, &metrics.role_region, EGUI_ALIGN_CENTER, role_color);
    }

    footer_w = egui_view_persona_group_footer_width(snapshot->summary, local->compact_mode, metrics.footer_region.size.width);
    text_region = metrics.footer_region;
    text_region.location.x = metrics.content_region.location.x + (metrics.content_region.size.width - footer_w) / 2;
    text_region.size.width = footer_w;
    egui_canvas_draw_round_rectangle_fill(text_region.location.x, text_region.location.y, text_region.size.width, text_region.size.height,
                                          text_region.size.height / 2, footer_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(text_region.location.x, text_region.location.y, text_region.size.width, text_region.size.height,
                                     text_region.size.height / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, 34));
    egui_view_persona_group_draw_text(local->meta_font, self, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, footer_text);
}
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_persona_group_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    uint8_t hit_index;

    if (local->snapshots == NULL || local->snapshot_count == 0 || !egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_persona_group_hit_index(local, self, event->location.x, event->location.y);
        if (hit_index >= EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS)
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_persona_group_hit_index(local, self, event->location.x, event->location.y);
        if (local->pressed_index == hit_index && hit_index < EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS)
        {
            egui_view_persona_group_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index < EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_persona_group_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_persona_group_t);
    const egui_view_persona_group_snapshot_t *snapshot = egui_view_persona_group_get_snapshot(local);
    uint8_t next_index;

    if (snapshot == NULL || snapshot->item_count == 0 || !egui_view_get_enable(self) || local->read_only_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_UP:
        next_index = local->current_index > 0 ? (local->current_index - 1) : 0;
        egui_view_persona_group_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_DOWN:
        next_index = local->current_index + 1 < snapshot->item_count ? (local->current_index + 1) : (snapshot->item_count - 1);
        egui_view_persona_group_set_current_index_inner(self, next_index, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        egui_view_persona_group_set_current_index_inner(self, 0, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        egui_view_persona_group_set_current_index_inner(self, snapshot->item_count - 1, 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
        next_index = local->current_index + 1;
        if (next_index >= snapshot->item_count)
        {
            next_index = 0;
        }
        egui_view_persona_group_set_current_index_inner(self, next_index, 1);
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_persona_group_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_persona_group_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_persona_group_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_persona_group_on_key_event,
#endif
};

void egui_view_persona_group_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_persona_group_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_persona_group_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_focus_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->section_color = EGUI_COLOR_HEX(0xF5F7FA);
    local->text_color = EGUI_COLOR_HEX(0x1E2933);
    local->muted_text_color = EGUI_COLOR_HEX(0x728191);
    local->accent_color = EGUI_COLOR_HEX(0x2E63DA);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB67619);
    local->neutral_color = EGUI_COLOR_HEX(0x7B8794);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_index = EGUI_VIEW_PERSONA_GROUP_MAX_ITEMS;

    egui_view_set_view_name(self, "egui_view_persona_group");
}
