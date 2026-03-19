#include "egui_view_token_input.h"

#include <string.h>

#include "resource/egui_icon_material_symbols.h"

#define TOKEN_STD_RADIUS      10
#define TOKEN_STD_PAD_X       10
#define TOKEN_STD_PAD_Y       8
#define TOKEN_STD_GAP_X       6
#define TOKEN_STD_GAP_Y       6
#define TOKEN_STD_ITEM_H      20
#define TOKEN_STD_TEXT_PAD_X  10
#define TOKEN_STD_INPUT_MIN_W 54
#define TOKEN_STD_REMOVE_W    12
#define TOKEN_STD_REMOVE_GAP  4

#define TOKEN_COMPACT_RADIUS      8
#define TOKEN_COMPACT_PAD_X       6
#define TOKEN_COMPACT_PAD_Y       6
#define TOKEN_COMPACT_GAP_X       4
#define TOKEN_COMPACT_GAP_Y       4
#define TOKEN_COMPACT_ITEM_H      16
#define TOKEN_COMPACT_TEXT_PAD_X  7
#define TOKEN_COMPACT_INPUT_MIN_W 34
#define TOKEN_COMPACT_REMOVE_W    10
#define TOKEN_COMPACT_REMOVE_GAP  2

typedef struct egui_view_token_input_metrics egui_view_token_input_metrics_t;
struct egui_view_token_input_metrics
{
    egui_region_t token_regions[EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS];
    egui_region_t token_text_regions[EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS];
    egui_region_t token_remove_regions[EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS];
    egui_region_t overflow_region;
    egui_region_t overflow_text_region;
    egui_region_t input_region;
    uint8_t token_show_remove[EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS];
    uint8_t visible_token_count;
    uint8_t overflow_count;
    uint8_t show_overflow;
    uint8_t show_input;
};

static void token_input_get_metrics(egui_view_token_input_t *local, egui_view_t *self, egui_view_token_input_metrics_t *metrics);

static uint8_t token_input_is_part_visible_in_metrics(uint8_t part, const egui_view_token_input_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return 0;
    }
    if (part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
    {
        return metrics->show_input;
    }
    if (part < metrics->visible_token_count)
    {
        return metrics->token_regions[part].size.width > 0 && metrics->token_regions[part].size.height > 0;
    }
    return 0;
}

static uint8_t token_input_get_first_part_for_metrics(const egui_view_token_input_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    }
    if (metrics->visible_token_count > 0)
    {
        return 0;
    }
    if (metrics->show_input)
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    }
    return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
}

static uint8_t token_input_get_last_part_for_metrics(const egui_view_token_input_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    }
    if (metrics->show_input)
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    }
    if (metrics->visible_token_count > 0)
    {
        return (uint8_t)(metrics->visible_token_count - 1);
    }
    return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
}

static void token_input_sync_pending_input_focus_restore(egui_view_token_input_t *local, const egui_view_token_input_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return;
    }

    if (!metrics->show_input)
    {
        if (local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT && local->draft_len > 0)
        {
            local->restore_input_focus = 1;
        }
        return;
    }

    if (local->restore_input_focus)
    {
        if (local->draft_len > 0)
        {
            local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
        }
        local->restore_input_focus = 0;
    }
}

static void token_input_normalize_state_for_layout(egui_view_token_input_t *local, egui_view_t *self)
{
    egui_view_token_input_metrics_t metrics;

    if (self->region.size.width <= 0 || self->region.size.height <= 0)
    {
        return;
    }

    token_input_get_metrics(local, self, &metrics);
    token_input_sync_pending_input_focus_restore(local, &metrics);
    if (!token_input_is_part_visible_in_metrics(local->current_part, &metrics))
    {
        local->current_part = token_input_get_last_part_for_metrics(&metrics);
    }
    if (!token_input_is_part_visible_in_metrics(local->pressed_part, &metrics))
    {
        local->pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
        local->pressed_remove = 0;
    }
}

static uint8_t token_input_can_edit_draft(egui_view_token_input_t *local, egui_view_t *self)
{
    egui_view_token_input_metrics_t metrics;

    if (local->read_only_mode)
    {
        return 0;
    }
    if (self->region.size.width <= 0 || self->region.size.height <= 0)
    {
        return 1;
    }

    token_input_get_metrics(local, self, &metrics);
    return metrics.show_input;
}

static void token_input_copy_text(char *dst, uint8_t capacity, const char *src)
{
    size_t len;

    if (dst == NULL || capacity == 0)
    {
        return;
    }

    if (src == NULL)
    {
        dst[0] = '\0';
        return;
    }

    len = strlen(src);
    if (len >= capacity)
    {
        len = capacity - 1;
    }
    if (len > 0)
    {
        memcpy(dst, src, len);
    }
    dst[len] = '\0';
}

static uint8_t token_input_is_trim_char(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static uint8_t token_input_copy_trimmed_text(char *dst, uint8_t capacity, const char *src)
{
    size_t start = 0;
    size_t end;
    size_t len;

    if (dst == NULL || capacity == 0)
    {
        return 0;
    }

    if (src == NULL)
    {
        dst[0] = '\0';
        return 0;
    }

    end = strlen(src);
    while (start < end && token_input_is_trim_char(src[start]))
    {
        start++;
    }
    while (end > start && token_input_is_trim_char(src[end - 1]))
    {
        end--;
    }
    len = end - start;
    if (len >= capacity)
    {
        len = capacity - 1;
    }
    if (len > 0)
    {
        memcpy(dst, src + start, len);
    }
    dst[len] = '\0';
    return len > 0 ? 1 : 0;
}

static uint8_t token_input_is_part_visible(egui_view_token_input_t *local, uint8_t part)
{
    if (part < EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS && part < local->token_count)
    {
        return 1;
    }
    if (part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
    {
        return local->read_only_mode ? 0 : 1;
    }
    return 0;
}

static uint8_t token_input_resolve_default_part(egui_view_token_input_t *local)
{
    if (!local->read_only_mode)
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    }
    if (local->token_count > 0)
    {
        return 0;
    }
    return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
}

static void token_input_normalize_state(egui_view_token_input_t *local)
{
    if (local->token_count > EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS)
    {
        local->token_count = EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS;
    }
    if (local->draft_len > EGUI_VIEW_TOKEN_INPUT_MAX_DRAFT_LEN)
    {
        local->draft_len = EGUI_VIEW_TOKEN_INPUT_MAX_DRAFT_LEN;
    }
    local->draft_text[local->draft_len] = '\0';
    if (local->draft_len == 0)
    {
        local->restore_input_focus = 0;
    }

    if (!token_input_is_part_visible(local, local->current_part))
    {
        local->current_part = token_input_resolve_default_part(local);
    }
    if (!token_input_is_part_visible(local, local->pressed_part))
    {
        local->pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
        local->pressed_remove = 0;
    }
}

static void token_input_notify_changed(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    if (local->on_changed != NULL)
    {
        local->on_changed(self, local->token_count, part);
    }
}

static void token_input_measure_text(const egui_font_t *font, const char *text, egui_dim_t *width, egui_dim_t *height)
{
    egui_dim_t measured_width = 0;
    egui_dim_t measured_height = 0;

    if (font != NULL && font->api != NULL && font->api->get_str_size != NULL && text != NULL && text[0] != '\0')
    {
        font->api->get_str_size(font, text, 0, 0, &measured_width, &measured_height);
    }
    if (measured_width <= 0)
    {
        measured_width = text == NULL ? 0 : (egui_dim_t)(strlen(text) * 6);
    }
    if (measured_height <= 0)
    {
        measured_height = 10;
    }

    if (width != NULL)
    {
        *width = measured_width;
    }
    if (height != NULL)
    {
        *height = measured_height;
    }
}

static egui_dim_t token_input_get_remove_width(egui_view_token_input_t *local)
{
    return local->compact_mode ? TOKEN_COMPACT_REMOVE_W : TOKEN_STD_REMOVE_W;
}

static egui_dim_t token_input_get_remove_gap(egui_view_token_input_t *local)
{
    return local->compact_mode ? TOKEN_COMPACT_REMOVE_GAP : TOKEN_STD_REMOVE_GAP;
}

static const egui_font_t *token_input_get_remove_icon_font(egui_view_token_input_t *local)
{
    EGUI_UNUSED(local);
    return EGUI_FONT_ICON_MS_16;
}

static void token_input_get_metrics(egui_view_token_input_t *local, egui_view_t *self, egui_view_token_input_metrics_t *metrics)
{
    egui_region_t region;
    egui_dim_t pad_x = local->compact_mode ? TOKEN_COMPACT_PAD_X : TOKEN_STD_PAD_X;
    egui_dim_t pad_y = local->compact_mode ? TOKEN_COMPACT_PAD_Y : TOKEN_STD_PAD_Y;
    egui_dim_t gap_x = local->compact_mode ? TOKEN_COMPACT_GAP_X : TOKEN_STD_GAP_X;
    egui_dim_t gap_y = local->compact_mode ? TOKEN_COMPACT_GAP_Y : TOKEN_STD_GAP_Y;
    egui_dim_t item_h = local->compact_mode ? TOKEN_COMPACT_ITEM_H : TOKEN_STD_ITEM_H;
    egui_dim_t text_pad_x = local->compact_mode ? TOKEN_COMPACT_TEXT_PAD_X : TOKEN_STD_TEXT_PAD_X;
    egui_dim_t input_min_w = local->compact_mode ? TOKEN_COMPACT_INPUT_MIN_W : TOKEN_STD_INPUT_MIN_W;
    egui_dim_t start_x;
    egui_dim_t x;
    egui_dim_t y;
    egui_dim_t max_x;
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t item_w;
    egui_dim_t remove_w = token_input_get_remove_width(local);
    egui_dim_t remove_gap = token_input_get_remove_gap(local);
    egui_dim_t text_region_w;
    egui_dim_t max_y;
    uint8_t allow_overflow_summary = (local->compact_mode || local->read_only_mode) ? 1 : 0;
    const char *input_text;
    uint8_t index;

    memset(metrics, 0, sizeof(*metrics));
    egui_view_get_work_region(self, &region);
    start_x = region.location.x + pad_x;
    x = start_x;
    y = region.location.y + pad_y;
    max_x = region.location.x + region.size.width - pad_x;
    max_y = region.location.y + region.size.height - item_h;
    metrics->show_input = local->read_only_mode ? 0 : 1;

    for (index = 0; index < local->token_count; index++)
    {
        metrics->token_show_remove[index] = local->read_only_mode ? 0 : 1;
        token_input_measure_text(local->font, local->tokens[index], &text_w, &text_h);
        item_w = text_w + text_pad_x * 2 + (local->compact_mode ? 4 : 6);
        if (metrics->token_show_remove[index])
        {
            item_w += remove_w + remove_gap;
        }
        if (item_w < (local->compact_mode ? 26 : 34))
        {
            item_w = local->compact_mode ? 26 : 34;
        }
        if (item_w > region.size.width - pad_x * 2)
        {
            item_w = region.size.width - pad_x * 2;
        }
        if (x != start_x && x + item_w > max_x)
        {
            x = start_x;
            y += item_h + gap_y;
        }
        if (allow_overflow_summary && y > max_y)
        {
            metrics->show_overflow = 1;
            metrics->overflow_count = (uint8_t)(local->token_count - index);
            break;
        }
        metrics->token_regions[index].location.x = x;
        metrics->token_regions[index].location.y = y;
        metrics->token_regions[index].size.width = item_w;
        metrics->token_regions[index].size.height = item_h;
        metrics->token_text_regions[index] = metrics->token_regions[index];
        metrics->token_text_regions[index].location.x = x + text_pad_x;
        metrics->token_text_regions[index].location.y = y;
        metrics->token_text_regions[index].size.height = item_h;
        metrics->token_remove_regions[index].location.x = 0;
        metrics->token_remove_regions[index].location.y = 0;
        metrics->token_remove_regions[index].size.width = 0;
        metrics->token_remove_regions[index].size.height = 0;

        text_region_w = item_w - text_pad_x * 2;
        if (metrics->token_show_remove[index])
        {
            metrics->token_remove_regions[index].location.x = x + item_w - text_pad_x - remove_w;
            metrics->token_remove_regions[index].location.y = y;
            metrics->token_remove_regions[index].size.width = remove_w;
            metrics->token_remove_regions[index].size.height = item_h;
            text_region_w -= remove_w + remove_gap;
        }
        metrics->token_text_regions[index].size.width = text_region_w;
        metrics->visible_token_count = (uint8_t)(index + 1);
        x += item_w + gap_x;
    }

    if (metrics->show_overflow)
    {
        if (metrics->visible_token_count > 0)
        {
            metrics->visible_token_count--;
            metrics->overflow_count++;
            metrics->overflow_region = metrics->token_regions[metrics->visible_token_count];
            metrics->overflow_text_region = metrics->token_text_regions[metrics->visible_token_count];
            metrics->token_regions[metrics->visible_token_count].size.width = 0;
            metrics->token_regions[metrics->visible_token_count].size.height = 0;
            metrics->token_text_regions[metrics->visible_token_count].size.width = 0;
            metrics->token_text_regions[metrics->visible_token_count].size.height = 0;
            metrics->token_remove_regions[metrics->visible_token_count].size.width = 0;
            metrics->token_remove_regions[metrics->visible_token_count].size.height = 0;
            metrics->token_show_remove[metrics->visible_token_count] = 0;
        }
        metrics->show_input = 0;
    }

    if (metrics->show_input)
    {
        input_text = local->draft_len > 0 ? local->draft_text : local->placeholder;
        token_input_measure_text(local->font, input_text, &text_w, &text_h);
        item_w = text_w + text_pad_x * 2;
        if (item_w < input_min_w)
        {
            item_w = input_min_w;
        }
        if (item_w > region.size.width - pad_x * 2)
        {
            item_w = region.size.width - pad_x * 2;
        }
        if (x != start_x && x + item_w > max_x)
        {
            x = start_x;
            y += item_h + gap_y;
        }
        if (y > max_y)
        {
            metrics->show_input = 0;
            return;
        }
        metrics->input_region.location.x = x;
        metrics->input_region.location.y = y;
        metrics->input_region.size.width = item_w;
        metrics->input_region.size.height = item_h;
    }
}

static uint8_t token_input_hit_part(egui_view_token_input_t *local, egui_view_t *self, egui_dim_t x, egui_dim_t y, uint8_t *hit_remove)
{
    egui_view_token_input_metrics_t metrics;
    egui_region_t work_region;
    egui_dim_t local_x = x - self->region_screen.location.x;
    egui_dim_t local_y = y - self->region_screen.location.y;
    uint8_t index;

    if (hit_remove != NULL)
    {
        *hit_remove = 0;
    }
    token_input_get_metrics(local, self, &metrics);
    for (index = 0; index < local->token_count; index++)
    {
        if (egui_region_pt_in_rect(&metrics.token_regions[index], local_x, local_y))
        {
            if (hit_remove != NULL && metrics.token_show_remove[index] && egui_region_pt_in_rect(&metrics.token_remove_regions[index], local_x, local_y))
            {
                *hit_remove = 1;
            }
            return index;
        }
    }
    if (metrics.show_input && egui_region_pt_in_rect(&metrics.input_region, local_x, local_y))
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    }
    egui_view_get_work_region(self, &work_region);
    if (!local->read_only_mode && metrics.show_input && egui_region_pt_in_rect(&work_region, local_x, local_y))
    {
        return EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    }
    return EGUI_VIEW_TOKEN_INPUT_PART_NONE;
}

static void token_input_draw_text(const egui_font_t *font, egui_view_t *self, const char *text, const egui_region_t *region, uint8_t align, egui_color_t color,
                                  egui_alpha_t alpha)
{
    egui_region_t draw_region = *region;

    if (text == NULL || text[0] == '\0')
    {
        return;
    }
    egui_canvas_draw_text_in_rect(font, text, &draw_region, align, color, egui_color_alpha_mix(self->alpha, alpha));
}

static void token_input_draw_item(egui_view_t *self, egui_view_token_input_t *local, const egui_region_t *region, const char *text, uint8_t focused,
                                  uint8_t pressed, uint8_t placeholder, const egui_region_t *text_region, uint8_t text_align, const egui_region_t *icon_region,
                                  uint8_t show_icon, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color, egui_color_t muted_color,
                                  egui_color_t accent_color)
{
    egui_color_t fill_color;
    egui_color_t item_border_color;
    egui_color_t item_text_color;
    egui_color_t item_icon_color;
    uint8_t radius = local->compact_mode ? 6 : 7;

    if (placeholder)
    {
        fill_color = egui_rgb_mix(surface_color, muted_color, focused ? 8 : 4);
        item_border_color = egui_rgb_mix(border_color, accent_color, focused ? 24 : 8);
        item_text_color = muted_color;
    }
    else
    {
        fill_color = egui_rgb_mix(surface_color, accent_color, pressed ? 22 : (focused ? 16 : 10));
        item_border_color = egui_rgb_mix(border_color, accent_color, focused ? 34 : 18);
        item_text_color = focused ? egui_rgb_mix(text_color, accent_color, 18) : text_color;
    }
    item_icon_color = focused ? egui_rgb_mix(item_text_color, accent_color, 24) : egui_rgb_mix(muted_color, text_color, 22);
    if (pressed)
    {
        item_icon_color = egui_rgb_mix(item_icon_color, accent_color, 30);
    }

    egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, region->size.width, region->size.height, radius, fill_color,
                                          egui_color_alpha_mix(self->alpha, placeholder ? 92 : 96));
    egui_canvas_draw_round_rectangle(region->location.x, region->location.y, region->size.width, region->size.height, radius, 1, item_border_color,
                                     egui_color_alpha_mix(self->alpha, focused ? 76 : 54));
    token_input_draw_text(local->font, self, text, text_region == NULL ? region : text_region, text_align, item_text_color, placeholder ? 86 : EGUI_ALPHA_100);
    if (show_icon && icon_region != NULL)
    {
        token_input_draw_text(token_input_get_remove_icon_font(local), self, EGUI_ICON_MS_CLOSE, icon_region, EGUI_ALIGN_CENTER, item_icon_color,
                              EGUI_ALPHA_100);
    }
}

static void egui_view_token_input_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    egui_region_t region;
    egui_view_token_input_metrics_t metrics;
    egui_color_t surface_color = local->surface_color;
    egui_color_t border_color = local->border_color;
    egui_color_t text_color = local->text_color;
    egui_color_t muted_color = local->muted_text_color;
    egui_color_t accent_color = local->accent_color;
    egui_color_t shadow_color = local->shadow_color;
    uint8_t enabled = egui_view_get_enable(self) ? 1 : 0;
    const char *input_text;
    char overflow_text[4] = "+0";
    uint8_t index;

    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0)
    {
        return;
    }

    if (local->read_only_mode)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 64);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 26);
        border_color = egui_rgb_mix(border_color, muted_color, 26);
        text_color = egui_rgb_mix(text_color, muted_color, 28);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 34);
    }
    if (!enabled)
    {
        accent_color = egui_rgb_mix(accent_color, muted_color, 70);
        surface_color = egui_rgb_mix(surface_color, EGUI_COLOR_HEX(0xFBFCFD), 28);
        border_color = egui_rgb_mix(border_color, muted_color, 30);
        text_color = egui_rgb_mix(text_color, muted_color, 34);
        shadow_color = egui_rgb_mix(shadow_color, surface_color, 38);
    }

    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y + 2, region.size.width, region.size.height,
                                          local->compact_mode ? TOKEN_COMPACT_RADIUS + 1 : TOKEN_STD_RADIUS + 1, shadow_color,
                                          egui_color_alpha_mix(self->alpha, enabled ? 16 : 10));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height,
                                          local->compact_mode ? TOKEN_COMPACT_RADIUS : TOKEN_STD_RADIUS, surface_color, egui_color_alpha_mix(self->alpha, 96));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height,
                                     local->compact_mode ? TOKEN_COMPACT_RADIUS : TOKEN_STD_RADIUS, 1,
                                     self->is_focused ? egui_rgb_mix(border_color, accent_color, 24) : border_color, egui_color_alpha_mix(self->alpha, 58));

    token_input_get_metrics(local, self, &metrics);
    for (index = 0; index < metrics.visible_token_count; index++)
    {
        token_input_draw_item(self, local, &metrics.token_regions[index], local->tokens[index], local->current_part == index ? 1 : 0,
                              (self->is_pressed && local->pressed_part == index) ? 1 : 0, 0, &metrics.token_text_regions[index], EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER,
                              &metrics.token_remove_regions[index], metrics.token_show_remove[index], surface_color, border_color, text_color, muted_color,
                              accent_color);
    }

    if (metrics.show_overflow)
    {
        overflow_text[1] = (char)('0' + metrics.overflow_count);
        token_input_draw_item(self, local, &metrics.overflow_region, overflow_text, 0, 0, 0, &metrics.overflow_text_region, EGUI_ALIGN_CENTER, NULL, 0,
                              surface_color, border_color, text_color, muted_color, accent_color);
    }

    if (metrics.show_input)
    {
        input_text = local->draft_len > 0 ? local->draft_text : local->placeholder;
        token_input_draw_item(self, local, &metrics.input_region, input_text, local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT ? 1 : 0,
                              (self->is_pressed && local->pressed_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT) ? 1 : 0, local->draft_len == 0 ? 1 : 0,
                              &metrics.input_region,
                              EGUI_ALIGN_CENTER, NULL, 0, surface_color, border_color, text_color, muted_color, accent_color);
    }
}

static void token_input_remove_at(egui_view_token_input_t *local, uint8_t index)
{
    uint8_t tail;

    if (index >= local->token_count)
    {
        return;
    }
    for (tail = index; tail + 1 < local->token_count; tail++)
    {
        memcpy(local->tokens[tail], local->tokens[tail + 1], sizeof(local->tokens[tail]));
    }
    local->tokens[local->token_count - 1][0] = '\0';
    if (local->token_count > 0)
    {
        local->token_count--;
    }
}

static void token_input_adjust_current_part_after_remove(egui_view_token_input_t *local, uint8_t removed_index)
{
    if (local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT || local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_NONE)
    {
        return;
    }

    if (local->token_count == 0)
    {
        local->current_part = token_input_resolve_default_part(local);
        return;
    }

    if (local->current_part > removed_index)
    {
        local->current_part--;
    }

    if (local->current_part >= local->token_count)
    {
        local->current_part = (uint8_t)(local->token_count - 1);
    }
}

static uint8_t token_input_commit_draft(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    char normalized[EGUI_VIEW_TOKEN_INPUT_MAX_TOKEN_LEN + 1];
    uint8_t result;

    if (local->draft_len == 0 || !token_input_can_edit_draft(local, self))
    {
        return 0;
    }
    if (!token_input_copy_trimmed_text(normalized, sizeof(normalized), local->draft_text))
    {
        local->draft_len = 0;
        local->draft_text[0] = '\0';
        egui_view_invalidate(self);
        return 1;
    }
    result = egui_view_token_input_add_token(self, normalized);
    if (result)
    {
        local->draft_len = 0;
        local->draft_text[0] = '\0';
        egui_view_invalidate(self);
    }
    return result;
}

static uint8_t token_input_append_char(egui_view_t *self, char ch)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    if (local->draft_len >= EGUI_VIEW_TOKEN_INPUT_MAX_DRAFT_LEN || !token_input_can_edit_draft(local, self))
    {
        return 0;
    }
    local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    local->draft_text[local->draft_len++] = ch;
    local->draft_text[local->draft_len] = '\0';
    egui_view_invalidate(self);
    return 1;
}

static uint8_t token_input_handle_navigation(egui_view_t *self, uint8_t key_code)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    egui_view_token_input_metrics_t metrics;
    uint8_t use_metrics = self->region.size.width > 0 && self->region.size.height > 0;
    uint8_t visible_token_count = local->token_count;
    uint8_t show_input = local->read_only_mode ? 0 : 1;
    uint8_t first_part = visible_token_count > 0 ? 0 : (show_input ? EGUI_VIEW_TOKEN_INPUT_PART_INPUT : EGUI_VIEW_TOKEN_INPUT_PART_NONE);
    uint8_t last_part =
            show_input ? EGUI_VIEW_TOKEN_INPUT_PART_INPUT : (visible_token_count > 0 ? (uint8_t)(visible_token_count - 1) : EGUI_VIEW_TOKEN_INPUT_PART_NONE);

    if (use_metrics)
    {
        token_input_get_metrics(local, self, &metrics);
        visible_token_count = metrics.visible_token_count;
        show_input = metrics.show_input;
        first_part = token_input_get_first_part_for_metrics(&metrics);
        last_part = token_input_get_last_part_for_metrics(&metrics);
        if (!token_input_is_part_visible_in_metrics(local->current_part, &metrics))
        {
            local->current_part = last_part;
        }
    }

    switch (key_code)
    {
    case EGUI_KEY_CODE_LEFT:
        if (local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
        {
            if (visible_token_count > 0)
            {
                if (!show_input)
                {
                    local->restore_input_focus = 0;
                }
                local->current_part = (uint8_t)(visible_token_count - 1);
                egui_view_invalidate(self);
            }
            return 1;
        }
        if (local->current_part > 0 && local->current_part < visible_token_count)
        {
            if (!show_input)
            {
                local->restore_input_focus = 0;
            }
            local->current_part--;
            egui_view_invalidate(self);
        }
        return 1;
    case EGUI_KEY_CODE_RIGHT:
        if (local->current_part < visible_token_count)
        {
            if (!show_input)
            {
                local->restore_input_focus = 0;
            }
            if (local->current_part + 1 < visible_token_count)
            {
                local->current_part++;
            }
            else if (show_input)
            {
                local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
            }
            egui_view_invalidate(self);
            return 1;
        }
        return 1;
    case EGUI_KEY_CODE_HOME:
        if (!show_input)
        {
            local->restore_input_focus = 0;
        }
        local->current_part = first_part;
        egui_view_invalidate(self);
        return 1;
    case EGUI_KEY_CODE_END:
        if (!show_input)
        {
            local->restore_input_focus = 0;
        }
        local->current_part = last_part;
        egui_view_invalidate(self);
        return 1;
    case EGUI_KEY_CODE_TAB:
        if (!show_input)
        {
            local->restore_input_focus = 0;
        }
        if (visible_token_count == 0)
        {
            local->current_part = last_part;
        }
        else if (local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
        {
            local->current_part = 0;
        }
        else if (local->current_part + 1 < visible_token_count)
        {
            local->current_part++;
        }
        else if (show_input)
        {
            local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
        }
        else
        {
            local->current_part = 0;
        }
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}

static uint8_t token_input_handle_key_inner(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    char ch;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }
    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);

    if (token_input_handle_navigation(self, event->key_code))
    {
        return 1;
    }

    switch (event->key_code)
    {
    case EGUI_KEY_CODE_ENTER:
    case EGUI_KEY_CODE_COMMA:
    case EGUI_KEY_CODE_SPACE:
        return token_input_commit_draft(self);
    case EGUI_KEY_CODE_BACKSPACE:
        if (local->current_part < local->token_count)
        {
            uint8_t removed_part = local->current_part;

            token_input_remove_at(local, local->current_part);
            token_input_adjust_current_part_after_remove(local, removed_part);
            token_input_normalize_state(local);
            token_input_notify_changed(self, removed_part);
            egui_view_invalidate(self);
            return 1;
        }
        if (local->draft_len > 0)
        {
            local->draft_len--;
            local->draft_text[local->draft_len] = '\0';
            egui_view_invalidate(self);
            return 1;
        }
        if (local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
        {
            if (local->token_count > 0)
            {
                token_input_remove_at(local, (uint8_t)(local->token_count - 1));
                token_input_notify_changed(self, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
                egui_view_invalidate(self);
            }
            return 1;
        }
        return 1;
    case EGUI_KEY_CODE_DELETE:
        if (local->current_part < local->token_count)
        {
            uint8_t removed_part = local->current_part;

            token_input_remove_at(local, local->current_part);
            token_input_adjust_current_part_after_remove(local, removed_part);
            token_input_normalize_state(local);
            token_input_notify_changed(self, removed_part);
            egui_view_invalidate(self);
            return 1;
        }
        return 1;
    default:
        ch = egui_key_event_to_char(event);
        if (ch != 0)
        {
            return token_input_append_char(self, ch);
        }
        return 0;
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static int egui_view_token_input_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    uint8_t hit_part;

    if (!egui_view_get_enable(self) || local->read_only_mode)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
        hit_part = token_input_hit_part(local, self, event->location.x, event->location.y, &local->pressed_remove);
        if (hit_part == EGUI_VIEW_TOKEN_INPUT_PART_NONE)
        {
            return 0;
        }
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (self->is_focusable)
        {
            egui_view_request_focus(self);
        }
#endif
        local->pressed_part = hit_part;
        local->restore_input_focus = 0;
        local->current_part = hit_part;
        egui_view_set_pressed(self, true);
        egui_view_invalidate(self);
        return 1;
    case EGUI_MOTION_EVENT_ACTION_MOVE:
        if (local->pressed_part == EGUI_VIEW_TOKEN_INPUT_PART_NONE)
        {
            return 0;
        }
        {
            uint8_t is_pressed = 0;
            uint8_t hit_remove = 0;

            hit_part = token_input_hit_part(local, self, event->location.x, event->location.y, &hit_remove);
            if (local->pressed_part == hit_part && local->pressed_remove == hit_remove)
            {
                is_pressed = 1;
            }
            if (self->is_pressed != is_pressed)
            {
                egui_view_set_pressed(self, is_pressed);
                egui_view_invalidate(self);
            }
        }
        return 1;
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        uint8_t pressed_part = local->pressed_part;
        uint8_t pressed_remove = local->pressed_remove;
        uint8_t was_pressed = self->is_pressed;
        uint8_t hit_remove = 0;
        uint8_t should_remove = 0;

        hit_part = token_input_hit_part(local, self, event->location.x, event->location.y, &hit_remove);
        should_remove = was_pressed && pressed_part != EGUI_VIEW_TOKEN_INPUT_PART_NONE && pressed_part == hit_part && pressed_remove && hit_remove;
        if (!should_remove && was_pressed && pressed_part != EGUI_VIEW_TOKEN_INPUT_PART_NONE && pressed_part == hit_part && pressed_remove == hit_remove)
        {
            local->restore_input_focus = 0;
            local->current_part = hit_part;
        }
        local->pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
        local->pressed_remove = 0;
        egui_view_set_pressed(self, false);
        if (should_remove)
        {
            egui_view_token_input_remove_token(self, hit_part);
            return 1;
        }
    }
        egui_view_invalidate(self);
        return hit_part != EGUI_VIEW_TOKEN_INPUT_PART_NONE ? 1 : 0;
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
        local->pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
        local->pressed_remove = 0;
        egui_view_set_pressed(self, false);
        egui_view_invalidate(self);
        return 1;
    default:
        return 0;
    }
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_token_input_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    switch (event->type)
    {
    case EGUI_KEY_EVENT_ACTION_DOWN:
        switch (event->key_code)
        {
        case EGUI_KEY_CODE_LEFT:
        case EGUI_KEY_CODE_RIGHT:
        case EGUI_KEY_CODE_HOME:
        case EGUI_KEY_CODE_END:
        case EGUI_KEY_CODE_TAB:
        case EGUI_KEY_CODE_ENTER:
        case EGUI_KEY_CODE_BACKSPACE:
        case EGUI_KEY_CODE_DELETE:
        case EGUI_KEY_CODE_SPACE:
        case EGUI_KEY_CODE_COMMA:
            return 1;
        default:
            return egui_key_event_to_char(event) != 0 ? 1 : 0;
        }
    case EGUI_KEY_EVENT_ACTION_UP:
        if (token_input_handle_key_inner(self, event))
        {
            return 1;
        }
        return egui_view_on_key_event(self, event);
    default:
        return 0;
    }
}
#endif

static egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_token_input_t) = {
        .draw = egui_view_draw,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_token_input_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .on_draw = egui_view_token_input_on_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_token_input_on_key_event,
#endif
};

void egui_view_token_input_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_token_input_t);

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_token_input_t);
    egui_view_set_padding_all(self, 2);
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    egui_view_set_focusable(self, true);
#endif

    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->meta_font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->on_changed = NULL;
    local->surface_color = EGUI_COLOR_HEX(0xFFFFFF);
    local->border_color = EGUI_COLOR_HEX(0xD7DFE7);
    local->text_color = EGUI_COLOR_HEX(0x1F2A35);
    local->muted_text_color = EGUI_COLOR_HEX(0x7A8794);
    local->accent_color = EGUI_COLOR_HEX(0x4A86E8);
    local->shadow_color = EGUI_COLOR_HEX(0xDCE5EE);
    token_input_copy_text(local->placeholder, sizeof(local->placeholder), "Add token");
    local->draft_text[0] = '\0';
    local->token_count = 0;
    local->draft_len = 0;
    local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    local->pressed_part = EGUI_VIEW_TOKEN_INPUT_PART_NONE;
    local->pressed_remove = 0;
    local->compact_mode = 0;
    local->read_only_mode = 0;
    local->restore_input_focus = 0;

    egui_view_set_view_name(self, "egui_view_token_input");
}

void egui_view_token_input_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_token_input_set_meta_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->meta_font = font == NULL ? (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT : font;
    egui_view_invalidate(self);
}

void egui_view_token_input_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t text_color,
                                       egui_color_t muted_text_color, egui_color_t accent_color, egui_color_t shadow_color)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->surface_color = surface_color;
    local->border_color = border_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    local->accent_color = accent_color;
    local->shadow_color = shadow_color;
    egui_view_invalidate(self);
}

void egui_view_token_input_set_placeholder(egui_view_t *self, const char *placeholder)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    token_input_copy_text(local->placeholder, sizeof(local->placeholder), placeholder);
    egui_view_invalidate(self);
}

void egui_view_token_input_set_tokens(egui_view_t *self, const char **tokens, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    uint8_t index;

    local->token_count = 0;
    for (index = 0; index < EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS; index++)
    {
        local->tokens[index][0] = '\0';
    }
    if (tokens != NULL)
    {
        for (index = 0; index < count && local->token_count < EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS; index++)
        {
            if (token_input_copy_trimmed_text(local->tokens[local->token_count], sizeof(local->tokens[local->token_count]), tokens[index]))
            {
                local->token_count++;
            }
        }
    }
    local->draft_len = 0;
    local->draft_text[0] = '\0';
    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    egui_view_invalidate(self);
}

uint8_t egui_view_token_input_add_token(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    char normalized[EGUI_VIEW_TOKEN_INPUT_MAX_TOKEN_LEN + 1];

    if (local->token_count >= EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS)
    {
        return 0;
    }
    if (!token_input_copy_trimmed_text(normalized, sizeof(normalized), text))
    {
        return 0;
    }
    token_input_copy_text(local->tokens[local->token_count], sizeof(local->tokens[local->token_count]), normalized);
    local->token_count++;
    local->current_part = EGUI_VIEW_TOKEN_INPUT_PART_INPUT;
    token_input_normalize_state_for_layout(local, self);
    token_input_notify_changed(self, EGUI_VIEW_TOKEN_INPUT_PART_INPUT);
    egui_view_invalidate(self);
    return 1;
}

uint8_t egui_view_token_input_remove_token(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    if (index >= local->token_count)
    {
        return 0;
    }
    token_input_remove_at(local, index);
    token_input_adjust_current_part_after_remove(local, index);
    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    token_input_notify_changed(self, index);
    egui_view_invalidate(self);
    return 1;
}

uint8_t egui_view_token_input_get_token_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    return local->token_count;
}

const char *egui_view_token_input_get_token(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    if (index >= local->token_count)
    {
        return NULL;
    }
    return local->tokens[index];
}

const char *egui_view_token_input_get_draft_text(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    return local->draft_text;
}

void egui_view_token_input_clear_draft(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->draft_len = 0;
    local->draft_text[0] = '\0';
    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    egui_view_invalidate(self);
}

void egui_view_token_input_set_current_part(egui_view_t *self, uint8_t part)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    egui_view_token_input_metrics_t metrics;

    if (self->region.size.width > 0 && self->region.size.height > 0)
    {
        token_input_get_metrics(local, self, &metrics);
        if (!token_input_is_part_visible_in_metrics(part, &metrics))
        {
            return;
        }
    }
    else if (!token_input_is_part_visible(local, part))
    {
        return;
    }
    local->restore_input_focus = 0;
    local->current_part = part;
    egui_view_invalidate(self);
}

uint8_t egui_view_token_input_get_current_part(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    return local->current_part;
}

void egui_view_token_input_set_compact_mode(egui_view_t *self, uint8_t compact_mode)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->compact_mode = compact_mode ? 1 : 0;
    token_input_normalize_state_for_layout(local, self);
    egui_view_invalidate(self);
}

void egui_view_token_input_set_read_only_mode(egui_view_t *self, uint8_t read_only_mode)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    uint8_t next_mode = read_only_mode ? 1 : 0;

    if (next_mode && !local->read_only_mode && local->draft_len > 0 && local->current_part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT)
    {
        local->restore_input_focus = 1;
    }
    local->read_only_mode = next_mode;
    token_input_normalize_state(local);
    token_input_normalize_state_for_layout(local, self);
    egui_view_invalidate(self);
}

void egui_view_token_input_set_on_changed_listener(egui_view_t *self, egui_view_on_token_input_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);

    local->on_changed = listener;
}

uint8_t egui_view_token_input_get_part_region(egui_view_t *self, uint8_t part, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    egui_view_token_input_metrics_t metrics;

    if (region == NULL)
    {
        return 0;
    }
    token_input_get_metrics(local, self, &metrics);
    if (part < EGUI_VIEW_TOKEN_INPUT_MAX_TOKENS && part < local->token_count)
    {
        if (metrics.token_regions[part].size.width <= 0 || metrics.token_regions[part].size.height <= 0)
        {
            return 0;
        }
        *region = metrics.token_regions[part];
    }
    else if (part == EGUI_VIEW_TOKEN_INPUT_PART_INPUT && metrics.show_input)
    {
        *region = metrics.input_region;
    }
    else
    {
        return 0;
    }
    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}

uint8_t egui_view_token_input_get_remove_region(egui_view_t *self, uint8_t index, egui_region_t *region)
{
    EGUI_LOCAL_INIT(egui_view_token_input_t);
    egui_view_token_input_metrics_t metrics;

    if (region == NULL || index >= local->token_count)
    {
        return 0;
    }
    token_input_get_metrics(local, self, &metrics);
    if (!metrics.token_show_remove[index] || metrics.token_remove_regions[index].size.width <= 0)
    {
        return 0;
    }
    *region = metrics.token_remove_regions[index];
    region->location.x += self->region_screen.location.x;
    region->location.y += self->region_screen.location.y;
    return 1;
}
