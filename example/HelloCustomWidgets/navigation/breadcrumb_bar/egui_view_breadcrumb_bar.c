#include <stdlib.h>

#include "egui_view_breadcrumb_bar.h"

#define EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM     0
#define EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW 1

typedef struct egui_view_breadcrumb_bar_display_entry egui_view_breadcrumb_bar_display_entry_t;
struct egui_view_breadcrumb_bar_display_entry
{
    uint8_t kind;
    uint8_t item_index;
};

static egui_dim_t egui_view_breadcrumb_bar_measure_entry(uint8_t compact_mode, uint8_t is_current, const char *text);

static uint8_t egui_view_breadcrumb_bar_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_BREADCRUMB_BAR_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_BREADCRUMB_BAR_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_breadcrumb_bar_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_BREADCRUMB_BAR_MAX_ITEMS)
    {
        return EGUI_VIEW_BREADCRUMB_BAR_MAX_ITEMS;
    }
    return count;
}

static egui_color_t egui_view_breadcrumb_bar_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 62);
}

static uint8_t egui_view_breadcrumb_bar_text_len(const char *text)
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

static egui_dim_t egui_view_breadcrumb_bar_separator_gap(uint8_t compact_mode)
{
    return compact_mode ? 5 : 10;
}

static void egui_view_breadcrumb_bar_copy_elided(char *buffer, uint8_t buffer_size, const char *text, uint8_t max_chars)
{
    uint8_t length = 0;
    uint8_t copy_length;
    uint8_t i;

    if (buffer_size == 0)
    {
        return;
    }

    buffer[0] = '\0';
    if (text == NULL)
    {
        return;
    }

    while (text[length] != '\0')
    {
        length++;
    }
    if (max_chars == 0)
    {
        return;
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

static void egui_view_breadcrumb_bar_set_item_entry(egui_view_breadcrumb_bar_display_entry_t *entry, uint8_t item_index)
{
    entry->kind = EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM;
    entry->item_index = item_index;
}

static void egui_view_breadcrumb_bar_set_overflow_entry(egui_view_breadcrumb_bar_display_entry_t *entry)
{
    entry->kind = EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW;
    entry->item_index = 0;
}

static uint8_t egui_view_breadcrumb_bar_prepare_entry_label(const egui_view_breadcrumb_bar_snapshot_t *snapshot, uint8_t compact_mode, uint8_t current_item,
                                                            const egui_view_breadcrumb_bar_display_entry_t *entry, char *label, uint8_t label_size)
{
    uint8_t is_current = 0;

    if (entry->kind == EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW)
    {
        label[0] = '.';
        label[1] = '.';
        label[2] = '.';
        label[3] = '\0';
        return 0;
    }

    is_current = entry->item_index == current_item;
    egui_view_breadcrumb_bar_copy_elided(label, label_size, snapshot->items[entry->item_index], compact_mode ? (is_current ? 6 : 5) : (is_current ? 10 : 8));
    return is_current;
}

static egui_dim_t egui_view_breadcrumb_bar_measure_entry_set(const egui_view_breadcrumb_bar_snapshot_t *snapshot, uint8_t compact_mode, uint8_t current_item,
                                                             const egui_view_breadcrumb_bar_display_entry_t *entries, uint8_t entry_count)
{
    egui_dim_t total_width = 0;
    uint8_t i;

    for (i = 0; i < entry_count; i++)
    {
        char label[16];
        uint8_t is_current = egui_view_breadcrumb_bar_prepare_entry_label(snapshot, compact_mode, current_item, &entries[i], label, sizeof(label));

        total_width += egui_view_breadcrumb_bar_measure_entry(compact_mode, is_current, label);
        if (i < entry_count - 1)
        {
            total_width += egui_view_breadcrumb_bar_separator_gap(compact_mode);
        }
    }

    return total_width;
}

static uint8_t egui_view_breadcrumb_bar_copy_entries(egui_view_breadcrumb_bar_display_entry_t *dst, const egui_view_breadcrumb_bar_display_entry_t *src,
                                                     uint8_t count)
{
    uint8_t i;

    for (i = 0; i < count; i++)
    {
        dst[i] = src[i];
    }
    return count;
}

static uint8_t egui_view_breadcrumb_bar_build_entries(const egui_view_breadcrumb_bar_snapshot_t *snapshot, uint8_t compact_mode, egui_dim_t available_width,
                                                      egui_view_breadcrumb_bar_display_entry_t *entries)
{
    uint8_t item_count = egui_view_breadcrumb_bar_clamp_item_count(snapshot->item_count);
    uint8_t current_item = snapshot->current_item;
    egui_view_breadcrumb_bar_display_entry_t candidates[EGUI_VIEW_BREADCRUMB_BAR_MAX_ITEMS];
    uint8_t count = 0;
    uint8_t i;

    if (item_count == 0)
    {
        return 0;
    }

    if (current_item >= item_count)
    {
        current_item = item_count - 1;
    }

    for (i = 0; i < item_count; i++)
    {
        egui_view_breadcrumb_bar_set_item_entry(&candidates[count], i);
        count++;
    }
    if (egui_view_breadcrumb_bar_measure_entry_set(snapshot, compact_mode, current_item, candidates, count) <= available_width)
    {
        return egui_view_breadcrumb_bar_copy_entries(entries, candidates, count);
    }

    count = 0;
    egui_view_breadcrumb_bar_set_item_entry(&candidates[count], 0);
    count++;

    if (compact_mode)
    {
        if (current_item > 1)
        {
            egui_view_breadcrumb_bar_set_overflow_entry(&candidates[count]);
            count++;
        }
        if (current_item > 0)
        {
            egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
            count++;
        }
        if (egui_view_breadcrumb_bar_measure_entry_set(snapshot, compact_mode, current_item, candidates, count) <= available_width)
        {
            return egui_view_breadcrumb_bar_copy_entries(entries, candidates, count);
        }

        if (current_item > 0)
        {
            count = 0;
            egui_view_breadcrumb_bar_set_item_entry(&candidates[count], 0);
            count++;
            egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
            count++;
            if (egui_view_breadcrumb_bar_measure_entry_set(snapshot, compact_mode, current_item, candidates, count) <= available_width)
            {
                return egui_view_breadcrumb_bar_copy_entries(entries, candidates, count);
            }
        }

        count = 0;
        egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
        return egui_view_breadcrumb_bar_copy_entries(entries, candidates, 1);
    }

    if (current_item > 2)
    {
        egui_view_breadcrumb_bar_set_overflow_entry(&candidates[count]);
        count++;
    }

    if (current_item > 1)
    {
        egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item - 1);
        count++;
    }

    if (current_item > 0)
    {
        egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
        count++;
    }
    if (egui_view_breadcrumb_bar_measure_entry_set(snapshot, compact_mode, current_item, candidates, count) <= available_width)
    {
        return egui_view_breadcrumb_bar_copy_entries(entries, candidates, count);
    }

    count = 0;
    egui_view_breadcrumb_bar_set_item_entry(&candidates[count], 0);
    count++;
    if (current_item > 1)
    {
        egui_view_breadcrumb_bar_set_overflow_entry(&candidates[count]);
        count++;
    }
    if (current_item > 0)
    {
        egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
        count++;
    }
    if (egui_view_breadcrumb_bar_measure_entry_set(snapshot, compact_mode, current_item, candidates, count) <= available_width)
    {
        return egui_view_breadcrumb_bar_copy_entries(entries, candidates, count);
    }

    count = 0;
    egui_view_breadcrumb_bar_set_item_entry(&candidates[count], current_item);
    return egui_view_breadcrumb_bar_copy_entries(entries, candidates, 1);
}

static egui_dim_t egui_view_breadcrumb_bar_measure_entry(uint8_t compact_mode, uint8_t is_current, const char *text)
{
    egui_dim_t width;
    uint8_t length = egui_view_breadcrumb_bar_text_len(text);

    if (text != NULL && text[0] == '.' && text[1] == '.' && text[2] == '.')
    {
        return compact_mode ? 10 : 14;
    }

    width = 8 + (egui_dim_t)length * (compact_mode ? 4 : 5);
    if (is_current)
    {
        width += compact_mode ? 10 : 12;
    }
    if (compact_mode)
    {
        if (width > (is_current ? 34 : 22))
        {
            width = is_current ? 30 : 22;
        }
    }
    else if (width > (is_current ? 68 : 46))
    {
        width = is_current ? 68 : 46;
    }
    if (width < (compact_mode ? 16 : 20))
    {
        width = compact_mode ? 16 : 20;
    }
    return width;
}

void egui_view_breadcrumb_bar_set_snapshots(egui_view_t *self, const egui_view_breadcrumb_bar_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_breadcrumb_bar_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (local->current_snapshot == snapshot_index)
    {
        return;
    }
    local->current_snapshot = snapshot_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_breadcrumb_bar_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    return local->current_snapshot;
}

void egui_view_breadcrumb_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_bar_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                          egui_color_t muted_text_color, egui_color_t accent_color)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    egui_view_invalidate(self);
}

static void egui_view_breadcrumb_bar_draw_text(egui_view_breadcrumb_bar_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y,
                                               egui_dim_t width, egui_dim_t height, uint8_t align, egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, align, color, self->alpha);
}

static void egui_view_breadcrumb_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_bar_t);
    egui_region_t region;
    const egui_view_breadcrumb_bar_snapshot_t *snapshot;
    egui_view_breadcrumb_bar_display_entry_t entries[4];
    egui_color_t fill_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t accent_color;
    egui_alpha_t fill_alpha;
    egui_alpha_t border_alpha;
    egui_dim_t container_radius;
    egui_dim_t current_radius;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_w;
    egui_dim_t content_h;
    egui_dim_t total_width;
    egui_dim_t cursor_x;
    egui_dim_t center_y;
    egui_dim_t entry_height;
    egui_dim_t current_x = 0;
    egui_dim_t current_w = 0;
    uint8_t entry_count;
    uint8_t item_count;
    uint8_t current_item;
    uint8_t i;
    uint8_t is_enabled;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    item_count = egui_view_breadcrumb_bar_clamp_item_count(snapshot->item_count);
    if (snapshot->items == NULL || item_count == 0)
    {
        return;
    }

    current_item = snapshot->current_item;
    if (current_item >= item_count)
    {
        current_item = item_count - 1;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    fill_color = egui_rgb_mix(local->surface_color, local->accent_color, local->compact_mode ? 2 : 1);
    border_color = local->border_color;
    text_color = local->text_color;
    muted_text_color = local->muted_text_color;
    accent_color = local->accent_color;
    fill_alpha = local->compact_mode ? 84 : 82;
    border_alpha = local->compact_mode ? 78 : 66;
    container_radius = local->compact_mode ? 5 : 8;
    current_radius = local->compact_mode ? 4 : 6;

    if (local->locked_mode)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_HEX(0xF7F9FB), 64);
        border_color = egui_rgb_mix(border_color, muted_text_color, 16);
        text_color = egui_rgb_mix(text_color, muted_text_color, 58);
        accent_color = egui_rgb_mix(accent_color, muted_text_color, 86);
        fill_alpha = 82;
        border_alpha = 52;
    }
    else if (local->compact_mode)
    {
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xFFFFFF), 14);
    }
    else if (!local->compact_mode)
    {
        fill_color = egui_rgb_mix(fill_color, EGUI_COLOR_HEX(0xFFFFFF), 16);
        border_color = egui_rgb_mix(border_color, EGUI_COLOR_HEX(0xFFFFFF), 28);
    }

    if (!is_enabled)
    {
        fill_color = egui_view_breadcrumb_bar_mix_disabled(fill_color);
        border_color = egui_view_breadcrumb_bar_mix_disabled(border_color);
        text_color = egui_view_breadcrumb_bar_mix_disabled(text_color);
        muted_text_color = egui_view_breadcrumb_bar_mix_disabled(muted_text_color);
        accent_color = egui_view_breadcrumb_bar_mix_disabled(accent_color);
        fill_alpha = 80;
        border_alpha = 58;
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, container_radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, container_radius, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, border_alpha));

    content_x = region.location.x + (local->compact_mode ? 8 : 10);
    content_y = region.location.y + (local->compact_mode ? 7 : 7);
    content_w = region.size.width - (local->compact_mode ? 16 : 20);
    content_h = region.size.height - (local->compact_mode ? 14 : 15);
    if (content_w <= 0 || content_h <= 0)
    {
        return;
    }

    entry_count = egui_view_breadcrumb_bar_build_entries(snapshot, local->compact_mode, content_w, entries);
    total_width = egui_view_breadcrumb_bar_measure_entry_set(snapshot, local->compact_mode, current_item, entries, entry_count);
    cursor_x = content_x;
    if (total_width < content_w)
    {
        cursor_x = content_x + (content_w - total_width) / 2;
    }
    center_y = content_y + content_h / 2;
    entry_height = local->compact_mode ? 15 : 19;

    for (i = 0; i < entry_count; i++)
    {
        char label[16];
        egui_dim_t entry_width;
        egui_dim_t entry_y;
        uint8_t is_current = 0;

        is_current = egui_view_breadcrumb_bar_prepare_entry_label(snapshot, local->compact_mode, current_item, &entries[i], label, sizeof(label));

        entry_width = egui_view_breadcrumb_bar_measure_entry(local->compact_mode, is_current, label);
        entry_y = center_y - entry_height / 2;

        if (entries[i].kind == EGUI_VIEW_BREADCRUMB_BAR_ENTRY_OVERFLOW)
        {
            egui_view_breadcrumb_bar_draw_text(local, self, label, cursor_x, entry_y, entry_width, entry_height, EGUI_ALIGN_CENTER, muted_text_color);
        }
        else if (is_current)
        {
            uint8_t fill_mix = local->locked_mode ? (local->compact_mode ? 4 : 5) : (local->compact_mode ? 9 : 6);
            uint8_t border_mix = local->locked_mode ? (local->compact_mode ? 8 : 10) : (local->compact_mode ? 22 : 16);
            egui_color_t current_fill = egui_rgb_mix(fill_color, accent_color, fill_mix);
            egui_color_t current_border = egui_rgb_mix(border_color, accent_color, border_mix);
            egui_alpha_t current_fill_alpha = local->compact_mode ? 88 : 82;
            egui_alpha_t current_border_alpha = local->compact_mode ? 94 : 86;

            egui_canvas_draw_round_rectangle_fill(cursor_x, entry_y, entry_width, entry_height, current_radius, current_fill,
                                                  egui_color_alpha_mix(self->alpha, current_fill_alpha));
            egui_canvas_draw_round_rectangle(cursor_x, entry_y, entry_width, entry_height, current_radius, 1, current_border,
                                             egui_color_alpha_mix(self->alpha, current_border_alpha));
            egui_view_breadcrumb_bar_draw_text(local, self, label, cursor_x + 2, entry_y, entry_width - 4, entry_height, EGUI_ALIGN_CENTER, text_color);
            current_x = cursor_x;
            current_w = entry_width;
        }
        else
        {
            egui_color_t entry_color = muted_text_color;
            uint8_t is_neighbor =
                    (i + 1 < entry_count) && entries[i + 1].kind == EGUI_VIEW_BREADCRUMB_BAR_ENTRY_ITEM && entries[i + 1].item_index == current_item;

            if (!local->compact_mode && is_neighbor)
            {
                entry_color = egui_rgb_mix(muted_text_color, text_color, 18);
            }
            else if (!local->compact_mode && i > 0)
            {
                entry_color = egui_rgb_mix(muted_text_color, text_color, 6);
            }
            egui_view_breadcrumb_bar_draw_text(local, self, label, cursor_x, entry_y, entry_width, entry_height, EGUI_ALIGN_LEFT, entry_color);
        }

        cursor_x += entry_width;
        if (i < entry_count - 1)
        {
            egui_dim_t chevron_x = cursor_x + (local->compact_mode ? 4 : 5);
            egui_dim_t chevron_y = center_y;
            egui_color_t chevron_color = local->compact_mode ? muted_text_color : egui_rgb_mix(muted_text_color, accent_color, 10);
            egui_alpha_t chevron_alpha = local->locked_mode ? 40 : (local->compact_mode ? 52 : 70);

            egui_canvas_draw_line(chevron_x, chevron_y - 3, chevron_x + 4, chevron_y, 1, chevron_color, egui_color_alpha_mix(self->alpha, chevron_alpha));
            egui_canvas_draw_line(chevron_x, chevron_y + 3, chevron_x + 4, chevron_y, 1, chevron_color, egui_color_alpha_mix(self->alpha, chevron_alpha));
            cursor_x += egui_view_breadcrumb_bar_separator_gap(local->compact_mode);
        }
    }

    if (!local->compact_mode)
    {
        egui_dim_t line_y = region.location.y + region.size.height - 6;

        egui_canvas_draw_line(content_x, line_y, content_x + content_w, line_y, 1, border_color, egui_color_alpha_mix(self->alpha, 24));
        if (current_w > 10)
        {
            egui_dim_t accent_start = current_x + 5;
            egui_dim_t accent_end = current_x + current_w - 5;

            egui_canvas_draw_line(accent_start, line_y, accent_end, line_y, 1, accent_color, egui_color_alpha_mix(self->alpha, 74));
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_breadcrumb_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_breadcrumb_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_breadcrumb_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_breadcrumb_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_breadcrumb_bar_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DEE7);
    local->text_color = EGUI_COLOR_HEX(0x1A2530);
    local->muted_text_color = EGUI_COLOR_HEX(0x66717F);
    local->accent_color = EGUI_COLOR_HEX(0x2563EB);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
