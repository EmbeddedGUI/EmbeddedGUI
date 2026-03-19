#include "egui_view_rating_control.h"

#define RC_STD_RADIUS      10
#define RC_STD_PAD_X       10
#define RC_STD_PAD_Y       8
#define RC_STD_ITEM_SIZE   24
#define RC_STD_ITEM_GAP    6
#define RC_STD_CLEAR_W     34
#define RC_STD_CLEAR_H     16
#define RC_STD_TITLE_H     10
#define RC_STD_ITEM_Y      27
#define RC_STD_EMPTY_MIX   8
#define RC_STD_EMPTY_ALPHA 84

#define RC_COMPACT_RADIUS      8
#define RC_COMPACT_PAD_X       7
#define RC_COMPACT_PAD_Y       6
#define RC_COMPACT_ITEM_SIZE   16
#define RC_COMPACT_ITEM_GAP    4
#define RC_COMPACT_EMPTY_MIX   10
#define RC_COMPACT_EMPTY_ALPHA 82

typedef struct egui_view_rating_control_metrics egui_view_rating_control_metrics_t;
struct egui_view_rating_control_metrics
{
    egui_region_t title_region;
    egui_region_t caption_region;
    egui_region_t low_region;
    egui_region_t high_region;
    egui_region_t clear_region;
    egui_region_t item_regions[EGUI_VIEW_RATING_CONTROL_MAX_ITEMS];
    egui_dim_t item_size;
    egui_dim_t item_gap;
    uint8_t show_title;
    uint8_t show_caption;
    uint8_t show_labels;
    uint8_t show_clear;
};

static uint8_t rating_has_text(const char *text)
{
    return (text != NULL && text[0] != '\0') ? 1 : 0;
}

static uint8_t rating_is_star(egui_view_rating_control_t *local, uint8_t part)
{
    return (part >= 1 && part <= local->item_count) ? 1 : 0;
}

static uint8_t rating_is_clear_visible(egui_view_rating_control_t *local)
{
    return (!local->compact_mode && !local->read_only_mode && local->clear_enabled && local->current_value > 0) ? 1 : 0;
}

static uint8_t rating_is_part_visible(egui_view_rating_control_t *local, uint8_t part)
{
    if (rating_is_star(local, part))
    {
        return 1;
    }
    if (part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
    {
        return rating_is_clear_visible(local);
    }
    return 0;
}

static uint8_t rating_is_enabled(egui_view_rating_control_t *local, egui_view_t *self, uint8_t part)
{
    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }
    return rating_is_part_visible(local, part);
}

static uint8_t rating_resolve_default_part(egui_view_rating_control_t *local)
{
    if (local->current_value >= 1 && local->current_value <= local->item_count)
    {
        return local->current_value;
    }
    return local->item_count > 0 ? 1 : EGUI_VIEW_RATING_CONTROL_PART_NONE;
}

static void rating_normalize_state(egui_view_rating_control_t *local)
{
    if (local->item_count < 1)
    {
        local->item_count = 1;
    }
    if (local->item_count > EGUI_VIEW_RATING_CONTROL_MAX_ITEMS)
    {
        local->item_count = EGUI_VIEW_RATING_CONTROL_MAX_ITEMS;
    }

    if (local->current_value > local->item_count)
    {
        local->current_value = local->item_count;
    }

    if (!rating_is_part_visible(local, local->current_part))
    {
        local->current_part = rating_resolve_default_part(local);
    }

    if (!rating_is_part_visible(local, local->pressed_part))
    {
        local->pressed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
    }
}

static const char *rating_get_value_label_for_value(egui_view_rating_control_t *local, uint8_t value)
{
    if (local->value_labels == NULL || value >= local->label_count)
    {
        return NULL;
    }
    return local->value_labels[value];
}

static void rating_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color)
{
    egui_region_t draw_region = *region;

    if (!rating_has_text(text))
    {
        return;
    }

    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, self->alpha);
}

static void rating_get_metrics(egui_view_rating_control_t *local, egui_view_t *self, egui_view_rating_control_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? RC_COMPACT_PAD_X : RC_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? RC_COMPACT_PAD_Y : RC_STD_PAD_Y;
    egui_dim_t item_size = local->compact_mode ? RC_COMPACT_ITEM_SIZE : RC_STD_ITEM_SIZE;
    egui_dim_t item_gap = local->compact_mode ? RC_COMPACT_ITEM_GAP : RC_STD_ITEM_GAP;
    egui_dim_t row_width;
    egui_dim_t available_width;
    egui_dim_t min_gap = local->compact_mode ? 2 : 4;
    egui_dim_t row_x;
    egui_dim_t row_y;
    uint8_t index;

    egui_view_get_work_region(self, &region);
    metrics->show_title = (!local->compact_mode && rating_has_text(local->title)) ? 1 : 0;
    metrics->show_caption = (!local->compact_mode && local->value_labels != NULL && local->label_count > 0) ? 1 : 0;
    metrics->show_labels = (!local->compact_mode && (rating_has_text(local->low_label) || rating_has_text(local->high_label))) ? 1 : 0;
    metrics->show_clear = rating_is_clear_visible(local);

    metrics->title_region.location.x = region.location.x + pad_x;
    metrics->title_region.location.y = region.location.y + pad_y;
    metrics->title_region.size.width = region.size.width - pad_x * 2 - (metrics->show_clear ? RC_STD_CLEAR_W + 6 : 0);
    metrics->title_region.size.height = metrics->show_title ? RC_STD_TITLE_H : 0;

    metrics->clear_region.location.x = region.location.x + region.size.width - pad_x - RC_STD_CLEAR_W;
    metrics->clear_region.location.y = region.location.y + pad_y - 1;
    metrics->clear_region.size.width = metrics->show_clear ? RC_STD_CLEAR_W : 0;
    metrics->clear_region.size.height = metrics->show_clear ? RC_STD_CLEAR_H : 0;

    available_width = region.size.width - pad_x * 2;
    if (metrics->show_clear)
    {
        available_width -= RC_STD_CLEAR_W + 6;
    }
    if (available_width < local->item_count * 10)
    {
        available_width = local->item_count * 10;
    }

    row_width = local->item_count * item_size + (local->item_count > 0 ? (local->item_count - 1) * item_gap : 0);
    if (row_width > available_width)
    {
        item_gap = local->item_count > 1 ? min_gap : 0;
        item_size = (available_width - (local->item_count > 1 ? (local->item_count - 1) * item_gap : 0)) / local->item_count;
        if (item_size < (local->compact_mode ? 12 : 14))
        {
            item_size = local->compact_mode ? 12 : 14;
            item_gap = local->item_count > 1 ? (available_width - local->item_count * item_size) / (local->item_count - 1) : 0;
            if (item_gap < 0)
            {
                item_gap = 0;
            }
        }
        row_width = local->item_count * item_size + (local->item_count > 0 ? (local->item_count - 1) * item_gap : 0);
    }
    metrics->item_size = item_size;
    metrics->item_gap = item_gap;

    row_x = region.location.x + (region.size.width - row_width) / 2;
    if (metrics->show_clear && row_x + row_width > metrics->clear_region.location.x - 6)
    {
        row_x = metrics->clear_region.location.x - 6 - row_width;
    }
    if (row_x < region.location.x + pad_x)
    {
        row_x = region.location.x + pad_x;
    }

    if (local->compact_mode)
    {
        row_y = region.location.y + (region.size.height - item_size) / 2;
    }
    else
    {
        row_y = region.location.y + (metrics->show_title ? RC_STD_ITEM_Y : (RC_STD_ITEM_Y - 6));
    }

    for (index = 0; index < local->item_count; index++)
    {
        metrics->item_regions[index].location.x = row_x + index * (item_size + item_gap);
        metrics->item_regions[index].location.y = row_y;
        metrics->item_regions[index].size.width = item_size;
        metrics->item_regions[index].size.height = item_size;
    }
    for (; index < EGUI_VIEW_RATING_CONTROL_MAX_ITEMS; index++)
    {
        metrics->item_regions[index].location.x = 0;
        metrics->item_regions[index].location.y = 0;
        metrics->item_regions[index].size.width = 0;
        metrics->item_regions[index].size.height = 0;
    }

    metrics->caption_region.location.x = region.location.x + pad_x;
    metrics->caption_region.location.y = row_y + item_size + 6;
    metrics->caption_region.size.width = region.size.width - pad_x * 2;
    metrics->caption_region.size.height = metrics->show_caption ? 10 : 0;

    metrics->low_region.location.x = metrics->item_regions[0].location.x;
    metrics->low_region.location.y = row_y + item_size + (metrics->show_caption ? 17 : 8);
    metrics->low_region.size.width = item_size * 2;
    metrics->low_region.size.height = metrics->show_labels ? 8 : 0;

    metrics->high_region.location.x = metrics->item_regions[local->item_count - 1].location.x - item_size;
    metrics->high_region.location.y = metrics->low_region.location.y;
    metrics->high_region.size.width = item_size * 2;
    metrics->high_region.size.height = metrics->show_labels ? 8 : 0;
}

static void rating_draw_focus(egui_view_t *self, const egui_region_t *region, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_round_rectangle(region->location.x - 1, region->location.y - 1, region->size.width + 2, region->size.height + 2, radius, 1, color,
                                     egui_color_alpha_mix(self->alpha, alpha));
}

static void rating_draw_star(egui_view_t *self, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t fill_color, egui_color_t border_color,
                             egui_alpha_t fill_alpha, egui_alpha_t border_alpha)
{
    static const int8_t points_template[20] = {0, -10, 3, -3, 10, -3, 4, 2, 6, 10, 0, 5, -6, 10, -4, 2, -10, -3, -3, -3};
    egui_dim_t points[20];
    uint8_t index;

    for (index = 0; index < 10; index++)
    {
        points[index * 2] = center_x + (points_template[index * 2] * radius) / 10;
        points[index * 2 + 1] = center_y + (points_template[index * 2 + 1] * radius) / 10;
    }
    egui_canvas_draw_polygon_fill(points, 10, fill_color, egui_color_alpha_mix(self->alpha, fill_alpha));
    egui_canvas_draw_polygon(points, 10, 1, border_color, egui_color_alpha_mix(self->alpha, border_alpha));
}

static void rating_notify_changed(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->current_value, part);
    }
}

static void rating_set_value_inner(egui_view_t *self, uint8_t value, uint8_t notify, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    uint8_t old_value = local->current_value;
    uint8_t old_part = local->current_part;
    uint8_t value_changed;
    uint8_t part_changed;

    if (value > local->item_count)
    {
        value = local->item_count;
    }

    local->current_value = value;
    if (value == 0)
    {
        local->current_part = rating_resolve_default_part(local);
    }
    else if (rating_is_star(local, part))
    {
        local->current_part = part;
    }
    else
    {
        local->current_part = value;
    }

    value_changed = old_value != local->current_value ? 1 : 0;
    part_changed = old_part != local->current_part ? 1 : 0;

    if (!value_changed && !part_changed)
    {
        return;
    }

    if (notify && value_changed)
    {
        rating_notify_changed(self, part);
    }
    egui_view_invalidate(self);
}

static void rating_set_current_part_inner(egui_view_t *self, uint8_t part, uint8_t notify)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    EGUI_UNUSED(notify);
    if (!rating_is_part_visible(local, part))
    {
        return;
    }

    if (local->current_part == part)
    {
        return;
    }

    local->current_part = part;
    egui_view_invalidate(self);
}

static void rating_commit(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    if (!rating_is_enabled(local, self, part))
    {
        return;
    }

    if (part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
    {
        rating_set_value_inner(self, 0, 1, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
    }
    else if (rating_is_star(local, part))
    {
        rating_set_value_inner(self, part, 1, part);
    }
}

static uint8_t rating_collect_parts(egui_view_rating_control_t *local, uint8_t *parts, uint8_t max_parts)
{
    uint8_t count = 0;
    uint8_t part;

    if (parts == NULL || max_parts == 0)
    {
        return 0;
    }

    for (part = 1; part <= local->item_count && count < max_parts; part++)
    {
        parts[count++] = part;
    }
    if (rating_is_clear_visible(local) && count < max_parts)
    {
        parts[count++] = EGUI_VIEW_RATING_CONTROL_PART_CLEAR;
    }
    return count;
}

static uint8_t rating_find_part_index(const uint8_t *parts, uint8_t count, uint8_t part)
{
    uint8_t index;

    for (index = 0; index < count; index++)
    {
        if (parts[index] == part)
        {
            return index;
        }
    }
    return 0;
}

static uint8_t rating_get_preview_value(egui_view_rating_control_t *local, egui_view_t *self)
{
    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return local->current_value;
    }

    if (local->pressed_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
    {
        return 0;
    }
    if (rating_is_star(local, local->pressed_part))
    {
        return local->pressed_part;
    }
    if (self->is_focused && local->current_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
    {
        return 0;
    }

    return local->current_value;
}

static uint8_t rating_hit_part(egui_view_rating_control_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y)
{
    egui_view_rating_control_metrics_t metrics;
    egui_region_t row_region;
    egui_dim_t local_x = x - self->region_screen.location.x;
    egui_dim_t local_y = y - self->region_screen.location.y;
    egui_dim_t row_pad_x;
    egui_dim_t row_pad_y;
    uint8_t index;

    rating_get_metrics(local, self, &metrics);

    if (metrics.show_clear && egui_region_pt_in_rect(&metrics.clear_region, local_x, local_y))
    {
        return EGUI_VIEW_RATING_CONTROL_PART_CLEAR;
    }

    for (index = 0; index < local->item_count; index++)
    {
        if (egui_region_pt_in_rect(&metrics.item_regions[index], local_x, local_y))
        {
            return (uint8_t)(index + 1);
        }
    }

    if (local->item_count == 0)
    {
        return EGUI_VIEW_RATING_CONTROL_PART_NONE;
    }

    row_pad_x = metrics.item_gap / 2 + (local->compact_mode ? 1 : 2);
    row_pad_y = local->compact_mode ? 2 : 4;
    row_region.location.x = metrics.item_regions[0].location.x - row_pad_x;
    row_region.location.y = metrics.item_regions[0].location.y - row_pad_y;
    row_region.size.width =
            metrics.item_regions[local->item_count - 1].location.x + metrics.item_regions[local->item_count - 1].size.width - row_region.location.x + row_pad_x;
    row_region.size.height = metrics.item_regions[0].size.height + row_pad_y * 2;

    if (egui_region_pt_in_rect(&row_region, local_x, local_y))
    {
        egui_dim_t best_distance = 0;
        uint8_t best_part = 1;

        for (index = 0; index < local->item_count; index++)
        {
            egui_dim_t center_x = metrics.item_regions[index].location.x + metrics.item_regions[index].size.width / 2;
            egui_dim_t distance = center_x > local_x ? (center_x - local_x) : (local_x - center_x);

            if (index == 0 || distance <= best_distance)
            {
                best_distance = distance;
                best_part = (uint8_t)(index + 1);
            }
        }

        return best_part;
    }

    return EGUI_VIEW_RATING_CONTROL_PART_NONE;
}

uint8_t egui_view_rating_control_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    egui_view_rating_control_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }

    rating_get_metrics(local, self, &metrics);
    if (part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
    {
        if (!metrics.show_clear)
        {
            return 0;
        }
        *region = metrics.clear_region;
    }
    else if (rating_is_star(local, part))
    {
        *region = metrics.item_regions[part - 1];
    }
    else
    {
        return 0;
    }

    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

static void rating_draw_clear(egui_view_t *self, egui_view_rating_control_t *local, const egui_region_t *region)
{
    uint8_t focused = local->current_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR ? 1 : 0;
    uint8_t pressed = local->pressed_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR ? 1 : 0;
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, focused ? local->accent_color : local->muted_text_color, pressed ? 18 : (focused ? 12 : 8));
    egui_color_t border_color = egui_rgb_mix(local->border_color, focused ? local->accent_color : local->muted_text_color, pressed ? 34 : (focused ? 28 : 16));
    egui_color_t text_color = egui_rgb_mix(local->text_color, focused ? local->accent_color : local->muted_text_color, focused ? 22 : 34);

    if (focused)
    {
        rating_draw_focus(self, region, 5, egui_rgb_mix(local->accent_color, EGUI_COLOR_WHITE, 6), 60);
    }
    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, 5, fill_color,
                                          egui_color_alpha_mix(self->alpha, focused ? 96 : 92));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, 5, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, focused ? 68 : 54));
    rating_draw_text(local->meta_font, self, "Clear", region, EGUI_ALIGN_CENTER, text_color);
}

static void egui_view_rating_control_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    egui_region_t region;
    egui_view_rating_control_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_color_t shadow_color = local->shadow_color;
    uint8_t enabled = egui_view_get_enable(self) ? 1 : 0;
    uint8_t preview_value;
    uint8_t preview_active;
    const char *preview_label;
    uint8_t index;

    rating_normalize_state(local);
    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 64);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 34);
        border_color = egui_rgb_mix(border_color, muted_color, 28);
        text_color = egui_rgb_mix(text_color, muted_color, 34);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 40);
    }
    if (!enabled)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 64);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 28);
        border_color = egui_rgb_mix(border_color, muted_color, 30);
        text_color = egui_rgb_mix(text_color, muted_color, 40);
        muted_color = egui_rgb_mix(muted_color, surface_color, 12);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 36);
    }

    rating_get_metrics(local, self, &metrics);
    preview_value = rating_get_preview_value(local, self);
    preview_active = preview_value != local->current_value ? 1 : 0;
    preview_label = rating_get_value_label_for_value(local, preview_value);

    if (!local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y + 2, region.size.width, region.size.height, RC_STD_RADIUS + 1, shadow_color,
                                              egui_color_alpha_mix(self->alpha, enabled ? 18 : 10));
    }
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height,
                                          local->compact_mode ? RC_COMPACT_RADIUS : RC_STD_RADIUS, surface_color,
                                          egui_color_alpha_mix(self->alpha, local->compact_mode ? 94 : 96));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? RC_COMPACT_RADIUS : RC_STD_RADIUS, 1, border_color,
                                     egui_color_alpha_mix(self->alpha, local->compact_mode ? 54 : 58));

    if (metrics.show_title)
    {
        rating_draw_text(local->font, self, local->title, &metrics.title_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color);
    }
    if (metrics.show_clear)
    {
        rating_draw_clear(self, local, &metrics.clear_region);
    }

    for (index = 0; index < local->item_count; index++)
    {
        egui_region_t *item_region = &metrics.item_regions[index];
        uint8_t selected = (index + 1 <= preview_value) ? 1 : 0;
        uint8_t focused = (local->current_part == (uint8_t)(index + 1) && enabled && !local->read_only_mode) ? 1 : 0;
        uint8_t pressed = local->pressed_part == (uint8_t)(index + 1) ? 1 : 0;
        egui_color_t fill_color =
                selected ? accent_color : egui_rgb_mix(surface_color, muted_color, local->compact_mode ? RC_COMPACT_EMPTY_MIX : RC_STD_EMPTY_MIX);
        egui_color_t item_border = selected ? egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 10) : egui_rgb_mix(border_color, muted_color, 24);
        egui_alpha_t fill_alpha = selected ? 94 : (local->compact_mode ? RC_COMPACT_EMPTY_ALPHA : RC_STD_EMPTY_ALPHA);
        egui_alpha_t border_alpha = selected ? 94 : 56;
        egui_dim_t star_radius = metrics.item_size / 2 - (local->compact_mode ? 3 : 4);

        if (star_radius < 4)
        {
            star_radius = 4;
        }

        if (focused)
        {
            fill_color = selected ? egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 8) : egui_rgb_mix(fill_color, accent_color, 18);
            item_border = egui_rgb_mix(item_border, accent_color, selected ? 16 : 28);
            rating_draw_focus(self, item_region, local->compact_mode ? 5 : 6, egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, 8), local->compact_mode ? 48 : 56);
        }
        if (pressed)
        {
            fill_color = egui_rgb_mix(fill_color, accent_color, selected ? 16 : 24);
            item_border = egui_rgb_mix(item_border, accent_color, 16);
        }

        rating_draw_star(self, item_region->location.x + item_region->size.width / 2, item_region->location.y + item_region->size.height / 2, star_radius,
                         fill_color, item_border, fill_alpha, border_alpha);
    }

    if (metrics.show_caption)
    {
        rating_draw_text(local->meta_font, self, preview_label, &metrics.caption_region, EGUI_ALIGN_CENTER,
                         egui_rgb_mix(text_color, muted_color, preview_active ? 52 : 38));
    }
    if (metrics.show_labels)
    {
        rating_draw_text(local->meta_font, self, local->low_label, &metrics.low_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, muted_color);
        rating_draw_text(local->meta_font, self, local->high_label, &metrics.high_region, EGUI_ALIGN_RIGHT | EGUI_ALIGN_VCENTER, muted_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_rating_control_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = rating_hit_part(local, self, event->location.x, event->location.y);
        if (!rating_is_enabled(local, self, hit_part))
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_part = hit_part;
        rating_set_current_part_inner(self, hit_part, 1);
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_RATING_CONTROL_PART_NONE)
        {
            return 0;
        }
        hit_part = rating_hit_part(local, self, event->location.x, event->location.y);
        if (!rating_is_enabled(local, self, hit_part))
        {
            hit_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
        }
        if (local->pressed_part != hit_part)
        {
            local->pressed_part = hit_part;
            if (hit_part != EGUI_VIEW_RATING_CONTROL_PART_NONE)
            {
                rating_set_current_part_inner(self, hit_part, 1);
            }
            else
            {
                egui_view_invalidate(self);
            }
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
        hit_part = rating_hit_part(local, self, event->location.x, event->location.y);
        if (!rating_is_enabled(local, self, hit_part))
        {
            hit_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
        }
        if (local->pressed_part != EGUI_VIEW_RATING_CONTROL_PART_NONE && local->pressed_part == hit_part)
        {
            rating_commit(self, hit_part);
        }
        local->pressed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_RATING_CONTROL_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

static uint8_t rating_handle_navigation_key_inner(egui_view_t *self, uint8_t key_code)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    uint8_t parts[EGUI_VIEW_RATING_CONTROL_MAX_ITEMS + 1];
    uint8_t count;
    uint8_t index;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    rating_normalize_state(local);
    count = rating_collect_parts(local, parts, EGUI_VIEW_RATING_CONTROL_MAX_ITEMS + 1);
    if (count == 0)
    {
        return 0;
    }

    index = rating_find_part_index(parts, count, local->current_part);
    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
    case EGUI_KEY_CODE_UP:
        if (local->current_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
        {
            if (local->current_value > 0)
            {
                rating_set_current_part_inner(self, local->current_value, 1);
            }
            else
            {
                rating_set_current_part_inner(self, rating_resolve_default_part(local), 1);
            }
        }
        else if (local->current_value > 1)
        {
            rating_commit(self, (uint8_t)(local->current_value - 1));
        }
        else
        {
            rating_commit(self, 1);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
    case EGUI_KEY_CODE_DOWN:
        if (local->current_part != EGUI_VIEW_RATING_CONTROL_PART_CLEAR)
        {
            if (local->current_value < local->item_count)
            {
                rating_commit(self, (uint8_t)(local->current_value + 1));
            }
            else
            {
                rating_commit(self, local->item_count);
            }
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        rating_commit(self, 1);
        return 1;
    case EGUI_KEY_CODE_END:
        rating_commit(self, local->item_count);
        return 1;
    case EGUI_KEY_CODE_TAB:
        index++;
        if (index >= count)
        {
            index = 0;
        }
        rating_set_current_part_inner(self, parts[index], 1);
        return 1;
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_SPACE:
        rating_commit(self, local->current_part);
        return 1;
    case EGUI_KEY_CODE_ESCAPE:
        if (local->current_part == EGUI_VIEW_RATING_CONTROL_PART_CLEAR && rating_is_clear_visible(local))
        {
            rating_commit(self, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
        }
        else
        {
            rating_set_current_part_inner(self, rating_resolve_default_part(local), 1);
        }
        return 1;
    default:
        return 0;
    }
#else
    EGUI_UNUSED(self);
    EGUI_UNUSED(key_code);
    return 0;
#endif
}

uint8_t egui_view_rating_control_handle_navigation_key(egui_view_t *self, uint8_t key_code)
{
    return rating_handle_navigation_key_inner(self, key_code);
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_rating_control_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    if (event->type != EGUI_KEY_EVENT_ACTION_UP)
    {
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_UP:
        case EGUI_KEY_CODE_DOWN:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_ESCAPE:
            return 1;
        default:
            return 0;
        }
    }

    if (rating_handle_navigation_key_inner(self, event->key_code))
    {
        return 1;
    }

    return egui_view_on_key_event(self, event);
}
#endif

void egui_view_rating_control_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                          egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t shadow_color)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->shadow_color = shadow_color;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_item_count(egui_view_t *self, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->item_count = item_count;
    rating_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    if (value > local->item_count)
    {
        value = local->item_count;
    }
    if (value == 0)
    {
        rating_set_value_inner(self, 0, 1, EGUI_VIEW_RATING_CONTROL_PART_CLEAR);
        return;
    }

    rating_set_value_inner(self, value, 1, value);
}

uint8_t egui_view_rating_control_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    return local->current_value;
}

void egui_view_rating_control_set_current_part(egui_view_t *self, uint8_t part)
{
    rating_set_current_part_inner(self, part, 1);
}

uint8_t egui_view_rating_control_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    return local->current_part;
}

void egui_view_rating_control_set_on_changed_listener(egui_view_t *self, egui_view_on_rating_control_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    local->on_changed = listener;
}

void egui_view_rating_control_set_title(egui_view_t *self, const char *title)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    local->title = title;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_low_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    local->low_label = label;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_high_label(egui_view_t *self, const char *label)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);
    local->high_label = label;
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_value_labels(egui_view_t *self, const char **labels, uint8_t label_count)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->value_labels = labels;
    local->label_count = label_count;
    if (local->label_count > EGUI_VIEW_RATING_CONTROL_MAX_ITEMS + 1)
    {
        local->label_count = EGUI_VIEW_RATING_CONTROL_MAX_ITEMS + 1;
    }
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->compact_mode = compact_mode ? 1 : 0;
    rating_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->read_only_mode = read_only_mode ? 1 : 0;
    rating_normalize_state(local);
    egui_view_invalidate(self);
}

void egui_view_rating_control_set_clear_enabled(egui_view_t *self, uint8_t clear_enabled)
{
    EGUI_LOCAL_INIT(egui_view_rating_control_t);

    local->clear_enabled = clear_enabled ? 1 : 0;
    rating_normalize_state(local);
    egui_view_invalidate(self);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_rating_control_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_rating_control_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_rating_control_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_rating_control_on_key_event,
#endif
};

void egui_view_rating_control_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_rating_control_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_rating_control_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_changed = NULL;
    local->title = NULL;
    local->low_label = NULL;
    local->high_label = NULL;
    local->value_labels = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x1D2732);
    local->muted_text_color = EGUI_COLOR_HEX(0x6E7C8B);
    local->accent_color = EGUI_COLOR_HEX(0xD6A12A);
    local->shadow_color = EGUI_COLOR_HEX(0xDCE4EC);
    local->label_count = 0;
    local->item_count = 5;
    local->current_value = 0;
    local->current_part = 1;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->clear_enabled = 1;
    local->pressed_part = EGUI_VIEW_RATING_CONTROL_PART_NONE;

    egui_view_set_view_name(self, "egui_view_rating_control");
}
