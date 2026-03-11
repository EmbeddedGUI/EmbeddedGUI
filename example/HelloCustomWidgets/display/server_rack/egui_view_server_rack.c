#include <stdlib.h>

#include "egui_view_server_rack.h"

static uint8_t egui_view_server_rack_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_SERVER_RACK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_SERVER_RACK_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_server_rack_clamp_unit_count(uint8_t count)
{
    if (count > EGUI_VIEW_SERVER_RACK_MAX_UNITS)
    {
        return EGUI_VIEW_SERVER_RACK_MAX_UNITS;
    }
    return count;
}

static egui_color_t egui_view_server_rack_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_server_rack_status_color(uint8_t status)
{
    switch (status)
    {
    case 1:
        return EGUI_COLOR_HEX(0xF59E0B);
    case 2:
        return EGUI_COLOR_HEX(0xFB7185);
    default:
        return EGUI_COLOR_HEX(0x34D399);
    }
}

void egui_view_server_rack_set_snapshots(egui_view_t *self, const egui_view_server_rack_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_server_rack_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_server_rack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
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

uint8_t egui_view_server_rack_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    return local->current_snapshot;
}

void egui_view_server_rack_set_focus_unit(egui_view_t *self, uint8_t unit_index)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->focus_unit = unit_index;
    egui_view_invalidate(self);
}

void egui_view_server_rack_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_server_rack_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_server_rack_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_server_rack_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_server_rack_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_server_rack_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_server_rack_snapshot_t *snapshot;
    egui_color_t panel_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t unit_count;
    uint8_t current_unit;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    unit_count = egui_view_server_rack_clamp_unit_count(snapshot->unit_count);
    if (snapshot->units == NULL || unit_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_server_rack_mix_disabled(panel_color);
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
    if (content_width <= 20 || content_height <= 20)
    {
        return;
    }

    if (!local->compact_mode && content_height > 14)
    {
        content_height -= 14;
    }

    current_unit = local->focus_unit < unit_count ? local->focus_unit : snapshot->focus_unit;
    if (current_unit >= unit_count)
    {
        current_unit = 0;
    }

    for (i = 0; i < unit_count; i++)
    {
        const egui_view_server_rack_unit_t *unit = &snapshot->units[i];
        egui_dim_t unit_x = content_x + 2;
        egui_dim_t unit_y = content_y + unit->y;
        egui_dim_t unit_w = content_width - 4;
        egui_dim_t unit_h = unit->height;
        egui_dim_t meter_x;
        egui_dim_t meter_y;
        egui_dim_t meter_w;
        egui_dim_t meter_h;
        egui_dim_t led_x;
        egui_color_t status_color;
        egui_color_t fill_color;
        egui_color_t border_color;
        uint8_t is_current = i == snapshot->focus_unit;
        uint8_t is_focus = i == current_unit;

        if (unit_y + unit_h > content_y + content_height)
        {
            continue;
        }

        status_color = egui_view_server_rack_status_color(unit->status);
        fill_color = egui_rgb_mix(local->surface_color, status_color, is_current ? EGUI_ALPHA_20 : EGUI_ALPHA_10);
        border_color = is_focus ? local->active_color : egui_rgb_mix(local->border_color, status_color, EGUI_ALPHA_30);
        if (!is_enabled)
        {
            fill_color = egui_view_server_rack_mix_disabled(fill_color);
            border_color = egui_view_server_rack_mix_disabled(border_color);
            status_color = egui_view_server_rack_mix_disabled(status_color);
        }

        egui_canvas_draw_round_rectangle_fill(unit_x, unit_y, unit_w, unit_h, 4, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
        egui_canvas_draw_round_rectangle(unit_x, unit_y, unit_w, unit_h, 4, is_current ? 2 : 1, border_color,
                                         egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_80 : EGUI_ALPHA_40));

        meter_x = unit_x + (local->compact_mode ? 8 : 34);
        meter_y = unit_y + (unit_h / 2) - 3;
        meter_w = local->compact_mode ? (unit_w - 28) : (unit_w - 58);
        meter_h = 6;
        if (meter_w < 12)
        {
            meter_w = 12;
        }
        egui_canvas_draw_round_rectangle_fill(meter_x, meter_y, meter_w, meter_h, 3, egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_20),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_round_rectangle_fill(meter_x, meter_y, (meter_w * unit->meter_level) / 100, meter_h, 3, status_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));

        led_x = unit_x + unit_w - 14;
        egui_canvas_draw_circle_fill(led_x, unit_y + unit_h / 2, 2, status_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));
        egui_canvas_draw_circle(led_x, unit_y + unit_h / 2, 3, 1, border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));

        if (!local->compact_mode && unit->label != NULL)
        {
            text_region.location.x = unit_x + 4;
            text_region.location.y = unit_y + 1;
            text_region.size.width = 26;
            text_region.size.height = unit_h - 2;
            egui_canvas_draw_text_in_rect(local->font, unit->label, &text_region, EGUI_ALIGN_LEFT, is_focus ? local->text_color : local->muted_text_color,
                                          self->alpha);
        }
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->units[current_unit].label != NULL ? snapshot->units[current_unit].label : snapshot->title;
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 2;
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

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_server_rack_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_server_rack_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_server_rack_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_server_rack_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_server_rack_t);
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
    local->focus_unit = 1;
    local->show_header = 1;
    local->compact_mode = 0;
}
