#include <stdlib.h>

#include "egui_view_pin_cluster.h"

static uint8_t egui_view_pin_cluster_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_PIN_CLUSTER_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_PIN_CLUSTER_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_pin_cluster_clamp_pin_count(uint8_t count)
{
    if (count > EGUI_VIEW_PIN_CLUSTER_MAX_PINS)
    {
        return EGUI_VIEW_PIN_CLUSTER_MAX_PINS;
    }
    return count;
}

static egui_color_t egui_view_pin_cluster_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static egui_color_t egui_view_pin_cluster_tone_color(uint8_t tone)
{
    switch (tone)
    {
    case 1:
        return EGUI_COLOR_HEX(0xF59E0B);
    case 2:
        return EGUI_COLOR_HEX(0xFB7185);
    default:
        return EGUI_COLOR_HEX(0x38BDF8);
    }
}

void egui_view_pin_cluster_set_snapshots(egui_view_t *self, const egui_view_pin_cluster_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_pin_cluster_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_pin_cluster_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
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

uint8_t egui_view_pin_cluster_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    return local->current_snapshot;
}

void egui_view_pin_cluster_set_focus_pin(egui_view_t *self, uint8_t pin_index)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->focus_pin = pin_index;
    egui_view_invalidate(self);
}

void egui_view_pin_cluster_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_pin_cluster_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_pin_cluster_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_pin_cluster_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t active_color)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->active_color = active_color;
    egui_view_invalidate(self);
}

static void egui_view_pin_cluster_draw_grid(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t col_step = width / 4;
    egui_dim_t row_step = height / 3;
    egui_dim_t i;

    for (i = 1; i < 4; i++)
    {
        egui_canvas_draw_line(x + col_step * i, y, x + col_step * i, y + height, 1, color, alpha);
    }
    for (i = 1; i < 3; i++)
    {
        egui_canvas_draw_line(x, y + row_step * i, x + width, y + row_step * i, 1, color, alpha);
    }
}

static void egui_view_pin_cluster_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_pin_cluster_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_pin_cluster_snapshot_t *snapshot;
    egui_color_t panel_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    uint8_t pin_count;
    uint8_t current_pin;
    uint8_t i;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    pin_count = egui_view_pin_cluster_clamp_pin_count(snapshot->pin_count);
    if (snapshot->pins == NULL || pin_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_pin_cluster_mix_disabled(panel_color);
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

    current_pin = local->focus_pin < pin_count ? local->focus_pin : snapshot->focus_pin;
    if (current_pin >= pin_count)
    {
        current_pin = 0;
    }

    egui_view_pin_cluster_draw_grid(content_x, content_y, content_width, content_height, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));

    if (!local->compact_mode && snapshot->badge_count > 0)
    {
        char badge[5];
        egui_dim_t badge_x = content_x + content_width - 24;
        badge[0] = '+';
        badge[1] = '0' + (snapshot->badge_count / 10);
        badge[2] = '0' + (snapshot->badge_count % 10);
        badge[3] = 0;
        if (snapshot->badge_count < 10)
        {
            badge[1] = '0' + snapshot->badge_count;
            badge[2] = 0;
        }
        egui_canvas_draw_round_rectangle_fill(badge_x, content_y + 2, 20, 10, 5, egui_rgb_mix(local->surface_color, local->active_color, EGUI_ALPHA_20), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        text_region.location.x = badge_x + 2;
        text_region.location.y = content_y + 2;
        text_region.size.width = 16;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(local->font, badge, &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);
    }

    for (i = 0; i < pin_count; i++)
    {
        const egui_view_pin_cluster_pin_t *pin = &snapshot->pins[i];
        egui_color_t tone_color = egui_view_pin_cluster_tone_color(pin->tone);
        egui_color_t fill_color;
        egui_color_t ring_color;
        egui_dim_t px = content_x + pin->x;
        egui_dim_t py = content_y + pin->y;
        uint8_t is_current = i == snapshot->focus_pin;
        uint8_t is_focus = i == current_pin;

        fill_color = egui_rgb_mix(local->surface_color, tone_color, is_current ? EGUI_ALPHA_30 : EGUI_ALPHA_20);
        ring_color = is_focus ? local->active_color : egui_rgb_mix(local->border_color, tone_color, EGUI_ALPHA_30);
        if (!is_enabled)
        {
            fill_color = egui_view_pin_cluster_mix_disabled(fill_color);
            ring_color = egui_view_pin_cluster_mix_disabled(ring_color);
            tone_color = egui_view_pin_cluster_mix_disabled(tone_color);
        }

        if (is_current || is_focus)
        {
            egui_canvas_draw_circle(px, py, pin->radius + 3, 1, ring_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        }
        egui_canvas_draw_triangle_fill(px, py + pin->radius + 6, px - 4, py + pin->radius - 1, px + 4, py + pin->radius - 1, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
        egui_canvas_draw_circle_fill(px, py, pin->radius, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
        egui_canvas_draw_circle(px, py, pin->radius, is_current ? 2 : 1, ring_color, egui_color_alpha_mix(self->alpha, is_current ? EGUI_ALPHA_80 : EGUI_ALPHA_50));
        egui_canvas_draw_circle_fill(px, py, EGUI_MAX(pin->radius / 3, 2), tone_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_90));

        if (!local->compact_mode && pin->label != NULL)
        {
            text_region.location.x = px - 14;
            text_region.location.y = py + pin->radius + 8;
            text_region.size.width = 28;
            text_region.size.height = 10;
            egui_canvas_draw_text_in_rect(local->font, pin->label, &text_region, EGUI_ALIGN_CENTER, is_focus ? local->text_color : local->muted_text_color, self->alpha);
        }
    }

    if (!local->compact_mode)
    {
        const char *caption = snapshot->pins[current_pin].label != NULL ? snapshot->pins[current_pin].label : snapshot->title;
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

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_pin_cluster_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_pin_cluster_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_pin_cluster_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_pin_cluster_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_pin_cluster_t);
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
    local->focus_pin = 1;
    local->show_header = 1;
    local->compact_mode = 0;
}
