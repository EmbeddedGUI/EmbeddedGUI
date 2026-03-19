#include "egui_view_menu_bar.h"

#include "resource/egui_icon_material_symbols.h"

static uint8_t egui_view_menu_bar_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_MENU_BAR_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_MENU_BAR_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_menu_bar_clamp_menu_count(uint8_t count)
{
    if (count > EGUI_VIEW_MENU_BAR_MAX_MENUS)
    {
        return EGUI_VIEW_MENU_BAR_MAX_MENUS;
    }
    return count;
}

static uint8_t egui_view_menu_bar_clamp_panel_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_MENU_BAR_MAX_PANEL_ITEMS)
    {
        return EGUI_VIEW_MENU_BAR_MAX_PANEL_ITEMS;
    }
    return count;
}

static egui_color_t egui_view_menu_bar_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, 64);
}

static uint8_t egui_view_menu_bar_text_len(const char *text)
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

static egui_color_t egui_view_menu_bar_tone_color(egui_view_menu_bar_t *local, uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return local->success_color;
    case 2:
        return local->warning_color;
    case 3:
        return local->danger_color;
    default:
        return local->accent_color;
    }
}

static egui_dim_t egui_view_menu_bar_measure_menu_width(uint8_t compact_mode, const char *title, uint8_t active, uint8_t emphasized)
{
    egui_dim_t width = (compact_mode ? 10 : 18) + (egui_dim_t)egui_view_menu_bar_text_len(title) * (compact_mode ? 3 : 5);

    if (active)
    {
        width += compact_mode ? 6 : 12;
    }
    else if (emphasized)
    {
        width += compact_mode ? 3 : 6;
    }

    if (compact_mode)
    {
        if (width < 18)
        {
            width = 18;
        }
        if (width > 38)
        {
            width = 38;
        }
    }
    else
    {
        if (width < 26)
        {
            width = 26;
        }
        if (width > 60)
        {
            width = 60;
        }
    }

    return width;
}

static void egui_view_menu_bar_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align,
                                         egui_color_t color, egui_alpha_t alpha)
{
    egui_region_t draw_region;

    if (font == NULL || text == NULL || text[0] == '\0' || region == NULL)
    {
        return;
    }

    draw_region = *region;
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, egui_color_alpha_mix(self->alpha, alpha));
}

static const egui_view_menu_bar_snapshot_t *egui_view_menu_bar_get_snapshot(egui_view_menu_bar_t *local)
{
    if (local->snapshots == NULL || local->snapshot_count == 0 || local->current_snapshot >= local->snapshot_count)
    {
        return NULL;
    }

    return &local->snapshots[local->current_snapshot];
}

typedef struct egui_view_menu_bar_layout egui_view_menu_bar_layout_t;
struct egui_view_menu_bar_layout
{
    egui_region_t region;
    egui_region_t top_region;
    egui_region_t top_item_regions[EGUI_VIEW_MENU_BAR_MAX_MENUS];
    egui_region_t panel_region;
    egui_region_t panel_item_regions[EGUI_VIEW_MENU_BAR_MAX_PANEL_ITEMS];
    uint8_t menu_count;
    uint8_t panel_item_count;
    uint8_t current_menu;
    uint8_t panel_visible;
};

static uint8_t egui_view_menu_bar_item_is_enabled(const egui_view_menu_bar_snapshot_t *snapshot, uint8_t item_index, uint8_t item_count)
{
    if (snapshot == NULL || snapshot->panel_items == NULL || item_index >= item_count)
    {
        return 0;
    }

    return snapshot->panel_items[item_index].enabled ? 1 : 0;
}

static uint8_t egui_view_menu_bar_menu_is_enabled(const egui_view_menu_bar_snapshot_t *snapshot, uint8_t menu_index, uint8_t menu_count)
{
    if (snapshot == NULL || snapshot->menus == NULL || menu_index >= menu_count)
    {
        return 0;
    }

    return snapshot->menus[menu_index].enabled ? 1 : 0;
}

static uint8_t egui_view_menu_bar_get_snapshot_menu_index(const egui_view_menu_bar_snapshot_t *snapshot)
{
    uint8_t menu_count;

    if (snapshot == NULL)
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    menu_count = egui_view_menu_bar_clamp_menu_count(snapshot->menu_count);
    if (menu_count == 0)
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    if (snapshot->current_menu < menu_count)
    {
        return snapshot->current_menu;
    }

    return 0;
}

static uint8_t egui_view_menu_bar_snapshot_is_enabled(egui_view_menu_bar_t *local, uint8_t snapshot_index)
{
    const egui_view_menu_bar_snapshot_t *snapshot;
    uint8_t menu_index;
    uint8_t menu_count;

    if (local == NULL || local->snapshots == NULL || snapshot_index >= local->snapshot_count)
    {
        return 0;
    }

    snapshot = &local->snapshots[snapshot_index];
    menu_count = egui_view_menu_bar_clamp_menu_count(snapshot->menu_count);
    menu_index = egui_view_menu_bar_get_snapshot_menu_index(snapshot);
    if (menu_index == EGUI_VIEW_MENU_BAR_HIT_NONE)
    {
        return 0;
    }

    return egui_view_menu_bar_menu_is_enabled(snapshot, menu_index, menu_count);
}

static uint8_t egui_view_menu_bar_resolve_snapshot_index(egui_view_menu_bar_t *local)
{
    uint8_t index;

    if (local == NULL || local->snapshot_count == 0)
    {
        return 0;
    }

    if (local->current_snapshot < local->snapshot_count && egui_view_menu_bar_snapshot_is_enabled(local, local->current_snapshot))
    {
        return local->current_snapshot;
    }

    for (index = 0; index < local->snapshot_count; index++)
    {
        if (egui_view_menu_bar_snapshot_is_enabled(local, index))
        {
            return index;
        }
    }

    if (local->current_snapshot < local->snapshot_count)
    {
        return local->current_snapshot;
    }

    return 0;
}

static uint8_t egui_view_menu_bar_find_enabled_snapshot(egui_view_menu_bar_t *local, int16_t start_index, int8_t step)
{
    int16_t index;

    if (local == NULL || local->snapshot_count == 0 || step == 0)
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    index = start_index;
    while (1)
    {
        index += step;
        if (index < 0 || index >= local->snapshot_count)
        {
            return EGUI_VIEW_MENU_BAR_HIT_NONE;
        }
        if (egui_view_menu_bar_snapshot_is_enabled(local, (uint8_t)index))
        {
            return (uint8_t)index;
        }
    }
}

static uint8_t egui_view_menu_bar_find_edge_enabled_snapshot(egui_view_menu_bar_t *local, uint8_t from_end)
{
    int16_t index;

    if (local == NULL || local->snapshot_count == 0)
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    if (from_end)
    {
        for (index = (int16_t)local->snapshot_count - 1; index >= 0; index--)
        {
            if (egui_view_menu_bar_snapshot_is_enabled(local, (uint8_t)index))
            {
                return (uint8_t)index;
            }
        }
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    for (index = 0; index < local->snapshot_count; index++)
    {
        if (egui_view_menu_bar_snapshot_is_enabled(local, (uint8_t)index))
        {
            return (uint8_t)index;
        }
    }

    return EGUI_VIEW_MENU_BAR_HIT_NONE;
}

static uint8_t egui_view_menu_bar_resolve_focus_item(const egui_view_menu_bar_snapshot_t *snapshot, uint8_t item_count)
{
    uint8_t index;

    if (snapshot == NULL || snapshot->panel_items == NULL || item_count == 0)
    {
        return EGUI_VIEW_MENU_BAR_ITEM_NONE;
    }

    if (snapshot->focus_item < item_count && egui_view_menu_bar_item_is_enabled(snapshot, snapshot->focus_item, item_count))
    {
        return snapshot->focus_item;
    }

    for (index = 0; index < item_count; index++)
    {
        if (egui_view_menu_bar_item_is_enabled(snapshot, index, item_count))
        {
            return index;
        }
    }

    if (snapshot->focus_item < item_count)
    {
        return snapshot->focus_item;
    }

    return 0;
}

static uint8_t egui_view_menu_bar_clamp_current_item(egui_view_menu_bar_t *local, const egui_view_menu_bar_snapshot_t *snapshot)
{
    uint8_t item_count = snapshot == NULL ? 0 : egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);

    if (item_count == 0)
    {
        return EGUI_VIEW_MENU_BAR_ITEM_NONE;
    }
    if (local->current_item < item_count && egui_view_menu_bar_item_is_enabled(snapshot, local->current_item, item_count))
    {
        return local->current_item;
    }
    return egui_view_menu_bar_resolve_focus_item(snapshot, item_count);
}

static void egui_view_menu_bar_notify_selection_changed(egui_view_t *self, egui_view_menu_bar_t *local)
{
    if (local->on_selection_changed != NULL)
    {
        local->on_selection_changed(self, local->current_snapshot, local->current_item);
    }
}

static void egui_view_menu_bar_notify_item_activated(egui_view_t *self, egui_view_menu_bar_t *local)
{
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    uint8_t item_count = snapshot == NULL ? 0 : egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);

    if (local->on_item_activated == NULL || snapshot == NULL || local->current_item >= item_count ||
        !egui_view_menu_bar_item_is_enabled(snapshot, local->current_item, item_count))
    {
        return;
    }

    local->on_item_activated(self, local->current_snapshot, local->current_item);
}

static void egui_view_menu_bar_apply_snapshot(egui_view_t *self, uint8_t snapshot_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot;
    uint8_t item_count;
    uint8_t next_item;
    uint8_t changed = 0;

    if (local->snapshot_count == 0 || snapshot_index >= local->snapshot_count)
    {
        return;
    }
    if (!egui_view_menu_bar_snapshot_is_enabled(local, snapshot_index))
    {
        return;
    }

    snapshot = &local->snapshots[snapshot_index];
    item_count = egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    next_item = egui_view_menu_bar_resolve_focus_item(snapshot, item_count);

    if (local->current_snapshot != snapshot_index || local->current_item != next_item)
    {
        changed = 1;
    }

    local->current_snapshot = snapshot_index;
    local->current_item = next_item;
    local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);

    if (notify && changed)
    {
        egui_view_menu_bar_notify_selection_changed(self, local);
    }
}

static void egui_view_menu_bar_set_current_item_inner(egui_view_t *self, uint8_t item_index, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    uint8_t item_count;
    uint8_t next_item;

    if (snapshot == NULL)
    {
        return;
    }

    item_count = egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    if (item_count == 0)
    {
        local->current_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
        local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
        egui_view_invalidate(self);
        return;
    }

    next_item = item_index;
    if (next_item >= item_count || !egui_view_menu_bar_item_is_enabled(snapshot, next_item, item_count))
    {
        next_item = egui_view_menu_bar_resolve_focus_item(snapshot, item_count);
    }

    if (local->current_item == next_item)
    {
        return;
    }

    local->current_item = next_item;
    egui_view_invalidate(self);
    if (notify)
    {
        egui_view_menu_bar_notify_selection_changed(self, local);
    }
}

static void egui_view_menu_bar_move_current_item(egui_view_t *self, int8_t step)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    uint8_t item_count;
    int16_t index;

    if (snapshot == NULL || step == 0)
    {
        return;
    }

    item_count = egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    if (item_count == 0)
    {
        return;
    }

    index = local->current_item;
    if (index < 0 || index >= item_count)
    {
        index = egui_view_menu_bar_resolve_focus_item(snapshot, item_count);
    }

    while (1)
    {
        index += step;
        if (index < 0 || index >= item_count)
        {
            return;
        }
        if (egui_view_menu_bar_item_is_enabled(snapshot, (uint8_t)index, item_count))
        {
            egui_view_menu_bar_set_current_item_inner(self, (uint8_t)index, 1);
            return;
        }
    }
}

static uint8_t egui_view_menu_bar_build_layout(egui_view_menu_bar_t *local, egui_view_t *self, const egui_view_menu_bar_snapshot_t *snapshot,
                                               egui_view_menu_bar_layout_t *layout)
{
    egui_dim_t top_h;
    egui_dim_t top_w;
    egui_dim_t top_content_x;
    egui_dim_t top_content_y;
    egui_dim_t top_content_w;
    egui_dim_t cursor_x;
    egui_dim_t total_menu_w = 0;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t row_h;
    egui_dim_t row_y;
    uint8_t index;

    egui_view_get_work_region(self, &layout->region);
    if (layout->region.size.width <= 0 || layout->region.size.height <= 0 || snapshot == NULL || snapshot->menus == NULL)
    {
        return 0;
    }

    layout->menu_count = egui_view_menu_bar_clamp_menu_count(snapshot->menu_count);
    layout->panel_item_count = egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    if (layout->menu_count == 0)
    {
        return 0;
    }

    layout->current_menu = snapshot->current_menu;
    if (layout->current_menu >= layout->menu_count)
    {
        layout->current_menu = 0;
    }

    top_h = local->compact_mode ? 22 : 26;
    top_w = layout->region.size.width - (local->compact_mode ? 8 : 10);
    layout->top_region.location.x = layout->region.location.x + (layout->region.size.width - top_w) / 2;
    layout->top_region.location.y = layout->region.location.y + (local->compact_mode ? 6 : 7);
    layout->top_region.size.width = top_w;
    layout->top_region.size.height = top_h;

    top_content_x = layout->top_region.location.x + (local->compact_mode ? 6 : 8);
    top_content_y = layout->top_region.location.y + 3;
    top_content_w = top_w - (local->compact_mode ? 12 : 16);

    for (index = 0; index < layout->menu_count; index++)
    {
        total_menu_w += egui_view_menu_bar_measure_menu_width(local->compact_mode, snapshot->menus[index].title, index == layout->current_menu,
                                                              snapshot->menus[index].emphasized);
        if (index + 1 < layout->menu_count)
        {
            total_menu_w += local->compact_mode ? 3 : 4;
        }
    }

    cursor_x = top_content_x;
    if (total_menu_w < top_content_w)
    {
        cursor_x = top_content_x + (top_content_w - total_menu_w) / 2;
    }

    for (index = 0; index < layout->menu_count; index++)
    {
        egui_dim_t item_w = egui_view_menu_bar_measure_menu_width(local->compact_mode, snapshot->menus[index].title, index == layout->current_menu,
                                                                  snapshot->menus[index].emphasized);

        layout->top_item_regions[index].location.x = cursor_x;
        layout->top_item_regions[index].location.y = top_content_y;
        layout->top_item_regions[index].size.width = item_w;
        layout->top_item_regions[index].size.height = local->compact_mode ? 15 : 18;
        cursor_x += item_w + (local->compact_mode ? 3 : 4);
    }

    layout->panel_visible = (snapshot->show_panel && layout->panel_item_count > 0 && !local->locked_mode) ? 1 : 0;
    if (!layout->panel_visible)
    {
        layout->panel_region.location.x = 0;
        layout->panel_region.location.y = 0;
        layout->panel_region.size.width = 0;
        layout->panel_region.size.height = 0;
        return 1;
    }

    panel_w = layout->region.size.width - (local->compact_mode ? 12 : 20);
    row_h = 16;
    panel_h = layout->panel_item_count * row_h + (local->compact_mode ? 12 : 10);
    layout->panel_region.location.x =
            layout->top_item_regions[layout->current_menu].location.x + layout->top_item_regions[layout->current_menu].size.width / 2 - panel_w / 2;
    if (layout->panel_region.location.x < layout->region.location.x + (local->compact_mode ? 3 : 6))
    {
        layout->panel_region.location.x = layout->region.location.x + (local->compact_mode ? 3 : 6);
    }
    if (layout->panel_region.location.x + panel_w > layout->region.location.x + layout->region.size.width - (local->compact_mode ? 3 : 6))
    {
        layout->panel_region.location.x = layout->region.location.x + layout->region.size.width - (local->compact_mode ? 3 : 6) - panel_w;
    }
    layout->panel_region.location.y = layout->top_region.location.y + layout->top_region.size.height + 4;
    layout->panel_region.size.width = panel_w;
    layout->panel_region.size.height = panel_h;

    row_y = layout->panel_region.location.y + (local->compact_mode ? 6 : 5);
    for (index = 0; index < layout->panel_item_count; index++)
    {
        layout->panel_item_regions[index].location.x = layout->panel_region.location.x + 4;
        layout->panel_item_regions[index].location.y = row_y;
        layout->panel_item_regions[index].size.width = layout->panel_region.size.width - 8;
        layout->panel_item_regions[index].size.height = row_h;
        row_y += row_h;
    }

    return 1;
}

static uint8_t egui_view_menu_bar_hit_panel_item(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    egui_view_menu_bar_layout_t layout;
    uint8_t index;

    if (!egui_view_menu_bar_build_layout(local, self, snapshot, &layout) || !layout.panel_visible)
    {
        return EGUI_VIEW_MENU_BAR_ITEM_NONE;
    }

    for (index = 0; index < layout.panel_item_count; index++)
    {
        if (egui_region_pt_in_rect(&layout.panel_item_regions[index], x, y))
        {
            return index;
        }
    }

    return EGUI_VIEW_MENU_BAR_ITEM_NONE;
}

void egui_view_menu_bar_set_snapshots(egui_view_t *self, const egui_view_menu_bar_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot;
    uint8_t item_count;

    local->snapshots = snapshots;
    local->snapshot_count = egui_view_menu_bar_clamp_snapshot_count(snapshot_count);
    local->current_snapshot = egui_view_menu_bar_resolve_snapshot_index(local);
    snapshot = egui_view_menu_bar_get_snapshot(local);
    item_count = snapshot == NULL ? 0 : egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    local->current_item = egui_view_menu_bar_resolve_focus_item(snapshot, item_count);
    local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
    egui_view_set_pressed(self, false);
    egui_view_invalidate(self);
}

void egui_view_menu_bar_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    egui_view_menu_bar_apply_snapshot(self, snapshot_index, 1);
}

uint8_t egui_view_menu_bar_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    return local->current_snapshot;
}

void egui_view_menu_bar_set_current_item(egui_view_t *self, uint8_t item_index)
{
    egui_view_menu_bar_set_current_item_inner(self, item_index, 1);
}

uint8_t egui_view_menu_bar_get_current_item(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    return local->current_item;
}

void egui_view_menu_bar_activate_current_item(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    egui_view_menu_bar_notify_item_activated(self, local);
}

uint8_t egui_view_menu_bar_hit_menu(egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    egui_view_menu_bar_layout_t layout;
    uint8_t index;

    if (!egui_view_menu_bar_build_layout(local, self, snapshot, &layout))
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }
    if (!egui_region_pt_in_rect(&layout.top_region, x, y))
    {
        return EGUI_VIEW_MENU_BAR_HIT_NONE;
    }

    for (index = 0; index < layout.menu_count; index++)
    {
        if (egui_region_pt_in_rect(&layout.top_item_regions[index], x, y))
        {
            return index;
        }
    }

    return EGUI_VIEW_MENU_BAR_HIT_NONE;
}

void egui_view_menu_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_menu_bar_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->meta_font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_menu_bar_set_on_selection_changed_listener(egui_view_t *self, egui_view_on_menu_bar_selection_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->on_selection_changed = listener;
}

void egui_view_menu_bar_set_on_item_activated_listener(egui_view_t *self, egui_view_on_menu_bar_item_activated_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->on_item_activated = listener;
}

void egui_view_menu_bar_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->compact_mode = compact_mode ? 1 : 0;
    local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
    egui_view_invalidate(self);
}

void egui_view_menu_bar_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->locked_mode = locked_mode ? 1 : 0;
    local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
    egui_view_invalidate(self);
}

void egui_view_menu_bar_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                    egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t success_color, egui_color_t warning_color,
                                    egui_color_t danger_color, egui_color_t shadow_color)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->success_color = success_color;
    local->warning_color = warning_color;
    local->danger_color = danger_color;
    local->shadow_color = shadow_color;
    egui_view_invalidate(self);
}

static uint8_t egui_view_menu_bar_get_summary_strip_region(egui_view_menu_bar_t *local, const egui_region_t *region, egui_region_t *strip_region)
{
    strip_region->location.x = region->location.x + (local->compact_mode ? 6 : 8);
    strip_region->location.y = region->location.y + region->size.height - (local->compact_mode ? 20 : 22);
    strip_region->size.width = region->size.width - (local->compact_mode ? 12 : 16);
    strip_region->size.height = local->compact_mode ? 14 : 16;
    if (strip_region->size.width <= 0 || strip_region->size.height <= 0)
    {
        return 0;
    }

    return 1;
}

static void egui_view_menu_bar_draw_summary_anchor(egui_view_t *self, egui_view_menu_bar_t *local, const egui_view_menu_bar_layout_t *layout,
                                                   egui_color_t fill_color, egui_color_t border_color)
{
    egui_region_t strip_region;
    egui_color_t cap_fill;
    egui_color_t cap_border;
    egui_dim_t cap_h;
    egui_dim_t cap_w;
    egui_dim_t cap_x;
    egui_dim_t cap_y;
    egui_dim_t center_x;
    egui_dim_t strip_right;

    if (layout->current_menu >= layout->menu_count)
    {
        return;
    }
    if (!egui_view_menu_bar_get_summary_strip_region(local, &layout->region, &strip_region))
    {
        return;
    }

    center_x = layout->top_item_regions[layout->current_menu].location.x + layout->top_item_regions[layout->current_menu].size.width / 2;
    cap_w = local->compact_mode ? 10 : 12;
    cap_h = local->compact_mode ? 3 : 4;
    cap_x = center_x - cap_w / 2;
    cap_y = strip_region.location.y - 1;
    strip_right = strip_region.location.x + strip_region.size.width;
    if (cap_x < strip_region.location.x + 6)
    {
        cap_x = strip_region.location.x + 6;
    }
    if (cap_x + cap_w > strip_right - 6)
    {
        cap_x = strip_right - 6 - cap_w;
    }
    if (cap_w <= 0 || cap_h <= 0)
    {
        return;
    }

    cap_fill = egui_rgb_mix(fill_color, local->accent_color, local->locked_mode ? 18 : 30);
    cap_border = egui_rgb_mix(border_color, local->accent_color, local->locked_mode ? 14 : 22);
    egui_canvas_draw_round_rectangle_fill(cap_x, cap_y, cap_w, cap_h, 1, cap_fill, egui_color_alpha_mix(self->alpha, local->locked_mode ? 78 : 94));
    egui_canvas_draw_round_rectangle(cap_x, cap_y, cap_w, cap_h, 1, 1, cap_border, egui_color_alpha_mix(self->alpha, local->locked_mode ? 60 : 76));
}

static void egui_view_menu_bar_draw_summary_strip(egui_view_t *self, egui_view_menu_bar_t *local, const egui_view_menu_bar_snapshot_t *snapshot,
                                                  egui_region_t *region, egui_color_t fill_color, egui_color_t border_color, egui_color_t text_color,
                                                  egui_color_t muted_text_color)
{
    egui_region_t cue_region;
    egui_region_t lock_region;
    egui_region_t meta_chip_region;
    egui_region_t strip_region;
    egui_region_t text_region;
    egui_region_t meta_region;
    egui_color_t cue_color;
    egui_color_t meta_chip_border_color;
    egui_color_t meta_chip_fill_color;
    egui_color_t meta_text_color;
    egui_color_t title_text_color;
    egui_dim_t meta_gap;
    egui_dim_t meta_chip_w;
    uint8_t menu_index;

    if (snapshot->panel_items == NULL || snapshot->panel_item_count == 0)
    {
        return;
    }

    menu_index = egui_view_menu_bar_get_snapshot_menu_index(snapshot);
    if (menu_index == EGUI_VIEW_MENU_BAR_HIT_NONE)
    {
        return;
    }

    if (!egui_view_menu_bar_get_summary_strip_region(local, region, &strip_region))
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(strip_region.location.x, strip_region.location.y, strip_region.size.width, strip_region.size.height,
                                          local->compact_mode ? 4 : 5, egui_rgb_mix(fill_color, local->accent_color, 5), egui_color_alpha_mix(self->alpha, 70));
    egui_canvas_draw_round_rectangle(strip_region.location.x, strip_region.location.y, strip_region.size.width, strip_region.size.height,
                                     local->compact_mode ? 4 : 5, 1, egui_rgb_mix(border_color, local->accent_color, 7), egui_color_alpha_mix(self->alpha, 48));

    text_region = strip_region;
    text_region.location.x += local->compact_mode ? 6 : 7;
    text_region.size.width -= local->compact_mode ? 12 : 14;

    cue_region.location.x = text_region.location.x;
    cue_region.location.y = strip_region.location.y + 2;
    cue_region.size.width = local->compact_mode ? 4 : 5;
    cue_region.size.height = strip_region.size.height - 4;
    cue_color = egui_view_menu_bar_tone_color(local, snapshot->panel_items[0].tone);
    if (snapshot->panel_items[0].tone == 0)
    {
        cue_color = egui_rgb_mix(cue_color, local->accent_color, 18);
    }
    if (!snapshot->panel_items[0].enabled)
    {
        cue_color = egui_view_menu_bar_mix_disabled(cue_color);
    }
    if (local->locked_mode)
    {
        cue_color = egui_rgb_mix(cue_color, muted_text_color, 22);
    }

    if (local->locked_mode)
    {
        lock_region.location.x = strip_region.location.x + 2;
        lock_region.location.y = strip_region.location.y + 2;
        lock_region.size.width = local->compact_mode ? 9 : 10;
        lock_region.size.height = strip_region.size.height - 4;
        egui_canvas_draw_round_rectangle_fill(lock_region.location.x, lock_region.location.y, lock_region.size.width, lock_region.size.height,
                                              local->compact_mode ? 3 : 4, egui_rgb_mix(fill_color, border_color, 14), egui_color_alpha_mix(self->alpha, 82));
        egui_canvas_draw_round_rectangle(lock_region.location.x, lock_region.location.y, lock_region.size.width, lock_region.size.height,
                                         local->compact_mode ? 3 : 4, 1, egui_rgb_mix(border_color, muted_text_color, 10),
                                         egui_color_alpha_mix(self->alpha, 56));
        egui_view_menu_bar_draw_text(EGUI_FONT_ICON_MS_16, self, EGUI_ICON_MS_LOCK, &lock_region, EGUI_ALIGN_CENTER,
                                     egui_rgb_mix(muted_text_color, border_color, 8), EGUI_ALPHA_100);
        text_region.location.x += local->compact_mode ? 10 : 11;
        text_region.size.width -= local->compact_mode ? 10 : 11;
        cue_region.location.x = text_region.location.x;
    }

    if (cue_region.size.width > 0 && cue_region.size.height > 0)
    {
        egui_canvas_draw_round_rectangle_fill(cue_region.location.x, cue_region.location.y, cue_region.size.width, cue_region.size.height,
                                              cue_region.size.width > 2 ? 1 : 0, cue_color, egui_color_alpha_mix(self->alpha, local->locked_mode ? 68 : 88));
        text_region.location.x += local->compact_mode ? 6 : 7;
        text_region.size.width -= local->compact_mode ? 6 : 7;
    }

    meta_region = text_region;
    meta_region.size.width = local->compact_mode ? 26 : 34;
    text_region.size.width -= meta_region.size.width;
    meta_region.location.x = text_region.location.x + text_region.size.width;
    meta_gap = local->compact_mode ? 2 : 3;
    if (text_region.size.width > 14 && meta_region.size.width > meta_gap + 14)
    {
        text_region.size.width -= meta_gap;
        meta_region.location.x += meta_gap;
        meta_region.size.width -= meta_gap;
    }
    meta_chip_w = (egui_dim_t)egui_view_menu_bar_text_len(snapshot->menus[menu_index].title) * (local->compact_mode ? 3 : 4) + (local->compact_mode ? 10 : 12);
    if (meta_chip_w < (local->compact_mode ? 16 : 20))
    {
        meta_chip_w = local->compact_mode ? 16 : 20;
    }
    if (meta_chip_w > meta_region.size.width)
    {
        meta_chip_w = meta_region.size.width;
    }
    meta_chip_region = meta_region;
    meta_chip_region.location.x = meta_region.location.x + meta_region.size.width - meta_chip_w;
    meta_chip_region.location.y = strip_region.location.y + 2;
    meta_chip_region.size.width = meta_chip_w;
    meta_chip_region.size.height = strip_region.size.height - 4;
    meta_text_color = egui_rgb_mix(muted_text_color, local->accent_color, local->locked_mode ? 8 : 12);
    title_text_color = text_color;
    if (snapshot->panel_items[0].tone != 0)
    {
        title_text_color = egui_rgb_mix(title_text_color, cue_color, local->locked_mode ? 6 : 10);
    }
    else if (snapshot->panel_items[0].enabled)
    {
        title_text_color = egui_rgb_mix(title_text_color, local->accent_color, local->locked_mode ? 4 : 6);
    }
    if (!snapshot->panel_items[0].enabled)
    {
        title_text_color = egui_rgb_mix(title_text_color, muted_text_color, 34);
    }
    meta_chip_fill_color = egui_rgb_mix(fill_color, local->accent_color, local->locked_mode ? 8 : 14);
    meta_chip_border_color = egui_rgb_mix(border_color, local->accent_color, local->locked_mode ? 10 : 16);

    if (meta_chip_region.size.width > (local->compact_mode ? 14 : 18) && meta_chip_region.size.height > 8)
    {
        egui_canvas_draw_round_rectangle_fill(meta_chip_region.location.x, meta_chip_region.location.y, meta_chip_region.size.width,
                                              meta_chip_region.size.height, local->compact_mode ? 3 : 4, meta_chip_fill_color,
                                              egui_color_alpha_mix(self->alpha, local->locked_mode ? 54 : 68));
        egui_canvas_draw_round_rectangle(meta_chip_region.location.x, meta_chip_region.location.y, meta_chip_region.size.width, meta_chip_region.size.height,
                                         local->compact_mode ? 3 : 4, 1, meta_chip_border_color,
                                         egui_color_alpha_mix(self->alpha, local->locked_mode ? 42 : 56));
    }

    egui_view_menu_bar_draw_text(local->font, self, snapshot->panel_items[0].title, &text_region, EGUI_ALIGN_LEFT, title_text_color, EGUI_ALPHA_100);
    egui_view_menu_bar_draw_text(local->meta_font, self, snapshot->menus[menu_index].title, &meta_chip_region, EGUI_ALIGN_CENTER, meta_text_color,
                                 EGUI_ALPHA_100);
}

static void egui_view_menu_bar_draw_submenu_affordance(egui_view_t *self, egui_view_menu_bar_t *local, const egui_region_t *row_region,
                                                       const egui_region_t *trailing_region, egui_color_t panel_fill_color, egui_color_t border_color,
                                                       egui_color_t row_tone, egui_color_t arrow_color, uint8_t enabled, uint8_t is_current, uint8_t is_pressed)
{
    egui_region_t affordance_region = *trailing_region;
    egui_region_t stub_region;
    egui_color_t affordance_fill;
    egui_color_t affordance_border;
    egui_color_t preview_color;
    egui_color_t stub_fill;
    egui_color_t stub_border;
    egui_dim_t preview_x;
    egui_dim_t preview_y;
    egui_dim_t preview_h;
    egui_dim_t stub_right_limit;
    egui_dim_t stub_inner_x;
    egui_dim_t bridge_y;
    egui_alpha_t stub_alpha;

    if (affordance_region.size.width <= 0 || affordance_region.size.height <= 0)
    {
        return;
    }

    affordance_region.location.x -= local->compact_mode ? 0 : 2;
    affordance_region.size.width += local->compact_mode ? 1 : 4;
    affordance_region.location.y = row_region->location.y + (local->compact_mode ? 3 : 2);
    affordance_region.size.height = row_region->size.height - (local->compact_mode ? 6 : 4);
    if (affordance_region.size.height < 6)
    {
        affordance_region.location.y = row_region->location.y + 1;
        affordance_region.size.height = row_region->size.height - 2;
    }

    affordance_fill = egui_rgb_mix(panel_fill_color, row_tone, is_current ? 18 : (is_pressed ? 24 : 6));
    affordance_border = egui_rgb_mix(border_color, row_tone, is_current ? 26 : (is_pressed ? 30 : 12));
    preview_color = egui_rgb_mix(row_tone, panel_fill_color, is_current ? 8 : 24);
    stub_fill = egui_rgb_mix(panel_fill_color, row_tone, is_pressed ? 16 : 10);
    stub_border = egui_rgb_mix(border_color, row_tone, is_pressed ? 32 : 20);
    stub_alpha = is_pressed ? 92 : (is_current ? 74 : 0);

    if (!enabled)
    {
        affordance_fill = egui_rgb_mix(affordance_fill, panel_fill_color, 34);
        affordance_border = egui_rgb_mix(affordance_border, border_color, 28);
        preview_color = egui_rgb_mix(preview_color, border_color, 34);
        stub_fill = egui_rgb_mix(stub_fill, panel_fill_color, 30);
        stub_border = egui_rgb_mix(stub_border, border_color, 20);
        stub_alpha = is_current ? 48 : (is_pressed ? 54 : 0);
        arrow_color = egui_rgb_mix(arrow_color, preview_color, 26);
    }

    egui_canvas_draw_round_rectangle_fill(
            affordance_region.location.x, affordance_region.location.y, affordance_region.size.width, affordance_region.size.height,
            local->compact_mode ? 3 : 4, affordance_fill,
            egui_color_alpha_mix(self->alpha, is_current ? (local->compact_mode ? 86 : 94)
                                                         : (is_pressed ? (local->compact_mode ? 80 : 88) : (local->compact_mode ? 32 : 42))));
    egui_canvas_draw_round_rectangle(
            affordance_region.location.x, affordance_region.location.y, affordance_region.size.width, affordance_region.size.height,
            local->compact_mode ? 3 : 4, 1, affordance_border,
            egui_color_alpha_mix(self->alpha, is_current ? (local->compact_mode ? 68 : 76)
                                                         : (is_pressed ? (local->compact_mode ? 62 : 70) : (local->compact_mode ? 26 : 34))));

    preview_x = row_region->location.x + row_region->size.width - (local->compact_mode ? 2 : 3);
    preview_y = row_region->location.y + 3;
    preview_h = row_region->size.height - 6;
    if (preview_h < 6)
    {
        preview_y = row_region->location.y + 2;
        preview_h = row_region->size.height - 4;
    }
    if (preview_h > 2)
    {
        egui_canvas_draw_round_rectangle_fill(preview_x, preview_y, 2, preview_h, 1, preview_color,
                                              egui_color_alpha_mix(self->alpha, is_current ? 92 : (is_pressed ? 74 : 30)));
    }

    if (stub_alpha > 0)
    {
        stub_region.location.x = row_region->location.x + row_region->size.width + (local->compact_mode ? 1 : 2);
        stub_region.location.y = row_region->location.y + (local->compact_mode ? 2 : 1);
        stub_region.size.width = local->compact_mode ? 6 : 10;
        stub_region.size.height = row_region->size.height - (local->compact_mode ? 4 : 2);
        stub_right_limit = self->region_screen.location.x + self->region_screen.size.width - 1;
        if (stub_region.location.x + stub_region.size.width > stub_right_limit)
        {
            stub_region.size.width = stub_right_limit - stub_region.location.x;
        }

        if (stub_region.size.width > 2 && stub_region.size.height > 4)
        {
            egui_canvas_draw_round_rectangle_fill(stub_region.location.x, stub_region.location.y, stub_region.size.width, stub_region.size.height,
                                                  local->compact_mode ? 3 : 4, stub_fill, egui_color_alpha_mix(self->alpha, stub_alpha));
            egui_canvas_draw_round_rectangle(stub_region.location.x, stub_region.location.y, stub_region.size.width, stub_region.size.height,
                                             local->compact_mode ? 3 : 4, 1, stub_border, egui_color_alpha_mix(self->alpha, is_pressed ? 84 : 66));

            bridge_y = row_region->location.y + row_region->size.height / 2;
            if (stub_region.location.x > affordance_region.location.x + affordance_region.size.width)
            {
                egui_canvas_draw_line(affordance_region.location.x + affordance_region.size.width - 1, bridge_y, stub_region.location.x, bridge_y, 1,
                                      stub_border, egui_color_alpha_mix(self->alpha, is_pressed ? 68 : 52));
            }

            if (stub_region.size.width > 5)
            {
                stub_inner_x = stub_region.location.x + 2;
                egui_canvas_draw_line(stub_inner_x, stub_region.location.y + 3, stub_region.location.x + stub_region.size.width - 2, stub_region.location.y + 3,
                                      1, preview_color, egui_color_alpha_mix(self->alpha, is_pressed ? 76 : 58));
                egui_canvas_draw_line(stub_inner_x, stub_region.location.y + stub_region.size.height - 4, stub_region.location.x + stub_region.size.width - 3,
                                      stub_region.location.y + stub_region.size.height - 4, 1, preview_color,
                                      egui_color_alpha_mix(self->alpha, is_pressed ? 58 : 42));
            }
        }
    }

    egui_view_menu_bar_draw_text(EGUI_FONT_ICON_MS_16, self, EGUI_ICON_MS_ARROW_FORWARD, trailing_region, EGUI_ALIGN_CENTER, arrow_color, EGUI_ALPHA_100);
}

static void egui_view_menu_bar_draw_row_state_rail(egui_view_t *self, egui_view_menu_bar_t *local, const egui_region_t *row_region, egui_color_t row_tone,
                                                   uint8_t enabled, uint8_t is_current, uint8_t is_pressed)
{
    egui_color_t rail_color;
    egui_dim_t rail_x;
    egui_dim_t rail_y;
    egui_dim_t rail_w;
    egui_dim_t rail_h;
    egui_dim_t rail_radius;
    egui_alpha_t rail_alpha;

    if (row_region == NULL)
    {
        return;
    }

    rail_w = local->compact_mode ? 2 : 3;
    rail_x = row_region->location.x + (local->compact_mode ? 3 : 4);
    rail_y = row_region->location.y + 3;
    rail_h = row_region->size.height - 6;
    rail_radius = rail_w > 1 ? (rail_w - 1) : 1;
    rail_color = row_tone;
    rail_alpha = 0;

    if (enabled)
    {
        if (is_pressed)
        {
            rail_alpha = 96;
        }
        else if (is_current)
        {
            rail_alpha = 74;
            rail_color = egui_rgb_mix(row_tone, EGUI_COLOR_HEX(0xFFFFFF), local->compact_mode ? 8 : 6);
        }
    }
    else
    {
        rail_alpha = is_current ? 42 : 28;
        rail_color = egui_rgb_mix(row_tone, EGUI_COLOR_DARK_GREY, 34);
    }

    if (rail_h <= 0 || rail_alpha == 0)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(rail_x, rail_y, rail_w, rail_h, rail_radius, rail_color, egui_color_alpha_mix(self->alpha, rail_alpha));
}

static void egui_view_menu_bar_draw_disabled_badge(egui_view_t *self, egui_view_menu_bar_t *local, const egui_region_t *trailing_region,
                                                   egui_color_t panel_fill_color, egui_color_t border_color, egui_color_t muted_text_color)
{
    egui_region_t badge_region;
    egui_color_t badge_fill;
    egui_color_t badge_border;
    egui_color_t badge_icon_color;

    if (trailing_region == NULL || trailing_region->size.width <= 0 || trailing_region->size.height <= 0)
    {
        return;
    }

    badge_region = *trailing_region;
    badge_region.location.y += 1;
    badge_region.size.height -= 2;
    if (badge_region.size.height < 8)
    {
        badge_region.location.y = trailing_region->location.y;
        badge_region.size.height = trailing_region->size.height;
    }

    badge_fill = egui_rgb_mix(panel_fill_color, border_color, 18);
    badge_border = egui_rgb_mix(border_color, muted_text_color, 10);
    badge_icon_color = egui_rgb_mix(muted_text_color, border_color, 8);

    egui_canvas_draw_round_rectangle_fill(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                          local->compact_mode ? 3 : 4, badge_fill, egui_color_alpha_mix(self->alpha, 88));
    egui_canvas_draw_round_rectangle(badge_region.location.x, badge_region.location.y, badge_region.size.width, badge_region.size.height,
                                     local->compact_mode ? 3 : 4, 1, badge_border, egui_color_alpha_mix(self->alpha, 60));
    egui_view_menu_bar_draw_text(EGUI_FONT_ICON_MS_16, self, EGUI_ICON_MS_LOCK, &badge_region, EGUI_ALIGN_CENTER, badge_icon_color, EGUI_ALPHA_100);
}

static void egui_view_menu_bar_draw_panel_anchor(egui_view_t *self, egui_view_menu_bar_t *local, const egui_view_menu_bar_layout_t *layout,
                                                 egui_color_t top_fill_color, egui_color_t panel_fill_color, egui_color_t border_color)
{
    egui_region_t anchor_region;
    egui_color_t anchor_fill;
    egui_color_t anchor_border;
    egui_color_t cap_color;
    egui_color_t stem_color;
    egui_dim_t cap_w;
    egui_dim_t cap_x;
    egui_dim_t cap_y;
    egui_dim_t center_x;
    egui_dim_t stem_w;
    egui_dim_t stem_x;
    egui_dim_t stem_y;
    egui_dim_t stem_h;
    egui_dim_t panel_right;

    if (layout == NULL || !layout->panel_visible || layout->current_menu >= layout->menu_count)
    {
        return;
    }

    anchor_region.size.width = local->compact_mode ? 9 : 12;
    anchor_region.size.height = local->compact_mode ? 4 : 5;
    center_x = layout->top_item_regions[layout->current_menu].location.x + layout->top_item_regions[layout->current_menu].size.width / 2;
    anchor_region.location.x = center_x - anchor_region.size.width / 2;
    anchor_region.location.y = layout->panel_region.location.y - (local->compact_mode ? 2 : 3);
    panel_right = layout->panel_region.location.x + layout->panel_region.size.width;
    if (anchor_region.location.x < layout->panel_region.location.x + 6)
    {
        anchor_region.location.x = layout->panel_region.location.x + 6;
    }
    if (anchor_region.location.x + anchor_region.size.width > panel_right - 6)
    {
        anchor_region.location.x = panel_right - 6 - anchor_region.size.width;
    }
    if (anchor_region.size.width <= 0 || anchor_region.size.height <= 0)
    {
        return;
    }

    anchor_fill = egui_rgb_mix(panel_fill_color, local->accent_color, local->compact_mode ? 8 : 12);
    anchor_fill = egui_rgb_mix(anchor_fill, top_fill_color, local->locked_mode ? 12 : 6);
    anchor_border = egui_rgb_mix(border_color, local->accent_color, local->locked_mode ? 10 : 18);
    cap_color = egui_rgb_mix(local->accent_color, border_color, local->compact_mode ? 10 : 6);
    stem_color = egui_rgb_mix(anchor_border, panel_fill_color, local->compact_mode ? 8 : 12);

    stem_w = local->compact_mode ? 1 : 2;
    stem_x = center_x - stem_w / 2;
    stem_y = layout->top_region.location.y + layout->top_region.size.height - 2;
    stem_h = anchor_region.location.y - stem_y + 1;
    if (stem_h > 1)
    {
        egui_canvas_draw_round_rectangle_fill(stem_x, stem_y, stem_w, stem_h, 1, stem_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 66 : 74));
    }

    egui_canvas_draw_round_rectangle_fill(anchor_region.location.x, anchor_region.location.y, anchor_region.size.width, anchor_region.size.height,
                                          local->compact_mode ? 2 : 3, anchor_fill, egui_color_alpha_mix(self->alpha, 92));
    egui_canvas_draw_round_rectangle(anchor_region.location.x, anchor_region.location.y, anchor_region.size.width, anchor_region.size.height,
                                     local->compact_mode ? 2 : 3, 1, anchor_border, egui_color_alpha_mix(self->alpha, local->compact_mode ? 70 : 78));

    cap_w = local->compact_mode ? 8 : 12;
    cap_x = center_x - cap_w / 2;
    cap_y = layout->panel_region.location.y + 1;
    if (cap_x < layout->panel_region.location.x + 6)
    {
        cap_x = layout->panel_region.location.x + 6;
    }
    if (cap_x + cap_w > panel_right - 6)
    {
        cap_x = panel_right - 6 - cap_w;
    }
    if (cap_w > 2)
    {
        egui_canvas_draw_round_rectangle_fill(cap_x, cap_y, cap_w, 2, 1, cap_color, egui_color_alpha_mix(self->alpha, local->compact_mode ? 56 : 66));
    }
}

static void egui_view_menu_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot;
    egui_view_menu_bar_layout_t layout;
    egui_color_t top_fill_color;
    egui_color_t panel_fill_color;
    egui_color_t border_color;
    egui_color_t text_color;
    egui_color_t muted_text_color;
    egui_color_t shadow_color;
    egui_dim_t menu_padding_x;
    egui_dim_t underline_padding_x;
    egui_dim_t row_padding_x;
    egui_dim_t separator_x;
    egui_dim_t separator_w;
    uint8_t is_enabled;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    uint8_t is_focused;
#endif
    uint8_t focus_item;
    uint8_t index;

    if (local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = egui_view_menu_bar_get_snapshot(local);
    if (!egui_view_menu_bar_build_layout(local, self, snapshot, &layout))
    {
        return;
    }

    focus_item = egui_view_menu_bar_clamp_current_item(local, snapshot);
    local->current_item = focus_item;

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    is_focused = self->is_focused ? 1 : 0;
#endif
    top_fill_color = egui_rgb_mix(local->surface_color, local->accent_color, local->compact_mode ? 3 : 5);
    panel_fill_color = egui_rgb_mix(local->surface_color, EGUI_COLOR_HEX(0xFFFFFF), local->compact_mode ? 6 : 10);
    border_color = egui_rgb_mix(local->border_color, EGUI_COLOR_HEX(0xFFFFFF), local->compact_mode ? 12 : 18);
    text_color = local->text_color;
    muted_text_color = local->muted_text_color;
    shadow_color = local->shadow_color;

    if (local->locked_mode)
    {
        top_fill_color = egui_rgb_mix(top_fill_color, local->surface_color, 14);
        panel_fill_color = egui_rgb_mix(panel_fill_color, top_fill_color, 16);
        border_color = egui_rgb_mix(border_color, muted_text_color, 12);
        text_color = egui_rgb_mix(text_color, muted_text_color, 42);
        muted_text_color = egui_rgb_mix(muted_text_color, text_color, 10);
    }
    if (!is_enabled)
    {
        top_fill_color = egui_view_menu_bar_mix_disabled(top_fill_color);
        panel_fill_color = egui_view_menu_bar_mix_disabled(panel_fill_color);
        border_color = egui_view_menu_bar_mix_disabled(border_color);
        text_color = egui_view_menu_bar_mix_disabled(text_color);
        muted_text_color = egui_view_menu_bar_mix_disabled(muted_text_color);
        shadow_color = egui_view_menu_bar_mix_disabled(shadow_color);
    }

    menu_padding_x = local->compact_mode ? 6 : 8;
    underline_padding_x = menu_padding_x + (local->compact_mode ? 1 : 2);
    egui_canvas_draw_round_rectangle_fill(layout.top_region.location.x, layout.top_region.location.y, layout.top_region.size.width,
                                          layout.top_region.size.height, local->compact_mode ? 6 : 8, top_fill_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(layout.top_region.location.x, layout.top_region.location.y, layout.top_region.size.width, layout.top_region.size.height,
                                     local->compact_mode ? 6 : 8, 1, border_color, egui_color_alpha_mix(self->alpha, 100));
    if (local->locked_mode)
    {
        egui_region_t lock_region;

        lock_region.location.x = layout.top_region.location.x + layout.top_region.size.width - (local->compact_mode ? 11 : 13);
        lock_region.location.y = layout.top_region.location.y + (local->compact_mode ? 5 : 6);
        lock_region.size.width = local->compact_mode ? 8 : 9;
        lock_region.size.height = local->compact_mode ? 8 : 9;
        egui_canvas_draw_round_rectangle_fill(lock_region.location.x, lock_region.location.y, lock_region.size.width, lock_region.size.height,
                                              local->compact_mode ? 3 : 4, egui_rgb_mix(top_fill_color, border_color, 12),
                                              egui_color_alpha_mix(self->alpha, 80));
        egui_canvas_draw_round_rectangle(lock_region.location.x, lock_region.location.y, lock_region.size.width, lock_region.size.height,
                                         local->compact_mode ? 3 : 4, 1, egui_rgb_mix(border_color, muted_text_color, 12),
                                         egui_color_alpha_mix(self->alpha, 56));
        egui_view_menu_bar_draw_text(EGUI_FONT_ICON_MS_16, self, EGUI_ICON_MS_LOCK, &lock_region, EGUI_ALIGN_CENTER,
                                     egui_rgb_mix(muted_text_color, border_color, 10), EGUI_ALPHA_100);
    }

    for (index = 0; index < layout.menu_count; index++)
    {
        egui_color_t item_text_color = text_color;
        egui_color_t current_fill;
        egui_dim_t current_pip_h;
        egui_dim_t current_pip_w;
        egui_dim_t current_pip_x;
        egui_dim_t current_pip_y;
        uint8_t is_pressed_menu = (self->is_pressed && index == local->pressed_menu) ? 1 : 0;
        uint8_t menu_enabled = snapshot->menus[index].enabled;

        if (index == layout.current_menu)
        {
            current_fill = egui_rgb_mix(top_fill_color, local->accent_color, local->locked_mode ? 8 : (is_pressed_menu ? 28 : 20));
            egui_canvas_draw_round_rectangle_fill(layout.top_item_regions[index].location.x, layout.top_item_regions[index].location.y,
                                                  layout.top_item_regions[index].size.width, layout.top_item_regions[index].size.height,
                                                  local->compact_mode ? 4 : 5, current_fill, egui_color_alpha_mix(self->alpha, 96));
            egui_canvas_draw_round_rectangle(layout.top_item_regions[index].location.x, layout.top_item_regions[index].location.y,
                                             layout.top_item_regions[index].size.width, layout.top_item_regions[index].size.height, local->compact_mode ? 4 : 5,
                                             1, egui_rgb_mix(border_color, local->accent_color, is_pressed_menu ? 28 : 18),
                                             egui_color_alpha_mix(self->alpha, is_pressed_menu ? 100 : 90));
            if (is_pressed_menu && menu_enabled && is_enabled && !local->locked_mode)
            {
                egui_canvas_draw_round_rectangle_fill(layout.top_item_regions[index].location.x + 3, layout.top_item_regions[index].location.y + 2,
                                                      layout.top_item_regions[index].size.width - 6, 2, 1, local->accent_color,
                                                      egui_color_alpha_mix(self->alpha, 84));
            }
            if (local->locked_mode && menu_enabled && is_enabled)
            {
                current_pip_w = local->compact_mode ? 1 : 2;
                current_pip_h = local->compact_mode ? 4 : 5;
                current_pip_x = layout.top_item_regions[index].location.x + 3;
                current_pip_y = layout.top_item_regions[index].location.y + (layout.top_item_regions[index].size.height - current_pip_h) / 2;
                egui_canvas_draw_round_rectangle_fill(current_pip_x, current_pip_y, current_pip_w, current_pip_h, 1, local->accent_color,
                                                      egui_color_alpha_mix(self->alpha, local->compact_mode ? 50 : 56));
            }
        }
        else if (!local->compact_mode && snapshot->menus[index].emphasized)
        {
            item_text_color = egui_rgb_mix(item_text_color, local->accent_color, 10);
        }
        else
        {
            item_text_color = egui_rgb_mix(item_text_color, muted_text_color, local->compact_mode ? 18 : 12);
        }
        if (!menu_enabled || local->locked_mode || !is_enabled)
        {
            item_text_color = egui_rgb_mix(item_text_color, muted_text_color, 44);
        }
        if (index == layout.current_menu && local->locked_mode && menu_enabled && is_enabled)
        {
            item_text_color = egui_rgb_mix(text_color, local->accent_color, 4);
        }

        egui_view_menu_bar_draw_text(local->font, self, snapshot->menus[index].title, &layout.top_item_regions[index], EGUI_ALIGN_CENTER, item_text_color,
                                     EGUI_ALPHA_100);
    }

    egui_canvas_draw_line(layout.top_region.location.x + 8, layout.top_region.location.y + layout.top_region.size.height - 4,
                          layout.top_region.location.x + layout.top_region.size.width - 8, layout.top_region.location.y + layout.top_region.size.height - 4, 1,
                          border_color, egui_color_alpha_mix(self->alpha, 26));
    if (layout.top_item_regions[layout.current_menu].size.width > 10)
    {
        egui_canvas_draw_line(
                layout.top_item_regions[layout.current_menu].location.x + underline_padding_x, layout.top_region.location.y + layout.top_region.size.height - 4,
                layout.top_item_regions[layout.current_menu].location.x + layout.top_item_regions[layout.current_menu].size.width - underline_padding_x,
                layout.top_region.location.y + layout.top_region.size.height - 4, 1, local->accent_color,
                egui_color_alpha_mix(self->alpha, local->locked_mode ? 42 : 74));
    }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    if (is_focused && is_enabled && !local->locked_mode && layout.top_item_regions[layout.current_menu].size.width > 6)
    {
        egui_dim_t focus_x = layout.top_item_regions[layout.current_menu].location.x - 1;
        egui_dim_t focus_y = layout.top_item_regions[layout.current_menu].location.y - 1;
        egui_dim_t focus_w = layout.top_item_regions[layout.current_menu].size.width + 2;
        egui_dim_t focus_h = (local->compact_mode ? 15 : 18) + 2;
        egui_dim_t focus_radius = local->compact_mode ? 5 : 6;

        if (focus_x < layout.top_region.location.x + 1)
        {
            focus_x = layout.top_region.location.x + 1;
        }
        if (focus_x + focus_w > layout.top_region.location.x + layout.top_region.size.width - 1)
        {
            focus_w = (layout.top_region.location.x + layout.top_region.size.width - 1) - focus_x;
        }
        if (focus_y < layout.top_region.location.y + 1)
        {
            focus_y = layout.top_region.location.y + 1;
        }
        if (focus_w > 4 && focus_h > 4)
        {
            egui_canvas_draw_round_rectangle(focus_x, focus_y, focus_w, focus_h, focus_radius, 2, local->accent_color, egui_color_alpha_mix(self->alpha, 100));
            egui_canvas_draw_round_rectangle(focus_x + 1, focus_y + 1, focus_w - 2, focus_h - 2, focus_radius > 2 ? (focus_radius - 2) : focus_radius, 1,
                                             local->accent_color, egui_color_alpha_mix(self->alpha, 52));
        }
    }
#endif

    if (layout.panel_visible)
    {
        row_padding_x = local->compact_mode ? 7 : 8;
        separator_x = layout.panel_region.location.x + row_padding_x;
        separator_w = layout.panel_region.size.width - row_padding_x * 2;

        egui_canvas_draw_round_rectangle_fill(layout.panel_region.location.x, layout.panel_region.location.y + 1, layout.panel_region.size.width,
                                              layout.panel_region.size.height, local->compact_mode ? 6 : 8, shadow_color,
                                              egui_color_alpha_mix(self->alpha, local->compact_mode ? 16 : 18));
        egui_canvas_draw_round_rectangle_fill(layout.panel_region.location.x, layout.panel_region.location.y, layout.panel_region.size.width,
                                              layout.panel_region.size.height, local->compact_mode ? 6 : 8, panel_fill_color,
                                              egui_color_alpha_mix(self->alpha, 100));
        egui_canvas_draw_round_rectangle(layout.panel_region.location.x, layout.panel_region.location.y, layout.panel_region.size.width,
                                         layout.panel_region.size.height, local->compact_mode ? 6 : 8, 1, border_color, egui_color_alpha_mix(self->alpha, 96));
        egui_view_menu_bar_draw_panel_anchor(self, local, &layout, top_fill_color, panel_fill_color, border_color);

        for (index = 0; index < layout.panel_item_count; index++)
        {
            const egui_view_menu_bar_panel_item_t *item = &snapshot->panel_items[index];
            egui_region_t title_region;
            egui_region_t meta_region;
            egui_region_t trailing_region;
            egui_color_t row_text_color;
            egui_color_t row_meta_color;
            egui_color_t row_tone = egui_view_menu_bar_tone_color(local, item->tone);
            egui_dim_t trailing_gap = local->compact_mode ? 1 : 2;
            uint8_t has_disabled_badge = item->enabled ? 0 : 1;
            uint8_t reserve_trailing = item->trailing_kind == EGUI_VIEW_MENU_BAR_TRAILING_SUBMENU || has_disabled_badge ? 1 : 0;
            uint8_t is_current = index == focus_item ? 1 : 0;
            uint8_t is_pressed = (self->is_pressed && index == local->pressed_item) ? 1 : 0;

            if (item->separator_before && index > 0)
            {
                egui_canvas_draw_line(separator_x, layout.panel_item_regions[index].location.y - 2, separator_x + separator_w,
                                      layout.panel_item_regions[index].location.y - 2, 1, border_color, egui_color_alpha_mix(self->alpha, 42));
            }

            if (is_current)
            {
                egui_canvas_draw_round_rectangle_fill(layout.panel_item_regions[index].location.x, layout.panel_item_regions[index].location.y,
                                                      layout.panel_item_regions[index].size.width, layout.panel_item_regions[index].size.height,
                                                      local->compact_mode ? 4 : 5, egui_rgb_mix(panel_fill_color, row_tone, 12),
                                                      egui_color_alpha_mix(self->alpha, 100));
                egui_canvas_draw_round_rectangle(layout.panel_item_regions[index].location.x, layout.panel_item_regions[index].location.y,
                                                 layout.panel_item_regions[index].size.width, layout.panel_item_regions[index].size.height,
                                                 local->compact_mode ? 4 : 5, 1, egui_rgb_mix(border_color, row_tone, 18),
                                                 egui_color_alpha_mix(self->alpha, 80));
            }
            if (is_pressed && item->enabled && is_enabled)
            {
                egui_canvas_draw_round_rectangle_fill(layout.panel_item_regions[index].location.x, layout.panel_item_regions[index].location.y,
                                                      layout.panel_item_regions[index].size.width, layout.panel_item_regions[index].size.height,
                                                      local->compact_mode ? 4 : 5, egui_rgb_mix(row_tone, panel_fill_color, 32),
                                                      egui_color_alpha_mix(self->alpha, 36));
            }
            egui_view_menu_bar_draw_row_state_rail(self, local, &layout.panel_item_regions[index], row_tone, item->enabled && is_enabled, is_current,
                                                   is_pressed);

            title_region = layout.panel_item_regions[index];
            title_region.location.x += row_padding_x;
            title_region.size.width -= row_padding_x * 2;
            meta_region = title_region;
            trailing_region = title_region;
            trailing_region.size.width = local->compact_mode ? 10 : 12;
            if (reserve_trailing)
            {
                title_region.size.width -= local->compact_mode ? 12 : 14;
            }
            meta_region.size.width = local->compact_mode ? 28 : 38;
            title_region.size.width -= meta_region.size.width;
            meta_region.location.x = title_region.location.x + title_region.size.width;
            if (reserve_trailing)
            {
                meta_region.size.width -= trailing_region.size.width + trailing_gap;
                trailing_region.location.x = meta_region.location.x + meta_region.size.width + trailing_gap;
            }

            row_text_color = item->enabled ? text_color : egui_rgb_mix(text_color, muted_text_color, 56);
            if (item->emphasized)
            {
                row_text_color = egui_rgb_mix(row_text_color, row_tone, 12);
            }
            row_meta_color = egui_rgb_mix(muted_text_color, row_tone, item->tone == 0 ? 0 : 10);
            if (item->trailing_kind == EGUI_VIEW_MENU_BAR_TRAILING_SUBMENU && (is_current || is_pressed))
            {
                row_meta_color = egui_rgb_mix(row_meta_color, row_tone, is_current ? 28 : 18);
            }
            if (has_disabled_badge)
            {
                row_meta_color = egui_rgb_mix(row_meta_color, muted_text_color, 28);
            }

            egui_view_menu_bar_draw_text(local->font, self, item->title, &title_region, EGUI_ALIGN_LEFT, row_text_color, EGUI_ALPHA_100);
            egui_view_menu_bar_draw_text(local->meta_font, self, item->meta, &meta_region, EGUI_ALIGN_RIGHT, row_meta_color, EGUI_ALPHA_100);
            if (item->trailing_kind == EGUI_VIEW_MENU_BAR_TRAILING_SUBMENU)
            {
                egui_view_menu_bar_draw_submenu_affordance(self, local, &layout.panel_item_regions[index], &trailing_region, panel_fill_color, border_color,
                                                           row_tone, row_meta_color, item->enabled && is_enabled, is_current, is_pressed);
            }
            else if (has_disabled_badge)
            {
                egui_view_menu_bar_draw_disabled_badge(self, local, &trailing_region, panel_fill_color, border_color, muted_text_color);
            }
        }
    }
    else
    {
        egui_view_menu_bar_draw_summary_anchor(self, local, &layout, panel_fill_color, border_color);
        egui_view_menu_bar_draw_summary_strip(self, local, snapshot, &layout.region, panel_fill_color, border_color, text_color, muted_text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_menu_bar_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    uint8_t hit_menu;
    uint8_t hit_item;
    uint8_t item_count;
    uint8_t menu_count;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    item_count = egui_view_menu_bar_clamp_panel_item_count(snapshot->panel_item_count);
    menu_count = egui_view_menu_bar_clamp_menu_count(snapshot->menu_count);

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        hit_menu = egui_view_menu_bar_hit_menu(self, event->location.x, event->location.y);
        if (hit_menu != EGUI_VIEW_MENU_BAR_HIT_NONE && egui_view_menu_bar_menu_is_enabled(snapshot, hit_menu, menu_count))
        {
            local->pressed_menu = hit_menu;
            local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
            egui_view_set_pressed(self, true);
            return 1;
        }

        hit_item = egui_view_menu_bar_hit_panel_item(self, event->location.x, event->location.y);
        if (hit_item != EGUI_VIEW_MENU_BAR_ITEM_NONE && egui_view_menu_bar_item_is_enabled(snapshot, hit_item, item_count))
        {
            local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
            local->pressed_item = hit_item;
            egui_view_menu_bar_set_current_item_inner(self, hit_item, 1);
            egui_view_set_pressed(self, true);
            return 1;
        }
        return 0;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_menu != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            uint8_t is_pressed = 0;

            hit_menu = egui_view_menu_bar_hit_menu(self, event->location.x, event->location.y);
            if (local->pressed_menu == hit_menu && egui_view_menu_bar_menu_is_enabled(snapshot, hit_menu, menu_count))
            {
                is_pressed = 1;
            }
            if (self->is_pressed != is_pressed)
            {
                egui_view_set_pressed(self, is_pressed);
                egui_view_invalidate(self);
            }
            return 1;
        }
        if (local->pressed_item != EGUI_VIEW_MENU_BAR_ITEM_NONE)
        {
            uint8_t is_pressed = 0;

            hit_item = egui_view_menu_bar_hit_panel_item(self, event->location.x, event->location.y);
            if (local->pressed_item == hit_item && egui_view_menu_bar_item_is_enabled(snapshot, hit_item, item_count))
            {
                is_pressed = 1;
            }
            if (self->is_pressed != is_pressed)
            {
                egui_view_set_pressed(self, is_pressed);
                egui_view_invalidate(self);
            }
            return 1;
        }
        return 0;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_menu = egui_view_menu_bar_hit_menu(self, event->location.x, event->location.y);
        if (local->pressed_menu != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            uint8_t was_pressed = self->is_pressed;

            if (was_pressed && local->pressed_menu == hit_menu && egui_view_menu_bar_menu_is_enabled(snapshot, hit_menu, menu_count))
            {
                egui_view_menu_bar_apply_snapshot(self, hit_menu, 1);
            }
            local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
            local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
            egui_view_set_pressed(self, false);
            egui_view_invalidate(self);
            return hit_menu != EGUI_VIEW_MENU_BAR_HIT_NONE;
        }

        hit_item = egui_view_menu_bar_hit_panel_item(self, event->location.x, event->location.y);
        if (local->pressed_item != EGUI_VIEW_MENU_BAR_ITEM_NONE)
        {
            uint8_t was_pressed = self->is_pressed;

            if (was_pressed && local->pressed_item == hit_item && egui_view_menu_bar_item_is_enabled(snapshot, hit_item, item_count))
            {
                egui_view_menu_bar_set_current_item_inner(self, hit_item, 1);
                egui_view_menu_bar_notify_item_activated(self, local);
            }
            local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
            local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
            egui_view_set_pressed(self, false);
            egui_view_invalidate(self);
            return hit_item != EGUI_VIEW_MENU_BAR_ITEM_NONE;
        }
        return 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
        local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_menu_bar_is_navigation_key(uint16_t key_code)
{
    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_UP:
    case EGUI_KEY_CODE_DOWN:
    case EGUI_KEY_CODE_HOME:
    case EGUI_KEY_CODE_END:
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        return 1;
    default:
        return 0;
    }
}

static int egui_view_menu_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_menu_bar_t);
    const egui_view_menu_bar_snapshot_t *snapshot = egui_view_menu_bar_get_snapshot(local);
    uint8_t target_snapshot;
    uint8_t snapshot_index;

    if (snapshot == NULL || !egui_view_get_enable(self) || local->locked_mode)
    {
        return 0;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        if ((event->key_code == EGUI_KEY_CODE_ENTER || event->key_code == EGUI_KEY_CODE_SPACE) && local->current_item != EGUI_VIEW_MENU_BAR_ITEM_NONE)
        {
            local->pressed_item = local->current_item;
            egui_view_invalidate(self);
            return 1;
        }
        return egui_view_menu_bar_is_navigation_key(event->key_code);
    }

    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        return egui_view_menu_bar_is_navigation_key(event->key_code);
    }

    snapshot_index = local->current_snapshot;
    if (snapshot_index >= local->snapshot_count)
    {
        snapshot_index = 0;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        target_snapshot = egui_view_menu_bar_find_enabled_snapshot(local, snapshot_index, -1);
        if (target_snapshot != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            egui_view_menu_bar_apply_snapshot(self, target_snapshot, 1);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        target_snapshot = egui_view_menu_bar_find_enabled_snapshot(local, snapshot_index, 1);
        if (target_snapshot != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            egui_view_menu_bar_apply_snapshot(self, target_snapshot, 1);
        }
        return 1;
    case EGUI_KEY_CODE_UP:
        egui_view_menu_bar_move_current_item(self, -1);
        return 1;
    case EGUI_KEY_CODE_DOWN:
        egui_view_menu_bar_move_current_item(self, 1);
        return 1;
    case EGUI_KEY_CODE_HOME:
        target_snapshot = egui_view_menu_bar_find_edge_enabled_snapshot(local, 0);
        if (target_snapshot != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            egui_view_menu_bar_apply_snapshot(self, target_snapshot, 1);
        }
        return 1;
    case EGUI_KEY_CODE_END:
        target_snapshot = egui_view_menu_bar_find_edge_enabled_snapshot(local, 1);
        if (target_snapshot != EGUI_VIEW_MENU_BAR_HIT_NONE)
        {
            egui_view_menu_bar_apply_snapshot(self, target_snapshot, 1);
        }
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        egui_view_menu_bar_notify_item_activated(self, local);
        local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
        egui_view_invalidate(self);
        return 1;
    default:
        local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
        return egui_view_on_key_event(self, event);
    }
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_menu_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_menu_bar_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_menu_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_menu_bar_on_key_event,
#endif
};

void egui_view_menu_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_menu_bar_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_menu_bar_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_selection_changed = NULL;
    local->on_item_activated = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD5DEE6);
    local->text_color = EGUI_COLOR_HEX(0x18222D);
    local->muted_text_color = EGUI_COLOR_HEX(0x6A7683);
    local->accent_color = EGUI_COLOR_HEX(0x2E63DA);
    local->success_color = EGUI_COLOR_HEX(0x178454);
    local->warning_color = EGUI_COLOR_HEX(0xB77719);
    local->danger_color = EGUI_COLOR_HEX(0xB13A32);
    local->shadow_color = EGUI_COLOR_HEX(0xCBD5DF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->current_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->compact_mode = 0;
    local->locked_mode = 0;
    local->pressed_item = EGUI_VIEW_MENU_BAR_ITEM_NONE;
    local->pressed_menu = EGUI_VIEW_MENU_BAR_HIT_NONE;

    egui_view_set_view_name(self, "egui_view_menu_bar");
}
