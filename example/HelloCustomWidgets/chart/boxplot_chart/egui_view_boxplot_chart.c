#include <stdlib.h>
#include <string.h>

#include "egui_view_boxplot_chart.h"

static uint8_t egui_view_boxplot_chart_clamp_item_count(uint8_t count)
{
    if (count > EGUI_VIEW_BOXPLOT_CHART_MAX_ITEMS)
    {
        return EGUI_VIEW_BOXPLOT_CHART_MAX_ITEMS;
    }
    return count;
}

static uint8_t egui_view_boxplot_chart_clamp_value_set_count(uint8_t count)
{
    if (count > EGUI_VIEW_BOXPLOT_CHART_MAX_VALUE_SETS)
    {
        return EGUI_VIEW_BOXPLOT_CHART_MAX_VALUE_SETS;
    }
    return count;
}

static egui_color_t egui_view_boxplot_chart_mix_disabled(egui_color_t color)
{
    return egui_rgb_mix(color, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_70);
}

static const char *egui_view_boxplot_chart_get_header_text(egui_view_boxplot_chart_t *local, uint8_t is_enabled)
{
    if (!is_enabled)
    {
        return "Locked";
    }
    return (local->current_value_set == 0) ? "Profile A" : ((local->current_value_set == 1) ? "Profile B" : "Profile C");
}

static egui_dim_t egui_view_boxplot_chart_value_to_y(uint8_t value, uint8_t min_value, uint8_t max_value, egui_dim_t top, egui_dim_t height)
{
    int32_t range = (int32_t)max_value - (int32_t)min_value;
    if (height <= 1 || range <= 0)
    {
        return top + (height / 2);
    }
    return (egui_dim_t)(top + (((int32_t)max_value - (int32_t)value) * (height - 1)) / range);
}

static void egui_view_boxplot_chart_find_bounds(const uint8_t *min_values, const uint8_t *max_values, uint8_t count, uint8_t *out_min_value,
                                                uint8_t *out_max_value)
{
    uint8_t i;
    uint8_t min_value = 100;
    uint8_t max_value = 0;

    for (i = 0; i < count; i++)
    {
        if (min_values[i] < min_value)
        {
            min_value = min_values[i];
        }
        if (max_values[i] > max_value)
        {
            max_value = max_values[i];
        }
    }
    if (max_value <= min_value)
    {
        if (max_value < 100)
        {
            max_value++;
        }
        else if (min_value > 0)
        {
            min_value--;
        }
    }
    *out_min_value = min_value;
    *out_max_value = max_value;
}

static void egui_view_boxplot_chart_draw_labels(egui_view_boxplot_chart_t *local, egui_region_t label_region, egui_alpha_t alpha)
{
    uint8_t i;
    egui_region_t text_region;

    if (!local->show_labels || local->item_labels == NULL || label_region.size.height <= 6)
    {
        return;
    }
    for (i = 0; i < local->item_count; i++)
    {
        egui_dim_t x0 = label_region.location.x + (egui_dim_t)(((int32_t)label_region.size.width * i) / local->item_count);
        egui_dim_t x1 = label_region.location.x + (egui_dim_t)(((int32_t)label_region.size.width * (i + 1)) / local->item_count);
        text_region.location.x = x0;
        text_region.location.y = label_region.location.y;
        text_region.size.width = x1 - x0;
        text_region.size.height = label_region.size.height;
        if (text_region.size.width < 7)
        {
            continue;
        }
        egui_canvas_draw_text_in_rect(local->font, local->item_labels[i], &text_region, EGUI_ALIGN_CENTER, local->muted_text_color, alpha);
    }
}

static void egui_view_boxplot_chart_draw_item(egui_view_boxplot_chart_t *local, uint8_t min_value, uint8_t q1_value, uint8_t median_value, uint8_t q3_value,
                                              uint8_t max_value, uint8_t min_bound, uint8_t max_bound, egui_dim_t chart_top, egui_dim_t chart_height,
                                              egui_dim_t x0, egui_dim_t x1, egui_alpha_t alpha, uint8_t is_enabled)
{
    egui_dim_t center_x;
    egui_dim_t min_y;
    egui_dim_t q1_y;
    egui_dim_t median_y;
    egui_dim_t q3_y;
    egui_dim_t max_y;
    egui_dim_t box_x;
    egui_dim_t box_width;
    egui_color_t whisker_color;
    egui_color_t box_fill_color;
    egui_color_t median_color;

    if (x1 <= x0 + 1)
    {
        return;
    }
    if (max_value < q3_value)
    {
        max_value = q3_value;
    }
    if (q3_value < median_value)
    {
        q3_value = median_value;
    }
    if (median_value < q1_value)
    {
        median_value = q1_value;
    }
    if (q1_value < min_value)
    {
        q1_value = min_value;
    }

    whisker_color = local->whisker_color;
    box_fill_color = local->box_fill_color;
    median_color = local->median_color;
    if (!is_enabled)
    {
        whisker_color = egui_view_boxplot_chart_mix_disabled(whisker_color);
        box_fill_color = egui_view_boxplot_chart_mix_disabled(box_fill_color);
        median_color = egui_view_boxplot_chart_mix_disabled(median_color);
    }

    center_x = (egui_dim_t)((x0 + x1) / 2);
    min_y = egui_view_boxplot_chart_value_to_y(min_value, min_bound, max_bound, chart_top, chart_height);
    q1_y = egui_view_boxplot_chart_value_to_y(q1_value, min_bound, max_bound, chart_top, chart_height);
    median_y = egui_view_boxplot_chart_value_to_y(median_value, min_bound, max_bound, chart_top, chart_height);
    q3_y = egui_view_boxplot_chart_value_to_y(q3_value, min_bound, max_bound, chart_top, chart_height);
    max_y = egui_view_boxplot_chart_value_to_y(max_value, min_bound, max_bound, chart_top, chart_height);

    egui_canvas_draw_line(center_x, max_y, center_x, min_y, 1, whisker_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));
    egui_canvas_draw_line(center_x - 2, max_y, center_x + 2, max_y, 1, whisker_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));
    egui_canvas_draw_line(center_x - 2, min_y, center_x + 2, min_y, 1, whisker_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_50));

    box_x = x0 + 2;
    box_width = x1 - x0 - 4;
    if (box_width < 4)
    {
        box_width = 4;
        box_x = center_x - 2;
    }
    if (q1_y < q3_y)
    {
        egui_dim_t temp = q1_y;
        q1_y = q3_y;
        q3_y = temp;
    }
    egui_canvas_draw_round_rectangle_fill(box_x, q3_y, box_width, q1_y - q3_y + 1, 2, box_fill_color, alpha);
    egui_canvas_draw_round_rectangle(box_x, q3_y, box_width, q1_y - q3_y + 1, 2, 1, local->border_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_60));
    egui_canvas_draw_line(box_x + 1, median_y, box_x + box_width - 2, median_y, 2, median_color, egui_color_alpha_mix(alpha, EGUI_ALPHA_80));
}

void egui_view_boxplot_chart_set_item_labels(egui_view_t *self, const char **item_labels, uint8_t item_count)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    local->item_labels = item_labels;
    local->item_count = egui_view_boxplot_chart_clamp_item_count(item_count);
    if (local->current_value_set >= local->value_set_count)
    {
        local->current_value_set = 0;
    }
    egui_view_invalidate(self);
}

void egui_view_boxplot_chart_set_value_set(egui_view_t *self, uint8_t set_index, const uint8_t *min_values, const uint8_t *q1_values,
                                           const uint8_t *median_values, const uint8_t *q3_values, const uint8_t *max_values)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    if (set_index >= EGUI_VIEW_BOXPLOT_CHART_MAX_VALUE_SETS)
    {
        return;
    }
    local->min_sets[set_index] = min_values;
    local->q1_sets[set_index] = q1_values;
    local->median_sets[set_index] = median_values;
    local->q3_sets[set_index] = q3_values;
    local->max_sets[set_index] = max_values;
    if (local->value_set_count <= set_index)
    {
        local->value_set_count = set_index + 1;
    }
    local->value_set_count = egui_view_boxplot_chart_clamp_value_set_count(local->value_set_count);
    egui_view_invalidate(self);
}

void egui_view_boxplot_chart_set_current_value_set(egui_view_t *self, uint8_t set_index)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    if (local->value_set_count == 0 || set_index >= local->value_set_count)
    {
        return;
    }
    if (local->current_value_set == set_index)
    {
        return;
    }
    local->current_value_set = set_index;
    egui_view_invalidate(self);
}

uint8_t egui_view_boxplot_chart_get_current_value_set(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    return local->current_value_set;
}

void egui_view_boxplot_chart_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    local->font = font ? font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_view_invalidate(self);
}

void egui_view_boxplot_chart_set_show_labels(egui_view_t *self, uint8_t show_labels)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    local->show_labels = show_labels ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_boxplot_chart_set_show_header(egui_view_t *self, uint8_t show_header)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    local->show_header = show_header ? 1 : 0;
    egui_view_invalidate(self);
}

void egui_view_boxplot_chart_set_palette(egui_view_t *self, egui_color_t surface_color, egui_color_t border_color, egui_color_t whisker_color,
                                         egui_color_t box_fill_color, egui_color_t median_color, egui_color_t text_color, egui_color_t muted_text_color)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    local->surface_color = surface_color;
    local->border_color = border_color;
    local->whisker_color = whisker_color;
    local->box_fill_color = box_fill_color;
    local->median_color = median_color;
    local->text_color = text_color;
    local->muted_text_color = muted_text_color;
    egui_view_invalidate(self);
}

static void egui_view_boxplot_chart_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_boxplot_chart_t);
    egui_region_t region;
    egui_region_t content_region;
    egui_region_t header_region;
    egui_region_t label_region;
    egui_region_t text_region;
    const uint8_t *min_values;
    const uint8_t *q1_values;
    const uint8_t *median_values;
    const uint8_t *q3_values;
    const uint8_t *max_values;
    const char *header_text;
    const char *footer_text;
    egui_color_t panel_color;
    egui_color_t shadow_color;
    egui_dim_t footer_x;
    egui_dim_t footer_width;
    uint8_t min_bound;
    uint8_t max_bound;
    uint8_t is_enabled;
    uint8_t i;
    char compact_badge[3] = {'L', 'K', '\0'};

    egui_view_get_work_region(self, &region);
    if (region.size.width <= 0 || region.size.height <= 0 || local->item_count == 0)
    {
        return;
    }

    min_values = local->min_sets[local->current_value_set];
    q1_values = local->q1_sets[local->current_value_set];
    median_values = local->median_sets[local->current_value_set];
    q3_values = local->q3_sets[local->current_value_set];
    max_values = local->max_sets[local->current_value_set];
    if (min_values == NULL || q1_values == NULL || median_values == NULL || q3_values == NULL || max_values == NULL)
    {
        return;
    }

    is_enabled = egui_view_get_enable(self) ? 1 : 0;
    panel_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_40);
    shadow_color = egui_rgb_mix(EGUI_COLOR_BLACK, local->surface_color, EGUI_ALPHA_10);
    if (!is_enabled)
    {
        panel_color = egui_view_boxplot_chart_mix_disabled(panel_color);
    }
    egui_canvas_draw_round_rectangle_fill(region.location.x + 1, region.location.y + 2, region.size.width, region.size.height, 8, shadow_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, 8, panel_color,
                                          egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
    egui_canvas_draw_round_rectangle(region.location.x, region.location.y, region.size.width, region.size.height, 8, 1, local->border_color,
                                     egui_color_alpha_mix(self->alpha, EGUI_ALPHA_50));
    egui_canvas_draw_round_rectangle(region.location.x + 2, region.location.y + 2, region.size.width - 4, region.size.height - 4, 6, 1,
                                     egui_rgb_mix(local->box_fill_color, local->border_color, EGUI_ALPHA_30), egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));

    content_region.location.x = region.location.x + 4;
    content_region.location.y = region.location.y + 4;
    content_region.size.width = region.size.width - 8;
    content_region.size.height = region.size.height - 8;

    if (!local->show_header)
    {
        if (is_enabled)
        {
            compact_badge[0] = (local->current_value_set == 0) ? 'A' : ((local->current_value_set == 1) ? 'B' : 'C');
            compact_badge[1] = '\0';
        }
        egui_canvas_draw_round_rectangle_fill(region.location.x + 12, region.location.y + 6, region.size.width - 20, 2, 1, local->box_fill_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        header_region.location.x = content_region.location.x + (content_region.size.width - 28) / 2;
        header_region.location.y = content_region.location.y + 6;
        header_region.size.width = 28;
        header_region.size.height = 10;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4,
                                              egui_rgb_mix(local->surface_color, local->box_fill_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_70));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 4, 1,
                                         egui_rgb_mix(local->border_color, local->box_fill_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, compact_badge, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        content_region.location.y += 12;
        content_region.size.height -= 12;
    }

    if (local->show_header)
    {
        egui_dim_t pill_w;
        header_text = egui_view_boxplot_chart_get_header_text(local, is_enabled);
        pill_w = 32 + (egui_dim_t)strlen(header_text) * 6;
        if (pill_w < 76)
        {
            pill_w = 76;
        }
        if (pill_w > content_region.size.width)
        {
            pill_w = content_region.size.width;
        }
        header_region.location.x = content_region.location.x + (content_region.size.width - pill_w) / 2;
        header_region.location.y = content_region.location.y;
        header_region.size.width = pill_w;
        header_region.size.height = 14;
        egui_canvas_draw_round_rectangle_fill(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6,
                                              egui_rgb_mix(local->surface_color, local->box_fill_color, EGUI_ALPHA_10),
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_60));
        egui_canvas_draw_round_rectangle(header_region.location.x, header_region.location.y, header_region.size.width, header_region.size.height, 6, 1,
                                         egui_rgb_mix(local->border_color, local->box_fill_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_30));
        egui_canvas_draw_text_in_rect(local->font, header_text, &header_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
        egui_canvas_draw_line(content_region.location.x + 5, content_region.location.y + 17, content_region.location.x + content_region.size.width - 6,
                              content_region.location.y + 17, 1, local->border_color, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        content_region.location.y += 20;
        content_region.size.height -= 20;
    }
    if (content_region.size.width <= 8 || content_region.size.height <= 8)
    {
        return;
    }

    label_region.location.x = content_region.location.x;
    label_region.location.y = content_region.location.y + content_region.size.height - 10;
    label_region.size.width = content_region.size.width;
    label_region.size.height = 10;
    if (!local->show_labels || content_region.size.height < 24)
    {
        label_region.size.height = 0;
    }
    else
    {
        content_region.size.height -= label_region.size.height;
    }
    if (local->show_header && content_region.size.height > 14)
    {
        content_region.size.height -= 14;
    }
    if (content_region.size.height <= 8)
    {
        return;
    }

    egui_view_boxplot_chart_find_bounds(min_values, max_values, local->item_count, &min_bound, &max_bound);
    for (i = 0; i < local->item_count; i++)
    {
        egui_dim_t x0 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * i) / local->item_count);
        egui_dim_t x1 = content_region.location.x + (egui_dim_t)(((int32_t)content_region.size.width * (i + 1)) / local->item_count);
        egui_view_boxplot_chart_draw_item(local, min_values[i], q1_values[i], median_values[i], q3_values[i], max_values[i], min_bound, max_bound,
                                          content_region.location.y + 1, content_region.size.height - 2, x0, x1, self->alpha, is_enabled);
    }

    if (!is_enabled)
    {
        egui_canvas_draw_round_rectangle_fill(content_region.location.x + 1, content_region.location.y + 1, content_region.size.width - 2,
                                              content_region.size.height - 2, 4, EGUI_COLOR_BLACK, egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_line(content_region.location.x + 8, content_region.location.y + 7, content_region.location.x + content_region.size.width - 9,
                              content_region.location.y + content_region.size.height - 8, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
        egui_canvas_draw_line(content_region.location.x + 8, content_region.location.y + content_region.size.height - 8,
                              content_region.location.x + content_region.size.width - 9, content_region.location.y + 7, 1, local->muted_text_color,
                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_10));
    }
    if (local->show_labels && label_region.size.height > 0)
    {
        egui_view_boxplot_chart_draw_labels(local, label_region, self->alpha);
    }

    if (local->show_header)
    {
        footer_text = (local->current_value_set == 0) ? "Range A" : ((local->current_value_set == 1) ? "Range B" : "Range C");
        footer_width = 76;
        footer_x = content_region.location.x + (content_region.size.width - footer_width) / 2;
        text_region.location.x = content_region.location.x;
        text_region.location.y = content_region.location.y + content_region.size.height + 3;
        text_region.size.width = content_region.size.width;
        text_region.size.height = 11;
        egui_canvas_draw_round_rectangle_fill(footer_x, content_region.location.y + content_region.size.height + 2, footer_width, 11, 5, panel_color,
                                              egui_color_alpha_mix(self->alpha, EGUI_ALPHA_40));
        egui_canvas_draw_round_rectangle(footer_x, content_region.location.y + content_region.size.height + 2, footer_width, 11, 5, 1,
                                         egui_rgb_mix(local->border_color, local->box_fill_color, EGUI_ALPHA_20),
                                         egui_color_alpha_mix(self->alpha, EGUI_ALPHA_20));
        egui_canvas_draw_text_in_rect(local->font, footer_text, &text_region, EGUI_ALIGN_CENTER, is_enabled ? local->text_color : local->muted_text_color,
                                      self->alpha);
    }
    else
    {
        egui_canvas_draw_round_rectangle_fill(content_region.location.x + 12, region.location.y + region.size.height - 8, content_region.size.width - 20, 2, 1,
                                              is_enabled ? local->box_fill_color : local->muted_text_color,
                                              egui_color_alpha_mix(self->alpha, is_enabled ? EGUI_ALPHA_20 : EGUI_ALPHA_10));
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_boxplot_chart_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_boxplot_chart_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_boxplot_chart_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_boxplot_chart_t);
    uint8_t i;

    egui_view_init(self);
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_boxplot_chart_t);
    egui_view_set_padding_all(self, 2);

    local->item_labels = NULL;
    for (i = 0; i < EGUI_VIEW_BOXPLOT_CHART_MAX_VALUE_SETS; i++)
    {
        local->min_sets[i] = NULL;
        local->q1_sets[i] = NULL;
        local->median_sets[i] = NULL;
        local->q3_sets[i] = NULL;
        local->max_sets[i] = NULL;
    }
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->surface_color = EGUI_COLOR_HEX(0x0F1728);
    local->border_color = EGUI_COLOR_HEX(0x536379);
    local->whisker_color = EGUI_COLOR_HEX(0x8EA2B6);
    local->box_fill_color = EGUI_COLOR_HEX(0x2563EB);
    local->median_color = EGUI_COLOR_HEX(0xF59E0B);
    local->text_color = EGUI_COLOR_HEX(0xE2E8F0);
    local->muted_text_color = EGUI_COLOR_HEX(0x93A5BC);
    local->item_count = 0;
    local->value_set_count = 0;
    local->current_value_set = 0;
    local->show_labels = 1;
    local->show_header = 1;
}
