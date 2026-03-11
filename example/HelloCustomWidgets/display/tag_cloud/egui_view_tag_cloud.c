#include <stdlib.h>

#include "egui_view_tag_cloud.h"

static const egui_color_t tag_palette[] = {
        EGUI_COLOR_HEX(0x38BDF8), EGUI_COLOR_HEX(0x34D399), EGUI_COLOR_HEX(0xFB7185),
        EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0xA78BFA), EGUI_COLOR_HEX(0x22C55E),
};

static uint8_t egui_view_tag_cloud_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_TAG_CLOUD_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_TAG_CLOUD_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_tag_cloud_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_TAG_CLOUD_MAX_ITEMS)
    {
        return EGUI_VIEW_TAG_CLOUD_MAX_ITEMS;
    }
    return count;
}

static egui_color_t egui_view_tag_cloud_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

void egui_view_tag_cloud_set_snapshots(egui_view_t *self, const egui_view_tag_cloud_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_tag_cloud_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_tag_cloud_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
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

uint8_t egui_view_tag_cloud_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    return local->current_snapshot;
}

void egui_view_tag_cloud_set_focus_item(egui_view_t *self, uint8_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->focus_item = item_index;
    egui_view_invalidate(self);
}

void egui_view_tag_cloud_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_tag_cloud_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tag_cloud_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_tag_cloud_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                     egui_color_t muted_text_color, egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_tag_cloud_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_tag_cloud_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_tag_cloud_snapshot_t *snapshot;
    egui_color_t panel_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t item_count;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t row_h;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    item_count = egui_view_tag_cloud_clamp_item_count(snapshot->item_count);
    if (snapshot->words == NULL || snapshot->weights == NULL || item_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_tag_cloud_mix_disabled(panel_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));

    content_x = region.location.x + 6;
    content_y = region.location.y + 4;
    content_width = region.size.width - 12;
    content_height = region.size.height - 8;

    if (local->show_header)
    {
        header_region.location.x = content_x;
        header_region.location.y = content_y;
        header_region.size.width = content_width;
        header_region.size.height = 12;
        egui_canvas_draw_text_in_rect(local->font, is_enabled ? snapshot->title : "Locked", &header_region, EGUI_ALIGN_LEFT,
                                      is_enabled ? local->muted_text_color : local->text_color, self->alpha);
        content_y += 14;
        content_height -= 14;
    }
    if (content_width <= 20 || content_height <= 16)
    {
        return;
    }

    x = content_x;
    y = content_y + 6;
    row_h = 0;

    for (i = 0; i < item_count; i++)
    {
        const char *word = snapshot->words[i];
        uint8_t weight = snapshot->weights[i];
        uint8_t len = 0;
        egui_dim_t chip_w;
        egui_dim_t chip_h;
        egui_color_t base_color = tag_palette[i % (sizeof(tag_palette) / sizeof(tag_palette[0]))];
        egui_color_t fill_color;
        egui_color_t border_color;
        egui_color_t text_color;
        uint8_t is_current = i == snapshot->current_item;
        uint8_t is_focus = i == local->focus_item;

        while (word != NULL && word[len] != 0)
        {
            len++;
        }

        if (weight > 4)
        {
            weight = 4;
        }
        chip_w = (local->compact_mode ? 14 : 18) + len * (local->compact_mode ? 3 : 4) + weight * (local->compact_mode ? 2 : 4);
        chip_h = (local->compact_mode ? 12 : 16) + weight * (local->compact_mode ? 1 : 2);
        if (chip_w > content_width)
        {
            chip_w = content_width;
        }
        if (x + chip_w > content_x + content_width)
        {
            x = content_x;
            y += row_h + 6;
            row_h = 0;
        }
        if (y + chip_h > content_y + content_height - (local->compact_mode ? 0 : 12))
        {
            break;
        }

        fill_color = egui_rgb_mix(local->surface_color, base_color, is_current ? EGUI_ALPHA_30 : EGUI_ALPHA_20);
        border_color = is_focus ? local->active_color : egui_rgb_mix(local->border_color, base_color, EGUI_ALPHA_30);
        text_color = is_current ? local->text_color : local->muted_text_color;

        if (!is_enabled)
        {
            fill_color = egui_view_tag_cloud_mix_disabled(fill_color);
            border_color = egui_view_tag_cloud_mix_disabled(border_color);
            text_color = egui_view_tag_cloud_mix_disabled(text_color);
        }

        egui_canvas_draw_round_rectangle_fill(x, y, chip_w, chip_h, chip_h / 2, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(x, y, chip_w, chip_h, chip_h / 2, is_current ? 2 : 1, border_color,
                                         egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_80 : EGUI_ALPHA_50));
        text_region.location.x = x + 4;
        text_region.location.y = y + 1;
        text_region.size.width = chip_w - 8;
        text_region.size.height = chip_h - 2;
        egui_canvas_draw_text_in_rect(local->font, word, &text_region, EGUI_ALIGN_CENTER, text_color, self->alpha);

        if (chip_h > row_h)
        {
            row_h = chip_h;
        }
        x += chip_w + 5;
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->words[(snapshot->current_item < item_count) ? snapshot->current_item : 0];
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height - 12;
        text_region.size.width = content_width;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(local->font, caption, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_line(content_x + 2, content_y + 2, content_x + content_width - 3, content_y + content_height - 3, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_line(content_x + 2, content_y + content_height - 3, content_x + content_width - 3, content_y + 2, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_tag_cloud_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_tag_cloud_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_tag_cloud_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_tag_cloud_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_tag_cloud_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->border_color = EGUI_COLOR_HEX(0x4A5568);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->active_color = EGUI_COLOR_HEX(0x38BDF8);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->focus_item = 2;
    local->show_header = 1;
    local->compact_mode = 0;
}
