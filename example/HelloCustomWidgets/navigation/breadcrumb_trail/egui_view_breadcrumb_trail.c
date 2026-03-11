#include <stdlib.h>
#include <string.h>

#include "egui_view_breadcrumb_trail.h"

static uint8_t egui_view_breadcrumb_trail_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_BREADCRUMB_TRAIL_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_BREADCRUMB_TRAIL_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_breadcrumb_trail_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_BREADCRUMB_TRAIL_MAX_ITEMS)
    {
        return EGUI_VIEW_BREADCRUMB_TRAIL_MAX_ITEMS;
    }
    return count;
}

static egui_color_t egui_view_breadcrumb_trail_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

void egui_view_breadcrumb_trail_set_snapshots(egui_view_t *self, const egui_view_breadcrumb_trail_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_breadcrumb_trail_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_trail_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
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

uint8_t egui_view_breadcrumb_trail_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    return local->current_snapshot;
}

void egui_view_breadcrumb_trail_set_focus_item(egui_view_t *self, uint8_t item_index)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->focus_item = item_index;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_trail_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_trail_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_trail_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_breadcrumb_trail_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                            egui_color_t muted_text_color, egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_breadcrumb_trail_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_breadcrumb_trail_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_breadcrumb_trail_snapshot_t *snapshot;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t item_count;
    uint8_t i;
    egui_dim_t chip_x;
    egui_dim_t chip_y;
    egui_dim_t chip_h;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    item_count = egui_view_breadcrumb_trail_clamp_item_count(snapshot->item_count);
    if (snapshot->items == NULL || item_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_breadcrumb_trail_mix_disabled(panel_color);
    }
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);

    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
    egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                     egui_rgb_mix(local->active_color, local->border_color, EGUI_ALPHA_30), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    if (local->compact_mode)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x + 10, region.location.y + 6, region.size.width - 20, 2, 1, local->active_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    }

    content_x = region.location.x + 6;
    content_y = region.location.y + 4;
    content_width = region.size.width - 12;
    content_height = region.size.height - 8;

    if (local->compact_mode)
    {
        size_t title_len = snapshot->title != NULL ? strlen(snapshot->title) : 0;
        if (is_enabled && title_len > 0)
        {
            compact_badge[0] = snapshot->title[title_len - 1];
            compact_badge[1] = '\0';
        }
        header_region.location.x = content_x + (content_width - 28) / 2;
        header_region.location.y = content_y + 6;
        header_region.size.width = 28;
        header_region.size.height = 8;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        content_y += 12;
        content_height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w = 32 + (egui_dim_t)strlen(is_enabled ? snapshot->title : "Locked") * 6;
        if (pill_w < 64)
        {
            pill_w = 64;
        }
        if (pill_w > content_width)
        {
            pill_w = content_width;
        }
        header_region.location.x = content_x + (content_width - pill_w) / 2;
        header_region.location.y = content_y;
        header_region.size.width = pill_w;
        header_region.size.height = 13;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_text_in_rect(local->font, is_enabled ? snapshot->title : "Locked", &header_region, EGUI_ALIGN_CENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
        egui_canvas_draw_line(header_region.location.x + 6, header_region.location.y + header_region.size.height - 2,
                              header_region.location.x + header_region.size.width - 7, header_region.location.y + header_region.size.height - 2, 1,
                              local->active_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_line(content_x + 5, content_y + 16, content_x + content_width - 6, content_y + 16, 1, local->border_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        content_y += 19;
        content_height -= 19;
    }
    if (content_width <= 20 || content_height <= 14)
    {
        return;
    }

    chip_x = content_x;
    chip_y = content_y + (local->compact_mode ? 12 : 20);
    chip_h = local->compact_mode ? 16 : 23;

    for (i = 0; i < item_count; i++)
    {
        uint8_t is_current = i == snapshot->current_item;
        uint8_t is_focus = i == local->focus_item;
        egui_dim_t chip_w = local->compact_mode ? 28 : 34;
        egui_color_t fill_color =
                egui_rgb_mix(local->surface_color, is_current ? local->active_color : local->border_color, is_current ? EGUI_ALPHA_40 : EGUI_ALPHA_20);
        egui_color_t border_color =
                is_focus ? local->active_color : egui_rgb_mix(local->border_color, is_current ? local->active_color : local->border_color, EGUI_ALPHA_30);
        egui_color_t text_color =
                is_current ? local->text_color : (is_focus ? egui_rgb_mix(local->text_color, local->muted_text_color, EGUI_ALPHA_20) : local->muted_text_color);

        if (!is_enabled)
        {
            fill_color = egui_view_breadcrumb_trail_mix_disabled(fill_color);
            border_color = egui_view_breadcrumb_trail_mix_disabled(border_color);
            text_color = egui_view_breadcrumb_trail_mix_disabled(text_color);
        }

        if (!local->compact_mode)
        {
            uint8_t len = 0;
            const char *name = snapshot->items[i];
            while (name != NULL && name[len] != 0)
            {
                len++;
            }
            chip_w += len * 4;
            if (chip_w > 56)
            {
                chip_w = 56;
            }
        }

        if (chip_x + chip_w > content_x + content_width)
        {
            chip_x = content_x;
            chip_y += chip_h + 8;
        }
        if (chip_y + chip_h > content_y + content_height)
        {
            break;
        }

        if (is_focus)
        {
            egui_canvas_draw_round_rectangle_fill(chip_x - 1, chip_y - 1, chip_w + 2, chip_h + 2, 6, border_color,
                                                  egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        }
        egui_canvas_draw_round_rectangle_fill(chip_x, chip_y, chip_w, chip_h, 5, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(chip_x, chip_y, chip_w, chip_h, 5, is_focus ? 2 : 1, border_color,
                                         egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_80 : EGUI_ALPHA_50));

        text_region.location.x = chip_x + 5;
        text_region.location.y = chip_y + 2;
        text_region.size.width = chip_w - 10;
        text_region.size.height = chip_h - 4;
        egui_canvas_draw_text_in_rect(local->font, snapshot->items[i], &text_region, EGUI_ALIGN_CENTER, text_color, self->alpha);

        if (i < item_count - 1)
        {
            egui_dim_t arrow_x = chip_x + chip_w + 3;
            egui_dim_t arrow_y = chip_y + chip_h / 2;
            if (arrow_x + 6 < content_x + content_width)
            {
                egui_color_t arrow_color = is_current ? local->active_color : local->muted_text_color;
                egui_canvas_draw_line(arrow_x, arrow_y - 3, arrow_x + 4, arrow_y, 1, arrow_color,
                                      egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_70 : EGUI_ALPHA_60));
                egui_canvas_draw_line(arrow_x, arrow_y + 3, arrow_x + 4, arrow_y, 1, arrow_color,
                                      egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_70 : EGUI_ALPHA_60));
            }
            chip_x += chip_w + 12;
        }
        else
        {
            chip_x += chip_w;
        }
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->items[(snapshot->current_item < item_count) ? snapshot->current_item : (item_count - 1)];
        egui_dim_t caption_w = 24 + (egui_dim_t)strlen(caption) * 6;
        if (caption_w < 52)
        {
            caption_w = 52;
        }
        if (caption_w > content_width - 24)
        {
            caption_w = content_width - 24;
        }
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height - 13;
        text_region.size.width = content_width;
        text_region.size.height = 12;
        egui_canvas_draw_round_rectangle_fill(content_x + (content_width - caption_w) / 2, content_y + content_height - 14, caption_w, 11, 5, panel_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(content_x + (content_width - caption_w) / 2, content_y + content_height - 14, caption_w, 11, 5, 1,
                                         egui_rgb_mix(local->border_color, local->active_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, caption, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 5, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_line(content_x + 4, content_y + 4, content_x + content_width - 5, content_y + content_height - 5, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_line(content_x + 4, content_y + content_height - 5, content_x + content_width - 5, content_y + 4, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_breadcrumb_trail_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_breadcrumb_trail_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_breadcrumb_trail_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_breadcrumb_trail_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_breadcrumb_trail_t);
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
    local->focus_item = 1;
    local->show_header = 1;
    local->compact_mode = 0;
}
