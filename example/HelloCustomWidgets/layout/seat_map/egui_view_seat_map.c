#include <stdlib.h>

#include "egui_view_seat_map.h"

static uint8_t egui_view_seat_map_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SEAT_MAP_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SEAT_MAP_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_seat_map_clamp_seat_count(uint8_t row_count, uint8_t col_count)
{
    uint8_t total = row_count * col_count;
    if (total > EGUI_VIEW_SEAT_MAP_MAX_SEATS)
    {
        return EGUI_VIEW_SEAT_MAP_MAX_SEATS;
    }
    return total;
}

static egui_color_t egui_view_seat_map_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_seat_map_state_color(uint8_t state)
{
    switch (state)
    {
    case 1:
        return EGUI_COLOR_HEX(0xF59E0B);
    case 2:
        return EGUI_COLOR_HEX(0xFB7185);
    default:
        return EGUI_COLOR_HEX(0x34D399);
    }
}

void egui_view_seat_map_set_snapshots(egui_view_t *self, const egui_view_seat_map_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_seat_map_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
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

uint8_t egui_view_seat_map_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    return local->current_snapshot;
}

void egui_view_seat_map_set_focus_row(egui_view_t *self, uint8_t row_index)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->focus_row = row_index;
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_focus_seat(egui_view_t *self, uint8_t seat_index)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->focus_seat = seat_index;
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_seat_map_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_seat_map_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_seat_map_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t label_region;
    egui_region_t text_region;
    const egui_view_seat_map_snapshot_t *snapshot;
    egui_color_t panel_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t row_count;
    uint8_t col_count;
    uint8_t total_seats;
    uint8_t current_row;
    uint8_t current_seat;
    egui_dim_t left_offset;
    egui_dim_t seat_w;
    egui_dim_t seat_h;
    egui_dim_t row_gap;
    egui_dim_t col_gap;
    uint8_t row;
    uint8_t col;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    row_count = snapshot->row_count;
    col_count = snapshot->col_count;
    total_seats = egui_view_seat_map_clamp_seat_count(row_count, col_count);
    if (snapshot->seat_states == NULL || row_count == 0 || col_count == 0 || total_seats == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_seat_map_mix_disabled(panel_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));

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
        egui_canvas_draw_text_in_rect(local->font, is_enabled ? snapshot->title : "Locked", &header_region, EGUI_ALIGN_LEFT, is_enabled ? local->muted_text_color : local->text_color, self->alpha);
        content_y += 14;
        content_height -= 14;
    }
    if (content_width <= 20 || content_height <= 20)
    {
        return;
    }

    if (!local->compact_mode && content_height > 14)
    {
        content_height -= 14;
    }

    left_offset = local->compact_mode ? 2 : 14;
    col_gap = local->compact_mode ? 3 : 4;
    row_gap = local->compact_mode ? 5 : 6;
    seat_w = (content_width - left_offset - (col_count - 1) * col_gap - ((snapshot->aisle_after_col > 0 && snapshot->aisle_after_col < col_count) ? 5 : 0)) / col_count;
    seat_h = (content_height - (row_count - 1) * row_gap) / row_count;
    if (seat_w < 8)
    {
        seat_w = 8;
    }
    if (seat_h < 8)
    {
        seat_h = 8;
    }

    current_row = local->focus_row < row_count ? local->focus_row : snapshot->focus_row;
    current_seat = local->focus_seat < total_seats ? local->focus_seat : snapshot->focus_seat;

    for (row = 0; row < row_count; row++)
    {
        egui_dim_t row_y = content_y + row * (seat_h + row_gap);
        if (!local->compact_mode)
        {
            char row_label[2];
            row_label[0] = 'A' + row;
            row_label[1] = 0;
            label_region.location.x = content_x;
            label_region.location.y = row_y - 1;
            label_region.size.width = 12;
            label_region.size.height = seat_h + 2;
            egui_canvas_draw_text_in_rect(local->font, row_label, &label_region, EGUI_ALIGN_LEFT, row == current_row ? local->text_color : local->muted_text_color, self->alpha);
        }

        for (col = 0; col < col_count; col++)
        {
            uint8_t seat_index = row * col_count + col;
            egui_dim_t extra_gap = (snapshot->aisle_after_col > 0 && col >= snapshot->aisle_after_col) ? 5 : 0;
            egui_dim_t seat_x = content_x + left_offset + col * (seat_w + col_gap) + extra_gap;
            egui_color_t state_color = egui_view_seat_map_state_color(snapshot->seat_states[seat_index]);
            egui_color_t fill_color;
            egui_color_t border_color;
            uint8_t is_current_row = row == snapshot->focus_row;
            uint8_t is_focus_row = row == current_row;
            uint8_t is_focus_seat = seat_index == current_seat;

            fill_color = egui_rgb_mix(local->surface_color, state_color, is_focus_seat ? EGUI_ALPHA_30 : EGUI_ALPHA_10);
            border_color = is_focus_seat ? local->active_color : egui_rgb_mix(local->border_color, state_color, is_current_row ? EGUI_ALPHA_40 : EGUI_ALPHA_20);
            if (!is_enabled)
            {
                fill_color = egui_view_seat_map_mix_disabled(fill_color);
                border_color = egui_view_seat_map_mix_disabled(border_color);
                state_color = egui_view_seat_map_mix_disabled(state_color);
            }

            if (is_focus_row && !local->compact_mode)
            {
                egui_canvas_draw_round_rectangle_fill(seat_x - 1, row_y - 1, seat_w + 2, seat_h + 2, 4, egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_10), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
            }
            egui_canvas_draw_round_rectangle_fill(seat_x, row_y, seat_w, seat_h, 3, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
            egui_canvas_draw_round_rectangle(seat_x, row_y, seat_w, seat_h, 3, is_focus_seat ? 2 : 1, border_color, egui_color_alpha_mix(self->alpha, is_focus_seat ? EGUI_ALPHA_80 : EGUI_ALPHA_40));
            egui_canvas_draw_circle_fill(seat_x + seat_w - 3, row_y + 3, 1, state_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));
        }
    }

    if (!local->compact_mode)
    {
        char caption[6];
        uint8_t row_index = current_seat / col_count;
        uint8_t col_index = current_seat % col_count;
        caption[0] = 'A' + row_index;
        caption[1] = '0' + (col_index + 1);
        caption[2] = 0;
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 2;
        text_region.size.width = content_width;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(local->font, caption, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_line(content_x + 2, content_y + 2, content_x + content_width - 3, content_y + content_height - 3, 1, local->muted_text_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_line(content_x + 2, content_y + content_height - 3, content_x + content_width - 3, content_y + 2, 1, local->muted_text_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_seat_map_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_seat_map_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_seat_map_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_seat_map_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_seat_map_t);
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
    local->focus_row = 1;
    local->focus_seat = 8;
    local->show_header = 1;
    local->compact_mode = 0;
}
