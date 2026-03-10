#include <stdlib.h>

#include "egui_view_kanban_board.h"

typedef struct egui_view_kanban_lane_rect egui_view_kanban_lane_rect_t;
struct egui_view_kanban_lane_rect
{
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t width;
    egui_dim_t height;
};

static uint8_t egui_view_kanban_board_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_KANBAN_BOARD_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_KANBAN_BOARD_MAX_SNAPSHOTS;
    }
    return count;
}

static uint8_t egui_view_kanban_board_get_lane_count(const egui_view_kanban_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return 0;
    }
    if (snapshot->lane_count > EGUI_VIEW_KANBAN_BOARD_MAX_LANES)
    {
        return EGUI_VIEW_KANBAN_BOARD_MAX_LANES;
    }
    return snapshot->lane_count;
}

static egui_color_t egui_view_kanban_board_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static void egui_view_kanban_board_draw_chip(
        const egui_font_t *font,
        egui_dim_t x,
        egui_dim_t y,
        egui_dim_t width,
        egui_dim_t height,
        egui_color_t fill_color,
        egui_color_t text_color,
        const char *text,
        egui_alpha_t alpha)
{
    egui_region_t text_region;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    egui_canvas_draw_round_rectangle_fill(x, y, width, height, 4, fill_color, alpha);
    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = height;
    egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, text_color, alpha);
}

static void egui_view_kanban_board_draw_card(
        egui_view_kanban_board_t *local,
        egui_view_kanban_lane_rect_t rect,
        egui_color_t base_color,
        const char *title,
        uint8_t draw_text,
        egui_alpha_t alpha,
        uint8_t is_enabled)
{
    egui_region_t text_region;
    egui_color_t fill_color = egui_rgb_mix(local->surface_color, base_color, EGUI_ALPHA_40);
    egui_color_t border_color = egui_rgb_mix(base_color, local->border_color, EGUI_ALPHA_50);
    egui_color_t text_color = local->text_color;

    if (!is_enabled)
    {
        fill_color = egui_view_kanban_board_mix_disabled(fill_color);
        border_color = egui_view_kanban_board_mix_disabled(border_color);
        text_color = egui_view_kanban_board_mix_disabled(text_color);
    }

    egui_canvas_draw_round_rectangle_fill(rect.x, rect.y, rect.width, rect.height, 4, fill_color, alpha);
    egui_canvas_draw_round_rectangle(rect.x, rect.y, rect.width, rect.height, 4, 1, border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));

    if (!draw_text || title == NULL || rect.width < 28 || rect.height < 12)
    {
        return;
    }

    text_region.location.x = rect.x + 3;
    text_region.location.y = rect.y + 1;
    text_region.size.width = rect.width - 6;
    text_region.size.height = rect.height - 2;
    egui_canvas_draw_text_in_rect(local->font, title, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, alpha);
}

static void egui_view_kanban_board_draw_lane(
        egui_view_kanban_board_t *local,
        const egui_view_kanban_lane_t *lane,
        egui_view_kanban_lane_rect_t rect,
        egui_alpha_t alpha,
        uint8_t is_enabled,
        uint8_t is_focused)
{
    egui_region_t text_region;
    egui_color_t accent_color;
    egui_color_t lane_fill;
    egui_color_t lane_border;
    egui_color_t title_color;
    egui_dim_t header_height = 16;
    egui_dim_t chip_width = 18;
    egui_dim_t card_top;
    egui_dim_t card_height;
    egui_dim_t gap = 4;
    egui_dim_t i;
    egui_dim_t card_count;
    char chip_text[4];

    if (lane == NULL || rect.width <= 8 || rect.height <= 8)
    {
        return;
    }

    accent_color = lane->accent_color;
    lane_fill = egui_rgb_mix(local->surface_color, accent_color, is_focused ? EGUI_ALPHA_40 : EGUI_ALPHA_20);
    lane_border = is_focused ? local->focus_color : egui_rgb_mix(local->border_color, accent_color, EGUI_ALPHA_30);
    title_color = is_focused ? local->text_color : local->muted_text_color;

    if (!is_enabled)
    {
        accent_color = egui_view_kanban_board_mix_disabled(accent_color);
        lane_fill = egui_view_kanban_board_mix_disabled(lane_fill);
        lane_border = egui_view_kanban_board_mix_disabled(lane_border);
        title_color = egui_view_kanban_board_mix_disabled(title_color);
    }

    egui_canvas_draw_round_rectangle_fill(rect.x, rect.y, rect.width, rect.height, 6, lane_fill, egui_color_alpha_mix(alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(rect.x, rect.y, rect.width, rect.height, 6, is_focused ? 2 : 1, lane_border, egui_color_alpha_mix(alpha, is_focused ? EGUI_ALPHA_90 : EGUI_ALPHA_50));
    egui_canvas_draw_round_rectangle_fill(rect.x + 2, rect.y + 2, rect.width - 4, 4, 2, accent_color, egui_color_alpha_mix(alpha, is_focused ? EGUI_ALPHA_90 : EGUI_ALPHA_50));

    text_region.location.x = rect.x + 4;
    text_region.location.y = rect.y + 6;
    text_region.size.width = rect.width - 8 - chip_width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(local->font, lane->title, &text_region, EGUI_ALIGN_LEFT, title_color, alpha);

    chip_text[0] = '0' + (lane->wip_count / 10);
    chip_text[1] = '0' + (lane->wip_count % 10);
    chip_text[2] = 0;
    if (chip_text[0] == '0')
    {
        chip_text[0] = chip_text[1];
        chip_text[1] = 0;
    }
    egui_view_kanban_board_draw_chip(
            local->font,
            rect.x + rect.width - chip_width - 4,
            rect.y + 5,
            chip_width,
            10,
            egui_rgb_mix(accent_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20),
            is_enabled ? local->text_color : egui_view_kanban_board_mix_disabled(local->text_color),
            chip_text,
            egui_color_alpha_mix(alpha, EGUI_ALPHA_70));

    card_top = rect.y + header_height + 4;
    card_count = lane->visible_card_count;
    if (card_count > 3)
    {
        card_count = 3;
    }
    if (card_count == 0)
    {
        text_region.location.x = rect.x + 4;
        text_region.location.y = card_top + 8;
        text_region.size.width = rect.width - 8;
        text_region.size.height = 12;
        egui_canvas_draw_text_in_rect(local->font, "Idle", &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, alpha);
        return;
    }

    card_height = (rect.height - header_height - 12 - gap * (card_count - 1)) / card_count;
    if (card_height < 14)
    {
        card_height = 14;
    }

    for (i = 0; i < card_count; i++)
    {
        egui_view_kanban_lane_rect_t card_rect;
        card_rect.x = rect.x + 4;
        card_rect.y = card_top + i * (card_height + gap);
        card_rect.width = rect.width - 8;
        card_rect.height = card_height;
        egui_view_kanban_board_draw_card(
                local,
                card_rect,
                accent_color,
                (lane->card_titles != NULL) ? lane->card_titles[i] : "",
                local->show_card_text,
                alpha,
                is_enabled);
    }

    if (lane->total_card_count > card_count)
    {
        char more_text[6];
        uint8_t extra = lane->total_card_count - card_count;
        more_text[0] = '+';
        more_text[1] = '0' + (extra / 10);
        more_text[2] = '0' + (extra % 10);
        more_text[3] = 0;
        if (more_text[1] == '0')
        {
            more_text[1] = more_text[2];
            more_text[2] = 0;
        }
        text_region.location.x = rect.x + 4;
        text_region.location.y = rect.y + rect.height - 11;
        text_region.size.width = rect.width - 8;
        text_region.size.height = 9;
        egui_canvas_draw_text_in_rect(local->font, more_text, &text_region, EGUI_ALIGN_RIGHT, local->muted_text_color, alpha);
    }
}

void egui_view_kanban_board_set_snapshots(egui_view_t *self, const egui_view_kanban_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_kanban_board_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_kanban_board_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
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

uint8_t egui_view_kanban_board_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    return local->current_snapshot;
}

void egui_view_kanban_board_set_focus_lane(egui_view_t *self, uint8_t lane_index)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->focus_lane = lane_index;
    egui_view_invalidate(self);
}

void egui_view_kanban_board_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_kanban_board_set_show_card_text(egui_view_t *self, uint8_t show_card_text)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->show_card_text = show_card_text ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_kanban_board_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_kanban_board_set_palette(
        egui_view_t *self,
        egui_color_t surface_color,
        egui_color_t border_color,
        egui_color_t text_color,
        egui_color_t muted_text_color,
        egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static void egui_view_kanban_board_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_kanban_board_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_view_kanban_lane_rect_t content;
    const egui_view_kanban_snapshot_t *snapshot;
    uint8_t lane_count;
    uint8_t i;
    egui_dim_t gap = 4;
    egui_dim_t lane_width;
    egui_color_t panel_color;
    uint8_t is_enabled;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    lane_count = egui_view_kanban_board_get_lane_count(snapshot);
    if (snapshot->lanes == NULL || lane_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    if (!is_enabled)
    {
        panel_color = egui_view_kanban_board_mix_disabled(panel_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));

    content.x = region.location.x + 4;
    content.y = region.location.y + 4;
    content.width = region.size.width - 8;
    content.height = region.size.height - 8;

    if (local->show_header)
    {
        header_region.location.x = content.x + 1;
        header_region.location.y = content.y;
        header_region.size.width = content.width - 2;
        header_region.size.height = 12;
        egui_canvas_draw_text_in_rect(local->font, is_enabled ? snapshot->title : "Locked", &header_region, (region.size.width < 120) ? EGUI_ALIGN_CENTER : EGUI_ALIGN_LEFT, is_enabled ? local->muted_text_color : local->text_color, self->alpha);
        content.y += 14;
        content.height -= 14;
    }
    if (content.width <= 10 || content.height <= 12)
    {
        return;
    }

    lane_width = (content.width - gap * (lane_count - 1)) / lane_count;
    if (lane_width <= 12)
    {
        return;
    }

    for (i = 0; i < lane_count; i++)
    {
        egui_view_kanban_lane_rect_t lane_rect;
        lane_rect.x = content.x + i * (lane_width + gap);
        lane_rect.y = content.y;
        lane_rect.width = lane_width;
        lane_rect.height = content.height;
        if (i == lane_count - 1)
        {
            lane_rect.width = content.x + content.width - lane_rect.x;
        }
        egui_view_kanban_board_draw_lane(local, &snapshot->lanes[i], lane_rect, self->alpha, is_enabled, (i == local->focus_lane));
    }

    if (!is_enabled)
    {
        egui_canvas_draw_line(
                content.x + 2,
                content.y + 2,
                content.x + content.width - 3,
                content.y + content.height - 3,
                1,
                local->muted_text_color,
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_line(
                content.x + 2,
                content.y + content.height - 3,
                content.x + content.width - 3,
                content.y + 2,
                1,
                local->muted_text_color,
                egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_kanban_board_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_kanban_board_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_kanban_board_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_kanban_board_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_kanban_board_t);
    egui_view_set_padding_all(self, 2);

    local->snapshots = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x101723);
    local->border_color = EGUI_COLOR_HEX(0x4A5568);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x94A3B8);
    local->focus_color = EGUI_COLOR_HEX(0x38BDF8);
    local->snapshot_count = 0;
    local->current_snapshot = 0;
    local->focus_lane = 1;
    local->show_card_text = 1;
    local->show_header = 1;
}
