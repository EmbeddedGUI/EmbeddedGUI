#define AVATAR_PANEL_SHADOW_ALPHA      20
#define AVATAR_PANEL_FILL_ALPHA        32
#define AVATAR_PANEL_BORDER_ALPHA      52
#define AVATAR_INNER_BORDER_ALPHA      20
#define AVATAR_HEADER_PILL_MIN_WIDTH   82
#define AVATAR_HEADER_LINE_ALPHA       22
#define AVATAR_FOOTER_WIDTH            84
#define AVATAR_FOOTER_FILL_ALPHA       44
#define AVATAR_FOOTER_BORDER_ALPHA     22
#define AVATAR_MINI_BADGE_WIDTH        30
#define AVATAR_MINI_BADGE_FILL_ALPHA   70
#define AVATAR_MINI_BADGE_BORDER_ALPHA 20
#define AVATAR_MINI_TOP_STRIP_ALPHA    18
#define AVATAR_MINI_BOTTOM_STRIP_ALPHA 18
#define AVATAR_DISABLED_OVERLAY_ALPHA  14
#define AVATAR_DISABLED_CROSS_ALPHA    12

#include <stdlib.h>
#include <string.h>

#include "egui_view_avatar_stack.h"

static const egui_color_t avatar_palette[] = {
        EGUI_COLOR_HEX(0x38BDF8), EGUI_COLOR_HEX(0xFB7185), EGUI_COLOR_HEX(0x22C55E),
        EGUI_COLOR_HEX(0xF59E0B), EGUI_COLOR_HEX(0xA78BFA), EGUI_COLOR_HEX(0x14B8A6),
};

static uint8_t egui_view_avatar_stack_clamp_snapshot_count(uint8_t count)
{
    if (count > EGUI_VIEW_AVATAR_STACK_MAX_SNAPSHOTS)
    {
        return EGUI_VIEW_AVATAR_STACK_MAX_SNAPSHOTS;
    }
    return count;
}

static egui_color_t egui_view_avatar_stack_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static const char *egui_view_avatar_stack_get_header_text(egui_view_avatar_stack_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return local->snapshots[local->current_snapshot].title;
}

static const char *egui_view_avatar_stack_get_footer_text(egui_view_avatar_stack_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Read Only";
    }
    return (local->current_snapshot == 0) ? "Group A" : ((local->current_snapshot == 1) ? "Group B" : "Group C");
}

static void egui_view_avatar_stack_draw_badge(const egui_font_t *font, egui_dim_t x, egui_dim_t y, egui_dim_t width, const char *text, egui_color_t fill_color,
                                              egui_color_t text_color, egui_alpha_t alpha)
{
    egui_region_t text_region;

    egui_canvas_draw_round_rectangle_fill(x, y, width, 10, 4, fill_color, alpha);
    text_region.location.x = x;
    text_region.location.y = y;
    text_region.size.width = width;
    text_region.size.height = 10;
    egui_canvas_draw_text_in_rect(font, text, &text_region, EGUI_ALIGN_CENTER, text_color, alpha);
}

void egui_view_avatar_stack_set_snapshots(egui_view_t *self, const egui_view_avatar_stack_snapshot_t *snapshots, uint8_t snapshot_count)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->snapshots = snapshots;
    local->snapshot_count = egui_view_avatar_stack_clamp_snapshot_count(snapshot_count);
    if (local->current_snapshot >= local->snapshot_count)
    {
        local->current_snapshot = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_avatar_stack_set_current_snapshot(egui_view_t *self, uint8_t snapshot_index)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
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

uint8_t egui_view_avatar_stack_get_current_snapshot(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    return local->current_snapshot;
}

void egui_view_avatar_stack_set_focus_member(egui_view_t *self, uint8_t member_index)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->focus_member = member_index;
    egui_view_invalidate(self);
}

void egui_view_avatar_stack_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_avatar_stack_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_avatar_stack_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->compact_mode = compact_mode ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_avatar_stack_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                        egui_color_t muted_text_color, egui_color_t focus_color)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->focus_color = focus_color;
    egui_view_invalidate(self);
}

static void egui_view_avatar_stack_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_avatar_stack_t);
    egui_region_t region;
    egui_region_t header_region;
    egui_region_t text_region;
    const egui_view_avatar_stack_snapshot_t *snapshot;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    uint8_t is_enabled;
    egui_dim_t content_x;
    egui_dim_t content_y;
    egui_dim_t content_width;
    egui_dim_t content_height;
    egui_dim_t radius;
    egui_dim_t overlap;
    egui_dim_t start_x;
    egui_dim_t center_y;
    egui_dim_t footer_height = 0;
    uint8_t visible_count;
    uint8_t i;
    char badge_text[5];
    char compact_badge[3] = {'L', 'K', '\0'};
    const char *header_text;
    const char *footer_text;

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->snapshots == NULL || local->snapshot_count == 0)
    {
        return;
    }

    snapshot = &local->snapshots[local->current_snapshot];
    visible_count = snapshot->visible_member_count;
    if (visible_count > EGUI_VIEW_AVATAR_STACK_MAX_MEMBERS)
    {
        visible_count = EGUI_VIEW_AVATAR_STACK_MAX_MEMBERS;
    }
    if (snapshot->initials == NULL || visible_count == 0)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_30);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_avatar_stack_mix_disabled(panel_color);
        shadow_color = egui_view_avatar_stack_mix_disabled(shadow_color);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, AVATAR_PANEL_SHADOW_ALPHA));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, AVATAR_PANEL_FILL_ALPHA));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, AVATAR_PANEL_BORDER_ALPHA));
    if (AVATAR_INNER_BORDER_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, AVATAR_INNER_BORDER_ALPHA));
    }

    content_x = region.location.x + 6;
    content_y = region.location.y + 4;
    content_width = region.size.width - 12;
    content_height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_snapshot == 0) ? 'A' : ((local->current_snapshot == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        if (AVATAR_MINI_TOP_STRIP_ALPHA > 0)
        {
            egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1,
                                                  is_enabled ? local->text_color : local->muted_text_color,
                                                  egui_color_alpha_mix(self->alpha, AVATAR_MINI_TOP_STRIP_ALPHA));
        }
        header_region.location.x = content_x + (content_width - AVATAR_MINI_BADGE_WIDTH) / 2;
        header_region.location.y = content_y + 6;
        header_region.size.width = AVATAR_MINI_BADGE_WIDTH;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, AVATAR_MINI_BADGE_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, AVATAR_MINI_BADGE_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        content_y += 12;
        content_height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w;

        header_text = egui_view_avatar_stack_get_header_text(local, is_enabled);
        pill_w = 34 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < AVATAR_HEADER_PILL_MIN_WIDTH)
        {
            pill_w = AVATAR_HEADER_PILL_MIN_WIDTH;
        }
        if (pill_w > content_width)
        {
            pill_w = content_width;
        }
        header_region.location.x = content_x + (content_width - pill_w) / 2;
        header_region.location.y = content_y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->border_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        if (AVATAR_HEADER_LINE_ALPHA > 0)
        {
            egui_canvas_draw_line(content_x + 5, content_y + 17, content_x + content_width - 6, content_y + 17, 1, local->border_color,
                                  egui_color_alpha_mix(self->alpha, AVATAR_HEADER_LINE_ALPHA));
        }
        content_y += 20;
        content_height -= 20;
        footer_height = 14;
    }
    if (content_width <= 20 || content_height <= 20)
    {
        return;
    }
    if (footer_height > 0 && content_height > footer_height)
    {
        content_height -= footer_height;
    }

    badge_text[0] = '+';
    badge_text[1] = '0' + (snapshot->total_member_count / 10);
    badge_text[2] = '0' + (snapshot->total_member_count % 10);
    badge_text[3] = 0;
    if (badge_text[1] == '0')
    {
        badge_text[1] = badge_text[2];
        badge_text[2] = 0;
    }
    egui_view_avatar_stack_draw_badge(local->font, region.location.x + region.size.width - 25, region.location.y + 7, 18, badge_text,
                                      is_enabled ? egui_rgb_mix(local->focus_color, EGUI_COLOR_WHITE, EGUI_ALPHA_20)
                                                 : egui_view_avatar_stack_mix_disabled(local->focus_color),
                                      local->text_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));

    radius = local->compact_mode ? 10 : 14;
    if (content_height < radius * 2 + 12)
    {
        radius = (content_height - 12) / 2;
    }
    if (radius < 7)
    {
        radius = 7;
    }
    overlap = radius + (local->compact_mode ? 4 : 6);
    start_x = content_x + radius;
    center_y = content_y + radius + (local->compact_mode ? 3 : 5);

    for (i = 0; i < visible_count; i++)
    {
        egui_dim_t center_x = start_x + i * overlap;
        egui_dim_t draw_radius = radius;
        egui_color_t fill_color = avatar_palette[i % (sizeof(avatar_palette) / sizeof(avatar_palette[0]))];
        egui_color_t ring_color = local->border_color;
        uint8_t is_focus = (i == snapshot->focus_member) || (i == local->focus_member);

        if (!is_enabled)
        {
            fill_color = egui_view_avatar_stack_mix_disabled(fill_color);
            ring_color = egui_view_avatar_stack_mix_disabled(ring_color);
        }
        if (is_focus)
        {
            draw_radius += 2;
            center_y -= 2;
            ring_color = local->focus_color;
        }

        egui_canvas_draw_circle_fill(center_x, center_y, draw_radius, fill_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_80));
        egui_canvas_draw_circle(center_x, center_y, draw_radius, is_focus ? 2 : 1, ring_color,
                                egui_color_alpha_mix(self->alpha, is_focus ? EGUI_ALPHA_90 : EGUI_ALPHA_50));

        text_region.location.x = center_x - draw_radius;
        text_region.location.y = center_y - 5;
        text_region.size.width = draw_radius * 2;
        text_region.size.height = 10;
        egui_canvas_draw_text_in_rect(local->font, snapshot->initials[i], &text_region, EGUI_ALIGN_CENTER, local->text_color, self->alpha);

        if (is_focus)
        {
            center_y += 2;
        }
    }

    if (!local->compact_mode && snapshot->captions != NULL)
    {
        uint8_t focus_index = snapshot->focus_member;

        if (focus_index >= visible_count)
        {
            focus_index = 0;
        }
        text_region.location.x = content_x;
        text_region.location.y = content_y + radius * 2 + 10;
        text_region.size.width = content_width;
        text_region.size.height = 12;
        egui_canvas_draw_text_in_rect(local->font, snapshot->captions[focus_index], &text_region, EGUI_ALIGN_CENTER,
                                      is_enabled ? local->text_color : local->muted_text_color, self->alpha);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 1, content_y + 1, content_width - 2, content_height - 2, 4, EGUI_COLOR_BLACK,
                                              egui_color_alpha_mix(self->alpha, AVATAR_DISABLED_OVERLAY_ALPHA));
        egui_canvas_draw_line(content_x + 2, content_y + 2, content_x + content_width - 3, content_y + content_height - 3, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, AVATAR_DISABLED_CROSS_ALPHA));
        egui_canvas_draw_line(content_x + 2, content_y + content_height - 3, content_x + content_width - 3, content_y + 2, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, AVATAR_DISABLED_CROSS_ALPHA));
    }

    if (local->show_header)
    {
        footer_text = egui_view_avatar_stack_get_footer_text(local, is_enabled);
        text_region.location.x = content_x;
        text_region.location.y = content_y + content_height + 3;
        text_region.size.width = content_width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(content_x + (content_width - AVATAR_FOOTER_WIDTH) / 2, content_y + content_height + 2, AVATAR_FOOTER_WIDTH, 11, 5,
                                              panel_color, egui_color_alpha_mix(self->alpha, AVATAR_FOOTER_FILL_ALPHA));
        egui_canvas_draw_round_rectangle(content_x + (content_width - AVATAR_FOOTER_WIDTH) / 2, content_y + content_height + 2, AVATAR_FOOTER_WIDTH, 11, 5, 1,
                                         egui_rgb_mix(local->border_color, local->surface_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, AVATAR_FOOTER_BORDER_ALPHA));
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }
    else if (AVATAR_MINI_BOTTOM_STRIP_ALPHA > 0)
    {
        egui_canvas_draw_round_rectangle_fill(content_x + 8, region.location.y + region.size.height - 8, content_width - 14, 2, 1,
                                              is_enabled ? local->text_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, AVATAR_MINI_BOTTOM_STRIP_ALPHA));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_avatar_stack_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_avatar_stack_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_avatar_stack_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_avatar_stack_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_avatar_stack_t);
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
    local->focus_member = 1;
    local->show_header = 1;
    local->compact_mode = 0;
}
