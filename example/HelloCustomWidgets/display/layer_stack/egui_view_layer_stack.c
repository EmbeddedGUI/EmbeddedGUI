#include <stdlib.h>
#include <string.h>

#include "egui_view_layer_stack.h"

static uint8_t egui_view_layer_stack_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_LAYER_STACK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_LAYER_STACK_MAX_SNAPSHOTS;
    }
    return count;
}

void egui_view_layer_stack_set_snapshots(egui_view_t *self, const egui_view_layer_stack_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_layer_stack_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_layer_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
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

uint8_t egui_view_layer_stack_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    return local->current_snapshot;
}

void egui_view_layer_stack_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_layer_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_layer_stack_set_locked_mode(egui_view_t *self, uint8_t locked_mode)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    local->locked_mode = locked_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_layer_stack_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t panel_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t accent_color,
        egui_color_t warn_color,
        egui_color_t lock_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    local->surface_color = surface_color;
    local->panel_color = panel_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->warn_color = warn_color;
    local->lock_color = lock_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static egui_color_t egui_view_layer_stack_get_status_color(egui_view_layer_stack_t *local, const egui_view_layer_stack_snapshot_t *snapshot)
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

static egui_dim_t egui_view_layer_stack_get_pill_width(const egui_view_layer_stack_snapshot_t *snapshot, uint8_t compact_mode)
{
    egui_dim_t width;
    size_t len;

    len = strlen(snapshot->status);
    width = compact_mode ? (egui_dim_t)(14 + len * 5) : (egui_dim_t)(16 + len * 6);
    if (compact_mode)
    {
        if (width < 32)
        {
            width = 32;
        }
        if (width > 42)
        {
            width = 42;
        }
    }
    else
    {
        if (width < 40)
        {
            width = 40;
        }
        if (width > 58)
        {
            width = 58;
        }
    }
    return width;
}

static void egui_view_layer_stack_draw_layer(
        egui_view_t *self,
        egui_view_layer_stack_t *local,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t w,
        egui_dim_t h,
        uint8_t visible,
        uint8_t active,
        egui_color_t accent_color)
{
    egui_color_t fill_color;
    egui_color_t stroke_color;

    fill_color = active ? accent_color : egui_rgb_mix(local->panel_color, local->surface_color, visible ? 24 : 18);
    stroke_color = active ? local->focus_color : (visible ? local->border_color : egui_rgb_mix(local->border_color, local->surface_color, 26));

    egui_canvas_draw_round_rectangle_fill(
            x,
            y,
            w,
            h,
            local->compact_mode ? 5 : 7,
            fill_color,
            egui_color_alpha_mix(self->alpha, active ? 74 : (visible ? 42 : 26)));
    egui_canvas_draw_round_rectangle(
            x,
            y,
            w,
            h,
            local->compact_mode ? 5 : 7,
            1,
            stroke_color,
            egui_color_alpha_mix(self->alpha, active ? 82 : (visible ? 42 : 24)));

    egui_canvas_draw_round_rectangle_fill(
            x + 5,
            y + 5,
            w - 10,
            2,
            1,
            local->text_color,
            egui_color_alpha_mix(self->alpha, active ? 66 : (visible ? 26 : 12)));
    egui_canvas_draw_round_rectangle_fill(
            x + 7,
            y + h - 8,
            w - 14,
            2,
            1,
            local->muted_text_color,
            egui_color_alpha_mix(self->alpha, active ? 54 : (visible ? 24 : 10)));
    if (active)
    {
        egui_canvas_draw_round_rectangle_fill(
                x + w / 2 - (local->compact_mode ? 4 : 6),
                y + 10,
                local->compact_mode ? 8 : 12,
                2,
                1,
                local->focus_color,
                egui_color_alpha_mix(self->alpha, 92));
    }
}

static void egui_view_layer_stack_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_layer_stack_t);
    egui_region_t region;
    egui_region_t text_region;
    const egui_view_layer_stack_snapshot_t *snapshot;
    egui_color_t status_color;
    egui_color_t shell_color;
    egui_color_t title_color;
    egui_color_t summary_color;
    egui_color_t footer_color;
    egui_dim_t panel_x;
    egui_dim_t panel_y;
    egui_dim_t panel_w;
    egui_dim_t panel_h;
    egui_dim_t outer_padding;
    egui_dim_t header_top;
    egui_dim_t pill_w;
    egui_dim_t pill_x;
    egui_dim_t title_w;
    egui_dim_t summary_y;
    egui_dim_t stack_x;
    egui_dim_t stack_y;
    egui_dim_t stack_w;
    egui_dim_t stack_h;
    egui_dim_t rail_x;
    egui_dim_t footer_y;
    uint8_t layer_count;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    status_color = egui_view_layer_stack_get_status_color(local, snapshot);
    shell_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, 28);
    title_color = local->compact_mode ? egui_rgb_mix(local->muted_text_color, status_color, local->locked_mode ? 24 : 46) : local->muted_text_color;
    summary_color = local->locked_mode ? egui_rgb_mix(local->text_color, local->muted_text_color, 42)
                                       : (local->compact_mode ? local->text_color : egui_rgb_mix(local->text_color, local->muted_text_color, 14));
    footer_color = local->locked_mode ? egui_rgb_mix(local->muted_text_color, local->border_color, 34)
                                      : (local->compact_mode ? egui_rgb_mix(local->muted_text_color, local->text_color, 10)
                                                             : egui_rgb_mix(local->muted_text_color, local->text_color, 18));

    panel_x = region.location.x;
    panel_y = region.location.y;
    panel_w = region.size.width;
    panel_h = region.size.height;
    outer_padding = local->compact_mode ? 12 : 15;
    header_top = local->compact_mode ? 8 : 10;
    pill_w = egui_view_layer_stack_get_pill_width(snapshot, local->compact_mode);

    egui_canvas_draw_round_rectangle_fill(
            panel_x,
            panel_y,
            panel_w,
            panel_h,
            10,
            shell_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? 36 : 52));
    egui_canvas_draw_round_rectangle(
            panel_x,
            panel_y,
            panel_w,
            panel_h,
            10,
            1,
            local->border_color,
            egui_color_alpha_mix(self->alpha, local->compact_mode ? 52 : 68));

    pill_x = panel_x + panel_w - outer_padding - pill_w;
    title_w = pill_x - panel_x - outer_padding - (local->compact_mode ? 10 : 14);

    text_region.location.x = panel_x + outer_padding;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = title_w;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->title, &text_region, EGUI_ALIGN_LEFT, title_color, self->alpha);

    egui_canvas_draw_round_rectangle_fill(
            pill_x,
            panel_y + header_top,
            pill_w,
            11,
            5,
            status_color,
            egui_color_alpha_mix(self->alpha, local->locked_mode ? 32 : 64));
    text_region.location.x = pill_x + 2;
    text_region.location.y = panel_y + header_top;
    text_region.size.width = pill_w - 4;
    text_region.size.height = 11;
    egui_canvas_draw_text_in_rect(local->font, snapshot->status, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

    summary_y = panel_y + (local->compact_mode ? 22 : 30);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 1 : 2);
    text_region.location.y = summary_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 2 : 4);
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, snapshot->summary, &text_region, EGUI_ALIGN_CENTER, summary_color, self->alpha);

    stack_x = panel_x + outer_padding + (local->compact_mode ? 0 : 6);
    stack_y = panel_y + (local->compact_mode ? 34 : 49);
    stack_w = panel_w - outer_padding * 2 - (local->compact_mode ? 8 : 16);
    stack_h = local->compact_mode ? 25 : 39;
    rail_x = panel_x + panel_w - outer_padding - (local->compact_mode ? 4 : 6);
    layer_count = local->compact_mode ? 3 : 4;

    for (i = 0; i < layer_count; i++)
    {
        egui_dim_t offset_x;
        egui_dim_t offset_y;
        egui_dim_t layer_x;
        egui_dim_t layer_y;
        egui_dim_t layer_w;
        egui_dim_t layer_h;
        uint8_t visible;
        uint8_t active;

        offset_x = (layer_count - 1 - i) * (local->compact_mode ? 6 : 9);
        offset_y = (layer_count - 1 - i) * (local->compact_mode ? 4 : 5);
        layer_x = stack_x + offset_x;
        layer_y = stack_y + offset_y;
        layer_w = stack_w - offset_x - (local->compact_mode ? 6 : 9);
        layer_h = stack_h - offset_y;
        visible = (snapshot->visible_mask & (1u << i)) ? 1 : 0;
        active = (snapshot->active_layer == i) ? 1 : 0;
        egui_view_layer_stack_draw_layer(self, local, layer_x, layer_y, layer_w, layer_h, visible, active, status_color);
    }

    for (i = 0; i < layer_count; i++)
    {
        egui_dim_t dot_y;
        uint8_t active;

        active = (snapshot->active_layer == i) ? 1 : 0;
        dot_y = stack_y + (local->compact_mode ? 4 : 5) + i * (local->compact_mode ? 8 : 11);
        egui_canvas_draw_round_rectangle_fill(
                rail_x,
                dot_y,
                local->compact_mode ? 4 : 4,
                local->compact_mode ? 4 : 4,
                2,
                active ? local->focus_color : local->muted_text_color,
                egui_color_alpha_mix(self->alpha, active ? 96 : 28));
    }

    footer_y = stack_y + stack_h + (local->compact_mode ? 9 : 10);
    text_region.location.x = panel_x + outer_padding + (local->compact_mode ? 2 : 4);
    text_region.location.y = footer_y;
    text_region.size.width = panel_w - outer_padding * 2 - (local->compact_mode ? 4 : 8);
    text_region.size.height = EGUI_MAX(panel_h - (footer_y - panel_y) - 6, 10);
    egui_canvas_draw_text_in_rect(local->font, snapshot->footer, &text_region, EGUI_ALIGN_CENTER, footer_color, self->alpha);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_layer_stack_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_layer_stack_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_layer_stack_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_layer_stack_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_layer_stack_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101722);
    local->panel_color = EGUI_COLOR_HEX(0x192333);
    local->border_color = EGUI_COLOR_HEX(0x415874);
    local->text_color = EGUI_COLOR_HEX(0xEDF5FF);
    local->muted_text_color = EGUI_COLOR_HEX(0x95A9BF);
    local->accent_color = EGUI_COLOR_HEX(0x67D7FF);
    local->warn_color = EGUI_COLOR_HEX(0xF2B15E);
    local->lock_color = EGUI_COLOR_HEX(0xC9A77D);
    local->focus_color = EGUI_COLOR_HEX(0x9EF0FF);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->compact_mode = 0;
    local->locked_mode = 0;
}
