#include "egui_view_command_bar.h"

#define EGUI_VIEW_COMMAND_BAR_STANDARD_RADIUS        9
#define EGUI_VIEW_COMMAND_BAR_STANDARD_FILL_ALPHA    94
#define EGUI_VIEW_COMMAND_BAR_STANDARD_BORDER_ALPHA  62
#define EGUI_VIEW_COMMAND_BAR_STANDARD_PAD_X         9
#define EGUI_VIEW_COMMAND_BAR_STANDARD_PAD_Y         8
#define EGUI_VIEW_COMMAND_BAR_STANDARD_HEADER_HEIGHT 12
#define EGUI_VIEW_COMMAND_BAR_STANDARD_HEADER_GAP    6
#define EGUI_VIEW_COMMAND_BAR_STANDARD_RAIL_HEIGHT   32
#define EGUI_VIEW_COMMAND_BAR_STANDARD_FOOTER_HEIGHT 12
#define EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_HEIGHT  18
#define EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_BASE_W  20
#define EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_CHAR_W  4
#define EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_HEIGHT   10
#define EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_BASE_W   16
#define EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_CHAR_W   4
#define EGUI_VIEW_COMMAND_BAR_STANDARD_GAP           4
#define EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MIN_W    28
#define EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MAX_W    54
#define EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_BASE_W   12
#define EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_CHAR_W   5
#define EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W       12
#define EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_H       12

#define EGUI_VIEW_COMMAND_BAR_COMPACT_RADIUS        7
#define EGUI_VIEW_COMMAND_BAR_COMPACT_FILL_ALPHA    90
#define EGUI_VIEW_COMMAND_BAR_COMPACT_BORDER_ALPHA  58
#define EGUI_VIEW_COMMAND_BAR_COMPACT_PAD_X         6
#define EGUI_VIEW_COMMAND_BAR_COMPACT_PAD_Y         5
#define EGUI_VIEW_COMMAND_BAR_COMPACT_HEADER_HEIGHT 10
#define EGUI_VIEW_COMMAND_BAR_COMPACT_HEADER_GAP    4
#define EGUI_VIEW_COMMAND_BAR_COMPACT_RAIL_HEIGHT   24
#define EGUI_VIEW_COMMAND_BAR_COMPACT_FOOTER_HEIGHT 9
#define EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_HEIGHT  14
#define EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_BASE_W  16
#define EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_CHAR_W  3
#define EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_HEIGHT   8
#define EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_BASE_W   12
#define EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_CHAR_W   3
#define EGUI_VIEW_COMMAND_BAR_COMPACT_GAP           3
#define EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_MIN_W    18
#define EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_MAX_W    22
#define EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_BASE_W   18

typedef struct egui_view_command_bar_metrics egui_view_command_bar_metrics_t;
struct egui_view_command_bar_metrics
{
    egui_region_t content_region;
    egui_region_t header_region;
    egui_region_t rail_region;
    egui_region_t footer_region;
    egui_region_t scope_region;
    egui_region_t item_regions[EGUI_VIEW_COMMAND_BAR_MAX_ITEMS];
    uint8_t visible_item_count;
};

static uint8_t egui_view_command_bar_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_COMMAND_BAR_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_COMMAND_BAR_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_command_bar_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_COMMAND_BAR_MAX_ITEMS)
    {
        return EGUI_VIEW_COMMAND_BAR_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_command_bar_text_len(const char *text)
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

static egui_color_t egui_view_command_bar_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 68);
}

static egui_color_t egui_view_command_bar_tone_color(egui_view_command_bar_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_COMMAND_BAR_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_COMMAND_BAR_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_COMMAND_BAR_TONE_DANGER:
        return local->danger_color;
    case EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const egui_view_command_bar_snapshot_t *egui_view_command_bar_get_snapshot(egui_view_command_bar_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }
    return &local->snapshots[local->current_snapshot];
}

static uint8_t egui_view_command_bar_item_is_enabled(egui_view_command_bar_t *local, egui_view_t *self, const egui_view_command_bar_item_t *item)
{
    if (item == NULL || item->enabled == 0 || local->disabled_mode || !egui_view_get_enable(self))
    {
        return 0;
    }
    return 1;
}

static uint8_t egui_view_command_bar_resolve_default_index(egui_view_command_bar_t *local, egui_view_t *self, const egui_view_command_bar_snapshot_t *snapshot,
                                                           uint8_t item_count)
{
    uint8_t i;

    if (snapshot == NULL || item_count == 0)
    {
        return 0;
    }

    if (snapshot->focus_index < item_count && egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[snapshot->focus_index]))
    {
        return snapshot->focus_index;
    }

    for (i = 0; i < item_count; ++i)
    {
        if (egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[i]))
        {
            return i;
        }
    }

    if (snapshot->focus_index < item_count)
    {
        return snapshot->focus_index;
    }

    return 0;
}

static void egui_view_command_bar_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                            egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0' || region->size.width <= 0 || region->size.height <= 0)
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_command_bar_measure_scope_width(uint8_t compact_mode, const char *text, egui_dim_t max_w)
{
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = (compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_BASE_W : EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_BASE_W) +
            egui_view_command_bar_text_len(text) * (compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_CHAR_W : EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_CHAR_W);
    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static egui_dim_t egui_view_command_bar_measure_pill_width(uint8_t compact_mode, const char *text, egui_dim_t max_w)
{
    egui_dim_t width;

    if (text == NULL || text[0] == '\0')
    {
        return 0;
    }

    width = (compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_BASE_W : EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_BASE_W) +
            egui_view_command_bar_text_len(text) * (compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_CHAR_W : EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_CHAR_W);
    if (width > max_w)
    {
        width = max_w;
    }
    return width;
}

static egui_dim_t egui_view_command_bar_measure_item_width(uint8_t compact_mode, const egui_view_command_bar_item_t *item)
{
    egui_dim_t width;

    if (item == NULL)
    {
        return 0;
    }

    if (item->kind == EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW)
    {
        return compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_BASE_W : 24;
    }

    if (compact_mode)
    {
        width = EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_BASE_W + (item->emphasized ? 2 : 0);
        if (width > EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_MAX_W)
        {
            width = EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_MAX_W;
        }
        return width;
    }

    width = EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_BASE_W + EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W + 5 +
            egui_view_command_bar_text_len(item->label) * EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_CHAR_W + (item->emphasized ? 4 : 0);
    if (width < EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MIN_W)
    {
        width = EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MIN_W;
    }
    if (width > EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MAX_W)
    {
        width = EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MAX_W;
    }
    return width;
}

static void egui_view_command_bar_reset_metrics(egui_view_command_bar_metrics_t *metrics)
{
    uint8_t i;

    metrics->visible_item_count = 0;
    for (i = 0; i < EGUI_VIEW_COMMAND_BAR_MAX_ITEMS; ++i)
    {
        metrics->item_regions[i].location.x = 0;
        metrics->item_regions[i].location.y = 0;
        metrics->item_regions[i].size.width = 0;
        metrics->item_regions[i].size.height = 0;
    }
}

static void egui_view_command_bar_get_metrics(egui_view_command_bar_t *local, egui_view_t *self, const egui_view_command_bar_snapshot_t *snapshot,
                                              uint8_t item_count, egui_view_command_bar_metrics_t *metrics)
{
    egui_region_t region;
    uint8_t compact_mode = local->compact_mode ? 1 : 0;
    egui_dim_t pad_x = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_PAD_X : EGUI_VIEW_COMMAND_BAR_STANDARD_PAD_X;
    egui_dim_t pad_y = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_PAD_Y : EGUI_VIEW_COMMAND_BAR_STANDARD_PAD_Y;
    egui_dim_t header_h = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_HEADER_HEIGHT : EGUI_VIEW_COMMAND_BAR_STANDARD_HEADER_HEIGHT;
    egui_dim_t header_gap = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_HEADER_GAP : EGUI_VIEW_COMMAND_BAR_STANDARD_HEADER_GAP;
    egui_dim_t rail_h = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_RAIL_HEIGHT : EGUI_VIEW_COMMAND_BAR_STANDARD_RAIL_HEIGHT;
    egui_dim_t footer_h = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_FOOTER_HEIGHT : EGUI_VIEW_COMMAND_BAR_STANDARD_FOOTER_HEIGHT;
    egui_dim_t gap = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_GAP : EGUI_VIEW_COMMAND_BAR_STANDARD_GAP;
    egui_dim_t scope_h = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_SCOPE_HEIGHT : EGUI_VIEW_COMMAND_BAR_STANDARD_SCOPE_HEIGHT;
    egui_dim_t scope_w;
    egui_dim_t left_x;
    egui_dim_t right_x;
    egui_dim_t available_w;
    egui_dim_t normal_total_w = 0;
    egui_dim_t min_item_w = compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_ITEM_MIN_W : EGUI_VIEW_COMMAND_BAR_STANDARD_ITEM_MIN_W;
    egui_dim_t widths[EGUI_VIEW_COMMAND_BAR_MAX_ITEMS];
    uint8_t normal_indices[EGUI_VIEW_COMMAND_BAR_MAX_ITEMS];
    uint8_t overflow_indices[EGUI_VIEW_COMMAND_BAR_MAX_ITEMS];
    uint8_t normal_count = 0;
    uint8_t overflow_count = 0;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    egui_view_command_bar_reset_metrics(metrics);

    metrics->content_region.location.x = region.location.x + pad_x;
    metrics->content_region.location.y = region.location.y + pad_y;
    metrics->content_region.size.width = region.size.width - pad_x * 2;
    metrics->content_region.size.height = region.size.height - pad_y * 2;
    if (metrics->content_region.size.width <= 0 || metrics->content_region.size.height <= 0)
    {
        return;
    }

    metrics->header_region.location.x = metrics->content_region.location.x;
    metrics->header_region.location.y = metrics->content_region.location.y;
    metrics->header_region.size.width = metrics->content_region.size.width;
    metrics->header_region.size.height = header_h;

    metrics->footer_region.location.x = metrics->content_region.location.x;
    metrics->footer_region.location.y = metrics->content_region.location.y + metrics->content_region.size.height - footer_h;
    metrics->footer_region.size.width = metrics->content_region.size.width;
    metrics->footer_region.size.height = footer_h;

    metrics->rail_region.location.x = metrics->content_region.location.x;
    metrics->rail_region.location.y = metrics->header_region.location.y + header_h + header_gap;
    metrics->rail_region.size.width = metrics->content_region.size.width;
    metrics->rail_region.size.height = rail_h;

    if (metrics->rail_region.location.y + metrics->rail_region.size.height > metrics->footer_region.location.y)
    {
        metrics->rail_region.size.height = metrics->footer_region.location.y - metrics->rail_region.location.y - 1;
    }
    if (metrics->rail_region.size.height <= 0)
    {
        return;
    }

    scope_w = egui_view_command_bar_measure_scope_width(compact_mode, snapshot == NULL ? NULL : snapshot->scope, metrics->rail_region.size.width / 3);
    metrics->scope_region.location.x = metrics->rail_region.location.x;
    metrics->scope_region.location.y = metrics->rail_region.location.y + (metrics->rail_region.size.height - scope_h) / 2;
    metrics->scope_region.size.width = scope_w;
    metrics->scope_region.size.height = scope_h;

    for (i = 0; i < item_count; ++i)
    {
        if (snapshot->items[i].kind == EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW)
        {
            overflow_indices[overflow_count++] = i;
        }
        else
        {
            normal_indices[normal_count++] = i;
            widths[i] = egui_view_command_bar_measure_item_width(compact_mode, &snapshot->items[i]);
            normal_total_w += widths[i];
        }
    }

    left_x = metrics->rail_region.location.x + scope_w + (scope_w > 0 ? gap : 0);
    right_x = metrics->rail_region.location.x + metrics->rail_region.size.width;

    for (i = 0; i < overflow_count; ++i)
    {
        uint8_t index = overflow_indices[overflow_count - 1 - i];
        egui_dim_t width = egui_view_command_bar_measure_item_width(compact_mode, &snapshot->items[index]);

        if (right_x <= left_x)
        {
            break;
        }
        right_x -= width;
        metrics->item_regions[index].location.x = right_x;
        metrics->item_regions[index].location.y = metrics->rail_region.location.y + (metrics->rail_region.size.height - (compact_mode ? 18 : 22)) / 2;
        metrics->item_regions[index].size.width = width;
        metrics->item_regions[index].size.height = compact_mode ? 18 : 22;
        if (right_x > left_x)
        {
            right_x -= gap;
        }
    }

    if (normal_count > 0)
    {
        egui_dim_t used_gap = gap * (normal_count > 0 ? (normal_count - 1) : 0);
        egui_dim_t extra_pool;

        available_w = right_x - left_x - used_gap;
        if (available_w < 0)
        {
            available_w = 0;
        }

        if (normal_total_w > available_w && available_w > 0)
        {
            egui_dim_t min_pool = min_item_w * normal_count;

            if (min_pool > available_w)
            {
                min_item_w = available_w / normal_count;
                if (min_item_w < 14)
                {
                    min_item_w = 14;
                }
                min_pool = min_item_w * normal_count;
            }

            if (min_pool >= available_w)
            {
                for (i = 0; i < normal_count; ++i)
                {
                    widths[normal_indices[i]] = min_item_w;
                }
            }
            else
            {
                egui_dim_t extra_total = 0;

                for (i = 0; i < normal_count; ++i)
                {
                    egui_dim_t current_w = widths[normal_indices[i]];

                    if (current_w > min_item_w)
                    {
                        extra_total += current_w - min_item_w;
                    }
                }

                extra_pool = available_w - min_pool;
                for (i = 0; i < normal_count; ++i)
                {
                    uint8_t index = normal_indices[i];
                    egui_dim_t base_w = min_item_w;
                    egui_dim_t current_w = widths[index];

                    if (current_w > min_item_w && extra_total > 0)
                    {
                        base_w += (extra_pool * (current_w - min_item_w)) / extra_total;
                    }
                    widths[index] = base_w;
                }
            }
        }

        for (i = 0; i < normal_count; ++i)
        {
            uint8_t index = normal_indices[i];

            metrics->item_regions[index].location.x = left_x;
            metrics->item_regions[index].location.y = metrics->rail_region.location.y + (metrics->rail_region.size.height - (compact_mode ? 18 : 22)) / 2;
            metrics->item_regions[index].size.width = widths[index];
            metrics->item_regions[index].size.height = compact_mode ? 18 : 22;
            left_x += widths[index] + gap;
        }
    }

    metrics->visible_item_count = item_count;
}

static uint8_t egui_view_command_bar_hit_item(egui_view_command_bar_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_command_bar_metrics_t metrics;
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t item_count;
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    egui_view_command_bar_get_metrics(local, self, snapshot, item_count, &metrics);
    for (i = 0; i < item_count; ++i)
    {
        if (metrics.item_regions[i].size.width > 0 && egui_region_pt_in_rect(&metrics.item_regions[i], x, y))
        {
            return i;
        }
    }

    return EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
}

static void egui_view_command_bar_set_current_index_inner(egui_view_t *self, uint8_t index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t item_count;

    if (snapshot == NULL)
    {
        local->current_index = 0;
        return;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        local->current_index = 0;
        return;
    }

    if (index >= item_count)
    {
        index = item_count - 1;
    }
    if (local->current_index == index)
    {
        return;
    }

    local->current_index = index;
    if (notify && local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, index);
    }
    egui_view_invalidate(self);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static uint8_t egui_view_command_bar_find_home_index(egui_view_command_bar_t *local, egui_view_t *self, uint8_t reverse)
{
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t item_count;
    int16_t index;
    int16_t step;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    }

    index = reverse ? (int16_t)item_count - 1 : 0;
    step = reverse ? -1 : 1;
    while (index >= 0 && index < item_count)
    {
        if (egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[index]))
        {
            return (uint8_t)index;
        }
        index += step;
    }

    return EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
}

static void egui_view_command_bar_move_focus(egui_view_t *self, int8_t step, uint8_t wrap)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t item_count;
    int16_t index;
    uint8_t loops = 0;

    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    index = local->current_index;
    if (index >= item_count)
    {
        index = egui_view_command_bar_resolve_default_index(local, self, snapshot, item_count);
    }

    while (loops < item_count)
    {
        index += step;
        if (index < 0 || index >= item_count)
        {
            if (!wrap)
            {
                break;
            }
            index = index < 0 ? item_count - 1 : 0;
        }
        if (egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[index]))
        {
            egui_view_command_bar_set_current_index_inner(self, (uint8_t)index, 1);
            return;
        }
        loops++;
    }
}
#endif

void egui_view_command_bar_set_snapshots(egui_view_t *self, const egui_view_command_bar_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot;
    uint8_t item_count;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_command_bar_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }

    snapshot = egui_view_command_bar_get_snapshot(local);
    item_count = snapshot == NULL ? 0 : egui_view_command_bar_clamp_item_count(snapshot->item_count);
    local->current_index = egui_view_command_bar_resolve_default_index(local, self, snapshot, item_count);
    local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_command_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot;
    uint8_t item_count;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    snapshot = egui_view_command_bar_get_snapshot(local);
    item_count = snapshot == NULL ? 0 : egui_view_command_bar_clamp_item_count(snapshot->item_count);
    local->current_index = egui_view_command_bar_resolve_default_index(local, self, snapshot, item_count);
    local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    egui_view_invalidate(self);
}

uint8_t egui_view_command_bar_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    return local->current_snapshot;
}

void egui_view_command_bar_set_current_index(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t item_count;

    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    if (item_count == 0 || index >= item_count)
    {
        return;
    }
    if (!egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[index]))
    {
        return;
    }

    egui_view_command_bar_set_current_index_inner(self, index, 1);
}

uint8_t egui_view_command_bar_get_current_index(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    return local->current_index;
}

void egui_view_command_bar_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_command_bar_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->on_selection_changed = listener;
}

void egui_view_command_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_command_bar_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_command_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->compact_mode = compact_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_command_bar_set_disabled_mode(egui_view_t *self, uint8_t disabled_mode)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->disabled_mode = disabled_mode ? 1 : 0;
    local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    egui_view_invalidate(self);
}

void egui_view_command_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t section_color, egui_color_t border_color,
                                       egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                       egui_color_t warning_color, egui_color_t danger_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    local->surface_color = surface_color;
    local->section_color = section_color;
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

static void egui_view_command_bar_draw_item(egui_view_t *self, egui_view_command_bar_t *local, const egui_view_command_bar_item_t *item,
                                            const egui_region_t *item_region, uint8_t index)
{
    egui_region_t text_region;
    egui_color_t tone_color = egui_view_command_bar_tone_color(local, item->tone);
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, tone_color, item->emphasized ? 12 : 6);
    egui_color_t border_color = egui_rgb_mix(local->border_color, tone_color, index == local->current_index ? 26 : 12);
    egui_color_t glyph_fill = egui_rgb_mix(local->surface_color, tone_color, index == local->current_index ? 24 : 12);
    egui_color_t glyph_text = index == local->current_index ? tone_color : egui_rgb_mix(local->text_color, tone_color, 12);
    egui_color_t text_color = item->tone == EGUI_VIEW_COMMAND_BAR_TONE_DANGER ? tone_color : local->text_color;
    egui_dim_t radius = local->compact_mode ? 5 : 6;
    uint8_t interactive = egui_view_command_bar_item_is_enabled(local, self, item);
    uint8_t selected = index == local->current_index;
    uint8_t pressed = index == local->pressed_index;
    const char *glyph_text_ptr = item->glyph;

    if (item->kind == EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW)
    {
        fill_color = egui_rgb_mix(local->surface_color, local->section_color, local->compact_mode ? 18 : 22);
        border_color = egui_rgb_mix(local->border_color, local->neutral_color, selected ? 26 : 10);
        glyph_fill = fill_color;
        glyph_text = selected ? egui_view_command_bar_tone_color(local, EGUI_VIEW_COMMAND_BAR_TONE_NEUTRAL) : local->muted_text_color;
        text_color = local->muted_text_color;
    }
    else if (selected)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, local->compact_mode ? 14 : 18);
        text_color = egui_rgb_mix(local->text_color, tone_color, local->compact_mode ? 12 : 18);
    }

    if (pressed && interactive)
    {
        fill_color = egui_rgb_mix(fill_color, tone_color, 22);
        border_color = egui_rgb_mix(border_color, tone_color, 16);
    }

    if (!interactive)
    {
        fill_color = egui_rgb_mix(fill_color, local->section_color, 24);
        border_color = egui_rgb_mix(border_color, local->muted_text_color, 22);
        glyph_fill = egui_rgb_mix(glyph_fill, local->section_color, 26);
        glyph_text = egui_rgb_mix(glyph_text, local->muted_text_color, 30);
        text_color = egui_rgb_mix(text_color, local->muted_text_color, 30);
    }

    if (!egui_view_get_enable(self))
    {
        fill_color = egui_view_command_bar_mix_disabled(fill_color);
        border_color = egui_view_command_bar_mix_disabled(border_color);
        glyph_fill = egui_view_command_bar_mix_disabled(glyph_fill);
        glyph_text = egui_view_command_bar_mix_disabled(glyph_text);
        text_color = egui_view_command_bar_mix_disabled(text_color);
    }

    egui_canvas_draw_round_rectangle_fill(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height, radius,
                                          fill_color, egui_color_alpha_mix(self->alpha, selected ? 94 : 82));
    egui_canvas_draw_round_rectangle(item_region->location.x, item_region->location.y, item_region->size.width, item_region->size.height, radius, 1,
                                     border_color, egui_color_alpha_mix(self->alpha, selected ? 56 : 34));

    if (selected)
    {
        egui_canvas_draw_round_rectangle_fill(item_region->location.x + 3, item_region->location.y + item_region->size.height - 3, item_region->size.width - 6,
                                              2, 1, tone_color, egui_color_alpha_mix(self->alpha, 88));
    }

    if (local->compact_mode || item->kind == EGUI_VIEW_COMMAND_BAR_ITEM_KIND_OVERFLOW || item_region->size.width < 36)
    {
        text_region.location.x = item_region->location.x;
        text_region.location.y = item_region->location.y;
        text_region.size.width = item_region->size.width;
        text_region.size.height = item_region->size.height;
        egui_view_command_bar_draw_text(local->meta_font, self, glyph_text_ptr == NULL ? "..." : glyph_text_ptr, &text_region, EGUI_ALIGN_CENTER, glyph_text);
        return;
    }

    egui_canvas_draw_round_rectangle_fill(
            item_region->location.x + 4, item_region->location.y + (item_region->size.height - EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_H) / 2,
            EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W, EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_H, 4, glyph_fill, egui_color_alpha_mix(self->alpha, 94));

    text_region.location.x = item_region->location.x + 4;
    text_region.location.y = item_region->location.y + (item_region->size.height - EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_H) / 2;
    text_region.size.width = EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W;
    text_region.size.height = EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_H;
    egui_view_command_bar_draw_text(local->meta_font, self, glyph_text_ptr == NULL ? "" : glyph_text_ptr, &text_region, EGUI_ALIGN_CENTER, glyph_text);

    text_region.location.x = item_region->location.x + 4 + EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W + 5;
    text_region.location.y = item_region->location.y;
    text_region.size.width = item_region->size.width - (4 + EGUI_VIEW_COMMAND_BAR_STANDARD_GLYPH_W + 10);
    text_region.size.height = item_region->size.height;
    egui_view_command_bar_draw_text(local->font, self, item->label, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
}

static void egui_view_command_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    egui_view_command_bar_metrics_t metrics;
    egui_region_t text_region;
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    const egui_view_command_bar_item_t *focus_item;
    egui_color_t focus_color;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t rail_fill;
    egui_color_t rail_border;
    egui_color_t title_color;
    egui_color_t muted_text_color;
    egui_color_t eyebrow_fill;
    egui_color_t eyebrow_border;
    egui_color_t eyebrow_text;
    egui_color_t scope_fill;
    egui_color_t scope_border;
    egui_color_t scope_text;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_dim_t eyebrow_w;
    uint8_t item_count;
    uint8_t current_index;
    uint8_t i;

    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_command_bar_clamp_item_count(snapshot->item_count);
    if (item_count == 0)
    {
        return;
    }

    current_index = local->current_index >= item_count ? egui_view_command_bar_resolve_default_index(local, self, snapshot, item_count) : local->current_index;
    local->current_index = current_index;
    focus_item = &snapshot->items[current_index];
    focus_color = egui_view_command_bar_tone_color(local, focus_item->tone);
    card_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 5 : 7);
    card_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 14 : 16);
    rail_fill = egui_rgb_mix(local->section_color, focus_color, local->compact_mode ? 4 : 6);
    rail_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 12 : 14);
    title_color = local->text_color;
    muted_text_color = local->muted_text_color;
    eyebrow_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 12 : 14);
    eyebrow_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 18 : 20);
    eyebrow_text = focus_color;
    scope_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 10 : 12);
    scope_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 16 : 18);
    scope_text = egui_rgb_mix(local->text_color, focus_color, local->compact_mode ? 10 : 12);
    footer_fill = egui_rgb_mix(local->surface_color, focus_color, local->compact_mode ? 8 : 10);
    footer_border = egui_rgb_mix(local->border_color, focus_color, local->compact_mode ? 14 : 16);
    footer_text = egui_rgb_mix(local->muted_text_color, focus_color, local->compact_mode ? 12 : 18);

    if (local->disabled_mode)
    {
        focus_color = egui_rgb_mix(focus_color, local->neutral_color, 68);
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 20);
        card_border = egui_rgb_mix(card_border, local->neutral_color, 18);
        rail_fill = egui_rgb_mix(rail_fill, local->surface_color, 18);
        rail_border = egui_rgb_mix(rail_border, local->neutral_color, 16);
        title_color = egui_rgb_mix(title_color, muted_text_color, 20);
        eyebrow_fill = egui_rgb_mix(eyebrow_fill, local->surface_color, 26);
        eyebrow_border = egui_rgb_mix(eyebrow_border, local->neutral_color, 22);
        eyebrow_text = egui_rgb_mix(eyebrow_text, local->muted_text_color, 24);
        scope_fill = egui_rgb_mix(scope_fill, local->surface_color, 24);
        scope_border = egui_rgb_mix(scope_border, local->neutral_color, 18);
        scope_text = egui_rgb_mix(scope_text, local->muted_text_color, 20);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 26);
        footer_border = egui_rgb_mix(footer_border, local->neutral_color, 18);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 22);
    }

    if (!egui_view_get_enable(self))
    {
        focus_color = egui_view_command_bar_mix_disabled(focus_color);
        card_fill = egui_view_command_bar_mix_disabled(card_fill);
        card_border = egui_view_command_bar_mix_disabled(card_border);
        rail_fill = egui_view_command_bar_mix_disabled(rail_fill);
        rail_border = egui_view_command_bar_mix_disabled(rail_border);
        title_color = egui_view_command_bar_mix_disabled(title_color);
        muted_text_color = egui_view_command_bar_mix_disabled(muted_text_color);
        eyebrow_fill = egui_view_command_bar_mix_disabled(eyebrow_fill);
        eyebrow_border = egui_view_command_bar_mix_disabled(eyebrow_border);
        eyebrow_text = egui_view_command_bar_mix_disabled(eyebrow_text);
        scope_fill = egui_view_command_bar_mix_disabled(scope_fill);
        scope_border = egui_view_command_bar_mix_disabled(scope_border);
        scope_text = egui_view_command_bar_mix_disabled(scope_text);
        footer_fill = egui_view_command_bar_mix_disabled(footer_fill);
        footer_border = egui_view_command_bar_mix_disabled(footer_border);
        footer_text = egui_view_command_bar_mix_disabled(footer_text);
    }

    egui_view_command_bar_get_metrics(local, self, snapshot, item_count, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(
            metrics.content_region.location.x - (local->compact_mode ? 1 : 2), metrics.content_region.location.y - (local->compact_mode ? 1 : 2),
            metrics.content_region.size.width + (local->compact_mode ? 2 : 4), metrics.content_region.size.height + (local->compact_mode ? 2 : 4),
            local->compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_RADIUS : EGUI_VIEW_COMMAND_BAR_STANDARD_RADIUS, card_fill,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_FILL_ALPHA : EGUI_VIEW_COMMAND_BAR_STANDARD_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(
            metrics.content_region.location.x - (local->compact_mode ? 1 : 2), metrics.content_region.location.y - (local->compact_mode ? 1 : 2),
            metrics.content_region.size.width + (local->compact_mode ? 2 : 4), metrics.content_region.size.height + (local->compact_mode ? 2 : 4),
            local->compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_RADIUS : EGUI_VIEW_COMMAND_BAR_STANDARD_RADIUS, 1, card_border,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_BORDER_ALPHA : EGUI_VIEW_COMMAND_BAR_STANDARD_BORDER_ALPHA));
    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x, metrics.content_region.location.y, metrics.content_region.size.width,
                                          local->compact_mode ? 3 : 4, 2, focus_color, egui_color_alpha_mix(self->alpha, local->disabled_mode ? 12 : 22));

    eyebrow_w = egui_view_command_bar_measure_pill_width(local->compact_mode, snapshot->eyebrow, metrics.header_region.size.width / 2);
    if (eyebrow_w > 0)
    {
        egui_dim_t pill_h = local->compact_mode ? EGUI_VIEW_COMMAND_BAR_COMPACT_PILL_HEIGHT : EGUI_VIEW_COMMAND_BAR_STANDARD_PILL_HEIGHT;
        egui_dim_t pill_y = metrics.header_region.location.y + (metrics.header_region.size.height - pill_h) / 2;

        egui_canvas_draw_round_rectangle_fill(metrics.header_region.location.x + metrics.header_region.size.width - eyebrow_w, pill_y, eyebrow_w, pill_h,
                                              pill_h / 2, eyebrow_fill, egui_color_alpha_mix(self->alpha, 96));
        egui_canvas_draw_round_rectangle(metrics.header_region.location.x + metrics.header_region.size.width - eyebrow_w, pill_y, eyebrow_w, pill_h, pill_h / 2,
                                         1, eyebrow_border, egui_color_alpha_mix(self->alpha, 42));

        text_region.location.x = metrics.header_region.location.x + metrics.header_region.size.width - eyebrow_w;
        text_region.location.y = pill_y;
        text_region.size.width = eyebrow_w;
        text_region.size.height = pill_h;
        egui_view_command_bar_draw_text(local->meta_font, self, snapshot->eyebrow, &text_region, EGUI_ALIGN_CENTER, eyebrow_text);
    }

    text_region.location.x = metrics.header_region.location.x;
    text_region.location.y = metrics.header_region.location.y;
    text_region.size.width = metrics.header_region.size.width - (eyebrow_w > 0 ? eyebrow_w + 6 : 0);
    text_region.size.height = metrics.header_region.size.height;
    egui_view_command_bar_draw_text(local->font, self, snapshot->title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

    egui_canvas_draw_round_rectangle_fill(metrics.rail_region.location.x, metrics.rail_region.location.y, metrics.rail_region.size.width,
                                          metrics.rail_region.size.height, local->compact_mode ? 6 : 7, rail_fill, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(metrics.rail_region.location.x, metrics.rail_region.location.y, metrics.rail_region.size.width,
                                     metrics.rail_region.size.height, local->compact_mode ? 6 : 7, 1, rail_border, egui_color_alpha_mix(self->alpha, 34));

    if (metrics.scope_region.size.width > 0)
    {
        egui_canvas_draw_round_rectangle_fill(metrics.scope_region.location.x, metrics.scope_region.location.y, metrics.scope_region.size.width,
                                              metrics.scope_region.size.height, metrics.scope_region.size.height / 2, scope_fill,
                                              egui_color_alpha_mix(self->alpha, 94));
        egui_canvas_draw_round_rectangle(metrics.scope_region.location.x, metrics.scope_region.location.y, metrics.scope_region.size.width,
                                         metrics.scope_region.size.height, metrics.scope_region.size.height / 2, 1, scope_border,
                                         egui_color_alpha_mix(self->alpha, 40));

        text_region.location.x = metrics.scope_region.location.x;
        text_region.location.y = metrics.scope_region.location.y;
        text_region.size.width = metrics.scope_region.size.width;
        text_region.size.height = metrics.scope_region.size.height;
        egui_view_command_bar_draw_text(local->meta_font, self, snapshot->scope, &text_region, EGUI_ALIGN_CENTER, scope_text);

        egui_canvas_draw_line(metrics.scope_region.location.x + metrics.scope_region.size.width + (local->compact_mode ? 2 : 3),
                              metrics.rail_region.location.y + 5,
                              metrics.scope_region.location.x + metrics.scope_region.size.width + (local->compact_mode ? 2 : 3),
                              metrics.rail_region.location.y + metrics.rail_region.size.height - 5, 1, rail_border, egui_color_alpha_mix(self->alpha, 22));
    }

    for (i = 0; i < item_count; ++i)
    {
        if (metrics.item_regions[i].size.width > 0)
        {
            egui_view_command_bar_draw_item(self, local, &snapshot->items[i], &metrics.item_regions[i], i);
        }
    }

    egui_canvas_draw_round_rectangle_fill(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                          metrics.footer_region.size.height, metrics.footer_region.size.height / 2, footer_fill,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 22 : 28));
    egui_canvas_draw_round_rectangle(metrics.footer_region.location.x, metrics.footer_region.location.y, metrics.footer_region.size.width,
                                     metrics.footer_region.size.height, metrics.footer_region.size.height / 2, 1, footer_border,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 28 : 34));

    text_region.location.x = metrics.footer_region.location.x + (local->compact_mode ? 4 : 6);
    text_region.location.y = metrics.footer_region.location.y;
    text_region.size.width = metrics.footer_region.size.width - (local->compact_mode ? 8 : 12);
    text_region.size.height = metrics.footer_region.size.height;
    egui_view_command_bar_draw_text(local->meta_font, self, snapshot->footer, &text_region,
                                    (local->compact_mode ? EGUI_ALIGN_CENTER : EGUI_ALIGN_LEFT) | EGUI_ALIGN_VCENTER, footer_text);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_command_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    const egui_view_command_bar_snapshot_t *snapshot = egui_view_command_bar_get_snapshot(local);
    uint8_t hit_index;

    if (!egui_view_get_enable(self) || local->disabled_mode || snapshot == NULL)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_index = egui_view_command_bar_hit_item(local, self, event->location.x, event->location.y);
        if (hit_index == EGUI_VIEW_COMMAND_BAR_INDEX_NONE || !egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[hit_index]))
        {
            return 0;
        }
        local->pressed_index = hit_index;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_index = egui_view_command_bar_hit_item(local, self, event->location.x, event->location.y);
        if (local->pressed_index != EGUI_VIEW_COMMAND_BAR_INDEX_NONE && local->pressed_index == hit_index &&
            egui_view_command_bar_item_is_enabled(local, self, &snapshot->items[hit_index]))
        {
            egui_view_command_bar_set_current_index_inner(self, hit_index, 1);
        }
        local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_index != EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_command_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_command_bar_t);
    uint8_t index;

    if (!egui_view_get_enable(self) || local->disabled_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        egui_view_command_bar_move_focus(self, -1, 0);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        egui_view_command_bar_move_focus(self, 1, 0);
        return 1;
    case EGUI_KEY_CODE_TAB:
        egui_view_command_bar_move_focus(self, 1, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        index = egui_view_command_bar_find_home_index(local, self, 0);
        if (index != EGUI_VIEW_COMMAND_BAR_INDEX_NONE)
        {
            egui_view_command_bar_set_current_index_inner(self, index, 1);
        }
        return 1;
    case EGUI_KEY_CODE_END:
        index = egui_view_command_bar_find_home_index(local, self, 1);
        if (index != EGUI_VIEW_COMMAND_BAR_INDEX_NONE)
        {
            egui_view_command_bar_set_current_index_inner(self, index, 1);
        }
        return 1;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_command_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_command_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_command_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_command_bar_on_key_event,
#endif
};

void egui_view_command_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_command_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_command_bar_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->section_color = EGUI_COLOR_HEX(0xF7F9FC);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x687889);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->danger_color = EGUI_COLOR_HEX(0xBA3C36);
    local->neutral_color = EGUI_COLOR_HEX(0x7C8A99);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_index = 0;
    local->compact_mode = 0;
    local->disabled_mode = 0;
    local->pressed_index = EGUI_VIEW_COMMAND_BAR_INDEX_NONE;

    egui_view_set_view_name(self, "egui_view_command_bar");
}
