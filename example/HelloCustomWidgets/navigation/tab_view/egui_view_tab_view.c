#include "egui_view_tab_view.h"

#define EGUI_VIEW_TAB_VIEW_STANDARD_PAD_X            8
#define EGUI_VIEW_TAB_VIEW_STANDARD_PAD_Y            7
#define EGUI_VIEW_TAB_VIEW_STANDARD_RADIUS           10
#define EGUI_VIEW_TAB_VIEW_STANDARD_HEADER_HEIGHT    12
#define EGUI_VIEW_TAB_VIEW_STANDARD_META_HEIGHT      9
#define EGUI_VIEW_TAB_VIEW_STANDARD_TABS_HEIGHT      22
#define EGUI_VIEW_TAB_VIEW_STANDARD_BODY_RADIUS      8
#define EGUI_VIEW_TAB_VIEW_STANDARD_ADD_WIDTH        16
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_BASE_WIDTH   22
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_CHAR_WIDTH   5
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_ACTIVE_BONUS 8
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MIN_WIDTH    30
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MAX_WIDTH    64
#define EGUI_VIEW_TAB_VIEW_STANDARD_TAB_GAP          5

#define EGUI_VIEW_TAB_VIEW_COMPACT_PAD_X            5
#define EGUI_VIEW_TAB_VIEW_COMPACT_PAD_Y            5
#define EGUI_VIEW_TAB_VIEW_COMPACT_RADIUS           8
#define EGUI_VIEW_TAB_VIEW_COMPACT_TABS_HEIGHT      18
#define EGUI_VIEW_TAB_VIEW_COMPACT_BODY_RADIUS      6
#define EGUI_VIEW_TAB_VIEW_COMPACT_ADD_WIDTH        14
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_BASE_WIDTH   14
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_CHAR_WIDTH   4
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_ACTIVE_BONUS 6
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MIN_WIDTH    22
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MAX_WIDTH    46
#define EGUI_VIEW_TAB_VIEW_COMPACT_TAB_GAP          4

typedef struct egui_view_tab_view_layout_item egui_view_tab_view_layout_item_t;
struct egui_view_tab_view_layout_item
{
    uint8_t tab_index;
    egui_region_t tab_region;
    egui_region_t close_region;
    char label[16];
};

typedef struct egui_view_tab_view_metrics egui_view_tab_view_metrics_t;
struct egui_view_tab_view_metrics
{
    egui_region_t content_region;
    egui_region_t tabs_region;
    egui_region_t add_region;
    egui_region_t body_region;
    egui_view_tab_view_layout_item_t items[EGUI_VIEW_TAB_VIEW_MAX_TABS];
    uint8_t visible_count;
    uint8_t has_add;
    uint8_t has_close;
};

typedef struct egui_view_tab_view_hit egui_view_tab_view_hit_t;
struct egui_view_tab_view_hit
{
    uint8_t part;
    uint8_t tab_index;
};

static uint8_t egui_view_tab_view_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TAB_VIEW_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TAB_VIEW_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_tab_view_text_len(const char *text)
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

static void egui_view_tab_view_copy_elided(char *buffer, uint8_t buffer_size, const char *text, uint8_t max_chars)
{
    uint8_t length = 0;
    uint8_t copy_length;
    uint8_t i;

    if (buffer_size == 0)
    {
        return;
    }

    buffer[0] = '\0';
    if (text == NULL || max_chars == 0)
    {
        return;
    }

    while (text[length] != '\0')
    {
        length++;
    }

    if (length <= max_chars)
    {
        copy_length = length;
        if (copy_length >= buffer_size)
        {
            copy_length = buffer_size - 1;
        }
        for (i = 0; i < copy_length; i++)
        {
            buffer[i] = text[i];
        }
        buffer[copy_length] = '\0';
        return;
    }

    if (max_chars <= 3)
    {
        copy_length = max_chars;
        if (copy_length >= buffer_size)
        {
            copy_length = buffer_size - 1;
        }
        for (i = 0; i < copy_length; i++)
        {
            buffer[i] = '.';
        }
        buffer[copy_length] = '\0';
        return;
    }

    copy_length = max_chars - 3;
    if (copy_length > buffer_size - 4)
    {
        copy_length = buffer_size - 4;
    }
    for (i = 0; i < copy_length; i++)
    {
        buffer[i] = text[i];
    }
    buffer[copy_length + 0] = '.';
    buffer[copy_length + 1] = '.';
    buffer[copy_length + 2] = '.';
    buffer[copy_length + 3] = '\0';
}

static egui_color_t egui_view_tab_view_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 64);
}

static egui_color_t egui_view_tab_view_tone_color(egui_view_tab_view_t *local, uint8_t tone)
{
    switch (tone)
    {
    case EGUI_VIEW_TAB_VIEW_TONE_SUCCESS:
        return local->success_color;
    case EGUI_VIEW_TAB_VIEW_TONE_WARNING:
        return local->warning_color;
    case EGUI_VIEW_TAB_VIEW_TONE_NEUTRAL:
        return local->neutral_color;
    default:
        return local->accent_color;
    }
}

static const egui_view_tab_view_snapshot_t *egui_view_tab_view_get_snapshot(egui_view_tab_view_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }
    return &local->snapshots[local->current_snapshot];
}

static const egui_view_tab_view_tab_t *egui_view_tab_view_get_tab(egui_view_tab_view_t *local, uint8_t tab_index)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL || snapshot->tabs == NULL || tab_index >= snapshot->tab_count)
    {
        return NULL;
    }
    return &snapshot->tabs[tab_index];
}

static uint8_t egui_view_tab_view_is_tab_closed_inner(egui_view_tab_view_t *local, uint8_t tab_index)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL || tab_index >= snapshot->tab_count)
    {
        return 0;
    }
    return (local->closed_mask & (1u << tab_index)) ? 1 : 0;
}

static uint8_t egui_view_tab_view_get_visible_count_inner(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    uint8_t count = 0;
    uint8_t i;

    if (snapshot == NULL)
    {
        return 0;
    }
    for (i = 0; i < snapshot->tab_count; i++)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, i))
        {
            count++;
        }
    }
    return count;
}

static uint8_t egui_view_tab_view_find_first_visible(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_TAB_VIEW_TAB_NONE;
    }
    for (i = 0; i < snapshot->tab_count; i++)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, i))
        {
            return i;
        }
    }
    return EGUI_VIEW_TAB_VIEW_TAB_NONE;
}

static uint8_t egui_view_tab_view_find_last_visible(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    uint8_t i;

    if (snapshot == NULL || snapshot->tab_count == 0)
    {
        return EGUI_VIEW_TAB_VIEW_TAB_NONE;
    }
    for (i = snapshot->tab_count; i > 0; i--)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, (uint8_t)(i - 1)))
        {
            return (uint8_t)(i - 1);
        }
    }
    return EGUI_VIEW_TAB_VIEW_TAB_NONE;
}

static uint8_t egui_view_tab_view_find_next_visible(egui_view_tab_view_t *local, uint8_t from_index)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    uint8_t i;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_TAB_VIEW_TAB_NONE;
    }
    for (i = (uint8_t)(from_index + 1); i < snapshot->tab_count; i++)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, i))
        {
            return i;
        }
    }
    for (i = 0; i < snapshot->tab_count; i++)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, i))
        {
            return i;
        }
    }
    return EGUI_VIEW_TAB_VIEW_TAB_NONE;
}

static uint8_t egui_view_tab_view_find_prev_visible(egui_view_tab_view_t *local, uint8_t from_index)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    uint8_t i;

    if (snapshot == NULL || snapshot->tab_count == 0)
    {
        return EGUI_VIEW_TAB_VIEW_TAB_NONE;
    }
    for (i = from_index; i > 0; i--)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, (uint8_t)(i - 1)))
        {
            return (uint8_t)(i - 1);
        }
    }
    for (i = snapshot->tab_count; i > 0; i--)
    {
        if (!egui_view_tab_view_is_tab_closed_inner(local, (uint8_t)(i - 1)))
        {
            return (uint8_t)(i - 1);
        }
    }
    return EGUI_VIEW_TAB_VIEW_TAB_NONE;
}

static void egui_view_tab_view_resolve_current_tab(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL || snapshot->tab_count == 0)
    {
        local->current_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
        return;
    }
    if (local->current_tab >= snapshot->tab_count || egui_view_tab_view_is_tab_closed_inner(local, local->current_tab))
    {
        local->current_tab = egui_view_tab_view_find_first_visible(local);
    }
}

static uint8_t egui_view_tab_view_has_add_button(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL)
    {
        return 0;
    }
    return snapshot->add_enabled ? 1 : 0;
}

static uint8_t egui_view_tab_view_has_close_part(egui_view_tab_view_t *local)
{
    const egui_view_tab_view_tab_t *tab = egui_view_tab_view_get_tab(local, local->current_tab);

    if (tab == NULL || local->compact_mode)
    {
        return 0;
    }
    return tab->closable ? 1 : 0;
}

static void egui_view_tab_view_notify_changed(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

    if (local->on_changed)
    {
        local->on_changed(self, local->current_snapshot, local->current_tab, local->current_part);
    }
}

static void egui_view_tab_view_notify_action(egui_view_t *self, uint8_t action, uint8_t tab_index)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

    if (local->on_action)
    {
        local->on_action(self, action, tab_index);
    }
}

static void egui_view_tab_view_normalize_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

    if (local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE && !egui_view_tab_view_has_close_part(local))
    {
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    }
    else if (local->current_part == EGUI_VIEW_TAB_VIEW_PART_ADD && !egui_view_tab_view_has_add_button(local))
    {
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    }
}

static void egui_view_tab_view_set_current_snapshot_inner(egui_view_t *self, uint8_t snapshot_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot;

    if (local->snapshots == NULL || local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }

    local->current_snapshot = snapshot_index;
    local->closed_mask = 0;
    snapshot = egui_view_tab_view_get_snapshot(local);
    if (snapshot != NULL)
    {
        local->current_tab = snapshot->current_index < snapshot->tab_count ? snapshot->current_index : 0;
        if (snapshot->tab_count == 0)
        {
            local->current_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
        }
    }
    local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    local->pressed_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
    local->pressed_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    egui_view_set_pressed(self, 0);
    egui_view_tab_view_resolve_current_tab(local);
    egui_view_tab_view_normalize_part(self);
    if (notify)
    {
        egui_view_tab_view_notify_changed(self);
    }
    egui_view_invalidate(self);
}

static void egui_view_tab_view_set_current_tab_inner(egui_view_t *self, uint8_t tab_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL || snapshot->tab_count == 0 || tab_index >= snapshot->tab_count || egui_view_tab_view_is_tab_closed_inner(local, tab_index))
    {
        return;
    }
    if (local->current_tab == tab_index)
    {
        return;
    }

    local->current_tab = tab_index;
    if (local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE && !egui_view_tab_view_has_close_part(local))
    {
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    }
    if (notify)
    {
        egui_view_tab_view_notify_changed(self);
    }
    egui_view_invalidate(self);
}

static void egui_view_tab_view_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                         egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static egui_dim_t egui_view_tab_view_measure_tab_width(egui_view_tab_view_t *local, const egui_view_tab_view_tab_t *tab, uint8_t active, const char *label)
{
    egui_dim_t width = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_TAB_BASE_WIDTH : EGUI_VIEW_TAB_VIEW_STANDARD_TAB_BASE_WIDTH;
    uint8_t length = egui_view_tab_view_text_len(label);

    width += (egui_dim_t)length * (local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_TAB_CHAR_WIDTH : EGUI_VIEW_TAB_VIEW_STANDARD_TAB_CHAR_WIDTH);
    if (active)
    {
        width += local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_TAB_ACTIVE_BONUS : EGUI_VIEW_TAB_VIEW_STANDARD_TAB_ACTIVE_BONUS;
        if (tab != NULL && tab->closable && !local->compact_mode)
        {
            width += 14;
        }
    }
    if (local->compact_mode)
    {
        if (width < EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MIN_WIDTH)
        {
            width = EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MIN_WIDTH;
        }
        if (width > EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MAX_WIDTH)
        {
            width = EGUI_VIEW_TAB_VIEW_COMPACT_TAB_MAX_WIDTH;
        }
    }
    else
    {
        if (width < EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MIN_WIDTH)
        {
            width = EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MIN_WIDTH;
        }
        if (width > EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MAX_WIDTH)
        {
            width = EGUI_VIEW_TAB_VIEW_STANDARD_TAB_MAX_WIDTH;
        }
    }
    return width;
}

static void egui_view_tab_view_build_metrics(egui_view_t *self, egui_view_tab_view_metrics_t *metrics)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    egui_region_t work_region;
    egui_dim_t pad_x = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_PAD_X : EGUI_VIEW_TAB_VIEW_STANDARD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_PAD_Y : EGUI_VIEW_TAB_VIEW_STANDARD_PAD_Y;
    egui_dim_t tabs_h = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_TABS_HEIGHT : EGUI_VIEW_TAB_VIEW_STANDARD_TABS_HEIGHT;
    egui_dim_t add_w = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_ADD_WIDTH : EGUI_VIEW_TAB_VIEW_STANDARD_ADD_WIDTH;
    egui_dim_t gap = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_TAB_GAP : EGUI_VIEW_TAB_VIEW_STANDARD_TAB_GAP;
    egui_dim_t title_block_h = local->compact_mode ? 0 : (EGUI_VIEW_TAB_VIEW_STANDARD_HEADER_HEIGHT + EGUI_VIEW_TAB_VIEW_STANDARD_META_HEIGHT + 2);
    egui_dim_t body_gap = local->compact_mode ? 4 : 5;
    egui_dim_t tabs_w;
    egui_dim_t cursor_x;
    egui_dim_t total_width = 0;
    uint8_t visible_count = 0;
    uint8_t count = 0;
    uint8_t i;

    egui_view_get_work_region(self, &work_region);
    metrics->content_region.location.x = work_region.location.x + pad_x;
    metrics->content_region.location.y = work_region.location.y + pad_y;
    metrics->content_region.size.width = work_region.size.width - pad_x * 2;
    metrics->content_region.size.height = work_region.size.height - pad_y * 2;
    metrics->tabs_region.location.x = metrics->content_region.location.x;
    metrics->tabs_region.location.y = metrics->content_region.location.y + title_block_h;
    metrics->tabs_region.size.width = metrics->content_region.size.width;
    metrics->tabs_region.size.height = tabs_h;
    metrics->has_add = egui_view_tab_view_has_add_button(local);
    metrics->has_close = egui_view_tab_view_has_close_part(local);
    metrics->add_region.location.x = metrics->content_region.location.x + metrics->content_region.size.width - add_w;
    metrics->add_region.location.y = metrics->tabs_region.location.y + (tabs_h - add_w) / 2;
    metrics->add_region.size.width = add_w;
    metrics->add_region.size.height = add_w;
    if (!metrics->has_add)
    {
        metrics->add_region.size.width = 0;
        metrics->add_region.size.height = 0;
    }

    metrics->body_region.location.x = metrics->content_region.location.x;
    metrics->body_region.location.y = metrics->tabs_region.location.y + tabs_h + body_gap;
    metrics->body_region.size.width = metrics->content_region.size.width;
    metrics->body_region.size.height = metrics->content_region.size.height - title_block_h - tabs_h - body_gap;
    if (metrics->body_region.size.height < (local->compact_mode ? 24 : 44))
    {
        metrics->body_region.size.height = local->compact_mode ? 24 : 44;
    }

    for (i = 0; i < EGUI_VIEW_TAB_VIEW_MAX_TABS; i++)
    {
        metrics->items[i].tab_index = EGUI_VIEW_TAB_VIEW_TAB_NONE;
        metrics->items[i].tab_region.size.width = 0;
        metrics->items[i].close_region.size.width = 0;
        metrics->items[i].label[0] = '\0';
    }

    if (snapshot == NULL || snapshot->tabs == NULL || snapshot->tab_count == 0)
    {
        metrics->visible_count = 0;
        return;
    }
    if (snapshot->tab_count >= 4 && gap > (local->compact_mode ? 3 : 4))
    {
        gap--;
    }

    tabs_w = metrics->content_region.size.width;
    if (metrics->has_add)
    {
        tabs_w -= add_w + gap;
    }
    if (tabs_w < 20)
    {
        tabs_w = 20;
    }

    for (i = 0; i < snapshot->tab_count; i++)
    {
        const egui_view_tab_view_tab_t *tab = &snapshot->tabs[i];
        uint8_t active = i == local->current_tab;
        uint8_t max_chars = local->compact_mode ? (active ? 6 : 5) : (active ? 9 : 7);

        if (egui_view_tab_view_is_tab_closed_inner(local, i))
        {
            continue;
        }
        if (snapshot->tab_count >= 4 && max_chars > 1)
        {
            max_chars--;
        }
        if (snapshot->tab_count >= 5 && max_chars > 1)
        {
            max_chars--;
        }
        egui_view_tab_view_copy_elided(metrics->items[count].label, sizeof(metrics->items[count].label), tab->title, max_chars);
        metrics->items[count].tab_index = i;
        metrics->items[count].tab_region.size.width = egui_view_tab_view_measure_tab_width(local, tab, active, metrics->items[count].label);
        total_width += metrics->items[count].tab_region.size.width;
        count++;
    }

    visible_count = count;
    metrics->visible_count = visible_count;
    if (visible_count == 0)
    {
        return;
    }
    if (visible_count > 1)
    {
        total_width += gap * (visible_count - 1);
    }
    if (total_width > tabs_w)
    {
        egui_dim_t fallback_width = (tabs_w - gap * (visible_count - 1)) / visible_count;

        if (fallback_width < (local->compact_mode ? 18 : 22))
        {
            fallback_width = local->compact_mode ? 18 : 22;
        }
        total_width = 0;
        for (i = 0; i < visible_count; i++)
        {
            metrics->items[i].tab_region.size.width = fallback_width;
            total_width += fallback_width;
        }
        if (visible_count > 1)
        {
            total_width += gap * (visible_count - 1);
        }
    }

    cursor_x = metrics->tabs_region.location.x;
    if (total_width < tabs_w)
    {
        cursor_x += (tabs_w - total_width) / 2;
    }
    for (i = 0; i < visible_count; i++)
    {
        const egui_view_tab_view_tab_t *tab = egui_view_tab_view_get_tab(local, metrics->items[i].tab_index);
        uint8_t active = metrics->items[i].tab_index == local->current_tab;
        egui_dim_t close_w = local->compact_mode ? 0 : 10;

        metrics->items[i].tab_region.location.x = cursor_x;
        metrics->items[i].tab_region.location.y = metrics->tabs_region.location.y + (local->compact_mode ? 1 : 0);
        metrics->items[i].tab_region.size.height = tabs_h - (local->compact_mode ? 2 : 0);
        metrics->items[i].close_region.size.width = 0;
        metrics->items[i].close_region.size.height = 0;
        if (active && tab != NULL && tab->closable && !local->compact_mode)
        {
            metrics->items[i].close_region.location.x = cursor_x + metrics->items[i].tab_region.size.width - close_w - 4;
            metrics->items[i].close_region.location.y = metrics->items[i].tab_region.location.y + (metrics->items[i].tab_region.size.height - close_w) / 2;
            metrics->items[i].close_region.size.width = close_w;
            metrics->items[i].close_region.size.height = close_w;
        }
        cursor_x += metrics->items[i].tab_region.size.width + gap;
    }
}

static void egui_view_tab_view_resolve_hit(egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_view_tab_view_hit_t *hit)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    egui_view_tab_view_metrics_t metrics;
    uint8_t i;

    hit->part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    hit->tab_index = EGUI_VIEW_TAB_VIEW_TAB_NONE;
    egui_view_tab_view_build_metrics(self, &metrics);
    if (metrics.has_close)
    {
        for (i = 0; i < metrics.visible_count; i++)
        {
            if (metrics.items[i].close_region.size.width > 0 && egui_region_pt_in_rect(&metrics.items[i].close_region, x, y))
            {
                hit->part = EGUI_VIEW_TAB_VIEW_PART_CLOSE;
                hit->tab_index = metrics.items[i].tab_index;
                return;
            }
        }
    }
    if (metrics.has_add && egui_region_pt_in_rect(&metrics.add_region, x, y))
    {
        hit->part = EGUI_VIEW_TAB_VIEW_PART_ADD;
        hit->tab_index = local->current_tab;
        return;
    }
    for (i = 0; i < metrics.visible_count; i++)
    {
        if (egui_region_pt_in_rect(&metrics.items[i].tab_region, x, y))
        {
            hit->part = EGUI_VIEW_TAB_VIEW_PART_TAB;
            hit->tab_index = metrics.items[i].tab_index;
            return;
        }
    }
}

void egui_view_tab_view_set_snapshots(egui_view_t *self, const egui_view_tab_view_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_tab_view_clamp_snapshot_count(snapshot_count);
    local->current_snapshot = 0;
    local->closed_mask = 0;
    local->current_tab = 0;
    egui_view_tab_view_set_current_snapshot_inner(self, 0, 1);
}

void egui_view_tab_view_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    egui_view_tab_view_set_current_snapshot_inner(self, snapshot_index, 1);
}

uint8_t egui_view_tab_view_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    return local->current_snapshot;
}

void egui_view_tab_view_set_current_tab(egui_view_t *self, uint8_t tab_index)
{
    egui_view_tab_view_set_current_tab_inner(self, tab_index, 1);
}

uint8_t egui_view_tab_view_get_current_tab(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    return local->current_tab;
}

void egui_view_tab_view_set_current_part(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

    local->current_part = part;
    egui_view_tab_view_normalize_part(self);
    egui_view_tab_view_notify_changed(self);
    egui_view_invalidate(self);
}

uint8_t egui_view_tab_view_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    return local->current_part;
}

uint8_t egui_view_tab_view_close_current_tab(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    const egui_view_tab_view_tab_t *tab = egui_view_tab_view_get_tab(local, local->current_tab);
    uint8_t closed_tab;

    if (snapshot == NULL || tab == NULL || !tab->closable || egui_view_tab_view_get_visible_count_inner(local) <= 1)
    {
        return 0;
    }

    closed_tab = local->current_tab;
    local->closed_mask |= (uint8_t)(1u << closed_tab);
    local->current_tab = egui_view_tab_view_find_next_visible(local, closed_tab);
    if (local->current_tab == EGUI_VIEW_TAB_VIEW_TAB_NONE)
    {
        local->current_tab = egui_view_tab_view_find_first_visible(local);
    }
    local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    egui_view_tab_view_notify_action(self, EGUI_VIEW_TAB_VIEW_ACTION_CLOSE, closed_tab);
    egui_view_tab_view_notify_changed(self);
    egui_view_invalidate(self);
    return 1;
}

uint8_t egui_view_tab_view_restore_tabs(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);

    if (snapshot == NULL || !snapshot->add_enabled)
    {
        return 0;
    }
    if (local->closed_mask == 0)
    {
        return 0;
    }

    local->closed_mask = 0;
    local->current_tab = snapshot->current_index < snapshot->tab_count ? snapshot->current_index : 0;
    local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    egui_view_tab_view_notify_action(self, EGUI_VIEW_TAB_VIEW_ACTION_ADD, local->current_tab);
    egui_view_tab_view_notify_changed(self);
    egui_view_invalidate(self);
    return 1;
}

uint8_t egui_view_tab_view_get_visible_tab_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    return egui_view_tab_view_get_visible_count_inner(local);
}

uint8_t egui_view_tab_view_is_tab_closed(egui_view_t *self, uint8_t tab_index)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    return egui_view_tab_view_is_tab_closed_inner(local, tab_index);
}

uint8_t egui_view_tab_view_get_tab_region(egui_view_t *self, uint8_t tab_index, egui_region_t *region)
{
    egui_view_tab_view_metrics_t metrics;
    uint8_t i;

    if (region == NULL)
    {
        return 0;
    }
    egui_view_tab_view_build_metrics(self, &metrics);
    for (i = 0; i < metrics.visible_count; i++)
    {
        if (metrics.items[i].tab_index == tab_index)
        {
            *region = metrics.items[i].tab_region;
            return 1;
        }
    }
    return 0;
}

uint8_t egui_view_tab_view_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    egui_view_tab_view_metrics_t metrics;
    uint8_t i;

    if (region == NULL)
    {
        return 0;
    }
    egui_view_tab_view_build_metrics(self, &metrics);
    if (part == EGUI_VIEW_TAB_VIEW_PART_ADD && metrics.has_add)
    {
        *region = metrics.add_region;
        return 1;
    }
    if (part == EGUI_VIEW_TAB_VIEW_PART_CLOSE && metrics.has_close)
    {
        for (i = 0; i < metrics.visible_count; i++)
        {
            if (metrics.items[i].tab_index == local->current_tab && metrics.items[i].close_region.size.width > 0)
            {
                *region = metrics.items[i].close_region;
                return 1;
            }
        }
    }
    return 0;
}

void egui_view_tab_view_set_on_changed_listener(egui_view_t *self, egui_view_on_tab_view_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->on_changed = listener;
}

void egui_view_tab_view_set_on_action_listener(egui_view_t *self, egui_view_on_tab_view_action_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->on_action = listener;
}

void egui_view_tab_view_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tab_view_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tab_view_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->compact_mode = compact_mode ? 1 : 0;
    if (local->compact_mode && local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE)
    {
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    }
    egui_view_invalidate(self);
}

void egui_view_tab_view_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    local->read_only_mode = read_only_mode ? 1 : 0;
    local->pressed_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
    local->pressed_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    egui_view_set_pressed(self, 0);
    egui_view_invalidate(self);
}

void egui_view_tab_view_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t section_color,
                                    egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color,
                                    egui_color_t warning_color, egui_color_t neutral_color)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);

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

static void egui_view_tab_view_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    const egui_view_tab_view_snapshot_t *snapshot = egui_view_tab_view_get_snapshot(local);
    const egui_view_tab_view_tab_t *tab = egui_view_tab_view_get_tab(local, local->current_tab);
    egui_view_tab_view_metrics_t metrics;
    egui_region_t text_region;
    egui_color_t card_fill;
    egui_color_t card_border;
    egui_color_t body_fill;
    egui_color_t body_border;
    egui_color_t title_color;
    egui_color_t meta_color;
    egui_color_t tone_color;
    egui_color_t badge_fill;
    egui_color_t badge_text;
    egui_color_t footer_fill;
    egui_color_t footer_border;
    egui_color_t footer_text;
    egui_dim_t radius = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_RADIUS : EGUI_VIEW_TAB_VIEW_STANDARD_RADIUS;
    egui_dim_t body_radius = local->compact_mode ? EGUI_VIEW_TAB_VIEW_COMPACT_BODY_RADIUS : EGUI_VIEW_TAB_VIEW_STANDARD_BODY_RADIUS;
    egui_dim_t footer_h = local->compact_mode ? 11 : 12;
    egui_dim_t footer_w;
    egui_dim_t footer_x;
    egui_dim_t footer_y;
    egui_dim_t cursor_y;
    egui_dim_t content_limit_y;
    egui_dim_t body_pad_x = local->compact_mode ? 6 : 8;
    egui_dim_t body_pad_top = local->compact_mode ? 6 : 5;
    egui_dim_t body_pad_bottom = local->compact_mode ? 5 : 5;
    egui_dim_t body_title_h = 10;
    egui_dim_t body_line_h = local->compact_mode ? 0 : 9;
    egui_dim_t body_footer_gap = local->compact_mode ? 2 : 3;
    egui_dim_t badge_h = local->compact_mode ? 0 : 11;
    egui_dim_t badge_gap = local->compact_mode ? 0 : 3;
    egui_dim_t eyebrow_h = local->compact_mode ? 0 : 8;
    egui_dim_t eyebrow_gap = local->compact_mode ? 0 : 1;
    egui_dim_t title_gap = local->compact_mode ? 0 : 2;
    uint8_t enabled = egui_view_get_enable(self) ? 1 : 0;
    uint8_t show_badge = 0;
    uint8_t show_eyebrow = 0;
    uint8_t show_body_primary = 0;
    uint8_t show_body_secondary = 0;
    char window_title_buf[28];
    char window_meta_buf[32];
    char eyebrow_buf[20];
    char body_title_buf[20];
    char body_primary_buf[32];
    char body_secondary_buf[32];
    char footer_buf[20];
    uint8_t i;

    if (snapshot == NULL)
    {
        return;
    }

    egui_view_tab_view_build_metrics(self, &metrics);
    if (metrics.content_region.size.width <= 0 || metrics.content_region.size.height <= 0)
    {
        return;
    }

    egui_view_tab_view_copy_elided(window_title_buf, sizeof(window_title_buf), snapshot->window_title, local->compact_mode ? 14 : 26);
    egui_view_tab_view_copy_elided(window_meta_buf, sizeof(window_meta_buf), snapshot->window_meta, local->compact_mode ? 16 : 28);
    egui_view_tab_view_copy_elided(eyebrow_buf, sizeof(eyebrow_buf), tab ? tab->eyebrow : NULL, local->compact_mode ? 10 : 18);
    egui_view_tab_view_copy_elided(body_title_buf, sizeof(body_title_buf), tab ? tab->title : NULL, local->compact_mode ? 12 : 18);
    egui_view_tab_view_copy_elided(body_primary_buf, sizeof(body_primary_buf), tab ? tab->body_primary : NULL, local->compact_mode ? 14 : 28);
    egui_view_tab_view_copy_elided(body_secondary_buf, sizeof(body_secondary_buf), tab ? tab->body_secondary : NULL, local->compact_mode ? 14 : 28);
    egui_view_tab_view_copy_elided(footer_buf, sizeof(footer_buf), tab ? tab->footer : NULL, local->compact_mode ? 10 : 14);

    tone_color = tab ? egui_view_tab_view_tone_color(local, tab->tone) : local->accent_color;
    card_fill = egui_rgb_mix(local->surface_color, tone_color, local->compact_mode ? 5 : 6);
    card_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 16 : 18);
    body_fill = egui_rgb_mix(local->section_color, tone_color, local->compact_mode ? 12 : 14);
    body_border = egui_rgb_mix(local->border_color, tone_color, local->compact_mode ? 18 : 24);
    title_color = local->text_color;
    meta_color = egui_rgb_mix(local->muted_text_color, tone_color, 14);
    badge_fill = egui_rgb_mix(local->surface_color, tone_color, 18);
    badge_text = tone_color;
    footer_fill = egui_rgb_mix(local->surface_color, tone_color, 16);
    footer_border = egui_rgb_mix(local->border_color, tone_color, 20);
    footer_text = tone_color;

    if (local->read_only_mode)
    {
        card_fill = egui_rgb_mix(card_fill, local->surface_color, 18);
        card_border = egui_rgb_mix(card_border, local->muted_text_color, 20);
        body_fill = egui_rgb_mix(body_fill, local->surface_color, 26);
        body_border = egui_rgb_mix(body_border, local->muted_text_color, 18);
        title_color = egui_rgb_mix(title_color, local->muted_text_color, 12);
        meta_color = egui_rgb_mix(meta_color, local->muted_text_color, 14);
        badge_fill = egui_rgb_mix(badge_fill, local->surface_color, 24);
        badge_text = egui_rgb_mix(badge_text, local->muted_text_color, 28);
        footer_fill = egui_rgb_mix(footer_fill, local->surface_color, 22);
        footer_border = egui_rgb_mix(footer_border, local->muted_text_color, 20);
        footer_text = egui_rgb_mix(footer_text, local->muted_text_color, 24);
    }
    if (!enabled)
    {
        card_fill = egui_view_tab_view_mix_disabled(card_fill);
        card_border = egui_view_tab_view_mix_disabled(card_border);
        body_fill = egui_view_tab_view_mix_disabled(body_fill);
        body_border = egui_view_tab_view_mix_disabled(body_border);
        title_color = egui_view_tab_view_mix_disabled(title_color);
        meta_color = egui_view_tab_view_mix_disabled(meta_color);
        badge_fill = egui_view_tab_view_mix_disabled(badge_fill);
        badge_text = egui_view_tab_view_mix_disabled(badge_text);
        footer_fill = egui_view_tab_view_mix_disabled(footer_fill);
        footer_border = egui_view_tab_view_mix_disabled(footer_border);
        footer_text = egui_view_tab_view_mix_disabled(footer_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                          metrics.content_region.size.height + 4, radius, card_fill, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(metrics.content_region.location.x - 2, metrics.content_region.location.y - 2, metrics.content_region.size.width + 4,
                                     metrics.content_region.size.height + 4, radius, 1, card_border, egui_color_alpha_mix(self->alpha, 54));

    if (!local->compact_mode)
    {
        text_region.location.x = metrics.content_region.location.x + 2;
        text_region.location.y = metrics.content_region.location.y;
        text_region.size.width = metrics.content_region.size.width - 4;
        text_region.size.height = EGUI_VIEW_TAB_VIEW_STANDARD_HEADER_HEIGHT;
        egui_view_tab_view_draw_text(local->font, self, window_title_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);

        text_region.location.y += EGUI_VIEW_TAB_VIEW_STANDARD_HEADER_HEIGHT + 1;
        text_region.size.height = EGUI_VIEW_TAB_VIEW_STANDARD_META_HEIGHT;
        egui_view_tab_view_draw_text(local->meta_font, self, window_meta_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
    }

    for (i = 0; i < metrics.visible_count; i++)
    {
        const egui_view_tab_view_tab_t *item = egui_view_tab_view_get_tab(local, metrics.items[i].tab_index);
        uint8_t active = metrics.items[i].tab_index == local->current_tab;
        egui_color_t tab_fill = egui_rgb_mix(local->surface_color, tone_color, active ? 10 : 4);
        egui_color_t tab_border = egui_rgb_mix(local->border_color, tone_color, active ? 22 : 10);
        egui_color_t tab_text = active ? title_color : egui_rgb_mix(local->muted_text_color, title_color, 10);
        egui_region_t label_region = metrics.items[i].tab_region;

        if (local->read_only_mode && active)
        {
            tab_fill = egui_rgb_mix(tab_fill, local->surface_color, 14);
            tab_border = egui_rgb_mix(tab_border, local->muted_text_color, 18);
        }
        if (!enabled)
        {
            tab_fill = egui_view_tab_view_mix_disabled(tab_fill);
            tab_border = egui_view_tab_view_mix_disabled(tab_border);
            tab_text = egui_view_tab_view_mix_disabled(tab_text);
        }

        egui_canvas_draw_round_rectangle_fill(label_region.location.x, label_region.location.y, label_region.size.width, label_region.size.height,
                                              local->compact_mode ? 5 : 6, tab_fill, egui_color_alpha_mix(self->alpha, active ? 98 : 84));
        egui_canvas_draw_round_rectangle(label_region.location.x, label_region.location.y, label_region.size.width, label_region.size.height,
                                         local->compact_mode ? 5 : 6, 1, tab_border, egui_color_alpha_mix(self->alpha, active ? 46 : 28));

        if (active)
        {
            egui_dim_t underline_inset = label_region.size.width / 5;

            if (underline_inset < 5)
            {
                underline_inset = 5;
            }
            if (underline_inset > 10)
            {
                underline_inset = 10;
            }
            egui_canvas_draw_line(label_region.location.x + underline_inset, label_region.location.y + label_region.size.height - 1,
                                  label_region.location.x + label_region.size.width - underline_inset, label_region.location.y + label_region.size.height - 1,
                                  1, tone_color, egui_color_alpha_mix(self->alpha, local->current_part == EGUI_VIEW_TAB_VIEW_PART_TAB ? 100 : 78));
        }
        if (item != NULL && item->dirty && !local->compact_mode)
        {
            egui_canvas_draw_round_rectangle_fill(label_region.location.x + 5, label_region.location.y + 5, 4, 4, 2, tone_color,
                                                  egui_color_alpha_mix(self->alpha, 100));
            label_region.location.x += 6;
            label_region.size.width -= 6;
        }

        if (metrics.items[i].close_region.size.width > 0)
        {
            egui_color_t close_fill = egui_rgb_mix(local->surface_color, tone_color, local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE ? 18 : 8);
            egui_color_t close_text = egui_rgb_mix(title_color, tone_color, 18);
            egui_color_t close_border = egui_rgb_mix(local->border_color, tone_color, local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE ? 34 : 16);

            egui_canvas_draw_round_rectangle_fill(metrics.items[i].close_region.location.x, metrics.items[i].close_region.location.y,
                                                  metrics.items[i].close_region.size.width, metrics.items[i].close_region.size.height, 4, close_fill,
                                                  egui_color_alpha_mix(self->alpha, 92));
            egui_canvas_draw_round_rectangle(metrics.items[i].close_region.location.x, metrics.items[i].close_region.location.y,
                                             metrics.items[i].close_region.size.width, metrics.items[i].close_region.size.height, 4, 1, close_border,
                                             egui_color_alpha_mix(self->alpha, local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE ? 62 : 28));
            text_region = metrics.items[i].close_region;
            egui_view_tab_view_draw_text(local->meta_font, self, "x", &text_region, EGUI_ALIGN_CENTER, close_text);
            label_region.size.width -= metrics.items[i].close_region.size.width + 4;
        }
        label_region.location.x += local->compact_mode ? 5 : 7;
        label_region.size.width -= local->compact_mode ? 10 : 14;
        egui_view_tab_view_draw_text(local->font, self, metrics.items[i].label, &label_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, tab_text);
    }

    if (metrics.has_add)
    {
        egui_color_t add_fill = egui_rgb_mix(local->surface_color, local->accent_color, local->current_part == EGUI_VIEW_TAB_VIEW_PART_ADD ? 16 : 8);
        egui_color_t add_border = egui_rgb_mix(local->border_color, local->accent_color, local->current_part == EGUI_VIEW_TAB_VIEW_PART_ADD ? 26 : 14);
        egui_color_t add_text = local->accent_color;

        if (local->read_only_mode)
        {
            add_fill = egui_rgb_mix(add_fill, local->surface_color, 20);
            add_border = egui_rgb_mix(add_border, local->muted_text_color, 18);
            add_text = egui_rgb_mix(add_text, local->muted_text_color, 22);
        }
        if (!enabled)
        {
            add_fill = egui_view_tab_view_mix_disabled(add_fill);
            add_border = egui_view_tab_view_mix_disabled(add_border);
            add_text = egui_view_tab_view_mix_disabled(add_text);
        }

        egui_canvas_draw_round_rectangle_fill(metrics.add_region.location.x, metrics.add_region.location.y, metrics.add_region.size.width,
                                              metrics.add_region.size.height, metrics.add_region.size.height / 2, add_fill,
                                              egui_color_alpha_mix(self->alpha, 92));
        egui_canvas_draw_round_rectangle(metrics.add_region.location.x, metrics.add_region.location.y, metrics.add_region.size.width,
                                         metrics.add_region.size.height, metrics.add_region.size.height / 2, 1, add_border,
                                         egui_color_alpha_mix(self->alpha, local->current_part == EGUI_VIEW_TAB_VIEW_PART_ADD ? 60 : 40));
        egui_view_tab_view_draw_text(local->font, self, "+", &metrics.add_region, EGUI_ALIGN_CENTER, add_text);
    }

    egui_canvas_draw_round_rectangle_fill(metrics.body_region.location.x, metrics.body_region.location.y, metrics.body_region.size.width,
                                          metrics.body_region.size.height, body_radius, body_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(metrics.body_region.location.x, metrics.body_region.location.y, metrics.body_region.size.width,
                                     metrics.body_region.size.height, body_radius, 1, body_border, egui_color_alpha_mix(self->alpha, 38));

    if (tab == NULL)
    {
        const char *empty_body = metrics.has_add ? "Use + to restore the track" : "No preview available";

        if (local->read_only_mode)
        {
            empty_body = "Reference shell is locked";
        }
        text_region.location.x = metrics.body_region.location.x + 8;
        text_region.location.y = metrics.body_region.location.y + 8;
        text_region.size.width = metrics.body_region.size.width - 16;
        text_region.size.height = 12;
        egui_view_tab_view_draw_text(local->font, self, "All tabs closed", &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
        text_region.location.y += 14;
        text_region.size.height = 10;
        egui_view_tab_view_draw_text(local->meta_font, self, empty_body, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
        return;
    }

    footer_x = metrics.body_region.location.x + body_pad_x;
    footer_y = metrics.body_region.location.y + metrics.body_region.size.height - footer_h - body_pad_bottom;
    if (footer_y < metrics.body_region.location.y + body_pad_top)
    {
        footer_y = metrics.body_region.location.y + body_pad_top;
    }
    content_limit_y = footer_y - body_footer_gap;

    cursor_y = metrics.body_region.location.y + body_pad_top;
    text_region.location.x = metrics.body_region.location.x + body_pad_x;
    text_region.location.y = cursor_y;
    text_region.size.width = metrics.body_region.size.width - body_pad_x * 2;

    if (!local->compact_mode && tab->badge != NULL && tab->badge[0] != '\0')
    {
        show_badge = 1;
    }
    if (!local->compact_mode && tab->eyebrow != NULL && tab->eyebrow[0] != '\0')
    {
        show_eyebrow = 1;
    }
    if (!local->compact_mode && tab->body_primary != NULL && tab->body_primary[0] != '\0')
    {
        show_body_primary = 1;
    }
    if (!local->compact_mode && tab->body_secondary != NULL && tab->body_secondary[0] != '\0')
    {
        show_body_secondary = 1;
    }

    if (show_badge &&
        cursor_y + badge_h + badge_gap + (show_eyebrow ? (eyebrow_h + eyebrow_gap) : 0) + body_title_h + (show_body_primary ? (title_gap + body_line_h) : 0) >
                content_limit_y)
    {
        show_badge = 0;
    }
    if (show_eyebrow &&
        cursor_y + (show_badge ? (badge_h + badge_gap) : 0) + eyebrow_h + eyebrow_gap + body_title_h + (show_body_primary ? (title_gap + body_line_h) : 0) >
                content_limit_y)
    {
        show_eyebrow = 0;
    }
    if (show_body_primary &&
        cursor_y + (show_badge ? (badge_h + badge_gap) : 0) + (show_eyebrow ? (eyebrow_h + eyebrow_gap) : 0) + body_title_h + title_gap + body_line_h >
                content_limit_y)
    {
        show_body_primary = 0;
    }
    if (show_body_secondary && cursor_y + (show_badge ? (badge_h + badge_gap) : 0) + (show_eyebrow ? (eyebrow_h + eyebrow_gap) : 0) + body_title_h +
                                               (show_body_primary ? (title_gap + body_line_h) : 0) + title_gap + body_line_h >
                                       content_limit_y)
    {
        show_body_secondary = 0;
    }

    if (show_badge)
    {
        egui_dim_t badge_w = 18 + egui_view_tab_view_text_len(tab->badge) * 4;
        egui_region_t badge_region;

        if (badge_w > text_region.size.width)
        {
            badge_w = text_region.size.width;
        }
        egui_canvas_draw_round_rectangle_fill(text_region.location.x, text_region.location.y, badge_w, badge_h, badge_h / 2, badge_fill,
                                              egui_color_alpha_mix(self->alpha, 98));
        badge_region.location.x = text_region.location.x;
        badge_region.location.y = text_region.location.y;
        badge_region.size.width = badge_w;
        badge_region.size.height = badge_h;
        egui_view_tab_view_draw_text(local->meta_font, self, tab->badge, &badge_region, EGUI_ALIGN_CENTER, badge_text);
        cursor_y += badge_h + badge_gap;
        text_region.location.y = cursor_y;
    }
    if (show_eyebrow)
    {
        text_region.size.height = eyebrow_h;
        egui_view_tab_view_draw_text(local->meta_font, self, eyebrow_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
        cursor_y += eyebrow_h + eyebrow_gap;
        text_region.location.y = cursor_y;
    }

    if (local->compact_mode)
    {
        text_region.size.height = body_title_h;
        egui_view_tab_view_draw_text(local->font, self, body_title_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
    }
    else
    {
        text_region.size.height = body_title_h;
        egui_view_tab_view_draw_text(local->font, self, body_title_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, title_color);
        if (show_body_primary)
        {
            cursor_y += body_title_h + title_gap;
            text_region.location.y = cursor_y;
            text_region.size.height = body_line_h;
            egui_view_tab_view_draw_text(local->meta_font, self, body_primary_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
            if (show_body_secondary)
            {
                cursor_y += body_line_h + title_gap;
                text_region.location.y = cursor_y;
                egui_view_tab_view_draw_text(local->meta_font, self, body_secondary_buf, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, meta_color);
            }
        }
    }

    if (footer_buf[0] == '\0')
    {
        return;
    }

    footer_w = (local->compact_mode ? 18 : 18) + egui_view_tab_view_text_len(footer_buf) * 4;
    if (footer_w > metrics.body_region.size.width - body_pad_x * 2)
    {
        footer_w = metrics.body_region.size.width - body_pad_x * 2;
    }
    egui_canvas_draw_round_rectangle_fill(footer_x, footer_y, footer_w, footer_h, footer_h / 2, footer_fill, egui_color_alpha_mix(self->alpha, 98));
    egui_canvas_draw_round_rectangle(footer_x, footer_y, footer_w, footer_h, footer_h / 2, 1, footer_border, egui_color_alpha_mix(self->alpha, 34));
    egui_view_tab_view_draw_text(local->meta_font, self, footer_buf, &((egui_region_t){{footer_x, footer_y}, {footer_w, footer_h}}), EGUI_ALIGN_CENTER,
                                 footer_text);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_tab_view_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    egui_view_tab_view_hit_t hit;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        egui_view_tab_view_resolve_hit(self, event->location.x, event->location.y, &hit);
        if (hit.tab_index == EGUI_VIEW_TAB_VIEW_TAB_NONE && hit.part != EGUI_VIEW_TAB_VIEW_PART_ADD)
        {
            return 0;
        }
        local->pressed_tab = hit.tab_index;
        local->pressed_part = hit.part;
        egui_view_set_pressed(self, 1);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        egui_view_tab_view_resolve_hit(self, event->location.x, event->location.y, &hit);
        if (local->pressed_part == hit.part && local->pressed_tab == hit.tab_index)
        {
            if (hit.part == EGUI_VIEW_TAB_VIEW_PART_TAB && hit.tab_index != EGUI_VIEW_TAB_VIEW_TAB_NONE)
            {
                egui_view_tab_view_set_current_tab_inner(self, hit.tab_index, 1);
            }
            else if (hit.part == EGUI_VIEW_TAB_VIEW_PART_CLOSE)
            {
                egui_view_tab_view_close_current_tab(self);
            }
            else if (hit.part == EGUI_VIEW_TAB_VIEW_PART_ADD)
            {
                egui_view_tab_view_restore_tabs(self);
            }
        }
        local->pressed_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
        local->pressed_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_set_pressed(self, 0);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
        local->pressed_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_set_pressed(self, 0);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_tab_view_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_tab_view_t);
    uint8_t next_part[3];
    uint8_t part_count = 0;

    if (!egui_view_get_enable(self) || local->read_only_mode || event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return 0;
    }

    next_part[part_count++] = EGUI_VIEW_TAB_VIEW_PART_TAB;
    if (egui_view_tab_view_has_close_part(local))
    {
        next_part[part_count++] = EGUI_VIEW_TAB_VIEW_PART_CLOSE;
    }
    if (egui_view_tab_view_has_add_button(local))
    {
        next_part[part_count++] = EGUI_VIEW_TAB_VIEW_PART_ADD;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_tab_view_set_current_tab_inner(self, egui_view_tab_view_find_prev_visible(local, local->current_tab), 1);
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_tab_view_set_current_tab_inner(self, egui_view_tab_view_find_next_visible(local, local->current_tab), 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_tab_view_set_current_tab_inner(self, egui_view_tab_view_find_first_visible(local), 1);
        return 1;
    case EGUI_KEY_CODE_END:
        local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
        egui_view_tab_view_set_current_tab_inner(self, egui_view_tab_view_find_last_visible(local), 1);
        return 1;
    case EGUI_KEY_CODE_TAB:
    {
        uint8_t i;

        for (i = 0; i < part_count; i++)
        {
            if (next_part[i] == local->current_part)
            {
                local->current_part = next_part[(i + 1) % part_count];
                egui_view_tab_view_notify_changed(self);
                egui_view_invalidate(self);
                return 1;
            }
        }
        local->current_part = next_part[0];
        egui_view_tab_view_notify_changed(self);
        egui_view_invalidate(self);
        return 1;
    }
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        if (local->current_part == EGUI_VIEW_TAB_VIEW_PART_CLOSE)
        {
            return egui_view_tab_view_close_current_tab(self);
        }
        if (local->current_part == EGUI_VIEW_TAB_VIEW_PART_ADD)
        {
            return egui_view_tab_view_restore_tabs(self);
        }
        return 0;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->current_part != EGUI_VIEW_TAB_VIEW_PART_TAB)
        {
            local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
            egui_view_tab_view_notify_changed(self);
            egui_view_invalidate(self);
            return 1;
        }
        return 0;
    default:
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tab_view_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_tab_view_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tab_view_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_tab_view_on_key_event,
#endif
};

void egui_view_tab_view_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tab_view_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tab_view_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_changed = NULL;
    local->on_action = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD6DEE7);
    local->section_color = EGUI_COLOR_HEX(0xF6F8FA);
    local->text_color = EGUI_COLOR_HEX(0x1E2933);
    local->muted_text_color = EGUI_COLOR_HEX(0x708090);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB87A16);
    local->neutral_color = EGUI_COLOR_HEX(0x7A8795);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
    local->current_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->pressed_tab = EGUI_VIEW_TAB_VIEW_TAB_NONE;
    local->pressed_part = EGUI_VIEW_TAB_VIEW_PART_TAB;
    local->closed_mask = 0;

    egui_view_set_view_name(self, "egui_view_tab_view");
}
