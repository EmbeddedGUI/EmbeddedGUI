#include <stdlib.h>
#include <string.h>

#include "egui_view_split_flap_board.h"

static uint8_t egui_view_split_flap_board_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SPLIT_FLAP_BOARD_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SPLIT_FLAP_BOARD_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_split_flap_board_get_slot_count(const char *text)
{
    uint8_t count;
    if (text == NULL)
    {
        return 0;
    }
    count = (uint8_t)strlen(text);
    if (count > 6)
    {
        count = 6;
    }
    return count;
}

void egui_view_split_flap_board_set_snapshots(egui_view_t *self, const egui_view_split_flap_board_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_split_flap_board_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_split_flap_board_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
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

uint8_t egui_view_split_flap_board_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    return local->current_snapshot;
}

void egui_view_split_flap_board_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_split_flap_board_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_split_flap_board_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                            egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t alert_color, egui_color_t split_color)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->alert_color = alert_color;
    local->split_color = split_color;
    egui_view_invalidate(self);
}

static void egui_view_split_flap_board_draw_slot(egui_view_split_flap_board_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y, egui_dim_t width,
                                                 egui_dim_t height, char symbol, uint8_t is_focus)
{
    egui_region_t text_region;
    egui_color_t top_color;
    egui_color_t bottom_color;
    egui_color_t ring_color;
    char text_value[2];

    top_color = egui_rgb_mix(local->surface_color, EGUI_COLOR_WHITE, EGUI_ALPHA_10);
    bottom_color = egui_rgb_mix(local->surface_color, EGUI_COLOR_BLACK, EGUI_ALPHA_40);
    ring_color = is_focus ? local->accent_color : local->border_color;

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, 6, local->surface_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
    egui_canvas_draw_round_rectangle_fill(x + 1, y + 1, width - 2, height / 2 - 1, 5, top_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
    egui_canvas_draw_round_rectangle_fill(x + 1, y + height / 2, width - 2, height / 2 - 1, 5, bottom_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
    egui_canvas_draw_round_rectangle(x, y, width, height, 6, is_focus ? 2 : 1, ring_color,
                                     egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_90 : EGUI_ALPHA_60));
    egui_canvas_draw_line(x + 2, y + height / 2, x + width - 3, y + height / 2, 1, local->split_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    text_value[0] = symbol ? symbol : ' ';
    text_value[1] = 0;
    text_region.location.x = x;
    text_region.location.y = y + (local->compact_mode ? 6 : 10);
    text_region.size.width = width;
    text_region.size.height = local->compact_mode ? 14 : 20;
    egui_canvas_draw_text_in_rect(local->font, text_value, &text_region, EGUI_ALIGN_CENTER, (symbol == ' ') ? local->muted_text_color : local->text_color,
                                  self->alpha);
}

static void egui_view_split_flap_board_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_split_flap_board_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_split_flap_board_snapshot_t *snapshot;
    uint8_t slot_count;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t header_h;
    egui_dim_t slot_h;
    egui_dim_t slot_gap;
    egui_dim_t slot_w;
    egui_dim_t total_w;
    egui_dim_t start_x;
    egui_dim_t slot_y;
    egui_dim_t footer_y;
    egui_dim_t footer_side_padding;
    egui_dim_t header_top_padding;
    egui_dim_t pill_w;
    egui_dim_t side_padding;
    egui_dim_t pill_right_padding;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    slot_count = egui_view_split_flap_board_get_slot_count(snapshot->value_text);
    if (slot_count == 0)
    {
        return;
    }

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    header_h = local->compact_mode ? 16 : 20;
    slot_h = local->compact_mode ? 30 : 44;
    slot_gap = local->compact_mode ? 4 : 4;
    header_top_padding = 6;
    pill_w = local->compact_mode ? 29 : 42;
    side_padding = local->compact_mode ? 8 : 10;
    pill_right_padding = 8;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 8, egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30),
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    text_region.location.x = panel_x + side_padding;
    text_region.location.y = panel_y + header_top_padding;
    text_region.size.width = panel_w - pill_w - side_padding - pill_right_padding - 4;
    text_region.size.height = 12;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "BOARD", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color,
                                  self->alpha);

    egui_canvas_draw_round_rectangle_fill(panel_x + panel_w - pill_w - pill_right_padding, panel_y + header_top_padding, pill_w, 10, 4,
                                          snapshot->is_alert ? local->alert_color : local->accent_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
    text_region.location.x = panel_x + panel_w - pill_w - pill_right_padding;
    text_region.location.y = panel_y + header_top_padding;
    text_region.size.width = pill_w;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "LIVE", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    slot_w = (panel_w - side_padding * 2 - ((slot_count - 1) * slot_gap)) / slot_count;
    if (slot_w < 18)
    {
        slot_w = 18;
    }
    total_w = slot_w * slot_count + (slot_count - 1) * slot_gap;
    start_x = panel_x + (panel_w - total_w) / 2;
    slot_y = panel_y + header_h + (local->compact_mode ? 8 : 11);

    for (i = 0; i < slot_count; i++)
    {
        egui_view_split_flap_board_draw_slot(local, self, start_x + i * (slot_w + slot_gap), slot_y, slot_w, slot_h, snapshot->value_text[i],
                                             i == snapshot->focus_slot);
    }

    footer_y = slot_y + slot_h + (local->compact_mode ? 6 : 11);
    footer_side_padding = side_padding + (local->compact_mode ? 0 : 1);
    text_region.location.x = panel_x + footer_side_padding;
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - footer_side_padding * 2;
    text_region.size.height = 12;
    if (local->compact_mode)
    {
        egui_canvas_draw_text_in_rect(local->font, snapshot->route_text ? snapshot->route_text : "STANDBY", &text_region, EGUI_ALIGN_CENTER,
                                      local->muted_text_color, self->alpha);
    }
    else
    {
        egui_canvas_draw_text_in_rect(local->font, snapshot->route_text ? snapshot->route_text : "ROUTE", &text_region, EGUI_ALIGN_LEFT,
                                      local->muted_text_color, self->alpha);
        egui_canvas_draw_text_in_rect(local->font, snapshot->time_text ? snapshot->time_text : "--:--", &text_region, EGUI_ALIGN_RIGHT, local->text_color,
                                      self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_split_flap_board_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_split_flap_board_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_split_flap_board_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_split_flap_board_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_split_flap_board_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x171D27);
    local->border_color = EGUI_COLOR_HEX(0x465365);
    local->text_color = EGUI_COLOR_HEX(0xE5EEF8);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->accent_color = EGUI_COLOR_HEX(0x38BDF8);
    local->alert_color = EGUI_COLOR_HEX(0xF59E0B);
    local->split_color = EGUI_COLOR_HEX(0x0F172A);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
}
