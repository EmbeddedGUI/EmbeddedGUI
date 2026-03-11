#include <stdlib.h>

#include "egui_view_step_sequencer.h"

static const char *const step_sequencer_track_labels[] = {"K", "S", "H"};

static uint8_t egui_view_step_sequencer_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_STEP_SEQUENCER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_STEP_SEQUENCER_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_step_sequencer_set_snapshots(egui_view_t *self, const egui_view_step_sequencer_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_step_sequencer_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_step_sequencer_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
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

uint8_t egui_view_step_sequencer_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    return local->current_snapshot;
}

void egui_view_step_sequencer_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_step_sequencer_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_step_sequencer_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_step_sequencer_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t panel_color, egui_color_t border_color,
                                          egui_color_t text_color, egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t warn_color,
                                          egui_color_t lock_color, egui_color_t playhead_color)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    local->surface_color = surface_color;
    local->panel_color = panel_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->playhead_color = playhead_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_step_sequencer_get_status_color(egui_view_step_sequencer_t *local, const egui_view_step_sequencer_snapshot_t *snapshot)
{
    if (snapshot->accent_mode >= 2)
    {
        return local->lock_color;
    }
    if (snapshot->accent_mode == 1)
    {
        return local->warn_color;
    }
    return local->accent_color;
}

static void egui_view_step_sequencer_draw_text(egui_view_step_sequencer_t *local, egui_view_t *self, const char *text, egui_dim_t x, egui_dim_t y,
                                               egui_dim_t width, uint8_t align, egui_color_t color)
{
    egui_region_t text_region;

    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, text, &text_region, align, color, self->alpha);
}

static void egui_view_step_sequencer_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_step_sequencer_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_step_sequencer_snapshot_t *snapshot;
    egui_color_t status_color;
    egui_color_t shell_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t outer_padding;
    egui_dim_t header_top;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t grid_x;
    egui_dim_t grid_y;
    egui_dim_t grid_w;
    egui_dim_t grid_h;
    egui_dim_t label_col_w;
    egui_dim_t cell_gap;
    egui_dim_t cell_size;
    egui_dim_t step_origin_x;
    egui_dim_t steps_total_w;
    egui_dim_t row_gap;
    egui_dim_t playhead_x;
    egui_dim_t row_y;
    egui_dim_t footer_y;
    egui_dim_t footer_h;
    uint8_t track;
    uint8_t step;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_step_sequencer_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 12 : 13;
    header_top = local->compact_mode ? 7 : 9;
    pill_w = local->compact_mode ? 36 : 42;

    egui_canvas_draw_round_rectangle_fill(panel_x, panel_y, panel_w, panel_h, 10, shell_color, egui_color_alpha_mix(self->alpha, 45));
    egui_canvas_draw_round_rectangle(panel_x, panel_y, panel_w, panel_h, 10, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 10 : 10);
    text_region.location.x = panel_x + outer_padding;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title ? snapshot->title : "STEP", &text_region, EGUI_ALIGN_LEFT, local->muted_text_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(pill_x, panel_y + header_top, pill_w, 11, 5, status_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 32 : 62));
    text_region.location.x = pill_x + 1;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 2;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status ? snapshot->status : "LIVE", &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    grid_x = panel_x + outer_padding;
    grid_y = panel_y + (local->compact_mode ? 21 : 26);
    grid_w = panel_w - outer_padding * 2;
    grid_h = local->compact_mode ? 28 : 44;
    label_col_w = local->compact_mode ? 0 : 12;
    cell_gap = local->compact_mode ? 2 : 3;
    cell_size = local->compact_mode ? 6 : 8;
    steps_total_w = cell_size * 8 + cell_gap * 7;
    step_origin_x = grid_x + label_col_w + 2;
    row_gap = local->compact_mode ? 3 : 4;

    if (local->compact_mode)
    {
        step_origin_x = grid_x + (grid_w - steps_total_w) / 2;
    }

    egui_canvas_draw_round_rectangle_fill(grid_x, grid_y, grid_w, grid_h, 8, local->panel_color, egui_color_alpha_mix(self->alpha, 72));
    egui_canvas_draw_round_rectangle(grid_x, grid_y, grid_w, grid_h, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));

    playhead_x = step_origin_x + snapshot->playhead_step * (cell_size + cell_gap) - 1;
    egui_canvas_draw_round_rectangle_fill(playhead_x, grid_y + 5, cell_size + 2, grid_h - 10, 4, local->playhead_color,
                                          egui_color_alpha_mix(self->alpha, local->locked_mode ? 18 : 26));

    for (track = 0; track < EGUI_VIEW_STEP_SEQUENCER_TRACK_COUNT; track++)
    {
        row_y = grid_y + (local->compact_mode ? 6 : 7) + track * (cell_size + row_gap);

        if (!local->compact_mode)
        {
            egui_view_step_sequencer_draw_text(local, self, step_sequencer_track_labels[track], grid_x + 1, row_y - 1, 10, EGUI_ALIGN_CENTER,
                                               track == snapshot->emphasis_track ? local->text_color : local->muted_text_color);
        }

        for (step = 0; step < 8; step++)
        {
            egui_dim_t cell_x;
            egui_color_t fill_color;
            egui_color_t stroke_color;
            egui_alpha_t fill_alpha;
            uint8_t is_active;
            uint8_t is_playhead;
            uint8_t is_emphasis_track;

            cell_x = step_origin_x + step * (cell_size + cell_gap);
            is_active = (snapshot->track_masks[track] >> step) & 0x01;
            is_playhead = step == snapshot->playhead_step;
            is_emphasis_track = track == snapshot->emphasis_track;
            fill_color = is_active ? status_color : egui_rgb_mix(local->panel_color, local->border_color, 22);
            stroke_color = is_active ? status_color : local->border_color;
            fill_alpha = is_active ? (is_playhead ? 95 : (is_emphasis_track ? 82 : 72)) : (is_playhead ? 35 : 18);

            egui_canvas_draw_round_rectangle_fill(cell_x, row_y, cell_size, cell_size, 2, fill_color, egui_color_alpha_mix(self->alpha, fill_alpha));
            egui_canvas_draw_round_rectangle(cell_x, row_y, cell_size, cell_size, 2, 1, stroke_color, egui_color_alpha_mix(self->alpha, is_active ? 65 : 28));
        }
    }

    footer_y = grid_y + grid_h + (local->compact_mode ? 2 : 2);
    footer_h = panel_h - (footer_y - panel_y) - (local->compact_mode ? 6 : 7);

    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 4 : 3);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 8 : 6);
    text_region.size.height = local->compact_mode ? footer_h : 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->pattern ? snapshot->pattern : "Pattern", &text_region, EGUI_ALIGN_CENTER, local->text_color,
                                  self->alpha);

    if (!local->compact_mode)
    {
        text_region.location.y = footer_y + 9;
        text_region.size.height = EGUI_MAX(footer_h - 10, 10);
        egui_canvas_draw_text_in_rect(local->font, snapshot->footer ? snapshot->footer : "swing", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color,
                                      self->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_step_sequencer_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_step_sequencer_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_step_sequencer_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_step_sequencer_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_step_sequencer_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101927);
    local->panel_color = EGUI_COLOR_HEX(0x162233);
    local->border_color = EGUI_COLOR_HEX(0x40536C);
    local->text_color = EGUI_COLOR_HEX(0xE4ECF8);
    local->muted_text_color = EGUI_COLOR_HEX(0x7F92AA);
    local->accent_color = EGUI_COLOR_HEX(0x53C9FF);
    local->warn_color = EGUI_COLOR_HEX(0xF5B24B);
    local->lock_color = EGUI_COLOR_HEX(0xE0AE73);
    local->playhead_color = EGUI_COLOR_HEX(0x65D4FF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
